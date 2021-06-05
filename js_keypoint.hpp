#ifndef JS_KEYPOINT_HPP
#define JS_KEYPOINT_HPP

#include "jsbindings.hpp"

typedef cv::KeyPoint JSKeyPointData;

extern "C" {

extern JSValue keypoint_proto, keypoint_class;
extern JSClassID js_keypoint_class_id;
}

extern "C" VISIBLE int js_keypoint_init(JSContext*, JSModuleDef*);
VISIBLE JSKeyPointData* js_keypoint_data(JSContext*, JSValueConst val);
VISIBLE JSKeyPointData* js_keypoint_data(JSValueConst val);

static inline JSValue
js_keypoint_new(JSContext* ctx, const JSKeyPointData& keypoint) {
  return js_keypoint_new(ctx, keypoint);
}

extern "C" int js_keypoint_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_KEYPOINT_HPP) */
