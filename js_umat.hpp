#ifndef JS_UMAT_HPP
#define JS_UMAT_HPP

#include "js_alloc.hpp"
#include "include/js_array.hpp"
#include "js_contour.hpp"
#include "js_line.hpp"
#include "js_mat.hpp"
#include "js_typed_array.hpp"
#include "include/jsbindings.hpp"
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>
#include <quickjs.h>
#include <stddef.h>
#include <cstdint>
#include <new>
#include <vector>

typedef cv::UMat JSUMatData;

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

static inline JSValue
js_umat_wrap(JSContext* ctx, const cv::UMat& umat) {
  JSValue ret = JS_NewObjectProtoClass(ctx, umat_proto, js_umat_class_id);
  JSUMatData* s = js_allocate<cv::UMat>(ctx);

  new(s) cv::UMat(umat);

  JS_SetOpaque(ret, s);
  return ret;
}

#endif /* defined(JS_UMAT_HPP) */
