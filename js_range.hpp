#ifndef JS_RANGE_HPP
#define JS_RANGE_HPP

#include "include/jsbindings.hpp"
#include <quickjs.h>

typedef cv::Range JSRangeData;

extern "C" int js_range_init(JSContext*, JSModuleDef*);

extern "C" {
extern thread_local JSValue range_proto, range_class;
extern thread_local JSClassID js_range_class_id;

int js_range_init(JSContext*, JSModuleDef* m);
JSValue js_range_create(JSContext* ctx, JSValueConst proto);
JSModuleDef* js_init_module_range(JSContext*, const char*);

JSRangeData* js_range_data2(JSContext*, JSValueConst val);
JSRangeData* js_range_data(JSValueConst val);
}

JSValue js_range_new(JSContext* ctx, JSValueConst proto, int start, int end);

static inline JSValue
js_range_new(JSContext* ctx, int start, int end) {
  return js_range_new(ctx, range_proto, start, end);
}

JSValue js_range_clone(JSContext* ctx, JSValueConst proto, const JSRangeData& range);

static inline JSValue
js_range_clone(JSContext* ctx, const JSRangeData& range) {
  return js_range_clone(ctx, range_proto, range);
}
template<class T>
static inline JSValue
js_range_new(JSContext* ctx, const JSRangeData& range) {
  return js_range_new(ctx, range_proto, range.start, range.end);
}

template<class T>
static inline int
js_range_read(JSContext* ctx, JSValueConst range, JSRangeData* out) {
  int ret = 1;
  JSValue start = JS_UNDEFINED, end = JS_UNDEFINED;

  if(JS_IsArray(ctx, range)) {
    start = JS_GetPropertyUint32(ctx, range, 0);
    end = JS_GetPropertyUint32(ctx, range, 1);
  } else if(JS_IsObject(range)) {
    start = JS_GetPropertyStr(ctx, range, "start");
    end = JS_GetPropertyStr(ctx, range, "end");
  }

  if(JS_IsNumber(start) && JS_IsNumber(end)) {
    ret &= js_number_read(ctx, start, &out->start);
    ret &= js_number_read(ctx, end, &out->end);
  } else {
    ret = 0;
  }

  if(!JS_IsUndefined(start))
    JS_FreeValue(ctx, start);
  if(!JS_IsUndefined(end))
    JS_FreeValue(ctx, end);

  return ret;
}

template<class T>
static inline void
js_range_write(JSContext* ctx, JSValueConst out, const JSRangeData& in) {
  JSValue start = js_number_new<T>(ctx, in.start);
  JSValue end = js_number_new<T>(ctx, in.end);

  if(js_is_arraylike(ctx, out)) {
    JS_SetPropertyUint32(ctx, out, 0, start);
    JS_SetPropertyUint32(ctx, out, 1, end);
  } else if(JS_IsObject(out)) {
    JS_SetPropertyStr(ctx, out, "start", start);
    JS_SetPropertyStr(ctx, out, "end", end);
  } else if(js_is_function(ctx, out)) {
    JSValueConst args[2];
    args[0] = start;
    args[1] = end;
    JS_Call(ctx, out, JS_UNDEFINED, 2, args);
  }

  JS_FreeValue(ctx, start);
  JS_FreeValue(ctx, end);
}

static inline JSRangeData
js_range_get(JSContext* ctx, JSValueConst range) {
  JSRangeData r;
  js_range_read(ctx, range, &r);
  return r;
}

static inline bool
js_is_range(JSContext* ctx, JSValueConst range) {
  JSRangeData r;

  if(js_range_data2(ctx, range))
    return true;

  if(js_range_read(ctx, range, &r))
    return true;

  return false;
}

extern "C" int js_range_init(JSContext*, JSModuleDef*);

template<class T>
static inline int
js_value_to(JSContext* ctx, JSValueConst value, JSRangeData& rg) {
  return js_range_read(ctx, value, &rg);
}

template<class T>
static inline JSValue
js_value_from(JSContext* ctx, const JSRangeData& rg) {
  return js_range_new(ctx, rg.start, rg.end);
}

#endif /* defined(JS_RANGE_HPP) */
