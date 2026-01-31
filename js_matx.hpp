#ifndef JS_MATX_HPP
#define JS_MATX_HPP

#include "jsbindings.hpp"
#include "util.hpp"
#include <quickjs.h>
#include <opencv2/core/types.hpp>

template<class T, size_t rows, size_t cols> using JSMatxData = cv::Matx<T, rows, cols>;

extern "C" int js_matx_init(JSContext*, JSModuleDef*);

extern "C" {
int js_matx_init(JSContext*, JSModuleDef*);
}

template<class T, size_t rows, size_t cols>
static inline JSValue
js_matx_new(JSContext* ctx, const JSMatxData<T, rows, cols>& matx) {
  JSValue ret = JS_NewArray(ctx);

  for(size_t y = 0; y < rows; ++y) {
    JSValue row = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, ret, y, row);

    for(size_t x = 0; x < cols; ++x) {
      JS_SetPropertyUint32(ctx, row, x, js_value_from<T>(ctx, matx(y, x)));
    }
    JS_FreeValue(ctx, row);
  }

  return ret;
}

template<class T, size_t rows, size_t cols>
static inline int
js_matx_read(JSContext* ctx, JSValueConst matx, JSMatxData<T, rows, cols>* out) {
  for(size_t y = 0; y < rows; ++y) {
    JSValue row = JS_GetPropertyUint32(ctx, matx, y);

    for(size_t x = 0; x < cols; ++x) {
      JSValue col = JS_GetPropertyUint32(ctx, row, x);

      if(js_value_to(ctx, col, out->operator()(y, x)))
        return 1;

      JS_FreeValue(ctx, col);
    }

    JS_FreeValue(ctx, row);
  }

  return 0;
}

template<class T, size_t rows, size_t cols>
static inline int
js_value_to(JSContext* ctx, JSValueConst value, JSMatxData<T, rows, cols>& matx) {
  return js_matx_read(ctx, value, &matx);
}

template<class T, size_t rows, size_t cols>
static inline JSValue
js_value_from(JSContext* ctx, const JSMatxData<T, rows, cols>& matx) {
  return js_matx_new(ctx, matx);
}

extern "C" int js_matx_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_MATX_HPP) */
