#include "jsbindings.hpp"
#include "js_array.hpp"
#include "js_umat.hpp"
#include <quickjs.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>

JSOutputArgument::JSOutputArgument(JSContext* ctx, JSValueConst val) : JSInputOutputArray(js_cv_inputoutputarray(ctx, val)), m_ctx(ctx), m_val(val) {
  if(empty()) {
    if(JS_IsArray(m_ctx, m_val))
      JSInputOutputArray::assign(cv::Mat());
  }
}

JSInputArgument::JSInputArgument(JSContext* ctx, JSValueConst val) : JSInputArray(js_input_array(ctx, val)) {
}

JSImageArgument::JSImageArgument(JSContext* ctx, JSValueConst val) : JSInputOutputArray(js_umat_or_mat(ctx, val)) {
}

void
js_arraybuffer_free(JSRuntime* rt, void* opaque, void* ptr) {
  JS_FreeValueRT(rt, JS_MKPTR(JS_TAG_OBJECT, opaque));
}

/** @addtogroup color
 *  @{
 */
int
js_color_read(JSContext* ctx, JSValueConst color, JSColorData<double>* out) {
  int ret = 1;
  std::array<double, 4> c = {0, 0, 0, 255};

  if(JS_IsObject(color)) {
    JSValue v[4];

    if(js_is_array(ctx, color)) {
      v[0] = JS_GetPropertyUint32(ctx, color, 0);
      v[1] = JS_GetPropertyUint32(ctx, color, 1);
      v[2] = JS_GetPropertyUint32(ctx, color, 2);
      v[3] = JS_GetPropertyUint32(ctx, color, 3);
    } else {
      v[0] = JS_GetPropertyStr(ctx, color, "r");
      v[1] = JS_GetPropertyStr(ctx, color, "g");
      v[2] = JS_GetPropertyStr(ctx, color, "b");
      v[3] = JS_GetPropertyStr(ctx, color, "a");
    }

    if(!(JS_IsNumber(v[0]) && JS_IsNumber(v[1]) && JS_IsNumber(v[2]) /* && JS_IsNumber(v[3])*/)) {
      ret = 0;
    } else {
      JS_ToFloat64(ctx, &c[0], v[0]);
      JS_ToFloat64(ctx, &c[1], v[1]);
      JS_ToFloat64(ctx, &c[2], v[2]);

      if(JS_IsNumber(v[3]))
        JS_ToFloat64(ctx, &c[3], v[3]);
    }

    JS_FreeValue(ctx, v[0]);
    JS_FreeValue(ctx, v[1]);
    JS_FreeValue(ctx, v[2]);
    JS_FreeValue(ctx, v[3]);
  } else if(JS_IsNumber(color)) {
    uint32_t value;

    JS_ToUint32(ctx, &value, color);

    c[0] = value & 0xff;
    c[1] = (value >> 8) & 0xff;
    c[2] = (value >> 16) & 0xff;
    c[3] = (value >> 24) & 0xff;
  } else {
    ret = 0;
  }

  std::copy(c.cbegin(), c.cend(), out->arr.begin());

  return ret;
}

int
js_color_read(JSContext* ctx, JSValueConst value, JSColorData<uint8_t>* out) {
  JSColorData<double> color;

  if(js_is_array(ctx, value)) {
    std::array<uint8_t, 4> a;

    if(js_array_to(ctx, value, a) >= 3) {
      out->arr[0] = a[0];
      out->arr[1] = a[1];
      out->arr[2] = a[2];
      out->arr[3] = a[3];

      return 1;
    }
  }

  if(js_color_read(ctx, value, &color)) {
    out->arr[0] = color.arr[0];
    out->arr[1] = color.arr[1];
    out->arr[2] = color.arr[2];
    out->arr[3] = color.arr[3];

    return 1;
  }

  return 0;
}
/**
 *  @}
 */

int
js_ref(JSContext* ctx, const char* name, JSValueConst arg, JSValue value) {
  if(js_is_function(ctx, arg)) {
    JSValueConst v = value;
    JSValue ret = JS_Call(ctx, arg, JS_UNDEFINED, 1, &v);
    JS_FreeValue(ctx, ret);
  } else if(js_is_array(ctx, arg)) {
    JS_SetPropertyUint32(ctx, arg, 0, value);
  } else if(JS_IsObject(arg)) {
    JS_SetPropertyStr(ctx, arg, name, value);
  } else {
    return 0;
  }

  return 1;
}

int
js_range_read(JSContext* ctx, JSValueConst value, cv::Range* range) {
  int ret = 0;
  cv::Vec<int, 2> vec{INT_MIN, INT_MAX};

  if(js_is_array(ctx, value) && js_array_length(ctx, value) == 2)
    ret = js_value_to(ctx, value, vec);

  if(range)
    *range = cv::Range(vec[0], vec[1]);

  return ret;
}

int
js_range_size(JSContext* ctx, JSValueConst value) {
  cv::Range r;

  if(js_range_read(ctx, value, &r))
    return r.size();

  return -1;
}

bool
js_range_empty(JSContext* ctx, JSValueConst value) {
  cv::Range r;

  if(js_range_read(ctx, value, &r))
    return r.empty();

  return true;
}

bool
js_range_valid(JSContext* ctx, JSValueConst value) {
  cv::Range r;

  return js_range_read(ctx, value, &r);
}
