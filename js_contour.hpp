#ifndef JS_CONTOUR_HPP
#define JS_CONTOUR_HPP

#include "geometry.hpp"
#include "js_alloc.hpp"
#include "jsbindings.hpp"
#include <quickjs.h>
#include <cstdint>
#include <new>
#include <vector>
#include <cassert>

extern "C" {
extern JSValue contour_class, contour_proto;
extern JSClassDef js_contour_class;
extern thread_local VISIBLE JSClassID js_contour_class_id;

JSValue js_contour_create(JSContext* ctx, JSValueConst proto);
void js_contour_finalizer(JSRuntime* rt, JSValue val);

JSValue js_contour_to_string(JSContext*, JSValueConst this_val, int argc, JSValueConst* argv);
int js_contour_init(JSContext*, JSModuleDef*);
JSModuleDef* js_init_module_contour(JSContext*, const char*);
void js_contour_constructor(JSContext* ctx, JSValue parent, const char* name);

VISIBLE JSContourData<double>* js_contour_data2(JSContext* ctx, JSValueConst val);
VISIBLE JSContourData<double>* js_contour_data(JSValueConst val);
};

JSValue js_contour_move(JSContext* ctx, JSContourData<double>&& points);

template<typename T>
static inline JSValue
js_contour_new(JSContext* ctx, const JSContourData<T>& points) {
  JSValue ret = js_contour_create(ctx, contour_proto);
  JSContourData<double>* contour = js_contour_data(ret);

  new(contour) JSContourData<double>();
  contour->resize(points.size());
  transform_points(points.cbegin(), points.cend(), contour->begin());

  return ret;
}

template<typename T>
static inline JSContourData<T>*
contour_allocate(JSContext* ctx) {
  return js_allocate<JSContourData<T>>(ctx);
}

template<typename T>
static inline void
contour_deallocate(JSContext* ctx, JSContourData<T>* contour) {
  return js_deallocate<JSContourData<T>>(ctx, contour);
}

template<typename T>
static inline void
contour_deallocate(JSRuntime* rt, JSContourData<T>* contour) {
  return js_deallocate<JSContourData<T>>(rt, contour);
}

template<typename T, typename U>
static inline void
contour_copy(const JSContourData<T>& src, JSContourData<U>& dst) {
  dst.resize(src.size());
  std::copy(src.begin(), src.end(), dst.begin());
}

static inline int
js_contour_read(JSContext* ctx, JSValueConst contour, JSContourData<double>* out) {
  int ret = 0;
  return ret;
}

static inline JSContourData<double>
js_contour_get(JSContext* ctx, JSValueConst contour) {
  JSContourData<double> r = {};
  js_contour_read(ctx, contour, &r);
  return r;
}

template<class T>
JSValue
js_contours_new(JSContext* ctx, const std::vector<JSContourData<T>>& contours) {

  JSValue ret = JS_NewArray(ctx);
  uint32_t i, size = contours.size();

  for(i = 0; i < size; i++) {
    JSValue contour = js_contour_new(ctx, contours[i]);
    JS_SetPropertyUint32(ctx, ret, i, contour);
  }

  return ret;
}

extern "C" int js_contour_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_CONTOUR_HPP) */
