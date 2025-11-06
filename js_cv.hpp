#ifndef JS_CV_HPP
#define JS_CV_HPP

#include "jsbindings.hpp"
#include <quickjs.h>

extern "C" JSValue cv_class;

extern "C" int js_cv_init(JSContext*, JSModuleDef*);

JSValue js_cv_throw(JSContext* ctx, const cv::Exception& e);

#endif /* defined(JS_CV_HPP) */
