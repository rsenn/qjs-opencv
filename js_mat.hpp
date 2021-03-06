#ifndef JS_MAT_HPP
#define JS_MAT_HPP

#include "jsbindings.hpp"
#include <quickjs.h>
#include <cstdint>

extern "C" VISIBLE int js_mat_init(JSContext*, JSModuleDef*);

extern "C" {
extern JSValue mat_proto, mat_class;
extern thread_local VISIBLE JSClassID js_mat_class_id;

VISIBLE JSValue js_mat_new(JSContext*, uint32_t, uint32_t, int);
int js_mat_init(JSContext*, JSModuleDef*);
JSModuleDef* js_init_module_mat(JSContext*, const char*);
void js_mat_constructor(JSContext* ctx, JSValue parent, const char* name);
VISIBLE JSValue js_mat_wrap(JSContext*, const cv::Mat& mat);
}

inline VISIBLE JSMatData*
js_mat_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSMatData*>(JS_GetOpaque2(ctx, val, js_mat_class_id));
}

static inline JSMatData*
js_mat_data_nothrow(JSValueConst val) {
  return static_cast<JSMatData*>(JS_GetOpaque(val, js_mat_class_id));
}

#endif /* defined(JS_MAT_HPP) */
