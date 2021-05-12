#include "jsbindings.hpp"
#include "js_point.hpp"
#include "js_rect.hpp"
#include "js_mat.hpp"
#include "js_array.hpp"
#include "js_alloc.hpp"
#include "js_typed_array.hpp"
#include "quickjs/cutils.h"
#include "quickjs/quickjs.h"

#include <list>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

std::vector<JSPointData<double>*> points;

extern "C" {

JSValue point_proto = JS_UNDEFINED;
JSClassID js_point_class_id = 0;

VISIBLE JSValue
js_point_new(JSContext* ctx, double x, double y) {
  JSValue ret;
  JSPointData<double>* s;

  if(JS_IsUndefined(point_proto))
    js_point_init(ctx, NULL);

  ret = JS_NewObjectProtoClass(ctx, point_proto, js_point_class_id);

  s = js_allocate<JSPointData<double>>(ctx);

  new(s) JSPointData<double>();
  s->x = x <= DBL_EPSILON ? 0 : x;
  s->y = y <= DBL_EPSILON ? 0 : y;

  points.push_back(s);

  JS_SetOpaque(ret, s);
  return ret;
}

VISIBLE JSValue
js_point_wrap(JSContext* ctx, const JSPointData<double>& point) {
  return js_point_new(ctx, point.x, point.y);
}

JSValue
js_point_clone(JSContext* ctx, const JSPointData<double>& point) {
  return js_point_new(ctx, point.x, point.y);
}

static JSValue
js_point_cross(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSPointData<double>* s = js_point_data(ctx, this_val);
  JSPointData<double>* other = js_point_data(ctx, argv[0]);
  double retval;
  if(!s || !other)
    return JS_EXCEPTION;
  retval = s->cross(*other);
  return JS_NewFloat64(ctx, retval);
}

static JSValue
js_point_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  double x, y;
  JSPointData<double> point;

  if(argc > 0) {
    if(js_point_read(ctx, argv[0], &point)) {
      x = point.x;
      y = point.y;
    } else {
      if(JS_ToFloat64(ctx, &x, argv[0]))
        return JS_EXCEPTION;
      if(argc < 2 || JS_ToFloat64(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    }
  }

  return js_point_new(ctx, x, y);
}

VISIBLE JSPointData<double>*
js_point_data(JSContext* ctx, JSValueConst val) {
  return static_cast<JSPointData<double>*>(JS_GetOpaque2(ctx, val, js_point_class_id));
}

static JSValue
js_point_ddot(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSPointData<double>* s = js_point_data(ctx, this_val);
  JSPointData<double>* other = js_point_data(ctx, argv[0]);
  double retval;
  if(!s || !other)
    return JS_EXCEPTION;
  retval = s->ddot(*other);
  return JS_NewFloat64(ctx, retval);
}

static JSValue
js_point_diff(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSPointData<double>* s = js_point_data(ctx, this_val);
  JSPointData<double>* other = js_point_data(ctx, argv[0]);

  JSValue ret;
  if(!s || !other)
    return JS_EXCEPTION;

  ret = js_point_new(ctx, s->x - other->x, s->y - other->y);
  return ret;
}

static JSValue
js_point_get_xy(JSContext* ctx, JSValueConst this_val, int magic) {
  JSPointData<double>* s = js_point_data(ctx, this_val);
  if(!s)
    return JS_EXCEPTION;
  if(magic == 0)
    return JS_NewFloat64(ctx, s->x);
  else if(magic == 1)
    return JS_NewFloat64(ctx, s->y);
  return JS_UNDEFINED;
}

static JSValue
js_point_inside(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSPointData<double>* s = js_point_data(ctx, this_val);
  JSRectData<double> r = js_rect_get(ctx, argv[0]);
  bool retval;
  if(!s /*|| !r*/)
    return JS_EXCEPTION;

  retval = s->inside(r);

  return JS_NewBool(ctx, retval);
}

static JSValue
js_point_norm(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSPointData<double>* s = js_point_data(ctx, this_val);
  if(!s)
    return JS_EXCEPTION;
  return JS_NewFloat64(ctx, sqrt((double)s->x * s->x + (double)s->y * s->y));
}

/*
static JSValue
js_point_mul(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSPointData<double>* s = js_point_data(ctx, this_val);
  double factor = 1.0;
  JS_ToFloat64(ctx, &factor, argv[0]);
  JSValue ret;
  if(!s || argc < 1)
    return JS_EXCEPTION;

  return js_point_new(ctx, s->x * factor, s->y * factor);
}

static JSValue
js_point_quot(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSPointData<double>* s = js_point_data(ctx, this_val);
  double divisor = 1.0;
  JS_ToFloat64(ctx, &divisor, argv[0]);
  JSValue ret;
  if(!s || argc < 1)
    return JS_EXCEPTION;

  ret = js_point_new(ctx, s->x / divisor, s->y / divisor);
  return ret;
}*/

static JSValue
js_point_set_xy(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSPointData<double>* s = js_point_data(ctx, this_val);
  double v;
  if(!s)
    return JS_EXCEPTION;
  if(JS_ToFloat64(ctx, &v, val))
    return JS_EXCEPTION;
  if(magic == 0)
    s->x = v;
  else
    s->y = v;
  return JS_UNDEFINED;
}

static JSValue
js_point_add(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSPointData<double> other, point, *s = js_point_data(ctx, this_val);
  double x, y;

  if(js_point_read(ctx, argv[0], &other)) {
    x = other.x;
    y = other.y;
  } else {
    JS_ToFloat64(ctx, &x, argv[0]);
    if(argc < 2)
      y = x;
    else
      JS_ToFloat64(ctx, &y, argv[1]);
  }

  switch(magic) {
    case 0:
      point.x = s->x + x;
      point.y = s->y + y;

      break;
    case 1:
      point.x = s->x - x;
      point.y = s->y - y;
      break;
    case 2:
      point.x = s->x * x;
      point.y = s->y * y;
      break;
    case 3:
      point.x = s->x / x;
      point.y = s->y / y;
      break;
  }
  return js_point_wrap(ctx, point);
}

static JSValue
js_point_to_string(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSPointData<double>* s = js_point_data(ctx, this_val);
  std::ostringstream os;
  JSValue xv, yv;
  const char* delim = ",";
  double x = -1, y = -1;
  /* if(!s)
     return JS_EXCEPTION;*/
  if(argc > 0)
    delim = JS_ToCString(ctx, argv[0]);

  xv = JS_GetPropertyStr(ctx, this_val, "x");
  yv = JS_GetPropertyStr(ctx, this_val, "y");

  if(JS_IsNumber(xv) && JS_IsNumber(yv)) {
    JS_ToFloat64(ctx, &x, xv);
    JS_ToFloat64(ctx, &y, yv);
  } else if(s) {
    x = s->x;
    y = s->y;
  }

  switch(magic) {
    case 0: {
      os << x << "," << y;
      break;
    }
    case 1: {
      os << "[" << x << "," << y << "]";
      break;
    }
    case 2: {

      os << "{x:" << x << ",y:" << y << "}";
      break;
    }
  }
  return JS_NewString(ctx, os.str().c_str());
}

static JSValue
js_point_to_array(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSPointData<double>* s = js_point_data(ctx, this_val);
  std::array<double, 2> arr;

  arr[0] = s->x;
  arr[1] = s->y;

  return js_array_from(ctx, arr.cbegin(), arr.cend());
}

static JSValue
js_point_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSPointData<double>* s = js_point_data(ctx, this_val);
  JSValue obj = JS_NewObjectProto(ctx, point_proto);

  JS_DefinePropertyValueStr(ctx, obj, "x", JS_NewFloat64(ctx, s->x), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "y", JS_NewFloat64(ctx, s->y), JS_PROP_ENUMERABLE);

  return obj;
}

static JSAtom iterator_symbol;

static JSValue
js_point_symbol_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSValue arr, iter;
  arr = js_point_to_array(ctx, this_val, argc, argv);

  if(iterator_symbol == 0)
    iterator_symbol = js_symbol_atom(ctx, "iterator");

  if(!JS_IsFunction(ctx, (iter = JS_GetProperty(ctx, arr, iterator_symbol))))
    return JS_EXCEPTION;
  return JS_Call(ctx, iter, arr, 0, argv);
}

