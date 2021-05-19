#ifndef JS_CV_HPP
#define JS_CV_HPP

#include <opencv2/core.hpp>
#include "jsbindings.hpp"
#include "js_contour.hpp"
#include "js_typed_array.hpp"

extern "C" JSClassID js_mat_class_id, js_umat_class_id, js_contour_class_id;

extern "C" VISIBLE int js_cv_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_CV_HPP) */