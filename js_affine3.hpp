#ifndef JS_AFFINE3_HPP
#define JS_AFFINE3_HPP

#include "jsbindings.hpp"
#include <quickjs.h>
#include <array>
#include <opencv2/core/affine.hpp>

extern "C" {

extern JSValue affine3_proto, affine3_class;
extern thread_local VISIBLE JSClassID js_affine3_class_id;

VISIBLE JSValue js_affine3_new(JSContext* ctx);
JSModuleDef* js_init_module_affine3(JSContext*, const char*);

VISIBLE cv::Affine3<double>* js_affine3_data2(JSContext*, JSValueConst val);
VISIBLE cv::Affine3<double>* js_affine3_data(JSValueConst val);

VISIBLE int js_affine3_init(JSContext*, JSModuleDef*);
}

extern "C" int js_affine3_init(JSContext*, JSModuleDef*);

VISIBLE JSValue js_affine3_wrap(JSContext* ctx, const cv::Affine3<double>& affine3);

VISIBLE cv::Affine3<double>* js_affine3_data(JSValueConst val);
VISIBLE cv::Affine3<double>* js_affine3_data2(JSContext*, JSValueConst val);

#endif /* defined(JS_AFFINE3_HPP) */
