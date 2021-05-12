#ifndef JS_POINT_HPP
#define JS_POINT_HPP

#include "jsbindings.hpp"

extern "C" VISIBLE int js_point_init(JSContext*, JSModuleDef*);

extern "C" JSValue js_point_clone(JSContext* ctx, const JSPointData<double>& point);
extern "C" {

extern JSValue point_proto, point_class;
extern JSClassID js_point_class_id;

VISIBLE JSValue js_point_new(JSContext*, double x, double y);
VISIBLE JSValue js_point_wrap(JSContext*, const JSPointData<double>&);
VISIBLE JSPointData<double>* js_point_data(JSContext*, JSValueConst val);

int js_point_init(JSContext*, JSModuleDef* m);
void js_point_constructor(JSContext* ctx, JSValue parent, const char* name);

JSModuleDef* js_init_point_module(JSContext*, const char* module_name);
}

template<class T>
static inline int
js_point_read(JSContext* ctx, JSValueConst point, JSPointData<T>* out) {
  int ret = 1;
  JSValue x = JS_UNDEFINED, y = JS_UNDEFINED;
  if(js_is_array_like(ctx, point)) {
    x = JS_GetPropertyUint32(ctx, point, 0);
    y = JS_GetPropertyUint32(ctx, point, 1);
  } else if(JS_IsObject(point)) {
    x = JS_GetPropertyStr(ctx, point, "x");
    y = JS_GetPropertyStr(ctx, point, "y");
  }
  if(JS_IsNumber(x) && JS_IsNumber(y)) {
    ret &= js_number_read(ctx, x, &out->x);
    ret &= js_number_read(ctx, y, &out->y);
  } else {
    ret = 0;
  }
  if(!JS_IsUndefined(x))
    JS_FreeValue(ctx, x);
  if(!JS_IsUndefined(y))
    JS_FreeValue(ctx, y);
  return ret;
}

template<class T>
static inline void
js_point_write(JSContext* ctx, JSValueConst out, const JSPointData<T>& in) {
  JSValue x = js_number_new<T>(ctx, in.x);
  JSValue y = js_number_new<T>(ctx, in.y);

  if(js_is_array_like(ctx, out)) {
    JS_SetPropertyUint32(ctx, out, 0, x);
    JS_SetPropertyUint32(ctx, out, 1, y);

  } else if(JS_IsObject(out)) {
    JS_SetPropertyStr(ctx, out, "x", x);
    JS_SetPropertyStr(ctx, out, "y", y);
  } else if(JS_IsFunction(ctx, out)) {
    JSValueConst args[2];
    args[0] = x;
    args[1] = y;
    JS_Call(ctx, out, JS_UNDEFINED, 2, args);
  }
  JS_FreeValue(ctx, x);
  JS_FreeValue(ctx, y);
}

static inline JSPointData<double>
js_point_get(JSContext* ctx, JSValueConst point) {
  JSPointData<double> r;
  js_point_read(ctx, point, &r);
  return r;
}

static inline bool
js_is_point(JSContext* ctx, JSValueConst point) {
  JSPointData<double> r;

  if(js_point_data(ctx, point))
    return true;

  if(js_point_read(ctx, point, &r))
    return true;

  return false;
}

extern "C" int js_point_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_POINT_HPP) */