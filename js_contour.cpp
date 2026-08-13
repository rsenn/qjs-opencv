#include "js_contour.hpp"
#include "cutils.h"
#include "include/geometry.hpp"
#include "js_alloc.hpp"
#include "include/js_array.hpp"
#include "js_mat.hpp"
#include "js_point.hpp"
#include "js_point_iterator.hpp"
#include "js_rect.hpp"
#include "js_rotated_rect.hpp"
#include "js_typed_array.hpp"
#include "js_umat.hpp"
#include "include/jsbindings.hpp"
#include "include/psimpl.hpp"
#include <quickjs.h>
#include "include/util.hpp"
#include <opencv2/core/hal/interface.h>
#include <stddef.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iterator>
/*#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>*/
#include <opencv2/imgproc.hpp>
#ifdef HAVE_OPENCV2_GEOMETRY_HPP
#include <opencv2/geometry.hpp>
#endif
#include <ostream>
#include <iostream>
#include <string>
#include <utility>

extern "C" {
thread_local JSValue contour_proto = JS_UNDEFINED, contour_class = JS_UNDEFINED;
thread_local JSClassID js_contour_class_id = 0;
}

static JSValue float64_array;

extern "C" JSValue
js_contour_create(JSContext* ctx, JSValueConst proto) {
  JSContourData<double>* c;
  JSValue obj;

  assert(js_contour_class_id);
  assert(JS_IsObject(proto));

  if(!(c = js_allocate<JSContourData<double>>(ctx)))
    return JS_EXCEPTION;

  new(c) JSContourData<double>();

  obj = JS_NewObjectProtoClass(ctx, proto, js_contour_class_id);

  JS_SetOpaque(obj, c);
  return obj;
}

extern "C" {
JSContourData<double>*
js_contour_data2(JSContext* ctx, JSValueConst val) {
  assert(js_contour_class_id);
  return static_cast<JSContourData<double>*>(JS_GetOpaque2(ctx, val, js_contour_class_id));
}

JSContourData<double>*
js_contour_data(JSValueConst val) {
  assert(js_contour_class_id);
  return static_cast<JSContourData<double>*>(JS_GetOpaque(val, js_contour_class_id));
}
}

JSValue
js_contour_move(JSContext* ctx, JSContourData<double>&& points) {
  assert(js_contour_class_id);
  JSValue obj = js_contour_create(ctx, contour_proto);
  JSContourData<double>* contour = js_contour_data(obj);

  new(contour) JSContourData<double>(std::move(points));

  JS_SetOpaque(obj, contour);
  return obj;
}

static JSValue
js_contour_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSContourData<double>*c, *other;
  JSValue obj, proto;

  if(!(c = js_allocate<JSContourData<double>>(ctx)))
    return JS_EXCEPTION;

  new(c) JSContourData<double>();

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_contour_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, c);

  if(argc > 0 && JS_IsNumber(argv[0])) {
    c->resize(js_value_to<uint32_t>(ctx, argv[0]));
  } else {
    for(int i = 0; i < argc; i++) {
      JSPointData<double> p;

      if((other = js_contour_data(argv[i]))) {
        std::copy(other->begin(), other->end(), std::back_inserter(*c));
      } else if(js_is_array(ctx, argv[i])) {
        JSContourData<double> tmp;
        js_array_to(ctx, argv[i], tmp);

        std::copy(tmp.begin(), tmp.end(), std::back_inserter(*c));
      } else if(!js_point_read(ctx, argv[i], &p)) {
        c->push_back(p);
      } else {
        JS_ThrowTypeError(ctx, "argument %d must be one of: Contour, Array, Point", i + 1);
        goto fail;
      }
    }
  }

  return obj;

fail:
  js_deallocate(ctx, c);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

static JSValue
js_contour_buffer(JSContext* ctx, JSValueConst this_val) {
  JSContourData<double>* contour;
  JSValue ret = JS_UNDEFINED;

  if((contour = js_contour_data2(ctx, this_val)) == nullptr)
    return ret;

  JSObject* obj = JS_VALUE_GET_OBJ(this_val);
  JS_DupValue(ctx, this_val);

  ret = JS_NewArrayBuffer(
      ctx,
      reinterpret_cast<uint8_t*>(contour->data()),
      contour->size() * sizeof(JSPointData<double>),
      [](JSRuntime* rt, void* opaque, void* ptr) { JS_FreeValueRT(rt, JS_MKPTR(JS_TAG_OBJECT, opaque)); },
      obj,
      false);

  return ret;
}

