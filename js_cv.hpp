#ifndef JS_CV_HPP
#define JS_CV_HPP

#include <opencv2/core.hpp>
#include "jsbindings.hpp"
#include "js_contour.hpp"
#include "js_typed_array.hpp"

extern "C" JSClassID js_mat_class_id, js_umat_class_id, js_contour_class_id;

extern "C" VISIBLE int js_cv_init(JSContext*, JSModuleDef*);

static inline JSInputOutputArray
js_cv_inputoutputarray(JSContext* ctx, JSValueConst value) {
  cv::Mat* mat;
  cv::UMat* umat;
  JSContourData<double>* contour;
  JSContoursData<double>* contours;

  if((mat = static_cast<cv::Mat*>(JS_GetOpaque(value, js_mat_class_id))))
    return JSInputOutputArray(*mat);
  if((umat = static_cast<cv::UMat*>(JS_GetOpaque(value, js_umat_class_id))))
    return JSInputOutputArray(*umat);
  if(js_contour_class_id) {
    if((contour = static_cast<JSContourData<double>*>(JS_GetOpaque(value, js_contour_class_id))))
      return JSInputOutputArray(cv::Mat(*contour));
  }

  if(js_is_arraybuffer(ctx, value)) {
    uint8_t* ptr;
    size_t size;
    ptr = JS_GetArrayBuffer(ctx, &size, value);

    return JSInputOutputArray(ptr, size);
  }

  if(js_is_typedarray(ctx, value))
    return js_typedarray_inputoutputarray(ctx, value);

  return cv::noArray();
}

template<class T>
static inline bool
js_is_noarray(const T& array) {
  return &array == &cv::noArray();
}

#endif /* defined(JS_CV_HPP) */