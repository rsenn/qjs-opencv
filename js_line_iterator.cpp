#include "js_mat.hpp"
#include "js_alloc.hpp"
#include "js_point.hpp"
#include "js_size.hpp"
#include "js_rect.hpp"
#include "js_line_iterator.hpp"
#include "js_typed_array.hpp"
#include "jsbindings.hpp"
#include <opencv2/imgproc.hpp>
#include <quickjs.h>
#include <cstdint>
#include <cstdio>
#include <new>

typedef cv::LineIterator JSLineIteratorData;

JSValue line_iterator_proto = JS_UNDEFINED, line_iterator_class = JS_UNDEFINED;
JSClassID js_line_iterator_class_id;

extern "C" int js_line_iterator_init(JSContext*, JSModuleDef*);

static JSValue
js_line_iterator_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSLineIteratorData* li;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;
  int optind = 0;
  JSMatData* mat = 0;
  JSRectData<double>* rect = 0;
  JSSizeData<double>* size = 0;
  JSPointData<double> pt1, pt2;
  int32_t connectivity = 8;
  BOOL left_to_right = false;
  /* double size, angle = -1, response = 0;
   int32_t octave = -1, class_id = -1;
 */

  if(!(li = js_allocate<JSLineIteratorData>(ctx)))
    return JS_EXCEPTION;

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_line_iterator_class_id);
  JS_FreeValue(ctx, proto);
  if(JS_IsException(obj))
    goto fail;

  if((mat = js_mat_data_nothrow(argv[0]))) {
    ++optind;
  } else if((rect = js_rect_data(argv[0]))) {
    ++optind;
  } else if((size = js_size_data(argv[0]))) {
    ++optind;
  }

  js_point_read(ctx, argv[optind++], &pt1);
  js_point_read(ctx, argv[optind++], &pt2);
  if(optind < argc) {
    js_value_to(ctx, argv[optind++], connectivity);

    if(optind < argc)
      js_value_to(ctx, argv[optind++], left_to_right);
  }

  if(mat)
    new(li) JSLineIteratorData(*mat, pt1, pt2, connectivity, left_to_right);
  else if(rect)
    new(li) JSLineIteratorData(*rect, pt1, pt2, connectivity, left_to_right);
  else if(size)
    new(li) JSLineIteratorData(*size, pt1, pt2, connectivity, left_to_right);
  else
    new(li) JSLineIteratorData(pt1, pt2, connectivity, left_to_right);

  JS_SetOpaque(obj, li);
  return obj;
fail:
  js_deallocate(ctx, li);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSLineIteratorData* js_line_iterator_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSLineIteratorData*>(JS_GetOpaque2(ctx, val, js_line_iterator_class_id));
}

JSLineIteratorData* js_line_iterator_data(JSValueConst val) {
  return static_cast<JSLineIteratorData*>(JS_GetOpaque(val, js_line_iterator_class_id));
}

JSValue js_line_iterator_wrap(JSContext* ctx, const cv::LineIterator& line_iterator) {
  JSValue ret;
  JSLineIteratorData* li;

  ret = JS_NewObjectProtoClass(ctx, line_iterator_proto, js_line_iterator_class_id);

  if(!(li = js_allocate<JSLineIteratorData>(ctx)))
    return JS_EXCEPTION;

  *li = line_iterator;

  JS_SetOpaque(ret, li);

  return ret;
}

void
js_line_iterator_finalizer(JSRuntime* rt, JSValue val) {
  JSLineIteratorData* li = static_cast<JSLineIteratorData*>(JS_GetOpaque(val, js_line_iterator_class_id));
  /* Note: 'li' can be NULL in case JS_SetOpaque() was not called */

  li->~JSLineIteratorData();
  js_deallocate(rt, li);
}

enum {
  METHOD_INIT = 0,
  METHOD_DEREF,
  METHOD_PREINCR,
  METHOD_POSTINCR,
  METHOD_POS,
};