/**
 * @brief      cv.Contour.prototype.arcLength
 *
 * @param      ctx       The context
 * @param[in]  this_val  The this value
 * @param[in]  argc      The count of arguments
 * @param      argv      The arguments array
 *
 * @return     The js value.
 */
/**
 * @brief      cv.Contour.prototype.length
 * @return     Contour length.
 */
static JSValue
js_contour_length(JSContext* ctx, JSValueConst this_val) {
  JSContourData<double>* v;
  JSValue ret;

  if(!(v = js_contour_data(this_val)))
    return JS_UNDEFINED;

  ret = JS_NewUint32(ctx, v->size());
  return ret;
}

enum {
  SIMPLIFY_REUMANN_WITKAM = 0,
  SIMPLIFY_OPHEIM,
  SIMPLIFY_LANG,
  SIMPLIFY_DOUGLAS_PEUCKER,
  SIMPLIFY_NTH_POINT,
  SIMPLIFY_RADIAL_DISTANCE,
  SIMPLIFY_PERPENDICULAR_DISTANCE
};

// Freestanding psimpl function for opencv.js compatibility
// Accepts: Mat CV_32SC2, Contour, or JS array as first argument
// Returns: Mat CV_32SC2
static JSValue
js_psimpl_simplify(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  if(argc < 1) {
    return JS_ThrowTypeError(ctx, "Expected at least 1 argument");
  }

  std::vector<cv::Point> points;
  JSMatData* mat = nullptr;
  JSContourData<double>* contour = nullptr;

  // Try Mat first
  if((mat = js_mat_data2(ctx, argv[0]))) {
    if(mat->type() != CV_32SC2) {
      return JS_ThrowTypeError(ctx, "Expected Mat CV_32SC2");
    }
    int n = mat->rows * mat->cols;
    cv::Point* data = mat->ptr<cv::Point>();
    points.assign(data, data + n);
  }
  // Try Contour
  else if((contour = js_contour_data2(ctx, argv[0]))) {
    points.reserve(contour->size());
    for(const auto& p : *contour) {
      points.emplace_back(static_cast<int>(p.x), static_cast<int>(p.y));
    }
  }
  // Try JS array
  else if(JS_IsArray(ctx, argv[0])) {
    js_array_to(ctx, argv[0], points);
  }
  else {
    return JS_ThrowTypeError(ctx, "Expected Mat, Contour, or array");
  }

  double arg1 = 0, arg2 = 0;
  if(argc > 1) {
    JS_ToFloat64(ctx, &arg1, argv[1]);
    if(argc > 2) {
      JS_ToFloat64(ctx, &arg2, argv[2]);
    }
  }

  // Run simplification
  std::vector<cv::Point> result;
  result.resize(points.size());

  double* start = reinterpret_cast<double*>(points.data());
  double* end = start + points.size() * 2;
  double* out_ptr = reinterpret_cast<double*>(result.data());

  switch(magic) {
    case SIMPLIFY_REUMANN_WITKAM:
      if(arg1 == 0) arg1 = 2;
      out_ptr = psimpl::simplify_reumann_witkam<2>(start, end, arg1, out_ptr);
      break;

    case SIMPLIFY_OPHEIM:
      if(arg1 == 0) arg1 = 2;
      if(arg2 == 0) arg2 = 10;
      out_ptr = psimpl::simplify_opheim<2>(start, end, arg1, arg2, out_ptr);
      break;

    case SIMPLIFY_LANG:
      if(arg1 == 0) arg1 = 2;
      if(arg2 == 0) arg2 = 10;
      out_ptr = psimpl::simplify_lang<2>(start, end, arg1, arg2, out_ptr);
      break;

    case SIMPLIFY_DOUGLAS_PEUCKER:
      if(arg1 == 0) arg1 = 2;
      out_ptr = psimpl::simplify_douglas_peucker<2>(start, end, arg1, out_ptr);
      break;

    case SIMPLIFY_NTH_POINT:
      if(arg1 == 0) arg1 = 2;
      out_ptr = psimpl::simplify_nth_point<2>(start, end, arg1, out_ptr);
      break;

    case SIMPLIFY_RADIAL_DISTANCE:
      if(arg1 == 0) arg1 = 2;
      out_ptr = psimpl::simplify_radial_distance<2>(start, end, arg1, out_ptr);
      break;

    case SIMPLIFY_PERPENDICULAR_DISTANCE:
      if(arg1 == 0) arg1 = 2;
      if(arg2 == 0) arg2 = 1;
      out_ptr = psimpl::simplify_perpendicular_distance<2>(start, end, arg1, arg2, out_ptr);
      break;
  }

  // Calculate actual size
  size_t result_size = (out_ptr - reinterpret_cast<double*>(result.data())) / 2;
  result.resize(result_size);

  // Return as Mat CV_32SC2
  cv::Mat out(result_size, 1, CV_32SC2);
  cv::Point* out_data = out.ptr<cv::Point>();
  std::copy(result.begin(), result.end(), out_data);

  return js_mat_wrap(ctx, out);
}

