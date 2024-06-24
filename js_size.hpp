#ifndef JS_SIZE_HPP
#define JS_SIZE_HPP

#include "jsbindings.hpp"
#include <quickjs.h>

extern "C" int js_size_init(JSContext*, JSModuleDef*);

extern "C" {
extern JSValue size_proto, size_class;
extern thread_local JSClassID js_size_class_id;

JSValue js_size_wrap(JSContext* ctx, const JSSizeData<double>& size);
JSSizeData<double>* js_size_data(JSValueConst val);
JSSizeData<double>* js_size_data2(JSContext*, JSValueConst val);

int js_size_init(JSContext*, JSModuleDef* m);
JSModuleDef* js_init_module_size(JSContext*, const char*);
void js_size_constructor(JSContext* ctx, JSValue parent, const char* name);
}
JSValue js_size_new(JSContext* ctx, double w, double h);

template<class T>
static inline JSValue
js_size_new(JSContext* ctx, const JSSizeData<T>& size) {
  return js_size_new(ctx, size.width, size.height);
}

template<class T>
static inline int
js_size_read(JSContext* ctx, JSValueConst size, JSSizeData<T>* out) {
  int ret = 1;
  JSValue width = JS_UNDEFINED, height = JS_UNDEFINED;

  if(js_is_array_like(ctx, size)) {
    width = JS_GetPropertyUint32(ctx, size, 0);
    height = JS_GetPropertyUint32(ctx, size, 1);
  } else if(JS_IsObject(size)) {
    width = JS_GetPropertyStr(ctx, size, "width");
    height = JS_GetPropertyStr(ctx, size, "height");
  }
  if(JS_IsNumber(width) && JS_IsNumber(height)) {
    ret &= js_number_read(ctx, width, &out->width);
    ret &= js_number_read(ctx, height, &out->height);
  } else {
    ret = 0;
  }
  if(!JS_IsUndefined(width))
    JS_FreeValue(ctx, width);
  if(!JS_IsUndefined(height))
    JS_FreeValue(ctx, height);
  return ret;
}

template<class T>
static inline void
js_size_write(JSContext* ctx, JSValueConst out, const JSSizeData<T>& in) {
  JSValue width = js_number_new<T>(ctx, in.width);
  JSValue height = js_number_new<T>(ctx, in.height);
  if(JS_IsArray(ctx, out)) {
    JS_SetPropertyUint32(ctx, out, 0, width);
    JS_SetPropertyUint32(ctx, out, 1, height);
  } else if(JS_IsObject(out)) {
    JS_SetPropertyStr(ctx, out, "x", width);
    JS_SetPropertyStr(ctx, out, "y", height);
  } else if(JS_IsFunction(ctx, out)) {
    JSValueConst args[2];
    args[0] = width;
    args[1] = height;
    JS_Call(ctx, out, JS_UNDEFINED, 2, args);
  }
  JS_FreeValue(ctx, width);
  JS_FreeValue(ctx, height);
}

static inline JSSizeData<double>
js_size_get(JSContext* ctx, JSValueConst size) {
  JSSizeData<double> r = {0, 0};
  js_size_read(ctx, size, &r);
  return r;
}

extern "C" int js_size_init(JSContext*, JSModuleDef*);

template<class T>
static inline BOOL
js_size_arg(JSContext* ctx, int argc, JSValueConst argv[], int& argind, JSSizeData<T>& size) {
  if(argind < argc && js_size_read(ctx, argv[argind], &size)) {
    ++argind;
    return TRUE;
  }

  if(argind + 1 < argc && js_number_read(ctx, argv[argind], &size.width) && js_number_read(ctx, argv[argind + 1], &size.height)) {
    argind += 2;
    return TRUE;
  }

  return FALSE;
}

#endif /* defined(JS_SIZE_HPP) */
