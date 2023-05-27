#ifndef JS_MAT_HPP
#define JS_MAT_HPP

#include "jsbindings.hpp"
#include <quickjs.h>
#include <cstdint>

extern "C" int js_mat_init(JSContext*, JSModuleDef*);

extern "C" {
extern JSValue mat_proto, mat_class;
extern thread_local JSClassID js_mat_class_id;

JSValue js_mat_new(JSContext*, uint32_t, uint32_t, int);
JSModuleDef* js_init_module_mat(JSContext*, const char*);
JSValue js_mat_wrap(JSContext*, const cv::Mat& mat);
void js_mat_constructor(JSContext* ctx, JSValue parent, const char* name);
int js_mat_init(JSContext*, JSModuleDef*);
}

static inline JSMatData*
js_mat_data(JSValueConst val) {
  return static_cast<JSMatData*>(JS_GetOpaque(val, js_mat_class_id));
}

static inline JSMatData*
js_mat_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSMatData*>(JS_GetOpaque2(ctx, val, js_mat_class_id));
}

static inline JSMatData*
js_mat_data_nothrow(JSValueConst val) {
  return static_cast<JSMatData*>(JS_GetOpaque(val, js_mat_class_id));
}

#endif /* defined(JS_MAT_HPP) */
