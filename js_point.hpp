#ifndef JS_POINT_HPP
#define JS_POINT_HPP

#include "jsbindings.hpp"
#include <quickjs.h>

template<class T> using JSPointData = cv::Point_<T>;

template<typename T> struct point_traits {};

template<> struct point_traits<uint16_t> { static const int type = CV_16UC2; };
template<> struct point_traits<int16_t> { static const int type = CV_16SC2; };
template<> struct point_traits<int32_t> { static const int type = CV_32SC2; };
template<> struct point_traits<float> { static const int type = CV_32FC2; };
template<> struct point_traits<double> { static const int type = CV_64FC2; };

extern "C" int js_point_init(JSContext*, JSModuleDef*);

extern "C" {

extern thread_local JSValue point_proto, point_class;
extern thread_local JSClassID js_point_class_id;

int js_point_init(JSContext*, JSModuleDef* m);
JSValue js_point_create(JSContext* ctx, JSValueConst proto);
JSModuleDef* js_init_module_point(JSContext*, const char*);
}

// JSValue js_point_new(JSContext*, double x, double y);

JSValue js_point_new(JSContext* ctx, JSValueConst proto, double x, double y);

static inline JSValue
js_point_new(JSContext* ctx, double x, double y) {
  return js_point_new(ctx, point_proto, x, y);
}

JSValue js_point_clone(JSContext* ctx, JSValueConst proto, const JSPointData<double>& point);

static inline JSValue
js_point_clone(JSContext* ctx, const JSPointData<double>& point) {
  return js_point_clone(ctx, point_proto, point);
}

JSPointData<double>* js_point_data2(JSContext*, JSValueConst val);
JSPointData<double>* js_point_data(JSValueConst val);

template<class T>
static inline JSValue
js_point_new(JSContext* ctx, const JSPointData<T>& point) {
  return js_point_new(ctx, point_proto, point.x, point.y);
}

template<class T>
static inline int
js_point_read(JSContext* ctx, JSValueConst point, JSPointData<T>* out) {
  int ret = 1;
  JSValue x = JS_UNDEFINED, y = JS_UNDEFINED;
  if(JS_IsArray(ctx, point)) {
    x = JS_GetPropertyUint32(ctx, point, 0);
    y = JS_GetPropertyUint32(ctx, point, 1);
  } else if(JS_IsObject(point)) {
    x = JS_GetPropertyStr(ctx, point, "x");
    y = JS_GetPropertyStr(ctx, point, "y");
  }
  if(JS_IsNumber(x) && JS_IsNumber(y)) {
    ret &= js_number_read(ctx, x, &out->x);
    ret &= js_number_read(ctx, y, &out->y);
  } else {
    ret = 0;
  }

  if(!JS_IsUndefined(x))
    JS_FreeValue(ctx, x);
  if(!JS_IsUndefined(y))
    JS_FreeValue(ctx, y);

  return ret;
}

template<class T>
static inline int
js_point_argument(JSContext* ctx, int argc, JSValueConst argv[], JSPointData<T>* out) {
  int ret = 0;

  if(JS_IsNumber(argv[0]) && JS_IsNumber(argv[1])) {
    JSPointData<double> pt;
    JS_ToFloat64(ctx, &pt.x, argv[0]);
    JS_ToFloat64(ctx, &pt.y, argv[1]);
    *out = pt;
    ret = 2;
  } else if(js_point_read(ctx, argv[0], out)) {
    ret = 1;
  }

  return ret;
}

template<class T>
static inline BOOL
js_point_arg(JSContext* ctx, int argc, JSValueConst argv[], int& argind, JSPointData<T>& point) {
  if(argind < argc && js_point_read(ctx, argv[argind], &point)) {
    ++argind;
    return TRUE;
  }

  if(argind + 1 < argc && js_number_read(ctx, argv[argind], &point.x) && js_number_read(ctx, argv[argind + 1], &point.y)) {
    argind += 2;
    return TRUE;
  }

  return FALSE;
}

