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

extern "C" int js_umat_init(JSContext*, JSModuleDef*);

extern "C" {

extern thread_local JSValue umat_proto, umat_class;
extern thread_local JSClassID js_umat_class_id;
}

JSValue js_umat_new(JSContext*, uint32_t, uint32_t, int);
int js_umat_init(JSContext*, JSModuleDef*);
JSModuleDef* js_init_umat_module(JSContext* ctx, const char* module_name);
void js_umat_constructor(JSContext* ctx, JSValue parent, const char* name);

JSUMatData* js_umat_data2(JSContext* ctx, JSValueConst val);
JSUMatData* js_umat_data(JSValueConst val);

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

template<class T>
void
copy_to_vector(TypedArrayProps& props, std::vector<T>& vec) {
  TypedArrayRange<T> range(props);

  vec.resize(props.size());
  std::copy(range.begin(), range.end(), vec.begin());
}

template<class T>
static inline JSInputArray
typed_input_array(TypedArrayProps& prop) {
  T* ptr = prop.ptr<T>();
  size_t sz = prop.size<T>();
  return JSInputArray(ptr, sz);
}

static inline JSInputArray
js_input_array(JSContext* ctx, JSValueConst value) {
  cv::Mat* mat;
  cv::UMat* umat;

  if((umat = js_umat_data(value)))
    return JSInputArray(*umat);
  if((mat = js_mat_data_nothrow(value)))
    return JSInputArray(*mat);

  if(js_contour_class_id) {
    JSContourData<double>* contour;
    if((contour = js_contour_data(value))) {
      cv::Mat m = contour_getmat(*contour);
      return JSInputArray(m);
    }
  }

  if(js_is_typedarray(ctx, value)) {
    TypedArrayProps props = js_typedarray_props(ctx, value);
    TypedArrayValue type = js_typedarray_type(ctx, value);

    switch(type) {
      case TYPEDARRAY_UINT8: return typed_input_array<uint8_t>(props);
      case TYPEDARRAY_INT8: return typed_input_array<int8_t>(props);
      case TYPEDARRAY_UINT16: return typed_input_array<uint16_t>(props);
      case TYPEDARRAY_INT16: return typed_input_array<int16_t>(props);
      case TYPEDARRAY_FLOAT32: return typed_input_array<float>(props);
      case TYPEDARRAY_FLOAT64: return typed_input_array<double>(props);
    }
  } else if(js_is_array(ctx, value)) {
    std::vector<double> arr;
    cv::Scalar scalar;
    js_array_to(ctx, value, arr);

    if(arr.size() >= 2 && arr.size() <= 4) {
      for(size_t i = 0; i < arr.size(); i++)
        scalar[i] = arr[i];
      return JSInputArray(scalar);
    } else {
      return JSInputArray(arr);
    }
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
      return JSInputOutputArray(*contour);
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

static inline JSOutputArray
js_cv_outputarray(JSContext* ctx, JSValueConst value) {
  cv::Mat* mat;
  cv::UMat* umat;

  if((mat = js_mat_data_nothrow(value)))
    return JSOutputArray(*mat);
  if((umat = js_umat_data(value)))
    return JSOutputArray(*umat);

  if(js_contour_class_id) {
    JSContourData<double>* contour;
    JSContoursData<double>* contours;
    if((contour = js_contour_data(value)))
      return JSOutputArray(*contour);
  }

  if(js_line_class_id) {
    JSLineData<double>* line;
    if((line = js_line_data(value)))
      return JSOutputArray(line->array);
  }

  if(js_is_arraybuffer(ctx, value)) {
    uint8_t* ptr;
    size_t size;
    ptr = JS_GetArrayBuffer(ctx, &size, value);

    return JSOutputArray(ptr, size);
  }

  if(js_is_typedarray(ctx, value))
    return js_typedarray_inputoutputarray(ctx, value);

  return cv::noArray();
}

/*template<typename  A =JSInputOutputArray>
 class JSOutputArg   {
public:
  JSOutputArg() : array() {}

template<typename T>
  JSOutputArray(const T& arg) : array(arg) {}
template<typename T>
  JSOutputArray( T* ptr, size_t size) : array(ptr,size) {}

  A array;

};*/

class JSInputArg {
public:
  JSInputArray m_array;

  JSInputArg(JSContext* ctx, JSValueConst arg) : m_array(js_input_array(ctx, arg)) {}
  /* JSInputArg() : array() {}

 template<typename T>
   JSInputArg(const T& arg) : array(arg) {}
 template<typename T>
   JSInputArg( T* ptr, size_t size) : array(ptr,size) {}*/
};
#endif /* defined(JS_UMAT_HPP) */