static JSValue
js_point_round(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSPointData<double> point, *s = js_point_data(ctx, this_val);
  double x, y;
  double prec = 1;
  point = *s;
  JSValue ret = JS_UNDEFINED;

  switch(magic) {
    case 0:
      if(argc > 0)
        JS_ToFloat64(ctx, &prec, argv[0]);

      x = round(s->x / prec);
      y = round(s->y / prec);
      ret = js_point_new(ctx, x * prec, y * prec);
      break;
    case 1:
      if(argc > 0)
        JS_ToFloat64(ctx, &prec, argv[0]);

      x = floor(s->x / prec);
      y = floor(s->y / prec);
      ret = js_point_new(ctx, x * prec, y * prec);
      break;
    case 2:
      if(argc > 0)
        JS_ToFloat64(ctx, &prec, argv[0]);

      x = ceil(s->x / prec);
      y = ceil(s->y / prec);
      ret = js_point_new(ctx, x * prec, y * prec);
      break;
  }
  return ret;
}

static JSValue
js_point_from(JSContext* ctx, JSValueConst point, int argc, JSValueConst* argv) {
  std::array<double, 2> array;
  JSValue ret = JS_EXCEPTION;

  if(JS_IsString(argv[0])) {
    const char* str = JS_ToCString(ctx, argv[0]);
    char* endptr = nullptr;
    for(size_t i = 0; i < 2; i++) {
      while(!isdigit(*str) && *str != '-' && *str != '+' && !(*str == '.' && isdigit(str[1]))) str++;
      if(*str == '\0')
        break;
      array[i] = strtod(str, &endptr);
      str = endptr;
    }
  } else if(js_is_array(ctx, argv[0])) {
    js_array_to<double, 2>(ctx, argv[0], array);
  }
  if(array[0] > 0 && array[1] > 0)
    ret = js_point_new(ctx, array[0], array[1]);
  return ret;
}

