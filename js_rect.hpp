#ifndef JS_RECT_HPP
#define JS_RECT_HPP

#include "jsbindings.hpp"

extern "C" VISIBLE int js_rect_init(JSContext*, JSModuleDef*);

extern "C" {
extern JSValue rect_proto, rect_class;
extern JSClassID js_rect_class_id;

VISIBLE JSRectData<double>* js_rect_data(JSContext*, JSValueConst val);
VISIBLE JSValue js_rect_wrap(JSContext*, const JSRectData<double>&);
int js_rect_init(JSContext*, JSModuleDef*);
JSModuleDef* js_init_rect_module(JSContext*, const char* module_name);

void js_rect_constructor(JSContext* ctx, JSValue parent, const char* name);
}

template<class T>
static inline int
js_rect_read(JSContext* ctx, JSValueConst rect, JSRectData<T>* out) {
  int ret = 1;
  JSValue x = JS_UNDEFINED, y = JS_UNDEFINED, w = JS_UNDEFINED, h = JS_UNDEFINED;
  if(js_is_array_like(ctx, rect)) {
    x = JS_GetPropertyUint32(ctx, rect, 0);
    y = JS_GetPropertyUint32(ctx, rect, 1);
    w = JS_GetPropertyUint32(ctx, rect, 2);
    h = JS_GetPropertyUint32(ctx, rect, 3);

  } else {
    x = JS_GetPropertyStr(ctx, rect, "x");
    y = JS_GetPropertyStr(ctx, rect, "y");
    w = JS_GetPropertyStr(ctx, rect, "width");
    h = JS_GetPropertyStr(ctx, rect, "height");
  }
  if(JS_IsNumber(x) && JS_IsNumber(y) && JS_IsNumber(w) && JS_IsNumber(h)) {
    ret &= js_number_read(ctx, x, &out->x);
    ret &= js_number_read(ctx, y, &out->y);
    ret &= js_number_read(ctx, w, &out->width);
    ret &= js_number_read(ctx, h, &out->height);
  } else {
    ret = 0;
  }
  if(!JS_IsUndefined(x))
    JS_FreeValue(ctx, x);
  if(!JS_IsUndefined(y))
    JS_FreeValue(ctx, y);
  if(!JS_IsUndefined(w))
    JS_FreeValue(ctx, w);
  if(!JS_IsUndefined(h))
    JS_FreeValue(ctx, h);
  return ret;
}

static JSRectData<double>
js_rect_get(JSContext* ctx, JSValueConst rect) {
  JSRectData<double> r = {0, 0, 0, 0};
  js_rect_read(ctx, rect, &r);
  return r;
}

static inline int
js_rect_write(JSContext* ctx, JSValue out, JSRectData<double> rect) {
  int ret = 0;
  ret += JS_SetPropertyStr(ctx, out, "x", JS_NewFloat64(ctx, rect.x));
  ret += JS_SetPropertyStr(ctx, out, "y", JS_NewFloat64(ctx, rect.y));
  ret += JS_SetPropertyStr(ctx, out, "width", JS_NewFloat64(ctx, rect.width));
  ret += JS_SetPropertyStr(ctx, out, "height", JS_NewFloat64(ctx, rect.height));
  return ret;
}

static JSRectData<double>
js_rect_set(JSContext* ctx, JSValue out, double x, double y, double w, double h) {
  const JSRectData<double> r = {x, y, w, h};
  js_rect_write(ctx, out, r);
  return r;
}

extern "C" int js_rect_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_RECT_HPP) */
