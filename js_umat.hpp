#ifndef JS_UMAT_HPP
#define JS_UMAT_HPP

#include "jsbindings.hpp"
#include "js_alloc.hpp"
#include "js_array.hpp"
#include "js_typed_array.hpp"

extern "C" VISIBLE int js_umat_init(JSContext*, JSModuleDef*);

extern "C" {

extern JSValue umat_proto, umat_class;
extern JSClassID js_umat_class_id;

VISIBLE JSValue js_umat_new(JSContext*, uint32_t, uint32_t, int);
int js_umat_init(JSContext*, JSModuleDef*);
JSModuleDef* js_init_umat_module(JSContext* ctx, const char* module_name);
void js_umat_constructor(JSContext* ctx, JSValue parent, const char* name);

VISIBLE JSUMatData* js_umat_data(JSContext* ctx, JSValueConst val);

static inline JSInputOutputArray
js_umat_or_mat(JSContext* ctx, JSValueConst value) {
  cv::Mat* mat;
  cv::UMat* umat;

  if((umat = static_cast<cv::UMat*>(JS_GetOpaque(value, js_umat_class_id))))
    return JSInputOutputArray(*umat);
  if((mat = static_cast<cv::Mat*>(JS_GetOpaque(value, js_mat_class_id))))
    return JSInputOutputArray(*mat);

  return cv::noArray();
}

static inline JSInputArray
js_input_array(JSContext* ctx, JSValueConst value) {
  cv::Mat* mat;
  cv::UMat* umat;

  if((umat = static_cast<cv::UMat*>(JS_GetOpaque(value, js_umat_class_id))))
    return JSInputArray(*umat);
  if((mat = static_cast<cv::Mat*>(JS_GetOpaque(value, js_mat_class_id))))
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
}

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
#endif /* defined(JS_UMAT_HPP) */