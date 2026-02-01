#ifndef JS_ARRAY_HPP
#define JS_ARRAY_HPP

#include "cutils.h"
#include "js_mat.hpp"
#include "js_point.hpp"
#include "js_rect.hpp"
#include "js_line.hpp"
#include "jsbindings.hpp"
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>
#include <quickjs.h>
#include <stddef.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

extern "C" int JS_DeletePropertyInt64(JSContext*, JSValueConst, int64_t, int);

static inline int64_t
js_array_length(JSContext* ctx, const JSValueConst& arr) {
  int64_t ret = -1;

  if(js_is_object(arr)) {
    uint32_t len;
    JSValue v = JS_GetPropertyStr(ctx, arr, "length");

    if(JS_IsNumber(v)) {
      JS_ToUint32(ctx, &len, v);
      JS_FreeValue(ctx, v);
      ret = len;
    }
  }

  return ret;
}

static inline int64_t
js_array_truncate(JSContext* ctx, const JSValueConst& arr, int64_t len) {
  int64_t newlen = -1;

  if(js_is_array(ctx, arr)) {
    int64_t top = js_array_length(ctx, arr);

    newlen = std::min(top, len < 0 ? top + len : len);

    while(--top >= newlen)
      JS_DeletePropertyInt64(ctx, arr, top, 0);

    JS_SetPropertyStr(ctx, arr, "length", JS_NewInt64(ctx, newlen));
  }

  return newlen;
}

static inline BOOL
js_array_clear(JSContext* ctx, const JSValueConst& arr) {
  int64_t newlen = -1;

  if(js_is_array(ctx, arr)) {
    int64_t top = js_array_length(ctx, arr);
    JSValueConst args[] = {JS_NewInt64(ctx, 0), JS_NewInt64(ctx, top)};

    JSValue ret = js_function_invoke(ctx, arr, "splice", countof(args), args);
    JS_FreeValue(ctx, ret);
    return TRUE;
  }

  return FALSE;
}

template<class T> class js_array {
public:
  static int64_t to_vector(JSContext* ctx, JSValueConst arr, std::vector<T>& out) {
  /*  if(!js_is_array(ctx, arr))
      return -1;*/

    int64_t i, n = js_array_length(ctx, arr);

    out.reserve(out.size() + n);

    for(i = 0; i < n; i++) {
      T value;
      JSValue item = JS_GetPropertyUint32(ctx, arr, (uint32_t)i);

      if(!js_value_to(ctx, item, value)) {
        JS_FreeValue(ctx, item);
        out.clear();
        return -1;
      }

      out.push_back(value);
      JS_FreeValue(ctx, item);
    }

    return n;
  }

  template<int N> static int64_t to_cvvector(JSContext* ctx, JSValueConst arr, cv::Vec<T, N>& out) {
    /*if(!js_is_array(ctx, arr))
      return -1;*/

    int64_t i, n = js_array_length(ctx, arr);

    if(n > N)
      n = N;

    for(i = 0; i < n; i++) {
      JSValue item = JS_GetPropertyUint32(ctx, arr, (uint32_t)i);

      js_value_to<T>(ctx, item, out[i]);

      JS_FreeValue(ctx, item);
    }

    return N;
  }

  template<class Container> static JSValue from(JSContext* ctx, const Container& in) {
    return from_sequence<typename Container::const_iterator>(ctx, in.begin(), in.end());
  }

  static JSValue from_vector(JSContext* ctx, const std::vector<T>& in) { return from_sequence(ctx, in.begin(), in.end()); }

  template<int N> static JSValue from_vector(JSContext* ctx, const cv::Vec<T, N>& in) { return from_sequence(ctx, in.begin(), in.end()); }

  template<class Iterator> static size_t copy_sequence(JSContext* ctx, JSValueConst arr, const Iterator& start, const Iterator& end) {
    size_t i = 0;

    for(Iterator it = start; it != end; ++it) {
      JSValue item = js_value_from(ctx, *it);
      JS_SetPropertyUint32(ctx, arr, i, item);
      ++i;
    }

    return i;
  }

  template<class Iterator> static JSValue from_sequence(JSContext* ctx, const Iterator& start, const Iterator& end) {
    JSValue arr = JS_NewArray(ctx);
    copy_sequence(ctx, arr, start, end);
    return arr;
  }

  template<size_t N> static int64_t to_array(JSContext* ctx, JSValueConst arr, std::array<T, N>& out);
  static int64_t to_scalar(JSContext* ctx, JSValueConst arr, cv::Scalar_<T>& out);
};

template<class T>
template<size_t N>
int64_t
js_array<T>::to_array(JSContext* ctx, JSValueConst arr, std::array<T, N>& out) {
  std::vector<T> tmp;

  to_vector(ctx, arr, tmp);

  if(tmp.size() < N)
    return -1;

  for(size_t i = 0; i < N; i++)
    out[i] = tmp[i];

  return N;
}

template<class T>
int64_t
js_array<T>::to_scalar(JSContext* ctx, JSValueConst arr, cv::Scalar_<T>& out) {
  size_t n;
  std::vector<T> tmp;

  to_vector(ctx, arr, tmp);

  if((n = tmp.size()) < 4)
    tmp.resize(4);

  for(size_t i = 0; i < 4; i++)
    out[i] = tmp[i];

  return n;
}

