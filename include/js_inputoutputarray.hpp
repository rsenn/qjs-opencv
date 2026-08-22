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

/**
 * @brief Backing storage for js_cv_inputarray()'s plain-JS-array branch.
 * cv::_InputArray never copies - its Matx (cv::Scalar) and std::vector
 * constructors store a raw pointer to the argument - so converting into a
 * local cv::Scalar/std::vector<double> hands back an _InputArray that
 * dereferences a dead stack slot (BUGS: js-cv-inputarray-scalar-dangling-
 * pointer). js_cv_inputarray() returns a bare cv::_InputArray to ~370 call
 * sites, so the storage can't live in the return value; instead each
 * conversion takes the next slot of this thread-local ring, which outlives
 * the call and is only recycled after JS_INPUTARRAY_SLOTS further
 * plain-array conversions - far more than the handful of array arguments
 * any single binding call resolves.
 */
#define JS_INPUTARRAY_SLOTS 16

struct JSInputArraySlot {
  cv::Scalar scalar;
  std::vector<double> vec;
};

static inline JSInputArraySlot&
js_inputarray_slot() {
  static thread_local JSInputArraySlot slots[JS_INPUTARRAY_SLOTS];
  static thread_local size_t next = 0;
  JSInputArraySlot& slot = slots[next];

  next = (next + 1) % JS_INPUTARRAY_SLOTS;

  return slot;
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
    JSInputArraySlot& slot = js_inputarray_slot();

    slot.vec.clear();
    js_array_to(ctx, value, slot.vec);

    if(slot.vec.size() >= 2 && slot.vec.size() <= 4) {
      slot.scalar = cv::Scalar();

      for(size_t i = 0; i < slot.vec.size(); i++)
        slot.scalar[i] = slot.vec[i];

      return JSInputArray(slot.scalar);
    } else {
      return JSInputArray(slot.vec);
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
 * @brief Storage for JSInputArrayOf<T>/JSOutputArrayOf<T>'s owned fallback
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
 * @brief Shared resolver behind JSInputArrayOf<T>/JSOutputArrayOf<T>:
 * a Mat/UMat, a matching JSVector<T> or an ArrayBuffer/TypedArray all
 * resolve to a zero-copy alias (ArrayT constructed straight over their
 * backing memory). Anything else (a plain JS array/array-like) falls back
 * to `vec`, an owned std::vector<T> built by iterating the JS value -
 * unless `owns` is null, meaning the caller (JSInputArrayOf, which is
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
class JSInputArrayOf : private JSArgumentStorage<T>, public JSInputArray {
public:
  JSInputArrayOf(JSContext* ctx, JSValueConst val) : JSInputArray(js_argument_array<T, JSInputArray>(ctx, val, this->vec)) {}
};

/**
 * @brief Call `fn(arg)`, then free both the argument and the result -
 * shared by JSOutputArrayOf<T>'s receiver-function branches below.
 */
static inline void
js_call_receiver(JSContext* ctx, JSValueConst fn, JSValue arg) {
  JSValue ret = JS_Call(ctx, fn, JS_UNDEFINED, 1, &arg);

  JS_FreeValue(ctx, arg);
  JS_FreeValue(ctx, ret);
}

/**
 * @brief OutputArray/InputOutputArray built from a JS argument of element
 * type T: a Mat/UMat, a matching JSVector<T>, or an ArrayBuffer/TypedArray
 * is aliased directly (zero-copy, mutated in place by the OpenCV call).
 * Anything else - a plain JS array, a callback function, ... - falls back
 * to an owned std::vector<T>, since OpenCV's OutputArray machinery can't
 * write into either of those directly. The destructor then hands that
 * vector's contents back to `val`:
 * - if `val` is a function and T is a plain scalar with a TypedArray
 *   counterpart (number_type<T>::typed_array, include/js_typed_array.hpp -
 *   true only for int8/16/32/64, uint8/16/32/64, float and double, i.e.
 *   exactly the "1-dimensional, single channel" element types), by calling
 *   it with a freshly built TypedArray view of the results;
 * - otherwise, if `val` is a function, by wrapping the results in a
 *   freshly constructed JSVector<T> and calling `val(vector)`;
 * - otherwise, by copying the results into `val` as a plain JS array via
 *   js_array_clear()/js_array_copy(), symmetric with JSInputArrayOf's
 *   read-side conversion.
 */
template<class T>
class JSOutputArrayOf : private JSArgumentStorage<T>, public JSInputOutputArray {
public:
  JSOutputArrayOf(JSContext* ctx, JSValueConst val)
      : JSInputOutputArray(js_argument_array<T, JSInputOutputArray>(ctx, val, this->vec, &this->owns)), m_ctx(ctx), m_val(val) {}

  JSOutputArrayOf(const JSOutputArrayOf&) = delete;
  JSOutputArrayOf& operator=(const JSOutputArrayOf&) = delete;

  ~JSOutputArrayOf() {
    if(!this->owns)
      return;

    if(js_is_function(m_ctx, m_val)) {
      if constexpr(number_type<T>::typed_array) {
        /* Not js_typedarray<T>::from_vector(m_ctx, this->vec) - it calls
         * from_sequence(vec.begin(), vec.end()), and js_typedarray_remain()
         * (used inside from_sequence) is only SFINAE-enabled for raw-pointer
         * iterators; std::vector<T>::begin()/end() return
         * __gnu_cxx::__normal_iterator, not T*, so from_vector() fails to
         * compile for any real std::vector<T> - see BUGS. Pointers sidestep
         * that entirely. */
        js_call_receiver(m_ctx, m_val, js_typedarray<T>::from_sequence(m_ctx, this->vec.data(), this->vec.data() + this->vec.size()));
      } else if(JSVector<T>::get_class_id() == 0) {
        /* A JSVector<T> class only exists for element types js_vector_init()
         * registered (Mat, Point, Point2f, Point3f, Rect, int, float, double,
         * char, string, ...). get_class_id() stays 0 for any other T, and 0
         * isn't "no class" to QuickJS - it's whatever class happened to be
         * registered first, so creating an object against it would silently
         * hand the callback an unrelated, wrong-class value instead of failing.
         *
         * A JS_Throw* here can't be turned into a catchable exception at the
         * call site either: this destructor runs while the bound native
         * function's own return value has already been fixed (during stack
         * unwind at its closing brace), so the pending exception it sets
         * doesn't get reported until some later, unrelated call happens to
         * check for one. So this is a binding-author bug (T was given the
         * function-receiver capability without a registered JSVector<T>) -
         * fail silently rather than leave a dangling, confusingly-timed
         * exception. */
      } else {
        JSVector<T>* vector = new JSVector<T>();
        *vector->vec = this->vec;

        js_call_receiver(m_ctx, m_val, vector->toJS(m_ctx));
      }
    } else {
      js_array_clear(m_ctx, m_val);
      js_array_copy(m_ctx, m_val, this->vec);
    }
  }

private:
  JSContext* m_ctx;
  JSValue m_val;
};

#endif // JS_INPUTOUTPUTARRAY_HPP
