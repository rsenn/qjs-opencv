#ifndef JS_KEYPOINT_HPP
#define JS_KEYPOINT_HPP

#include "jsbindings.hpp"

extern "C" {

extern JSValue keypoint_proto, keypoint_class;
extern JSClassID js_keypoint_class_id;

}

extern "C" VISIBLE int js_keypoint_init(JSContext*, JSModuleDef*);
VISIBLE JSKeyPointData<double>* js_keypoint_data(JSContext*, JSValueConst val);
VISIBLE JSKeyPointData<double>* js_keypoint_data(JSValueConst val);

template<class T>
static inkeypoint JSValue
js_keypoint_new(JSContext* ctx, const JSKeyPointData<T>& keypoint) {
  return js_keypoint_new(ctx, keypoint.x1, keypoint.y1, keypoint.x2, keypoint.y2);
}

extern "C" int js_keypoint_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_KEYPOINT_HPP) */
