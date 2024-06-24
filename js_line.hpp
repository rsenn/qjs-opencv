#ifndef JS_LINE_HPP
#define JS_LINE_HPP

#include "jsbindings.hpp"
#include <quickjs.h>
#include <array>

extern "C" {

extern JSValue line_proto, line_class;
extern thread_local JSClassID js_line_class_id;

JSValue js_line_new(JSContext* ctx, double x1, double y1, double x2, double y2);
JSModuleDef* js_init_module_line(JSContext*, const char*);

JSLineData<double>* js_line_data2(JSContext*, JSValueConst val);
JSLineData<double>* js_line_data(JSValueConst val);

int js_line_init(JSContext*, JSModuleDef*);
}

template<class T>
static inline JSValue
js_line_new(JSContext* ctx, const JSLineData<T>& line) {
  return js_line_new(ctx, line.x1, line.y1, line.x2, line.y2);
}

static inline JSValue
js_line_new(JSContext* ctx, const JSPointData<double>& a, const JSPointData<double>& b) {
  return js_line_new(ctx, a.x, a.y, b.x, b.y);
}

template<class T>
static inline int
js_line_read(JSContext* ctx, JSValueConst line, std::array<T, 4>& out) {
  int ret = 1;
  JSValue x1 = JS_UNDEFINED, y1 = JS_UNDEFINED, x2 = JS_UNDEFINED, y2 = JS_UNDEFINED;
  /*if(js_is_array_like(ctx, line)) {
    js_array_to(ctx, line, out);
    return 1;
  } else */
  if(js_is_iterable(ctx, line)) {
    if(js_iterable_to(ctx, line, out) == 4)
      return 1;
  }
  x1 = JS_GetPropertyStr(ctx, line, "x1");
  y1 = JS_GetPropertyStr(ctx, line, "y1");
  x2 = JS_GetPropertyStr(ctx, line, "x2");
  y2 = JS_GetPropertyStr(ctx, line, "y2");

  if(JS_IsNumber(x1) && JS_IsNumber(y1) && JS_IsNumber(x2) && JS_IsNumber(y2)) {
    ret &= js_number_read(ctx, x1, &out[0]);
    ret &= js_number_read(ctx, y1, &out[1]);
    ret &= js_number_read(ctx, x2, &out[2]);
    ret &= js_number_read(ctx, y2, &out[3]);
  } else {
    ret = 0;
  }
  if(!JS_IsUndefined(x1))
    JS_FreeValue(ctx, x1);
  if(!JS_IsUndefined(y1))
    JS_FreeValue(ctx, y1);
  if(!JS_IsUndefined(x2))
    JS_FreeValue(ctx, x2);
  if(!JS_IsUndefined(y2))
    JS_FreeValue(ctx, y2);
  return ret;
}

template<class T>
static inline int
js_line_read(JSContext* ctx, JSValueConst line, JSLineData<T>* out) {
  return js_line_read(ctx, line, out->array);
}

template<class T>
static inline int
js_line_read(JSContext* ctx, JSValueConst value, JSPointData<T>* a, JSPointData<T>* b) {
  JSLineData<T> line;
  if(js_line_read(ctx, value, &line)) {
    if(a) {
      a->x = line.a.x;
      a->y = line.a.y;
    }
    if(b) {
      b->x = line.b.x;
      b->y = line.b.y;
    }

    return 1;
  }

  return 0;
}

template<class T>
static inline BOOL
js_line_arg(JSContext* ctx, int argc, JSValueConst argv[], int& argind, JSLineData<T>& line) {
  if(argind < argc && js_line_read(ctx, argv[argind], &line)) {
    ++argind;
    return TRUE;
  }
  if(argind + 1 < argc && js_point_read(ctx, argv[argind], &line.a) && js_point_read(ctx, argv[argind + 1], &line.b)) {
    argind += 2;
    return TRUE;
  }
  if(argind + 3 < argc && js_number_read(ctx, argv[argind], &line.x1) && js_number_read(ctx, argv[argind + 1], &line.y1) &&
     js_number_read(ctx, argv[argind + 2], &line.x2) && js_number_read(ctx, argv[argind + 3], &line.y2)) {
    argind += 4;
    return TRUE;
  }

  return FALSE;
}

template<class T>
static std::array<T, 4>
js_line_get(JSContext* ctx, JSValueConst line) {
  std::array<T, 4> r{0, 0, 0, 0};
  js_line_read(ctx, line, r);
  return r;
}

template<class T>
static inline JSLineData<T>
js_line_from(T x1, T y1, T x2, T y2) {
  std::array<T, 4> arr{x1, y1, x2, y2};
  return *reinterpret_cast<JSLineData<T>*>(&arr);
}
template<class T>
static inline JSLineData<T>
js_line_from(const cv::Vec<T, 4>& v) {
  std::array<T, 4> arr{v[0], v[1], v[2], v[3]};

  return *reinterpret_cast<JSLineData<T>*>(&arr);
}

template<class T>
static inline JSLineData<T>
js_line_from(const JSPointData<T>& a, const JSPointData<T>& b) {
  std::array<T, 4> arr{a.x, a.y, b.x, b.y};
  return *reinterpret_cast<JSLineData<T>*>(&arr);
}

static inline int
js_line_write(JSContext* ctx, JSValue out, JSLineTraits<double>::array_type line) {
  int ret = 0;
  ret += JS_SetPropertyStr(ctx, out, "x1", JS_NewFloat64(ctx, line[0]));
  ret += JS_SetPropertyStr(ctx, out, "y1", JS_NewFloat64(ctx, line[1]));
  ret += JS_SetPropertyStr(ctx, out, "x2", JS_NewFloat64(ctx, line[2]));
  ret += JS_SetPropertyStr(ctx, out, "y2", JS_NewFloat64(ctx, line[3]));
  return ret;
}

template<class T>
static JSLineData<T>
js_line_set(JSContext* ctx, JSValue out, T x1, T y1, T x2, T y2) {
  std::array<T, 4> r{x1, y1, x2, y2};
  js_line_write(ctx, out, r);
  return r;
}

extern "C" int js_line_init(JSContext*, JSModuleDef*);

JSValue js_line_wrap(JSContext* ctx, const JSLineData<double>& line);
JSValue js_line_wrap(JSContext* ctx, const JSLineData<int>& line);

JSLineData<double>* js_line_data(JSValueConst val);
JSLineData<double>* js_line_data2(JSContext*, JSValueConst val);

#endif /* defined(JS_LINE_HPP) */
