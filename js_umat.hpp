#ifndef JS_UMAT_HPP
#define JS_UMAT_HPP

#include "js_alloc.hpp"
#include "js_array.hpp"
#include "js_contour.hpp"
#include "js_line.hpp"
#include "js_mat.hpp"
#include "js_typed_array.hpp"
#include "jsbindings.hpp"
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>
#include <quickjs.h>
#include <stddef.h>
#include <cstdint>
#include <new>
#include <vector>

extern "C" VISIBLE int js_umat_init(JSContext*, JSModuleDef*);

extern "C" {

extern JSValue umat_proto, umat_class;
extern JSClassID js_umat_class_id;
}

VISIBLE JSValue js_umat_new(JSContext*, uint32_t, uint32_t, int);
int js_umat_init(JSContext*, JSModuleDef*);
JSModuleDef* js_init_umat_module(JSContext* ctx, const char* module_name);
void js_umat_constructor(JSContext* ctx, JSValue parent, const char* name);

VISIBLE JSUMatData* js_umat_data2(JSContext* ctx, JSValueConst val);
VISIBLE JSUMatData* js_umat_data(JSValueConst val);

static inline JSInputOutputArray
js_umat_or_mat(JSContext* ctx, JSValueConst value) {
  cv::Mat* mat;
  cv::UMat* umat;

  if((umat = js_umat_data(value)))
    return JSInputOutputArray(*umat);
  if((mat = js_mat_data_nothrow(value)))
    return JSInputOutputArray(*mat);

  return cv::noArray();
}

static inline JSInputArray
js_input_array(JSContext* ctx, JSValueConst value) {
  cv::Mat* mat;
  cv::UMat* umat;

  if((umat = js_umat_data(value)))
    return JSInputArray(*umat);
  if((mat = js_mat_data_nothrow(value)))
    return JSInputArray(*mat);

  if(js_is_array(ctx, value)) {
    std::vector<double> arr;
    cv::Scalar scalar;
    js_array_to(ctx, value, arr);
    for(size_t i = 0; i < arr.size(); i++) scalar[i] = arr[i];
    return JSInputArray(scalar);
  }

  return cv::noArray();
}

static inline JSValue
js_umat_wrap(JSContext* ctx, const cv::UMat& umat) {
  JSValue ret;
  JSUMatData* s;
  ret = JS_NewObjectProtoClass(ctx, umat_proto, js_umat_class_id);

  s = js_allocate<cv::UMat>(ctx);

  new(s) cv::UMat(umat);

  JS_SetOpaque(ret, s);
  return ret;
}

static inline JSInputOutputArray
js_cv_inputoutputarray(JSContext* ctx, JSValueConst value) {
  cv::Mat* mat;
  cv::UMat* umat;

  if((mat = js_mat_data_nothrow(value)))
    return JSInputOutputArray(*mat);
  if((umat = js_umat_data(value)))
    return JSInputOutputArray(*umat);

  if(js_contour_class_id) {
    JSContourData<double>* contour;
    JSContoursData<double>* contours;
    if((contour = js_contour_data(value)))
      return JSInputOutputArray(cv::Mat(*contour));
  }

  if(js_line_class_id) {
    JSLineData<double>* line;
    if((line = js_line_data(value)))
      return JSInputOutputArray(line->array);
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
#endif /* defined(JS_UMAT_HPP) */