static JSValue
js_line_iterator_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSLineIteratorData* li = static_cast<JSLineIteratorData*>(JS_GetOpaque2(ctx, this_val, js_line_iterator_class_id));
  JSValue ret = JS_UNDEFINED;

  switch(magic) {
    case METHOD_INIT: {
      JSMatData* mat;
      JSRectData<double> rect;
      JSPointData<double> pt1, pt2;
      int32_t connectivity;
      BOOL left_to_right;

      if(!(mat = js_mat_data2(ctx, argv[0])))
        return JS_EXCEPTION;

      if(!js_rect_read(ctx, argv[1], &rect))
        return JS_ThrowTypeError(ctx, "argument 2 must be a Rect");

      if(!js_point_read(ctx, argv[2], &pt1))
        return JS_ThrowTypeError(ctx, "argument 3 must be a Rect");

      if(!js_point_read(ctx, argv[3], &pt2))
        return JS_ThrowTypeError(ctx, "argument 4 must be a Rect");

      if(JS_ToInt32(ctx, &connectivity, argv[4]))
        return JS_ThrowTypeError(ctx, "argument 5 must be a Number");

      left_to_right = JS_ToBool(ctx, argv[5]);

      li->init(mat, rect, pt1, pt2, connectivity, left_to_right);
      break;
    }
    case METHOD_DEREF: {
      size_t offset = *(*li) - li->ptr0;
      switch(li->elemSize) {
        case 1: {
          std::array<uchar, 1>* arr = reinterpret_cast<std::array<uchar, 1>*>(*(*li) - offset);
          ret = js_typedarray_from(ctx, *arr, offset);
          break;
        }
        case 2: {
          std::array<uchar, 2>* arr = reinterpret_cast<std::array<uchar, 2>*>(*(*li) - offset);
          ret = js_typedarray_from(ctx, *arr, offset);
          break;
        }
        case 3: {
          std::array<uchar, 3>* arr = reinterpret_cast<std::array<uchar, 3>*>(*(*li) - offset);
          ret = js_typedarray_from(ctx, *arr, offset);
          break;
        }
        case 4: {
          std::array<uchar, 4>* arr = reinterpret_cast<std::array<uchar, 4>*>(*(*li) - offset);
          ret = js_typedarray_from(ctx, *arr, offset);
          break;
        }
        case 8: {
          std::array<double, 1>* arr = reinterpret_cast<std::array<double, 1>*>(*(*li) - offset);
          ret = js_typedarray_from(ctx, *arr, offset);
          break;
        }
      }
      break;
    }
    case METHOD_PREINCR: {
      ++(*li);
      ret = JS_DupValue(ctx, this_val);
      break;
    }
    case METHOD_POSTINCR: {
      JSLineIteratorData li2 = (*li)++;
      ret = js_line_iterator_wrap(ctx, li2);
      break;
    }
    case METHOD_POS: {
      ret = js_point_new(ctx, li->pos());
      break;
    }
  }
  return ret;
}

enum {
  PROP_COUNT = 0,
  PROP_ELEMSIZE,
  PROP_ERR,
  PROP_MINUSDELTA,
  PROP_MINUSSHIFT,
  PROP_MINUSSTEP,
  PROP_P,
  PROP_PLUSDELTA,
  PROP_PLUSSHIFT,
  PROP_PLUSSTEP,
  PROP_PTMODE,
  PROP_PTR,
  PROP_PTR0,
  PROP_STEP,
};