template<class T>
static inline void
js_point_write(JSContext* ctx, JSValueConst out, const JSPointData<T>& in) {
  JSValue x = js_number_new<T>(ctx, in.x);
  JSValue y = js_number_new<T>(ctx, in.y);

  if(js_is_arraylike(ctx, out)) {
    JS_SetPropertyUint32(ctx, out, 0, x);
    JS_SetPropertyUint32(ctx, out, 1, y);
  } else if(JS_IsObject(out)) {
    JS_SetPropertyStr(ctx, out, "x", x);
    JS_SetPropertyStr(ctx, out, "y", y);
  } else if(js_is_function(ctx, out)) {
    JSValueConst args[2];
    args[0] = x;
    args[1] = y;
    JS_Call(ctx, out, JS_UNDEFINED, 2, args);
  }
  JS_FreeValue(ctx, x);
  JS_FreeValue(ctx, y);
}

static inline JSPointData<double>
js_point_get(JSContext* ctx, JSValueConst point) {
  JSPointData<double> r;
  js_point_read(ctx, point, &r);
  return r;
}

static inline bool
js_is_point(JSContext* ctx, JSValueConst point) {
  JSPointData<double> r;

  if(js_point_data2(ctx, point))
    return true;

  if(js_point_read(ctx, point, &r))
    return true;

  return false;
}

extern "C" int js_point_init(JSContext*, JSModuleDef*);

template<typename T>
static inline cv::Point_<T>
point_difference(const cv::Point_<T>& a, const cv::Point_<T>& b) {
  return cv::Point_<T>(b.x - a.x, b.y - a.y);
}

template<typename T>
static inline cv::Point_<T>
point_abs(const cv::Point_<T>& p) {
  return cv::Point_<T>(p.x >= 0 ? p.x : -p.x, p.y >= 0 ? p.y : -p.y);
}

template<typename T>
static inline bool
point_adjacent(const cv::Point_<T>& a, const cv::Point_<T>& b) {
  cv::Point_<T> diff = point_abs(point_difference(a, b));
  T d = diff.x + diff.y;

  return (d > 1 && diff.x <= 1 && diff.y <= 1) || d == 1;
}

template<typename T>
static inline cv::Point_<T>
point_sum(const cv::Point_<T>& a, const cv::Point_<T>& b) {
  return cv::Point_<T>(a.x + b.x, a.y + b.y);
}

template<typename T>
static inline cv::Point_<T>
point_normalize(const cv::Point_<T>& a) {
  T q = std::max(std::abs(a.x), std::abs(a.y));

  return cv::Point_<T>(a.x / q, a.y / q);
}

template<typename T>
static inline bool
point_equal(const cv::Point_<T>& a, const cv::Point_<T>& b) {
  return a.x == b.x && a.y == b.y;
}

template<typename T> struct point_compare {

  typedef cv::Point_<T> point_type;

  point_compare(const point_type& point) : m_pt(point) {}

  bool operator()(const point_type& other) const { return point_equal(m_pt, other); }

private:
  cv::Point_<T> m_pt;
};

template<typename T, typename M = cv::Mat>
static inline bool
point_inside(const cv::Point_<T>& pt, const M& mat) {
  return pt.x >= 0 && pt.x < mat.cols && pt.y >= 0 && pt.y < mat.rows;
}

template<typename T>
static inline bool
point_inside(const cv::Point_<T>& pt, const cv::Size& sz) {
  return pt.x >= 0 && pt.x < sz.width && pt.y >= 0 && pt.y < sz.height;
}

/*template<class T>
inline std::ostream&
operator<<(std::ostream& os, const cv::Point_<T>& p) {
  os << p.x << "," << p.y;
  return os;
}*/

template<class T>
inline bool
point_parse(std::istream& is, cv::Point_<T>& p) {
  is >> std::skipws >> p.x;
  if(is.peek() == ',') {
    is.get();
    is >> std::skipws >> p.y;
    return true;
  }

  return false;
}

template<class T>
inline std::istream&
operator>>(std::istream& is, cv::Point_<T>& p) {
  point_parse(is, p);
  return is;
}

template<class T>
static inline int
js_value_to(JSContext* ctx, JSValueConst value, cv::Point_<T>& pt) {
  return js_point_read(ctx, value, &pt);
}

template<class T>
static inline JSValue
js_value_from(JSContext* ctx, const cv::Point_<T>& pt) {
  return js_point_new(ctx, pt.x, pt.y);
}

#endif /* defined(JS_POINT_HPP) */
