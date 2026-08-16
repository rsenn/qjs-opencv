#ifndef JS_INPUTOUTPUTARRAY_HPP
#define JS_INPUTOUTPUTARRAY_HPP

#include "include/js_array.hpp"
#include "include/js_typed_array.hpp"
#include "include/jsbindings.hpp"
#include "js_umat.hpp"
#include "js_vector.hpp"

#include <opencv2/core.hpp>

JSInputArray js_vector_inputarray(JSValueConst value);
JSInputOutputArray js_vector_inputoutputarray(JSValueConst value);

static inline JSInputArray
js_cv_inputarray(JSContext* ctx, JSValueConst value) {
  cv::Mat* mat;
  cv::UMat* umat;

  if((umat = js_umat_data(value)))
    return JSInputArray(*umat);

  if((mat = js_mat_data_nothrow(value)))
    return JSInputArray(*mat);

  JSInputArray inputArray = js_vector_inputarray(value);

  if(inputArray.kind() != JSInputArray::NONE)
    return inputArray;

  if(js_is_typedarray(ctx, value)) {
    TypedArrayProps props = js_typedarray_props(ctx, value);
    TypedArrayValue type = js_typedarray_type(ctx, value);

    switch(type) {
      case TYPEDARRAY_UINT8: return typed_input_array<uint8_t>(props);
      case TYPEDARRAY_INT8: return typed_input_array<int8_t>(props);
      case TYPEDARRAY_UINT16: return typed_input_array<uint16_t>(props);
      case TYPEDARRAY_INT16: return typed_input_array<int16_t>(props);
      case TYPEDARRAY_UINT32: JS_ThrowTypeError(ctx, "No cv::InputArray for uint32_t"); break;
      case TYPEDARRAY_INT32: return typed_input_array<int32_t>(props);
      case TYPEDARRAY_FLOAT32 | TYPEDARRAY_SIGNED:
      case TYPEDARRAY_FLOAT32: return typed_input_array<float>(props);
      case TYPEDARRAY_FLOAT64 | TYPEDARRAY_SIGNED:
      case TYPEDARRAY_FLOAT64:

        if(props.size() == 4) {
          cv::Scalar* sc = reinterpret_cast<cv::Scalar*>(props.ptr<double>());
          return JSInputArray(*sc);
        }

        return typed_input_array<double>(props);

      case TYPEDARRAY_BIGUINT64: JS_ThrowTypeError(ctx, "No cv::InputArray for uint64_t"); break;
      case TYPEDARRAY_BIGINT64: JS_ThrowTypeError(ctx, "No cv::InputArray for int64_t"); break;
      default: JS_ThrowTypeError(ctx, "No cv::InputArray for %s", JS_ToCString(ctx, value)); break;
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

static inline JSInputOutputArray
js_cv_inputoutputarray(JSContext* ctx, JSValueConst value) {
  cv::Mat* mat;
  cv::UMat* umat;

  if((mat = js_mat_data_nothrow(value)))
    return JSInputOutputArray(*mat);

  if((umat = js_umat_data(value)))
    return JSInputOutputArray(*umat);

  JSInputOutputArray inputOutputArray = js_vector_inputoutputarray(value);

  if(inputOutputArray.kind() != JSInputOutputArray::NONE)
    return inputOutputArray;

  if(js_line_class_id) {
    JSLineData<double>* line;
    if((line = js_line_data(value)))
      return JSInputOutputArray(line->array);
  }

  if(js_is_arraybuffer(ctx, value)) {
    size_t size;
    uint8_t* ptr = JS_GetArrayBuffer(ctx, &size, value);

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

  JSOutputArray outputArray = js_vector_inputoutputarray(value);

  if(outputArray.kind() != JSOutputArray::NONE)
    return outputArray;

  if(js_line_class_id) {
    JSLineData<double>* line;

    if((line = js_line_data(value)))
      return JSOutputArray(line->array);
  }

  if(js_is_arraybuffer(ctx, value)) {
    size_t size;
    uint8_t* ptr = JS_GetArrayBuffer(ctx, &size, value);

    return JSOutputArray(ptr, size);
  }

  if(js_is_typedarray(ctx, value))
    return js_typedarray_inputoutputarray(ctx, value);

  return cv::noArray();
}

#endif // JS_INPUTOUTPUTARRAY_HPP