static void
js_point_finalizer(JSRuntime* rt, JSValue val) {
  JSPointData<double>* s = static_cast<JSPointData<double>*>(JS_GetOpaque(val, js_point_class_id));

  if(s != nullptr) {
    js_deallocate(rt, s);
  }

  // JS_FreeValueRT(rt, val);

  /*  if(points.size() == 0)
      JS_FreeValueRT(rt, point_proto);*/
}

JSValue point_class = JS_UNDEFINED;

JSClassDef js_point_class = {
    .class_name = "Point",
    .finalizer = js_point_finalizer,
};

const JSCFunctionListEntry js_point_proto_funcs[] = {
    JS_CGETSET_ENUMERABLE_DEF("x", js_point_get_xy, js_point_set_xy, 0),
    JS_CGETSET_ENUMERABLE_DEF("y", js_point_get_xy, js_point_set_xy, 1),
    JS_CFUNC_DEF("cross", 1, js_point_cross),
    JS_CFUNC_DEF("dot", 1, js_point_ddot),
    JS_CFUNC_DEF("inside", 1, js_point_inside),
    JS_CFUNC_DEF("diff", 1, js_point_diff),
    JS_CFUNC_MAGIC_DEF("add", 1, js_point_add, 0),
    JS_CFUNC_MAGIC_DEF("sub", 1, js_point_add, 1),
    JS_CFUNC_MAGIC_DEF("mul", 1, js_point_add, 2),
    JS_CFUNC_MAGIC_DEF("div", 1, js_point_add, 3),
    JS_CFUNC_DEF("norm", 0, js_point_norm),
    JS_CFUNC_MAGIC_DEF("round", 0, js_point_round, 0),
    JS_CFUNC_MAGIC_DEF("floor", 0, js_point_round, 1),
    JS_CFUNC_MAGIC_DEF("ceil", 0, js_point_round, 2),
    JS_CFUNC_MAGIC_DEF("toString", 0, js_point_to_string, 0),
    JS_CFUNC_DEF("toArray", 0, js_point_to_array),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, js_point_symbol_iterator),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Point", JS_PROP_CONFIGURABLE),

    // JS_CFUNC_MAGIC_DEF("[Symbol.toStringTag]", 0, js_point_to_string, 1),
};
const JSCFunctionListEntry js_point_static_funcs[] = {JS_CFUNC_DEF("from", 1, js_point_from)};

int
js_point_init(JSContext* ctx, JSModuleDef* m) {

  if(js_point_class_id == 0) {
    /* create the Point class */
    JS_NewClassID(&js_point_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_point_class_id, &js_point_class);

    point_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, point_proto, js_point_proto_funcs, countof(js_point_proto_funcs));
    JS_SetClassProto(ctx, js_point_class_id, point_proto);

    point_class = JS_NewCFunction2(ctx, js_point_ctor, "Point", 0, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, point_class, point_proto);
    JS_SetPropertyFunctionList(ctx, point_class, js_point_static_funcs, countof(js_point_static_funcs));

    js_set_inspect_method(ctx, point_proto, js_point_inspect);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Point", point_class);
  /* else
     JS_SetPropertyStr(ctx, *static_cast<JSValue*>(m), name, point_class);*/
  return 0;
}

void
js_point_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(point_class))
    js_point_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "Point", point_class);
}

extern "C" VISIBLE void
js_point_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Point");
}

#if defined(JS_POINT_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_point
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_point_init);
  if(!m)
    return NULL;
  js_point_export(ctx, m);
  return m;
}
}