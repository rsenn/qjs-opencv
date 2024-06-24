#ifndef JS_KEYPOINT_HPP
#define JS_KEYPOINT_HPP

#include "jsbindings.hpp"
#include <quickjs.h>
#include <stddef.h>
#include <cstdint>
#include <opencv2/core/types.hpp>
#include <vector>

typedef cv::KeyPoint JSKeyPointData;

extern "C" {

extern JSValue keypoint_proto, keypoint_class;
extern JSClassID js_keypoint_class_id;

JSKeyPointData* js_keypoint_data2(JSContext*, JSValueConst val);
JSKeyPointData* js_keypoint_data(JSValueConst val);
}

extern "C" int js_keypoint_init(JSContext*, JSModuleDef*);

JSValue js_keypoint_new(JSContext* ctx, const JSKeyPointData& keypoint);

extern "C" int js_keypoint_init(JSContext*, JSModuleDef*);

template<> class js_array<JSKeyPointData> {
public:
  static int64_t
  to_vector(JSContext* ctx, JSValueConst arr, std::vector<JSKeyPointData>& out) {
    int64_t i, n;
    JSValue len;
    if(!js_is_array(ctx, arr))
      return -1;
    len = JS_GetPropertyStr(ctx, arr, "length");
    JS_ToInt64(ctx, &n, len);
    out.reserve(out.size() + n);
    for(i = 0; i < n; i++) {
      JSKeyPointData* kp;
      JSValue item = JS_GetPropertyUint32(ctx, arr, (uint32_t)i);
      if(!(kp = js_keypoint_data2(ctx, item))) {
        JS_FreeValue(ctx, item);
        out.clear();
        return -1;
      }
      out.push_back(*kp);
      JS_FreeValue(ctx, item);
    }

    return n;
  }

  template<class Iterator>
  static size_t
  copy_sequence(JSContext* ctx, JSValueConst arr, const Iterator& start, const Iterator& end) {
    size_t i = 0;
    for(Iterator it = start; it != end; ++it) {
      JS_SetPropertyUint32(ctx, arr, i, js_keypoint_new(ctx, *it));
      ++i;
    }

    return i;
  }
  template<class Iterator>
  static JSValue
  from_sequence(JSContext* ctx, const Iterator& start, const Iterator& end) {
    JSValue arr = JS_NewArray(ctx);
    copy_sequence(ctx, arr, start, end);
    return arr;
  }
};

#endif /* defined(JS_KEYPOINT_HPP) */
