#ifndef JS_CV_HPP
#define JS_CV_HPP

#include "jsbindings.hpp"  // for VISIBLE
#include <quickjs.h>       // for JSClassID, JSContext, JSModuleDef, JSValue

extern "C" JSClassID js_mat_class_id, js_umat_class_id, js_contour_class_id;
extern "C" JSValue cv_class;

extern "C" VISIBLE int js_cv_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_CV_HPP) */