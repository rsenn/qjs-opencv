#ifndef JS_CONTOUR_HPP
#define JS_CONTOUR_HPP

#include "js_alloc.hpp"
#include "geometry.hpp"

extern "C" {
extern JSValue contour_class, contour_proto;
extern JSClassDef js_contour_class;
extern JSClassID js_contour_class_id;

extern "C" VISIBLE int js_contour_init(JSContext*, JSModuleDef*);

void js_contour_finalizer(JSRuntime* rt, JSValue val);

JSValue js_contour_to_string(JSContext*, JSValueConst this_val, int argc, JSValueConst* argv);
int js_contour_init(JSContext*, JSModuleDef*);
JSModuleDef* js_init_contour_module(JSContext* ctx, const char* module_name);
void js_contour_constructor(JSContext* ctx, JSValue parent, const char* name);
};

static inline JSContourData<double>*
js_contour_data(JSContext* ctx, JSValueConst val) {
  return js_contour_class_id ? static_cast<JSContourData<double>*>(JS_GetOpaque2(ctx, val, js_contour_class_id)) : 0;
}

JSValue js_contour_new(JSContext* ctx, const JSContourData<double>& points);

template<class T>
static inline JSValue
js_contour_new(JSContext* ctx, const JSContourData<T>& points) {
  JSValue ret;
  JSContourData<double>* contour;
  ret = JS_NewObjectProtoClass(ctx, contour_proto, js_contour_class_id);
  contour = js_allocate<JSContourData<double>>(ctx);
  new(contour) JSContourData<double>();
  contour->resize(points.size());
  transform_points(points.cbegin(), points.cend(), contour->begin());

  JS_SetOpaque(ret, contour);
  return ret;
};

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
