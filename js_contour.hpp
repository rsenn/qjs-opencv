#ifndef JS_CONTOUR_HPP
#define JS_CONTOUR_HPP

#include "geometry.hpp"
#include "js_alloc.hpp"
#include "js_point.hpp"
#include "jsbindings.hpp"
#include <quickjs.h>
#include <cstdint>
#include <new>
#include <vector>
#include <cassert>

extern "C" {
extern JSValue contour_class, contour_proto;
extern JSClassDef js_contour_class;
extern thread_local JSClassID js_contour_class_id;

JSValue js_contour_create(JSContext* ctx, JSValueConst proto);
void js_contour_finalizer(JSRuntime* rt, JSValue val);

JSValue js_contour_to_string(JSContext*, JSValueConst this_val, int argc, JSValueConst argv[]);
int js_contour_init(JSContext*, JSModuleDef*);
JSModuleDef* js_init_module_contour(JSContext*, const char*);
void js_contour_constructor(JSContext* ctx, JSValue parent, const char* name);

JSContourData<double>* js_contour_data2(JSContext* ctx, JSValueConst val);
JSContourData<double>* js_contour_data(JSValueConst val);
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
static inline size_t
contour_copy(const JSContourData<T>& src, JSContourData<U>& dst) {
  dst.resize(src.size());
  std::copy(src.begin(), src.end(), dst.begin());
  return src.size();
}

template<typename T, typename U = double>
static inline JSContourData<T>
contour_convert(const JSContourData<U>& src) {
  JSContourData<T> dst;
  dst.resize(src.size());
  std::copy(src.begin(), src.end(), dst.begin());
  return dst;
}

template<typename T>
static inline cv::Mat
contour_getmat(JSContourData<T>& contour) {
  JSMatDimensions size;
  int t = point_traits<T>::type;

  size.rows = 1;
  size.cols = contour.size();

  return cv::Mat(cv::Size(size), t, static_cast<void*>(contour.data()));
}

template<typename T>
static inline bool
contour_adjacent(const JSContourData<T>& contour, const JSPointData<T>& point) {
  for(const JSPointData<T>& pt : contour) {
    if(point_adjacent<int>(pt, point))
      return true;
  }
  return false;
}

template<typename T>
static inline bool
contour_adjacent(const JSContourData<T>& contour, const JSContourData<T>& other) {
  for(const JSPointData<T>& pt : contour) {
    if(contour_adjacent(other, pt))
      return true;
  }
  return false;
}

template<typename T> struct contour_line_iterator : JSContourData<T>::const_iterator {
  typedef typename JSContourData<T>::const_iterator point_iterator;
  typedef const JSPointData<T>* point_pointer;

  contour_line_iterator(const point_iterator& it) : point_iterator(it) {}

  const Line<T>&
  operator*() const {
    return reinterpret_cast<const Line<T>&>(*reinterpret_cast<const point_iterator&>(*this));
  }

  const Line<T>*
  operator->() const {
    return &reinterpret_cast<const Line<T>&>(*reinterpret_cast<const point_iterator&>(*this));
  }

  bool
  operator==(const point_iterator& other) const {
    if(point_iterator(*this) == other)
      return true;
    if(point_iterator(*this) + 1 == other)
      return true;
    return false;
  }

  bool
  operator!=(const point_iterator& other) const {
    return !this->operator==(other);
  }
};

template<typename T>
static inline bool
contour_intersect(const JSContourData<T>& a, const JSContourData<T>& b, std::array<ssize_t, 2>* indexes, JSPointData<T>* intersection) {
  const auto *ita = a.data(), *itb = b.data();
  const auto *aend = ita + a.size() - 1, *bend = itb + b.size() - 1;

  /*while(ita != aend) {
    while(itb != bend) {

      if(reinterpret_cast<const Line<T>*>(ita)->intersect(*reinterpret_cast<const Line<T>*>(itb), intersection)) {
        if(indexes)
          (*indexes) = std::array<ssize_t, 2>{ita - a.data(), itb - b.data()};
        return true;
      }

      ++itb;
    }
    ++ita;
  }*/
  const auto it = std::find_first_of(ita, aend, itb, bend, [intersection](const JSPointData<T>& a, const JSPointData<T>& b) -> bool {
    const auto& la = *reinterpret_cast<const Line<T>*>(&a);
    const auto& lb = *reinterpret_cast<const Line<T>*>(&b);
    return la.intersect(lb, intersection);
  });

  if(it != aend) {
    if(indexes)
      (*indexes)[0] = it - a.data();

    return true;
  }

  return false;
}

template<typename T = double>
static inline int
js_contour_read(JSContext* ctx, JSValueConst contour, JSContourData<T>* out) {
  int ret = 0;
  JSContourData<double>* c;

  if((c = js_contour_data(contour))) {
    ret = contour_copy(*c, *out);
  } else if(js_is_iterable(ctx, contour)) {
    JSValue iter = js_iterator_new(ctx, contour);
    IteratorValue result;
    JSPointData<double> pt;
    uint32_t i;

    for(i = 0;; ++i) {
      result = js_iterator_next(ctx, iter);

      if(result.done)
        break;

      if(js_point_read(ctx, result.value, &pt)) {
        out->push_back(pt);
      } else {
        JS_FreeValue(ctx, result.value);
        break;
      }
      JS_FreeValue(ctx, result.value);
    }
    JS_FreeValue(ctx, iter);
    ret = out->size();
  }

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