static JSValue
js_line_iterator_getter(JSContext* ctx, JSValueConst this_val, int magic) {
  JSLineIteratorData* li = static_cast<JSLineIteratorData*>(JS_GetOpaque2(ctx, this_val, js_line_iterator_class_id));
  JSValue ret = JS_UNDEFINED;

  switch(magic) {
    case PROP_COUNT: {
      ret = JS_NewInt32(ctx, li->count);
      break;
    }
    case PROP_ELEMSIZE: {
      ret = JS_NewInt32(ctx, li->elemSize);
      break;
    }
    case PROP_ERR: {
      ret = JS_NewInt32(ctx, li->err);
      break;
    }
    case PROP_MINUSDELTA: {
      ret = JS_NewInt32(ctx, li->minusDelta);
      break;
    }
    case PROP_MINUSSHIFT: {
      ret = JS_NewInt32(ctx, li->minusShift);
      break;
    }
    case PROP_MINUSSTEP: {
      ret = JS_NewInt32(ctx, li->minusStep);
      break;
    }
    case PROP_P: {
      ret = js_point_new(ctx, li->p.x, li->p.y);
      break;
    }
    case PROP_PLUSDELTA: {
      ret = JS_NewInt32(ctx, li->plusDelta);
      break;
    }
    case PROP_PLUSSHIFT: {
      ret = JS_NewInt32(ctx, li->plusShift);
      break;
    }
    case PROP_PLUSSTEP: {
      ret = JS_NewInt32(ctx, li->plusStep);
      break;
    }
    case PROP_PTMODE: {
      ret = JS_NewBool(ctx, li->ptmode);
      break;
    }
    case PROP_PTR: {
      char buf[256];
      sprintf(buf, "%p", li->ptr);
      ret = JS_NewString(ctx, buf);
      /* XXX */
      break;
    }
    case PROP_PTR0: {
      char buf[256];
      sprintf(buf, "%p", li->ptr0);
      ret = JS_NewString(ctx, buf);
      /* XXX */
      break;
    }
    case PROP_STEP: {
      ret = JS_NewInt32(ctx, li->step);
      break;
    }
  }
  return ret;
}

static JSValue
js_line_iterator_setter(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic) {
  JSLineIteratorData* li = static_cast<JSLineIteratorData*>(JS_GetOpaque2(ctx, this_val, js_line_iterator_class_id));

  switch(magic) {
    case PROP_COUNT: {
      int32_t count;
      if(!JS_ToInt32(ctx, &count, value))
        li->count = count;
      break;
    }
    case PROP_ELEMSIZE: {
      int32_t elem_size;
      if(!JS_ToInt32(ctx, &elem_size, value))
        li->elemSize = elem_size;
      break;
    }
    case PROP_ERR: {
      int32_t err;
      if(!JS_ToInt32(ctx, &err, value))
        li->err = err;
      break;
    }
    case PROP_MINUSDELTA: {
      int32_t minus_delta;
      if(!JS_ToInt32(ctx, &minus_delta, value))
        li->minusDelta = minus_delta;
      break;
    }
    case PROP_MINUSSHIFT: {
      int32_t minus_shift;
      if(!JS_ToInt32(ctx, &minus_shift, value))
        li->minusShift = minus_shift;
      break;
    }
    case PROP_MINUSSTEP: {
      int32_t minus_step;
      if(!JS_ToInt32(ctx, &minus_step, value))
        li->minusStep = minus_step;
      break;
    }
    case PROP_P: {
      JSPointData<double> p;
      if(js_point_read(ctx, value, &p))
        li->p = p;
      break;
    }
    case PROP_PLUSDELTA: {
      int32_t plus_delta;
      if(!JS_ToInt32(ctx, &plus_delta, value))
        li->plusDelta = plus_delta;
      break;
    }
    case PROP_PLUSSHIFT: {
      int32_t plus_shift;
      if(!JS_ToInt32(ctx, &plus_shift, value))
        li->plusShift = plus_shift;
      break;
    }
    case PROP_PLUSSTEP: {
      int32_t plus_step;
      if(!JS_ToInt32(ctx, &plus_step, value))
        li->plusStep = plus_step;
      break;
    }
    case PROP_PTMODE: {
      li->ptmode = JS_ToBool(ctx, value);
      break;
    }
    case PROP_PTR: {
      /* XXX */
      break;
    }
    case PROP_PTR0: {
      /* XXX */
      break;
    }
    case PROP_STEP: {
      int32_t step;
      if(!JS_ToInt32(ctx, &step, value))
        li->step = step;
      break;
    }
  }

  return JS_UNDEFINED;
}