/**
 * @brief      cv.Contour.prototype.push
 * @param      value     Pushed value
 * @return     undefined
 */
/**
 * @brief      cv.Contour.prototype.pop
 * @return     Tail value
 */
/**
 * @brief      cv.Contour.prototype.unshift
 * @param      value     Added value
 * @return     undefined
 */
/**
 * @brief      cv.Contour.prototype.shift
 * @return     Head value
 */
/**
 * @brief      cv.Contour.prototype.splice
 * @return     Head value
 */
enum {
  INDEX_OF,
  LAST_INDEX_OF,
  FIND_ITEM,
  FIND_INDEX,
  FIND_LAST_ITEM,
  FIND_LAST_INDEX,
};

/**
 * @brief      cv.Contour.prototype.find
 * @return     Head value
 */
/**
 * @brief      cv.Contour.prototype.concat
 * @param      other     Other contour
 * @return     concatenated
 */
/**
 * @brief      cv.Contour.prototype.toArray
 * @return     Array
 */
/**
 * @brief      cv.Contour.prototype.toString
 * @return     String
 */
/**
 * @brief      cv.Contour.prototype.toSource
 * @return     String
 */
/**
 * @brief      cv.Contour.prototype.rect
 * @param    {Number}  x        Horizontal position
 * @param    {Number}   y        Vertical position
 * @param    {Number}   width    Horizontal size
 * @param    {Number}   height   Vertical size
 * @return   {Object Contour}   New Contour
 */
