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

/**
 * @brief Look up a JS Mat/UMat and wrap it as ArrayT (JSInputArray,
 * JSInputOutputArray or JSOutputArray - all zero-copy aliases). Shared by
 * every JS-argument-to-cv::_InputArray-family resolver in this file.
 */
template<class ArrayT>
static inline bool
js_mat_umat_array(JSValueConst value, ArrayT& out) {
  cv::Mat* mat;
  cv::UMat* umat;

  if((mat = js_mat_data_nothrow(value))) {
    out = ArrayT(*mat);
    return true;
  }

  if((umat = js_umat_data(value))) {
    out = ArrayT(*umat);
    return true;
  }

  return false;
}

static inline JSInputArray
js_cv_inputarray(JSContext* ctx, JSValueConst value) {
  JSInputArray arr;

  if(js_mat_umat_array(value, arr))
    return arr;

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

/**
 * @brief Shared resolver behind js_cv_inputoutputarray()/js_cv_outputarray() -
 * those two differ only in the wrapper type they hand back (JSInputOutputArray
 * vs JSOutputArray), everything else about resolving a JS value to a mutable
 * cv array is identical.
 */
template<class ArrayT>
static inline ArrayT
js_cv_output_argument(JSContext* ctx, JSValueConst value) {
  ArrayT arr;

  if(js_mat_umat_array(value, arr))
    return arr;

  ArrayT vectorArray = js_vector_inputoutputarray(value);

  if(vectorArray.kind() != ArrayT::NONE)
    return vectorArray;

  if(js_line_class_id) {
    JSLineData<double>* line;
    if((line = js_line_data(value)))
      return ArrayT(line->array);
  }

  if(js_is_arraybuffer(ctx, value)) {
    size_t size;
    uint8_t* ptr = JS_GetArrayBuffer(ctx, &size, value);

    return ArrayT(ptr, size);
  }

  if(js_is_typedarray(ctx, value))
    return js_typedarray_inputoutputarray(ctx, value);

  return cv::noArray();
}

static inline JSInputOutputArray
js_cv_inputoutputarray(JSContext* ctx, JSValueConst value) {
  return js_cv_output_argument<JSInputOutputArray>(ctx, value);
}

static inline JSOutputArray
js_cv_outputarray(JSContext* ctx, JSValueConst value) {
  return js_cv_output_argument<JSOutputArray>(ctx, value);
}

/**
 * @brief Storage for JSInputArgument<T>/JSOutputArgument<T>'s owned fallback
 * vector. Must be a base class listed before JSInputArray/JSInputOutputArray
 * in those templates (base classes construct in declaration order, ahead of
 * any data members) so `vec`/`owns` already exist by the time the
 * InputArray/InputOutputArray base's constructor runs and takes a reference
 * to `vec`.
 */
template<class T>
struct JSArgumentStorage {
  std::vector<T> vec;
  bool owns = false;
};

/**
 * @brief Shared resolver behind JSInputArgument<T>/JSOutputArgument<T>:
 * a Mat/UMat, a matching JSVector<T> or an ArrayBuffer/TypedArray all
 * resolve to a zero-copy alias (ArrayT constructed straight over their
 * backing memory). Anything else (a plain JS array/array-like) falls back
 * to `vec`, an owned std::vector<T> built by iterating the JS value -
 * unless `owns` is null, meaning the caller (JSInputArgument, which is
 * read-only and has nothing to write back) doesn't need to know a fallback
 * happened, just the converted data.
 */
template<class T, class ArrayT>
static inline ArrayT
js_argument_array(JSContext* ctx, JSValueConst val, std::vector<T>& vec, bool* owns = nullptr) {
  ArrayT arr;
  JSVector<T>* vector;

  if(js_mat_umat_array(val, arr))
    return arr;

  if((vector = JSVector<T>::fromJS(val)))
    return ArrayT(*vector->vec);

  if(js_is_arraybuffer(ctx, val)) {
    size_t size;
    uint8_t* ptr = JS_GetArrayBuffer(ctx, &size, val);
    return ArrayT(reinterpret_cast<T*>(ptr), int(size / sizeof(T)));
  }

  if(js_is_typedarray(ctx, val)) {
    TypedArrayProps props = js_typedarray_props(ctx, val);
    return ArrayT(props.ptr<T>(), props.size<T>());
  }

  if(owns)
    *owns = true;
  else
    js_array_to(ctx, val, vec);

  return ArrayT(vec);
}

/**
 * @brief InputArray built from a JS argument of element type T: a Mat/UMat,
 * a matching JSVector<T> (aliased, zero-copy), an ArrayBuffer/TypedArray
 * (aliased by pointer), or otherwise a plain JS array/array-like, whose
 * elements are converted once into an owned std::vector<T>.
 */
template<class T>
class JSInputArgument : private JSArgumentStorage<T>, public JSInputArray {
public:
  JSInputArgument(JSContext* ctx, JSValueConst val) : JSInputArray(js_argument_array<T, JSInputArray>(ctx, val, this->vec)) {}
};

/**
 * @brief OutputArray/InputOutputArray built from a JS argument of element
 * type T: a Mat/UMat, a matching JSVector<T>, or an ArrayBuffer/TypedArray
 * is aliased directly (zero-copy, mutated in place by the OpenCV call). A
 * plain JS array falls back to an owned std::vector<T> - OpenCV's
 * OutputArray machinery can't write into a plain JS Array directly - which
 * the destructor copies back into the JS array via js_array_clear()/
 * js_array_copy(), symmetric with JSInputArgument's read-side conversion.
 */
template<class T>
class JSOutputArgument : private JSArgumentStorage<T>, public JSInputOutputArray {
public:
  JSOutputArgument(JSContext* ctx, JSValueConst val)
      : JSInputOutputArray(js_argument_array<T, JSInputOutputArray>(ctx, val, this->vec, &this->owns)), m_ctx(ctx), m_val(val) {}

  JSOutputArgument(const JSOutputArgument&) = delete;
  JSOutputArgument& operator=(const JSOutputArgument&) = delete;

  ~JSOutputArgument() {
    if(this->owns) {
      js_array_clear(m_ctx, m_val);
      js_array_copy(m_ctx, m_val, this->vec);
    }
  }

private:
  JSContext* m_ctx;
  JSValue m_val;
};

#endif // JS_INPUTOUTPUTARRAY_HPP
