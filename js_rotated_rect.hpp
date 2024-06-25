#ifndef JS_ROTATED_RECT_HPP
#define JS_ROTATED_RECT_HPP

#include "jsbindings.hpp"
#include <quickjs.h>

extern "C" {
extern thread_local JSValue rotated_rect_proto, rotated_rect_class;
extern thread_local JSClassID js_rotated_rect_class_id;
}

extern "C" int js_rotated_rect_init(JSContext*, JSModuleDef*);

JSValue js_rotated_rect_new(JSContext* ctx, JSValueConst proto, const JSPointData<float>& center, const JSSizeData<float>& size, float angle);

static inline JSValue
js_rotated_rect_new(JSContext* ctx, const JSPointData<float>& center, const JSSizeData<float>& size, float angle) {
  return js_rotated_rect_new(ctx, rotated_rect_proto, center, size, angle);
}

static inline JSValue
js_rotated_rect_new(JSContext* ctx, JSValueConst proto, const JSRotatedRectData& rotated_rect) {
  return js_rotated_rect_new(ctx, proto, rotated_rect.center, rotated_rect.size, rotated_rect.angle);
}

static inline JSValue
js_rotated_rect_new(JSContext* ctx, const JSRotatedRectData& rrect) {
  return js_rotated_rect_new(ctx, rotated_rect_proto, rrect);
}

JSRotatedRectData* js_rotated_rect_data2(JSContext*, JSValueConst val);
JSRotatedRectData* js_rotated_rect_data(JSValueConst val);

#endif /* defined(JS_ROTATED_RECT_HPP) */