static JSValue
js_contour_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* contour;

  if(!(contour = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

void
js_contour_finalizer(JSRuntime* rt, JSValue this_val) {
  JSContourData<double>* contour;

  assert(js_contour_class_id);

  if((contour = js_contour_data(this_val))) {
    // printf("js_contour_finalizer  cid=%i this_val=%p contour=%p\n", JS_GetClassID(this_val),
    // JS_VALUE_GET_OBJ(this_val), contour);

    contour->~JSContourData<double>();
    js_deallocate(rt, contour);
  }
}

extern "C" {

static int
js_contour_get_own_property(JSContext* ctx, JSPropertyDescriptor* pdesc, JSValueConst obj, JSAtom prop) {
  JSContourData<double>* contour;
  JSValue value = JS_UNDEFINED;
  uint32_t index;

  if(!(contour = js_contour_data(obj)))
    return FALSE;

  if(JS_HasProperty(ctx, contour_proto, prop))
    return FALSE;

  if(js_atom_is_symbol(ctx, prop))
    return FALSE;

  if(js_atom_is_index(ctx, prop, &index)) {
    if(index < contour->size()) {
      value = js_point_new(ctx, (*contour)[index]);

      if(pdesc) {
        pdesc->flags = JS_PROP_ENUMERABLE;
        pdesc->value = value;
        pdesc->getter = JS_UNDEFINED;
        pdesc->setter = JS_UNDEFINED;
      }

      return TRUE;
    }
  } else if(js_atom_is_length(ctx, prop)) {
    value = JS_NewUint32(ctx, contour->size());

    if(pdesc) {
      pdesc->flags = JS_PROP_CONFIGURABLE;
      pdesc->value = value;
      pdesc->getter = JS_UNDEFINED;
      pdesc->setter = JS_UNDEFINED;
    }

    return TRUE;
  }

  return FALSE;
}

static int
js_contour_get_own_property_names(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst obj) {
  JSContourData<double>* contour;
  uint32_t i, len;
  JSPropertyEnum* props;

  if(!(contour = js_contour_data(obj)))
    return 0;

  len = contour->size();

  if((props = js_allocate<JSPropertyEnum>(ctx, len + 1))) {
    for(i = 0; i < len; i++) {
      props[i].is_enumerable = TRUE;
      props[i].atom = i | (1U << 31);
    }

    props[len].is_enumerable = FALSE;
    props[len].atom = JS_NewAtom(ctx, "length");

    *ptab = props;
    *plen = len + 1;
  }

  return 0;
}

static int
js_contour_has_property(JSContext* ctx, JSValueConst obj, JSAtom prop) {
  JSContourData<double>* contour = js_contour_data(obj);
  uint32_t index;

  if(js_atom_is_index(ctx, prop, &index)) {
    if(index < contour->size())
      return TRUE;
  } else if(js_atom_is_length(ctx, prop)) {
    return TRUE;
  } else {
    JSValue proto = JS_GetPrototype(ctx, obj);

    if(JS_IsObject(proto) && JS_HasProperty(ctx, proto, prop))
      return TRUE;
  }

  return FALSE;
}

static JSValue
js_contour_get_property(JSContext* ctx, JSValueConst obj, JSAtom prop, JSValueConst receiver) {
  JSContourData<double>* contour = js_contour_data(obj);
  JSValue value = JS_UNDEFINED;
  uint32_t index;

  if(js_atom_is_index(ctx, prop, &index)) {
    if(index < contour->size())
      value = js_point_new(ctx, (*contour)[index]);
  } else if(js_atom_is_length(ctx, prop)) {
    value = JS_NewUint32(ctx, contour->size());
  } else {
    JSValue proto = JS_GetPrototype(ctx, obj);

    if(JS_IsObject(proto)) {
      JSPropertyDescriptor desc = {0, JS_UNDEFINED, JS_UNDEFINED, JS_UNDEFINED};

      if(JS_GetOwnProperty(ctx, &desc, proto, prop) > 0) {
        if(js_is_function(ctx, desc.getter))
          value = JS_Call(ctx, desc.getter, obj, 0, 0);
        else if(js_is_function(ctx, desc.value))
          value = JS_DupValue(ctx, desc.value);
      }
    }
  }

  return value;
}

static int
js_contour_set_property(JSContext* ctx, JSValueConst obj, JSAtom prop, JSValueConst value, JSValueConst receiver, int flags) {
  JSContourData<double>* contour = js_contour_data(obj);
  uint32_t index;

  if(js_atom_is_index(ctx, prop, &index)) {
    JSPointData<double> point;
    if(index >= contour->size())
      contour->resize(index + 1);

    js_point_read(ctx, value, &point);
    (*contour)[index] = point;
    return TRUE;
  } else if(js_atom_is_length(ctx, prop)) {
    uint32_t len;
    JS_ToUint32(ctx, &len, value);
    contour->resize(len);
    return TRUE;
  }

  return FALSE;
}

JSClassExoticMethods js_contour_exotic_methods = {
    .get_own_property = js_contour_get_own_property,
    .get_own_property_names = js_contour_get_own_property_names,
    /*.has_property = js_contour_has_property,
    .get_property = js_contour_get_property,*/
    .set_property = js_contour_set_property,
};

JSClassDef js_contour_class = {
    .class_name = "Contour",
    .finalizer = js_contour_finalizer,
    .exotic = &js_contour_exotic_methods,
};

JSValue
js_contour_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSContourData<double>* s;

  if(!(s = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  return js_point_iterator_new(ctx, this_val, 0, s->size(), magic);
}

const JSCFunctionListEntry js_contour_proto_funcs[] = {
    JS_CGETSET_DEF("length", js_contour_length, NULL),
    JS_CFUNC_MAGIC_DEF("lines", 0, js_contour_iterator, NEXT_LINE),
    JS_CFUNC_MAGIC_DEF("points", 0, js_contour_iterator, NEXT_POINT),
    JS_ALIAS_DEF("[Symbol.iterator]", "points"),
    JS_ALIAS_DEF("size", "length"),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Contour", JS_PROP_CONFIGURABLE),
};

// psimpl namespace functions for opencv.js compatibility
const JSCFunctionListEntry js_psimpl_funcs[] = {
    JS_CFUNC_MAGIC_DEF("reumannWitkam", 1, js_psimpl_simplify, SIMPLIFY_REUMANN_WITKAM),
    JS_CFUNC_MAGIC_DEF("opheim", 1, js_psimpl_simplify, SIMPLIFY_OPHEIM),
    JS_CFUNC_MAGIC_DEF("lang", 1, js_psimpl_simplify, SIMPLIFY_LANG),
    JS_CFUNC_MAGIC_DEF("douglasPeucker", 1, js_psimpl_simplify, SIMPLIFY_DOUGLAS_PEUCKER),
    JS_CFUNC_MAGIC_DEF("nthPoint", 1, js_psimpl_simplify, SIMPLIFY_NTH_POINT),
    JS_CFUNC_MAGIC_DEF("radialDistance", 1, js_psimpl_simplify, SIMPLIFY_RADIAL_DISTANCE),
    JS_CFUNC_MAGIC_DEF("perpendicularDistance", 1, js_psimpl_simplify, SIMPLIFY_PERPENDICULAR_DISTANCE),
};

const JSCFunctionListEntry js_contour_static_funcs[] = {
    JS_PROP_INT32_DEF("FORMAT_XY", 0x00, 0),
    JS_PROP_INT32_DEF("FORMAT_01", 0x02, 0),
    JS_PROP_INT32_DEF("FORMAT_SPACE", 0x10, 0),
    JS_PROP_INT32_DEF("FORMAT_COMMA", 0x00, 0),
    JS_PROP_INT32_DEF("FORMAT_BRACKET", 0x00, 0),
    JS_PROP_INT32_DEF("FORMAT_NOBRACKET", 0x100, 0),
};

int
js_contour_init(JSContext* ctx, JSModuleDef* m) {

  if(js_contour_class_id == 0) {
    /* create the Contour class */
    JS_NewClassID(&js_contour_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_contour_class_id, &js_contour_class);

    contour_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, contour_proto, js_contour_proto_funcs, countof(js_contour_proto_funcs));
    JS_SetClassProto(ctx, js_contour_class_id, contour_proto);

    contour_class = JS_NewCFunction2(ctx, js_contour_constructor, "Contour", 2, JS_CFUNC_constructor, 0);

    /* set proto.constructor and ctor.prototype */
    JS_SetPropertyFunctionList(ctx, contour_class, js_contour_static_funcs, countof(js_contour_static_funcs));

    JS_SetConstructor(ctx, contour_class, contour_proto);

    JSValue array_proto = js_global_prototype(ctx, "Array");

    JS_SetPrototype(ctx, contour_proto, array_proto);

    JS_FreeValue(ctx, array_proto);

    // js_object_inspect(ctx, contour_proto, js_contour_inspect);
  }

  JSValue global = JS_GetGlobalObject(ctx);
  float64_array = JS_GetPropertyStr(ctx, global, "Float64Array");

  if(m) {
    JS_SetModuleExport(ctx, m, "Contour", contour_class);
    
    // Create and export psimpl namespace
    JSValue psimpl_object = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, psimpl_object, js_psimpl_funcs, countof(js_psimpl_funcs));
    JS_SetModuleExport(ctx, m, "psimpl", psimpl_object);
  }

  return 0;
}

extern "C" void
js_contour_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Contour");
  JS_AddModuleExport(ctx, m, "psimpl");
}

#if defined(JS_CONTOUR_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_contour
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_contour_init)))
    return NULL;

  js_contour_export(ctx, m);
  return m;
}
}