template<> class js_array<JSValue> {
public:
  static int64_t to_vector(JSContext* ctx, JSValueConst arr, std::vector<JSValue>& out) {
    if(!js_is_array(ctx, arr))
      return -1;

    int64_t i, n = js_array_length(ctx, arr);
    out.reserve(out.size() + n);

    for(i = 0; i < n; i++) {
      JSValue value = JS_GetPropertyUint32(ctx, arr, (uint32_t)i);
      out.push_back(value);
    }

    return n;
  }

  template<class Iterator> static size_t copy_sequence(JSContext* ctx, JSValueConst arr, const Iterator& start, const Iterator& end) {
    size_t i = 0;

    for(Iterator it = start; it != end; ++it) {
      JS_SetPropertyUint32(ctx, arr, i, *it);
      ++i;
    }

    return i;
  }

  template<class Iterator> static JSValue from_sequence(JSContext* ctx, const Iterator& start, const Iterator& end) {
    JSValue arr = JS_NewArray(ctx);
    copy_sequence(ctx, arr, start, end);
    return arr;
  }

  template<size_t N> static int64_t to_array(JSContext* ctx, JSValueConst arr, std::array<JSValue, N>& out);
};

template<size_t N>
int64_t
js_array<JSValue>::to_array(JSContext* ctx, JSValueConst arr, std::array<JSValue, N>& out) {
  int64_t i;

  if(!js_is_array(ctx, arr))
    return -1;

  for(i = 0; i < N; i++) {
    JSValue value = JS_GetPropertyUint32(ctx, arr, (uint32_t)i);
    out[i] = value;
  }

  return i;
}

template<class T>
static inline int64_t
js_array_to(JSContext* ctx, JSValueConst arr, std::vector<T>& out) {
  out.clear();
  return js_array<T>::to_vector(ctx, arr, out);
}

template<class T, int N>
static inline int64_t
js_array_to(JSContext* ctx, JSValueConst arr, cv::Vec<T, N>& out) {
  return js_array<T>::to_cvvector(ctx, arr, out);
}

template<class T, int rows, int cols>
static inline int64_t
js_array_to(JSContext* ctx, JSValueConst arr, cv::Matx<T, rows, cols>& mat) {
  int64_t y, len = js_array_length(ctx, arr);
  std::array<T, cols>* rowptr = reinterpret_cast<std::array<T, cols>*>(mat_ptr(mat));

  for(y = 0; y < len; y++) {
    JSValue row = JS_GetPropertyUint32(ctx, arr, y);

    if(cols > js_array_to(ctx, row, rowptr[y]))
      break;
  }

  return y;
}

template<class T, size_t N>
static inline int64_t
js_array_to(JSContext* ctx, JSValueConst arr, std::array<T, N>& out) {
  return js_array<T>::to_array(ctx, arr, out);
}

template<class T>
static inline int64_t
js_array_to(JSContext* ctx, JSValueConst arr, cv::Scalar_<T>& out) {
  return js_array<T>::to_scalar(ctx, arr, out);
}

template<class T, int N>
static inline JSValue
js_array_from(JSContext* ctx, const cv::Vec<T, N>& v) {
  JSValue ret = JS_NewArray(ctx);

  for(size_t i = 0; i < N; i++)
    JS_SetPropertyUint32(ctx, ret, i, js_value_from(ctx, v[i]));

  return ret;
}

template<class T, int rows, int cols>
static inline JSValue
js_array_from(JSContext* ctx, const cv::Matx<T, rows, cols>& mat) {
  JSValue ret = JS_NewArray(ctx);

  for(size_t y = 0; y < rows; y++) {
    JSValue row = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, ret, y, row);

    for(size_t x = 0; x < cols; x++)
      JS_SetPropertyUint32(ctx, row, x, js_value_from(ctx, mat(y, x)));
  }

  return ret;
}

template<class Iterator>
static inline typename std::enable_if<std::is_pointer<Iterator>::value, JSValue>::type
js_array_from(JSContext* ctx, const Iterator& start, const Iterator& end) {
  return js_array<typename std::remove_pointer<Iterator>::type>::from_sequence(ctx, start, end);
}

template<class Iterator>
static inline typename std::enable_if<Iterator::value_type, JSValue>::type
js_array_from(JSContext* ctx, const Iterator& start, const Iterator& end) {
  return js_array<typename Iterator::value_type>::from_sequence(ctx, start, end);
}

template<class Container>
static inline JSValue
js_array_from(JSContext* ctx, const Container& v) {
  return js_array<typename Container::value_type>::from_sequence(ctx, v.begin(), v.end());
}

template<class Iterator>
static inline typename std::enable_if<std::is_pointer<Iterator>::value, BOOL>::type
js_array_copy(JSContext* ctx, JSValueConst array, const Iterator& start, const Iterator& end) {
  return js_array<typename std::remove_pointer<Iterator>::type>::copy_sequence(ctx, array, start, end);
}

template<class Iterator>
static inline typename std::enable_if<Iterator::value_type, BOOL>::type
js_array_copy(JSContext* ctx, JSValueConst array, const Iterator& start, const Iterator& end) {
  return js_array<typename Iterator::value_type>::copy_sequence(ctx, array, start, end);
}

template<class Container>
static inline void
js_array_copy(JSContext* ctx, JSValueConst array, const Container& v) {
  js_array<typename Container::value_type>::copy_sequence(ctx, array, v.begin(), v.end());
}

#endif /* defined(JS_ARRAY_HPP) */