JSClassDef js_line_iterator_class = {
    .class_name = "LineIterator",
    .finalizer = js_line_iterator_finalizer,
};

const JSCFunctionListEntry js_line_iterator_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("init", 6, js_line_iterator_method, METHOD_INIT),
    JS_CFUNC_MAGIC_DEF("deref", 0, js_line_iterator_method, METHOD_DEREF),
    JS_CFUNC_MAGIC_DEF("preIncr", 0, js_line_iterator_method, METHOD_PREINCR),
    JS_CFUNC_MAGIC_DEF("postIncr", 0, js_line_iterator_method, METHOD_POSTINCR),
    JS_CFUNC_MAGIC_DEF("pos", 0, js_line_iterator_method, METHOD_POS),
    JS_CGETSET_MAGIC_DEF("count", js_line_iterator_getter, js_line_iterator_setter, PROP_COUNT),
    JS_CGETSET_MAGIC_DEF("elemSize", js_line_iterator_getter, js_line_iterator_setter, PROP_ELEMSIZE),
    JS_CGETSET_MAGIC_DEF("err", js_line_iterator_getter, js_line_iterator_setter, PROP_ERR),
    JS_CGETSET_MAGIC_DEF("minusDelta", js_line_iterator_getter, js_line_iterator_setter, PROP_MINUSDELTA),
    JS_CGETSET_MAGIC_DEF("minusShift", js_line_iterator_getter, js_line_iterator_setter, PROP_MINUSSHIFT),
    JS_CGETSET_MAGIC_DEF("minusStep", js_line_iterator_getter, js_line_iterator_setter, PROP_MINUSSTEP),
    JS_CGETSET_MAGIC_DEF("p", js_line_iterator_getter, js_line_iterator_setter, PROP_P),
    JS_CGETSET_MAGIC_DEF("plusDelta", js_line_iterator_getter, js_line_iterator_setter, PROP_PLUSDELTA),
    JS_CGETSET_MAGIC_DEF("plusShift", js_line_iterator_getter, js_line_iterator_setter, PROP_PLUSSHIFT),
    JS_CGETSET_MAGIC_DEF("plusStep", js_line_iterator_getter, js_line_iterator_setter, PROP_PLUSSTEP),
    JS_CGETSET_MAGIC_DEF("ptmode", js_line_iterator_getter, js_line_iterator_setter, PROP_PTMODE),
    JS_CGETSET_MAGIC_DEF("ptr", js_line_iterator_getter, js_line_iterator_setter, PROP_PTR),
    JS_CGETSET_MAGIC_DEF("ptr0", js_line_iterator_getter, js_line_iterator_setter, PROP_PTR0),
    JS_CGETSET_MAGIC_DEF("step", js_line_iterator_getter, js_line_iterator_setter, PROP_STEP),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "LineIterator", JS_PROP_CONFIGURABLE),
};

extern "C" int
js_line_iterator_init(JSContext* ctx, JSModuleDef* m) {

  /* create the LineIterator class */
  JS_NewClassID(&js_line_iterator_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_line_iterator_class_id, &js_line_iterator_class);

  line_iterator_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, line_iterator_proto, js_line_iterator_proto_funcs, countof(js_line_iterator_proto_funcs));
  JS_SetClassProto(ctx, js_line_iterator_class_id, line_iterator_proto);

  /* XXX */
  // js_set_inspect_method(ctx, line_iterator_proto, js_line_iterator_inspect);

  line_iterator_class = JS_NewCFunction2(ctx, js_line_iterator_ctor, "LineIterator", 2, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, line_iterator_class, line_iterator_proto);

  if(m) {
    JS_SetModuleExport(ctx, m, "LineIterator", line_iterator_class);
  }

  return 0;
}

#ifdef JS_LINE_ITERATOR_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_line_iterator
#endif

extern "C" void js_line_iterator_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "LineIterator");
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_line_iterator_init);
  if(!m)
    return NULL;
  js_line_iterator_export(ctx, m);
  return m;
}
