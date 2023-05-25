#include "js_contour.hpp"
#include "cutils.h"
#include "geometry.hpp"
#include "js_alloc.hpp"
#include "js_array.hpp"
#include "js_mat.hpp"
#include "js_point.hpp"
#include "js_point_iterator.hpp"
#include "js_rect.hpp"
#include "js_rotated_rect.hpp"
#include "js_typed_array.hpp"
#include "js_umat.hpp"
#include "jsbindings.hpp"
#include "psimpl.hpp"
#include <quickjs.h>
#include "util.hpp"
//#include <ext/alloc_traits.h>
#include <opencv2/core/hal/interface.h>
#include <stddef.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <ostream>
#include <iostream>
#include <string>
#include <utility>

extern "C" {
JSValue contour_proto = JS_UNDEFINED, contour_class = JS_UNDEFINED;
thread_local VISIBLE JSClassID js_contour_class_id = 0;
}

static JSValue float64_array;

extern "C" JSValue
js_contour_create(JSContext* ctx, JSValueConst proto) {
  assert(js_contour_class_id);
  assert(JS_IsObject(proto));
  JSValue this_val = JS_NewObjectProtoClass(ctx, proto, js_contour_class_id);

  // printf("js_contour_create   cid=%i this_val=%p\n", JS_GetClassID(this_val), JS_VALUE_GET_OBJ(this_val));

  JS_SetOpaque(this_val, contour_allocate<double>(ctx));
  return this_val;
}

extern "C" {
VISIBLE JSContourData<double>*
js_contour_data2(JSContext* ctx, JSValueConst val) {
  assert(js_contour_class_id);
  return static_cast<JSContourData<double>*>(JS_GetOpaque2(ctx, val, js_contour_class_id));
}

VISIBLE JSContourData<double>*
js_contour_data(JSValueConst val) {
  assert(js_contour_class_id);
  return static_cast<JSContourData<double>*>(JS_GetOpaque(val, js_contour_class_id));
}
}

JSValue
js_contour_move(JSContext* ctx, JSContourData<double>&& points) {
  assert(js_contour_class_id);
  JSValue obj = js_contour_create(ctx, contour_proto);
  JSContourData<double>* contour = js_contour_data(obj);

  new(contour) JSContourData<double>(std::move(points));

  JS_SetOpaque(obj, contour);
  return obj;
}

static JSValue
js_contour_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    return JS_ThrowTypeError(ctx, "new target requiring prototype");

  JSValue obj = js_contour_create(ctx, proto);
  JSContourData<double>*other, *contour = static_cast<JSContourData<double>*>(JS_GetOpaque(obj, js_contour_class_id));

  if(argc > 0) {
    for(int i = 0; i < argc; i++) {
      JSPointData<double> p;

      if(i == 0 && (other = js_contour_data(argv[0]))) {
        *contour = *other;
        break;
      } else if(i == 0 && js_is_array(ctx, argv[i])) {
        js_array_to(ctx, argv[i], *contour);
        break;
      }

      if(js_point_read(ctx, argv[i], &p)) {
        contour->push_back(p);
      } else {
        JS_ThrowTypeError(ctx, "argument %d must be one of: Contour, Array, Point", i + 1);
        goto fail;
      }
    }
  }

  return obj;

fail:
  contour_deallocate(ctx, contour);
  JS_SetOpaque(obj, 0);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

static JSValue
js_contour_buffer(JSContext* ctx, JSValueConst this_val) {
  JSContourData<double>* contour;
  JSValue ret = JS_UNDEFINED;

  if((contour = js_contour_data2(ctx, this_val)) == nullptr)
    return ret;

  JSObject* obj = JS_VALUE_GET_OBJ(this_val);
  JS_DupValue(ctx, this_val);

  ret = JS_NewArrayBuffer(
      ctx,
      reinterpret_cast<uint8_t*>(contour->data()),
      contour->size() * sizeof(JSPointData<double>),
      [](JSRuntime* rt, void* opaque, void* ptr) { JS_FreeValueRT(rt, JS_MKPTR(JS_TAG_OBJECT, opaque)); },
      obj,
      false);

  return ret;
}

static JSValue
js_contour_array(JSContext* ctx, JSValueConst this_val) {
  JSValue ret;
  JSValueConst buffer = js_contour_buffer(ctx, this_val);

  ret = JS_CallConstructor(ctx, float64_array, 1, &buffer);
  return ret;
}

static JSValue
js_contour_approxpolydp(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;
  double epsilon;
  bool closed = false;
  std::vector<cv::Point> contour;
  JSContourData<float> approxCurve;
  JSContourData<double>*out, *v;

  v = js_contour_data2(ctx, this_val);

  if(!v)
    return JS_EXCEPTION;
  out = js_contour_data2(ctx, argv[0]);

  if(argc > 1) {
    JS_ToFloat64(ctx, &epsilon, argv[1]);

    if(argc > 2)
      closed = !!JS_ToBool(ctx, argv[2]);
  }

  contour_copy(*v, contour);
  cv::approxPolyDP(contour, approxCurve, epsilon, closed);
  contour_copy(approxCurve, *out);
  return JS_UNDEFINED;
}

/**
 * @brief      cv.Contour.prototype.arcLength
 *
 * @param      ctx       The context
 * @param[in]  this_val  The this value
 * @param[in]  argc      The count of arguments
 * @param      argv      The arguments array
 *
 * @return     The js value.
 */
static JSValue
js_contour_arclength(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* v;
  JSValue ret = JS_UNDEFINED;
  bool closed = false;
  JSContourData<float> contour;
  double len;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(argc > 0)
    closed = !!JS_ToBool(ctx, argv[0]);

  contour_copy(*v, contour);
  len = cv::arcLength(contour, closed);
  ret = JS_NewFloat64(ctx, len);
  return ret;
}

static JSValue
js_contour_area(JSContext* ctx, JSValueConst this_val) {
  JSContourData<double>* v;
  JSValue ret = JS_UNDEFINED;
  double area;
  JSContourData<float> contour;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  contour_copy(*v, contour);
  area = cv::contourArea(contour);
  ret = JS_NewFloat64(ctx, area);
  return ret;
}

static JSValue
js_contour_boundingrect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;
  JSContourData<double>* v;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(!v->empty()) {
    JSContourData<double>& pts = *v;
    JSPointData<double> tl, br;
    size_t i, n = pts.size();
    tl = br = pts[0];
    for(i = 1; i < n; ++i) {
      if(tl.x > pts[i].x)
        tl.x = pts[i].x;
      if(tl.y > pts[i].y)
        tl.y = pts[i].y;
      if(br.x < pts[i].x)
        br.x = pts[i].x;
      if(br.y < pts[i].y)
        br.y = pts[i].y;
    }
    ret = js_rect_new(ctx, tl.x, tl.y, br.x - tl.x, br.y - tl.y);
  }
  return ret;
}

static JSValue
js_contour_center(JSContext* ctx, JSValueConst this_val) {
  JSContourData<double>* v;
  JSValue ret = JS_UNDEFINED;
  double area;
  std::vector<cv::Point> points;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  contour_copy(*v, points);
  cv::Moments mu = cv::moments(points);
  cv::Point centroid = cv::Point(mu.m10 / mu.m00, mu.m01 / mu.m00);

  ret = js_point_new(ctx, point_proto, centroid.x, centroid.y);
  return ret;
}

static JSValue
js_contour_convexhull(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;
  bool clockwise = false, returnPoints = true;
  JSContourData<float> contour, hull;
  JSContourData<double>*out, *v;
  std::vector<int> hullIndices;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(argc > 0) {
    clockwise = !!JS_ToBool(ctx, argv[0]);
    if(argc > 1)
      returnPoints = !!JS_ToBool(ctx, argv[1]);
  }

  contour_copy(*v, contour);

  if(returnPoints)
    cv::convexHull(contour, hull, clockwise, true);
  else
    cv::convexHull(contour, hullIndices, clockwise, false);

  if(returnPoints) {
    ret = js_contour_new(ctx, hull);
  } else {
    uint32_t i, size = hullIndices.size();
    ret = JS_NewArray(ctx);
    for(i = 0; i < size; i++) {
      JSValue value = JS_NewInt32(ctx, hullIndices[i]);
      JS_SetPropertyUint32(ctx, ret, i, value);
    }
  }

  return ret;
}

static JSValue
js_contour_convexitydefects(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* s = js_contour_data2(ctx, this_val);
  JSValue ret = JS_UNDEFINED;

  std::vector<int> hullIndices;
  std::vector<cv::Vec4i> defects;

  if(argc > 0) {
    int64_t n = js_array_to(ctx, argv[0], hullIndices);
    if(n == 0)
      return JS_EXCEPTION;
  }

  if(s->size() == 0 || hullIndices.size() == 0)
    return JS_EXCEPTION;

  defects.resize(hullIndices.size());
  cv::convexityDefects(*s, hullIndices, defects);

  ret = js_array_from(ctx, defects);

  return ret;
}

static JSValue
js_contour_fitellipse(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* v;
  JSRotatedRectData rr;
  JSValue ret = JS_UNDEFINED;
  JSContourData<float> contour;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  contour_copy(*v, contour);
  rr = cv::fitEllipse(contour);

  return js_rotated_rect_new(ctx, rr);
}

static JSValue
js_contour_fitline(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* v;
  JSValue ret = JS_UNDEFINED;
  int64_t distType = cv::DIST_FAIR;
  double param = 0, reps = 0.01, aeps = 0.01;
  JSInputOutputArray line;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  line = js_cv_inputoutputarray(ctx, argv[0]);

  if(js_is_noarray(line))
    return JS_EXCEPTION;

  if(argc >= 2) {
    JS_ToInt64(ctx, &distType, argv[1]);
    if(argc >= 3) {
      JS_ToFloat64(ctx, &param, argv[2]);
      if(argc >= 4) {
        JS_ToFloat64(ctx, &reps, argv[3]);
        if(argc >= 5) {
          JS_ToFloat64(ctx, &aeps, argv[4]);
        }
      }
    }
  }

  cv::fitLine(*v, line, distType, param, reps, aeps);

  return ret;
}

static JSValue
js_contour_getmat(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* v;
  int32_t type = CV_64FC2;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  JSMatData mat = contour_getmat(*v);

  if(argc > 0)
    JS_ToInt32(ctx, &type, argv[0]);

  type &= ~CV_MAT_CN_MASK;
  type |= mat.type() & CV_MAT_CN_MASK;

  if(mat.depth() != CV_MAT_DEPTH(type))
    mat.convertTo(mat, type);

  return js_mat_wrap(ctx, mat);
}

static JSValue
js_contour_adjacent(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>*v, *w;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  for(int i = 0; i < argc; i++) {
    JSPointData<double> pt;
    if((w = js_contour_data(argv[i]))) {
      if(contour_adjacent(*v, *w))
        return JS_TRUE;

    } else if(js_point_read(ctx, argv[i], &pt)) {
      if(contour_adjacent(*v, pt))
        return JS_TRUE;
    }
  }
  return JS_FALSE;
}

static JSValue
js_contour_intersectconvex(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  bool handleNested = true;
  JSContourData<float> a, b, intersection;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(argc > 0) {
    other = js_contour_data2(ctx, argv[0]);

    if(argc > 1) {
      handleNested = !!JS_ToBool(ctx, argv[1]);
    }
  }

  contour_copy(*v, a);
  contour_copy(*other, b);

  cv::intersectConvexConvex(a, b, intersection, handleNested);

  ret = js_contour_new(ctx, intersection);
  return ret;
}

static JSValue
js_contour_isconvex(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  bool isConvex;
  JSContourData<float> contour;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  contour_copy(*v, contour);

  isConvex = cv::isContourConvex(contour);

  ret = JS_NewBool(ctx, isConvex);

  return ret;
}

/**
 * @brief      cv.Contour.prototype.length
 * @return     Contour length.
 */
static JSValue
js_contour_length(JSContext* ctx, JSValueConst this_val) {
  JSContourData<double>* v;
  JSValue ret;

  if(!(v = js_contour_data(this_val)))
    return JS_UNDEFINED;

  ret = JS_NewUint32(ctx, v->size());
  return ret;
}

static JSValue
js_contour_minarearect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  cv::RotatedRect rr;

  JSContourData<float> contour, minarea;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  contour_copy(*v, contour);

  rr = cv::minAreaRect(contour);
  minarea.resize(5);
  rr.points(minarea.data());

  ret = js_contour_new(ctx, minarea);
  return ret;
}

static JSValue
js_contour_minenclosingcircle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  cv::RotatedRect rr;
  JSContourData<float> contour;
  // JSPointData<float> center;
  float radius;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  contour_copy(*v, contour);

  cv::minEnclosingCircle(contour, rr.center, radius);

  rr.size.height = rr.size.width = radius * 2;

  ret = JS_NewObject(ctx);

  JS_SetPropertyStr(ctx, ret, "center", js_point_new(ctx, rr.center));
  JS_SetPropertyStr(ctx, ret, "radius", JS_NewFloat64(ctx, radius));

  return ret;
}

static JSValue
js_contour_minenclosingtriangle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  cv::RotatedRect rr;

  JSContourData<float> contour, triangle;
  JSPointData<float> center;
  float radius;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  contour_copy(*v, contour);

  cv::minEnclosingTriangle(contour, triangle);

  ret = js_contour_new(ctx, triangle);

  return ret;
}

static JSValue
js_contour_pointpolygontest(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* v /*, *other = nullptr, *ptr*/;
  JSValue ret = JS_UNDEFINED;
  JSPointData<float> pt;
  BOOL measureDist = FALSE;
  JSContourData<float> contour /*, triangle*/;
  JSPointData<double> point;
  double dist;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(argc > 0) {
    point = js_point_get(ctx, argv[0]);

    pt.x = point.x;
    pt.y = point.y;

    if(argc > 1) {
      measureDist = !!JS_ToBool(ctx, argv[1]);
    }
  }

  contour_copy(*v, contour);
  dist = cv::pointPolygonTest(contour, pt, measureDist);
  ret = JS_NewFloat64(ctx, dist);
  return ret;
}

enum {
  SIMPLIFY_REUMANN_WITKAM = 0,
  SIMPLIFY_OPHEIM,
  SIMPLIFY_LANG,
  SIMPLIFY_DOUGLAS_PEUCKER,
  SIMPLIFY_NTH_POINT,
  SIMPLIFY_RADIAL_DISTANCE,
  SIMPLIFY_PERPENDICULAR_DISTANCE
};

static JSValue
js_contour_psimpl(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSContourData<double>* s = js_contour_data2(ctx, this_val);
  int32_t shift = 1;
  uint32_t size = s->size();
  JSValue ret = JS_UNDEFINED;
  JSContourData<double> r;
  double arg1 = 0, arg2 = 0;
  double* it;
  JSPointData<double>* start = &(*s)[0];
  JSPointData<double>* end = start + size;
  r.resize(size);
  it = (double*)&r[0];

  if(!s)
    return JS_EXCEPTION;

  if(argc > 0) {
    JS_ToFloat64(ctx, &arg1, argv[0]);
    if(argc > 1) {
      JS_ToFloat64(ctx, &arg2, argv[1]);
    }
  }
  switch(magic) {
    case SIMPLIFY_REUMANN_WITKAM: {
      if(arg1 == 0)
        arg1 = 2;
      it = psimpl::simplify_reumann_witkam<2>((double*)start, (double*)end, arg1, it);
      break;
    }
    case SIMPLIFY_OPHEIM: {
      if(arg1 == 0)
        arg1 = 2;
      if(arg2 == 0)
        arg2 = 10;
      it = psimpl::simplify_opheim<2>((double*)start, (double*)end, arg1, arg2, it);
      break;
    }
    case SIMPLIFY_LANG: {
      if(arg1 == 0)
        arg1 = 2;
      if(arg2 == 0)
        arg2 = 10;
      it = psimpl::simplify_lang<2>((double*)start, (double*)end, arg1, arg2, it);
      break;
    }
    case SIMPLIFY_DOUGLAS_PEUCKER: {
      if(arg1 == 0)
        arg1 = 2;
      it = psimpl::simplify_douglas_peucker<2>((double*)start, (double*)end, arg1, it);
      break;
    }
    case SIMPLIFY_NTH_POINT: {
      if(arg1 == 0)
        arg1 = 2;
      it = psimpl::simplify_nth_point<2>((double*)start, (double*)end, arg1, it);
      break;
    }
    case SIMPLIFY_RADIAL_DISTANCE: {
      if(arg1 == 0)
        arg1 = 2;
      it = psimpl::simplify_radial_distance<2>((double*)start, (double*)end, arg1, it);
      break;
    }
    case SIMPLIFY_PERPENDICULAR_DISTANCE: {
      if(arg1 == 0)
        arg1 = 2;
      if(arg2 == 0)
        arg2 = 1;
      it = psimpl::simplify_perpendicular_distance<2>((double*)start, (double*)end, arg1, arg2, it);
      break;
    }
  }

  size = it - (double*)&r[0];
  r.resize(size / 2);
  ret = js_contour_new(ctx, r);
  return ret;
}

static JSValue
js_contour_collapse(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>*v, c;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  JSPointData<double> prev(0, 0), prevdiff(0, 0);

  c.push_back(v->front());

  for(int i = 1; i < v->size() - 1; ++i) {
    const JSPointData<double> diffs[2] = {
        point_normalize(point_difference(v->at(i), v->at(i - 1))),
        point_normalize(point_difference(v->at(i + 1), v->at(i))),
    };

    bool skip = point_equal(diffs[0], diffs[1]);

    if(skip)
      continue;

    c.push_back(v->at(i));
  }

  c.push_back(v->back());

  return js_contour_new(ctx, c);
}

static JSValue
js_contour_fill(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>*v, c;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  JSPointData<double> prev = v->front();

  c.push_back(prev);

  for(int i = 1; i < v->size(); ++i) {
    bresenham<int, double>(prev, v->at(i), c);

    prev = v->at(i);
  }

  return js_contour_new(ctx, c);
}

/**
 * @brief      cv.Contour.prototype.push
 * @param      value     Pushed value
 * @return     undefined
 */
static JSValue
js_contour_push(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* v;
  int i;
  double x, y;
  JSValueConst xv, yv;
  JSPointData<double> point;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  for(i = 0; i < argc; i++) {
    if(JS_IsObject(argv[i])) {
      xv = JS_GetPropertyStr(ctx, argv[i], "x");
      yv = JS_GetPropertyStr(ctx, argv[i], "y");
    } else if(js_is_array(ctx, argv[i])) {
      xv = JS_GetPropertyUint32(ctx, argv[i], 0);
      yv = JS_GetPropertyUint32(ctx, argv[i], 1);
    } else if(i + 1 < argc) {
      xv = argv[i++];
      yv = argv[i];
    }
    JS_ToFloat64(ctx, &point.x, xv);
    JS_ToFloat64(ctx, &point.y, yv);

    v->push_back(point);
  }
  return JS_UNDEFINED;
}

/**
 * @brief      cv.Contour.prototype.pop
 * @return     Tail value
 */
static JSValue
js_contour_pop(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* v;
  JSValue ret;
  JSValue x, y;
  JSPointData<double>*ptr, point;
  int64_t n = 0;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;
  n = v->size();
  if(n > 0) {
    point = (*v)[n - 1];
    v->pop_back();
  } else {
    return JS_EXCEPTION;
  }

  ret = js_point_new(ctx, point_proto, point.x, point.y);

  return ret;
}

/**
 * @brief      cv.Contour.prototype.unshift
 * @param      value     Added value
 * @return     undefined
 */
static JSValue
js_contour_unshift(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* v;
  int i;
  double x, y;
  JSValueConst xv, yv;
  JSPointData<double> point;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  for(i = 0; i < argc; i++) {
    if(JS_IsObject(argv[i])) {
      xv = JS_GetPropertyStr(ctx, argv[i], "x");
      yv = JS_GetPropertyStr(ctx, argv[i], "y");
    } else if(js_is_array(ctx, argv[i])) {

      xv = JS_GetPropertyUint32(ctx, argv[i], 0);
      yv = JS_GetPropertyUint32(ctx, argv[i], 1);
    } else if(i + 1 < argc) {
      xv = argv[i++];
      yv = argv[i];
    }
    JS_ToFloat64(ctx, &point.x, xv);
    JS_ToFloat64(ctx, &point.y, yv);

    v->insert(v->begin(), point);
  }
  return JS_UNDEFINED;
}

/**
 * @brief      cv.Contour.prototype.shift
 * @return     Head value
 */
static JSValue
js_contour_shift(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* v;
  JSValue ret;
  JSValue x, y;
  JSPointData<double>*ptr, point;
  int64_t n = 0;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;
  n = v->size();
  if(n > 0) {
    point = (*v)[0];
    v->erase(v->begin());
  } else {
    return JS_EXCEPTION;
  }

  ret = js_point_new(ctx, point_proto, point.x, point.y);

  return ret;
}

/**
 * @brief      cv.Contour.prototype.splice
 * @return     Head value
 */
static JSValue
js_contour_splice(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>*v, removed, add;
  JSValue ret;
  JSValue x, y;
  JSPointData<double>*ptr, point;
  int64_t i, start = -1, end, num = -1, n = 0;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  start = 0;
  num = 0;

  if(argc > 0)
    JS_ToInt64(ctx, &start, argv[0]);
  if(argc > 1)
    JS_ToInt64(ctx, &num, argv[1]);

  end = start + num;

  if(start < 0)
    start = ((start % v->size()) + v->size());
  start %= v->size();

  if(end < 0)
    end = ((end % v->size()) + v->size());
  end %= v->size();

  if(start > end)
    start = end;

  num = end - start;

  auto first = v->begin() + start;
  auto last = v->begin() + end;

  if(first != last) {
    for(auto it = first; it != last; ++it)
      removed.push_back(*it);

    v->erase(first, last);
  }

  auto args = argument_range(argc - 2, argv + 2);

  for(const JSValueConst& arg : args) {
    JSPointData<double> pt;
    js_point_read(ctx, arg, &pt);
    add.push_back(pt);
  }

  v->insert(v->begin() + start, add.begin(), add.end());

  return js_contour_new(ctx, removed);
}

enum {
  INDEX_OF,
  LAST_INDEX_OF,
  FIND_ITEM,
  FIND_INDEX,
  FIND_LAST_ITEM,
  FIND_LAST_INDEX,
};

/**
 * @brief      cv.Contour.prototype.find
 * @return     Head value
 */
static JSValue
js_contour_find(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSContourData<double>* v;
  int64_t i = 0, index = -1;
  JSValue ret = JS_UNDEFINED;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case INDEX_OF: {
      JSPointData<double> needle;
      js_point_read(ctx, argv[0], &needle);

      for(const auto& pt : *v) {
        if(point_equal(pt, needle)) {
          index = i;
          break;
        }
        ++i;
      }
      ret = JS_NewInt64(ctx, index);
      break;
    }
    case LAST_INDEX_OF: {
      JSPointData<double> needle;
      js_point_read(ctx, argv[0], &needle);
      i = v->size() - 1;

      for(JSContourData<double>::reverse_iterator it = v->rbegin(); it != v->rend(); ++it) {
        if(point_equal(*it, needle)) {
          index = i;
          break;
        }
        --i;
      }
      ret = JS_NewInt64(ctx, index);
      break;
    }
    case FIND_ITEM:
    case FIND_INDEX: {
      for(const auto& pt : *v) {
        JSValueConst args[3] = {
            js_point_new(ctx, pt),
            JS_NewInt64(ctx, i),
            this_val,
        };
        JSValue ret = JS_Call(ctx, argv[0], argc > 1 ? argv[1] : JS_UNDEFINED, 3, args);
        BOOL result = JS_ToBool(ctx, ret);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, args[1]);
        JS_FreeValue(ctx, ret);

        if(result) {
          index = i;
          break;
        }
        ++i;
      }

      ret = magic == FIND_INDEX ? JS_NewInt64(ctx, index) : index == -1 ? JS_NULL : js_point_new(ctx, v->at(index));
      break;
    }
    case FIND_LAST_ITEM:
    case FIND_LAST_INDEX: {
      i = v->size() - 1;

      for(JSContourData<double>::reverse_iterator it = v->rbegin(); it != v->rend(); ++it) {
        JSValueConst args[3] = {
            js_point_new(ctx, *it),
            JS_NewInt64(ctx, i),
            this_val,
        };
        JSValue ret = JS_Call(ctx, argv[0], argc > 1 ? argv[1] : JS_UNDEFINED, 3, args);
        BOOL result = JS_ToBool(ctx, ret);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, args[1]);
        JS_FreeValue(ctx, ret);

        if(result) {
          index = i;
          break;
        }
        --i;
      }

      ret = magic == FIND_LAST_INDEX ? JS_NewInt64(ctx, index) : index == -1 ? JS_NULL : js_point_new(ctx, v->at(index));

      break;
    }
  }

  return ret;
}

/**
 * @brief      cv.Contour.prototype.concat
 * @param      other     Other contour
 * @return     concatenated
 */
static JSValue
js_contour_concat(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>*v, *other, *r;
  int i;
  double x, y;
  JSValue ret;
  JSValueConst xv, yv;
  JSPointData<double> point;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(!(other = js_contour_data2(ctx, argv[0])))
    return JS_EXCEPTION;

  ret = js_contour_new(ctx, *v);

  r = js_contour_data2(ctx, ret);
  contour_copy(*other, *r);

  return ret;
}

static JSValue
js_contour_rotatedrectangleintersection(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  bool handleNested = true;
  JSContourData<float> a, b, intersection;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(argc > 0) {
    other = js_contour_data2(ctx, argv[0]);
  }

  contour_copy(*v, a);
  contour_copy(*other, b);

  {
    cv::RotatedRect rra(a[0], a[1], a[2]);
    cv::RotatedRect rrb(b[0], b[1], b[2]);

    cv::rotatedRectangleIntersection(rra, rrb, intersection);

    ret = js_contour_new(ctx, intersection);
  }
  return ret;
}

static JSValue
js_contour_rotatepoints(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* s;
  int32_t shift = 1;
  uint32_t size;
  if(!(s = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(argc > 0)
    JS_ToInt32(ctx, &shift, argv[0]);

  size = s->size();
  shift %= size;

  if(shift > 0) {
    std::rotate(s->begin(), s->begin() + shift, s->end());

  } else if(shift < 0) {
    std::rotate(s->rbegin(), s->rbegin() + (-shift), s->rend());
  }
  return JSValue(this_val);
}

/**
 * @brief      cv.Contour.prototype.toArray
 * @return     Array
 */
static JSValue
js_contour_toarray(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* s;
  uint32_t i, size;
  JSValue ret = JS_UNDEFINED;

  if(!(s = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;
  size = s->size();
  ret = JS_NewArray(ctx);

  for(i = 0; i < size; i++) {
    JSValue point = js_point_clone(ctx, (*s)[i]);
    JS_SetPropertyUint32(ctx, ret, i, point);
  }

  return ret;
}

/**
 * @brief      cv.Contour.prototype.toString
 * @return     String
 */
static JSValue
js_contour_tostring(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSContourData<double>* s;
  std::ostringstream os;
  int i = 0;
  int prec = 9;
  int32_t flags;

  if(!(s = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  std::for_each(s->begin(), s->end(), [&i, &os, flags, prec](const JSPointData<double>& point) {
    if(i > 0)
      os << ' ';

    os << std::setprecision(prec) << point.x << "," << point.y;

    i++;
  });

  return JS_NewString(ctx, os.str().c_str());
}

/**
 * @brief      cv.Contour.prototype.toSource
 * @return     String
 */
static JSValue
js_contour_tosource(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSContourData<double>* s;
  std::ostringstream os;
  int i = 0;
  int prec = 9;
  int32_t flags;

  if(!(s = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(magic == 1 && !(flags & 0x100))
    os << "new Contour(";

  if(argc > 0)
    JS_ToInt32(ctx, &flags, argv[0]);
  else
    flags = 0;

  if(!(flags & 0x100))
    os << '[';

  std::for_each(s->begin(), s->end(), [&i, &os, flags, prec](const JSPointData<double>& point) {
    if(i > 0)
      os << ((flags & 0x10) ? " " : ",");

    if(!(flags & 0x100))
      os << ((flags & 0x03) == 0 ? "{" : "[");

    if(flags & 0x03)
      os << std::setprecision(prec) << point.x << "," << point.y;
    else
      os << "x:" << std::setprecision(prec) << point.x << ",y:" << point.y;

    if(!(flags & 0x100))
      os << ((flags & 0x03) == 0 ? "}" : "]");

    i++;
  });
  if(!(flags & 0x100))
    os << ']'; // << std::endl;

  if(magic == 1 && !(flags & 0x100))
    os << ")";

  return JS_NewString(ctx, os.str().c_str());
}

/**
 * @brief      cv.Contour.prototype.rect
 * @param    {Number}  x        Horizontal position
 * @param    {Number}   y        Vertical position
 * @param    {Number}   width    Horizontal size
 * @param    {Number}   height   Vertical size
 * @return   {Object Contour}   New Contour
 */
static JSValue
js_contour_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;
  JSContourData<double> points;
  JSRectData<double> s = js_rect_get(ctx, argv[0]);

  points.push_back(JSPointData<double>(s.x, s.y));
  points.push_back(JSPointData<double>(s.x + s.width, s.y));
  points.push_back(JSPointData<double>(s.x + s.width, s.y + s.height));
  points.push_back(JSPointData<double>(s.x, s.y + s.height));
  points.push_back(JSPointData<double>(s.x, s.y));

  ret = js_contour_new(ctx, points);
  return ret;
}

static JSValue
js_contour_fromstr(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;
  const char* str;
  JSContourData<double> points;

  str = JS_ToCString(ctx, argv[0]);

  std::istringstream is(str);

  while(!is.eof()) {
    JSPointData<double> pt;
    if(!point_parse(is, pt))
      break;

    points.push_back(pt);
  }

  if(points.size())
    ret = js_contour_new(ctx, points);
  return ret;
}

static JSValue
js_contour_from(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;
  JSContourData<double> points;
  int64_t i, len = js_array_length(ctx, argv[0]);

  for(i = 0; i < len; i++) {
    JSValue item = JS_GetPropertyUint32(ctx, argv[0], i);
    JSPointData<double> point;

    if(!js_point_read(ctx, item, &point))
      return JS_ThrowTypeError(ctx, "item %" PRId64 " is not a cv::Point", i);

    points.push_back(point);
  }
  if(points.size())
    ret = js_contour_new(ctx, points);

  return ret;
}

static JSValue
js_contour_match(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;
  JSContourData<double>*a, *b;

  if(!(a = js_contour_data2(ctx, argv[0])))
    return JS_EXCEPTION;

  if(!(b = js_contour_data2(ctx, argv[1])))
    return JS_EXCEPTION;

  const auto it = std::find_first_of(a->begin(), a->end(), b->begin(), b->end(), &point_equal<double>);

  if(it != a->end()) {
    const auto it2 = std::find_if(b->begin(), b->end(), point_compare(*it));

    if(it2 != b->end()) {
      std::array<int64_t, 2> indexes = {it - a->begin(), it2 - b->begin()};
      return js_array_from(ctx, indexes);
    }
  }

  return JS_NULL;
}

static JSValue
js_contour_intersect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;
  JSContourData<double>*a, *b;
  JSPointData<double> pt;
  std::array<ssize_t, 2> indexes = {-1, -1};

  if(!(a = js_contour_data2(ctx, argv[0])))
    return JS_EXCEPTION;

  if(!(b = js_contour_data2(ctx, argv[1])))
    return JS_EXCEPTION;

  bool result = contour_intersect(*a, *b, &indexes, &pt);

  ret = JS_NewBool(ctx, result);

  if(result) {
    if(argc > 2)
      js_array_copy(ctx, argv[2], indexes.begin(), indexes.end());

    if(argc > 3)
      js_point_write(ctx, argv[3], pt);
  }

  return ret;
}

enum { PROP_ASPECT_RATIO = 0, PROP_EXTENT, PROP_SOLIDITY, PROP_EQUIVALENT_DIAMETER, PROP_ORIENTATION, PROP_BOUNDING_RECT };

static JSValue
js_contour_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSContourData<double>* contour;
  JSValue ret = JS_UNDEFINED;

  if(!(contour = js_contour_data(this_val))) {
    // printf("js_contour_get   cid=%i this_val=%p contour=%p\n", JS_GetClassID(this_val), JS_VALUE_GET_OBJ(this_val), contour);
    return JS_UNDEFINED;
  }

  switch(magic) {
    case PROP_ASPECT_RATIO: {
      std::vector<cv::Point2f> points;
      JSRectData<float> rect;
      contour_copy(*contour, points);
      rect = cv::boundingRect(points);
      ret = JS_NewFloat64(ctx, (double)rect.width / rect.height);
      break;
    }
    case PROP_EXTENT: {
      std::vector<cv::Point2f> points;
      contour_copy(*contour, points);
      float area = cv::contourArea(points);
      JSRectData<float> rect = cv::boundingRect(points);

      ret = JS_NewFloat64(ctx, area / (rect.width * rect.height));
      break;
    }
    case PROP_SOLIDITY: {
      std::vector<cv::Point> points, hull;
      contour_copy(*contour, points);
      float area = cv::contourArea(points);
      cv::convexHull(points, hull);
      float hullArea = cv::contourArea(hull);

      ret = JS_NewFloat64(ctx, area / cv::contourArea(hull));
      break;
    }
    case PROP_EQUIVALENT_DIAMETER: {
      std::vector<cv::Point2f> points;
      contour_copy(*contour, points);
      float area = cv::contourArea(points);

      ret = JS_NewFloat64(ctx, std::sqrt(4 * area / 3.14159265358979323846));
      break;
    }
    case PROP_ORIENTATION: {
      std::vector<cv::Point2f> points;
      contour_copy(*contour, points);
      if(points.size() >= 5) {
        cv::RotatedRect rect = cv::fitEllipse(points);
        ret = JS_NewFloat64(ctx, rect.angle);
      }
      break;
    }
    case PROP_BOUNDING_RECT: {
      ret = js_contour_boundingrect(ctx, this_val, 0, 0);
      break;
    }
  }
  return ret;
}

static JSValue
js_contour_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* contour;

  if(!(contour = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;
  assert(js_contour_class_id);

  JSValue array_proto = js_global_prototype(ctx, "Array");

  JSValue obj = JS_NewObjectProtoClass(ctx, contour_proto, js_contour_class_id);
  JS_FreeValue(ctx, array_proto);

  JS_DefinePropertyValueStr(ctx, obj, "length", JS_NewUint32(ctx, contour->size()), JS_PROP_ENUMERABLE);
  return obj;
}

void
js_contour_finalizer(JSRuntime* rt, JSValue this_val) {
  JSContourData<double>* contour;

  assert(js_contour_class_id);
  contour = js_contour_data(this_val);

  if(contour) {
    // printf("js_contour_finalizer  cid=%i this_val=%p contour=%p\n", JS_GetClassID(this_val), JS_VALUE_GET_OBJ(this_val), contour);

    contour_deallocate(rt, contour);
  }
  JS_FreeValueRT(rt, this_val);
}

extern "C" {

static int
js_contour_get_own_property(JSContext* ctx, JSPropertyDescriptor* pdesc, JSValueConst obj, JSAtom prop) {
  JSContourData<double>* contour;
  JSValue value = JS_UNDEFINED;
  uint32_t index;

  if(!(contour = static_cast<JSContourData<double>*>(JS_GetOpaque(obj, js_contour_class_id))))
    return FALSE;

  if(js_atom_is_index(ctx, &index, prop)) {
    if(index < contour->size()) {
      value = js_point_new(ctx, (*contour)[index]);

      if(pdesc) {
        pdesc->flags = JS_PROP_ENUMERABLE;
        pdesc->value = value;
        pdesc->getter = JS_UNDEFINED;
        pdesc->setter = JS_UNDEFINED;
      }
      return TRUE;
    }
  } else if(js_atom_is_length(ctx, prop)) {
    value = JS_NewUint32(ctx, contour->size());

    if(pdesc) {
      pdesc->flags = JS_PROP_CONFIGURABLE;
      pdesc->value = value;
      pdesc->getter = JS_UNDEFINED;
      pdesc->setter = JS_UNDEFINED;
    }
    return TRUE;
  }

  return FALSE;
}

static int
js_contour_get_own_property_names(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst obj) {
  JSContourData<double>* contour;
  uint32_t i, len;
  JSPropertyEnum* props;
  if((contour = js_contour_data(obj)))
    len = contour->size();
  else {
    JSValue length = JS_GetPropertyStr(ctx, obj, "length");
    JS_ToUint32(ctx, &len, length);
    JS_FreeValue(ctx, length);
  }

  props = js_allocate<JSPropertyEnum>(ctx, len + 1);

  for(i = 0; i < len; i++) {
    props[i].is_enumerable = TRUE;
    props[i].atom = i | (1U << 31);
  }

  props[len].is_enumerable = FALSE;
  props[len].atom = JS_NewAtom(ctx, "length");

  *ptab = props;
  *plen = len + 1;
  return 0;
}

static int
js_contour_has_property(JSContext* ctx, JSValueConst obj, JSAtom prop) {
  JSContourData<double>* contour = js_contour_data(obj);
  uint32_t index;

  if(js_atom_is_index(ctx, &index, prop)) {
    if(index < contour->size())
      return TRUE;
  } else if(js_atom_is_length(ctx, prop)) {
    return TRUE;
  } else {
    JSValue proto = JS_GetPrototype(ctx, obj);
    if(JS_IsObject(proto) && JS_HasProperty(ctx, proto, prop))
      return TRUE;
  }

  return FALSE;
}

static JSValue
js_contour_get_property(JSContext* ctx, JSValueConst obj, JSAtom prop, JSValueConst receiver) {
  JSContourData<double>* contour = js_contour_data(obj);
  JSValue value = JS_UNDEFINED;
  uint32_t index;

  if(js_atom_is_index(ctx, &index, prop)) {
    if(index < contour->size())
      value = js_point_new(ctx, (*contour)[index]);
  } else if(js_atom_is_length(ctx, prop)) {
    value = JS_NewUint32(ctx, contour->size());
  } else {
    JSValue proto = JS_GetPrototype(ctx, obj);
    if(JS_IsObject(proto)) {
      JSPropertyDescriptor desc = {0, JS_UNDEFINED, JS_UNDEFINED, JS_UNDEFINED};
      if(JS_GetOwnProperty(ctx, &desc, proto, prop) > 0) {
        if(JS_IsFunction(ctx, desc.getter))
          value = JS_Call(ctx, desc.getter, obj, 0, 0);
        else if(JS_IsFunction(ctx, desc.value))
          value = JS_DupValue(ctx, desc.value);
      }
    }
  }

  return value;
}

static int
js_contour_set_property(JSContext* ctx, JSValueConst obj, JSAtom prop, JSValueConst value, JSValueConst receiver, int flags) {
  JSContourData<double>* contour = js_contour_data(obj);
  uint32_t index;

  if(js_atom_is_index(ctx, &index, prop)) {
    JSPointData<double> point;
    if(index >= contour->size())
      contour->resize(index + 1);

    js_point_read(ctx, value, &point);
    (*contour)[index] = point;
    return TRUE;
  } else if(js_atom_is_length(ctx, prop)) {
    uint32_t len;
    JS_ToUint32(ctx, &len, value);
    contour->resize(len);
    return TRUE;
  }

  return FALSE;
}

JSClassExoticMethods js_contour_exotic_methods = {
    .get_own_property = js_contour_get_own_property,
    .get_own_property_names = js_contour_get_own_property_names,
    .has_property = js_contour_has_property,
    .get_property = js_contour_get_property,
    .set_property = js_contour_set_property,
};

JSClassDef js_contour_class = {
    .class_name = "Contour",
    .finalizer = js_contour_finalizer,
    .exotic = &js_contour_exotic_methods,
};

JSValue
js_contour_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSContourData<double>* s;

  if(!(s = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  return js_point_iterator_new(ctx, s->data(), s->data() + s->size(), magic);
}

const JSCFunctionListEntry js_contour_proto_funcs[] = {
    JS_CFUNC_DEF("collapse", 0, js_contour_collapse),
    JS_CFUNC_DEF("fill", 0, js_contour_fill),
    JS_CFUNC_DEF("push", 1, js_contour_push),
    JS_CFUNC_DEF("pop", 0, js_contour_pop),
    JS_CFUNC_DEF("unshift", 1, js_contour_unshift),
    JS_CFUNC_DEF("shift", 0, js_contour_shift),
    JS_CFUNC_DEF("splice", 0, js_contour_splice),
    JS_CFUNC_MAGIC_DEF("indexOf", 1, js_contour_find, INDEX_OF),
    JS_CFUNC_MAGIC_DEF("lastIndexOf", 1, js_contour_find, LAST_INDEX_OF),
    JS_CFUNC_MAGIC_DEF("find", 1, js_contour_find, FIND_ITEM),
    JS_CFUNC_MAGIC_DEF("findIndex", 1, js_contour_find, FIND_INDEX),
    JS_CFUNC_MAGIC_DEF("findLast", 1, js_contour_find, FIND_LAST_ITEM),
    JS_CFUNC_MAGIC_DEF("findLastIndex", 1, js_contour_find, FIND_LAST_INDEX),
    JS_CFUNC_DEF("concat", 1, js_contour_concat),
    JS_CFUNC_DEF("getMat", 0, js_contour_getmat),
    JS_CFUNC_DEF("adjacent", 1, js_contour_adjacent),
    JS_CGETSET_DEF("length", js_contour_length, NULL),
    JS_CGETSET_DEF("area", js_contour_area, NULL),
    JS_CGETSET_DEF("buffer", js_contour_buffer, NULL),
    JS_CGETSET_DEF("array", js_contour_array, NULL),
    JS_CGETSET_MAGIC_DEF("aspectRatio", js_contour_get, NULL, PROP_ASPECT_RATIO),
    JS_CGETSET_MAGIC_DEF("extent", js_contour_get, NULL, PROP_EXTENT),
    JS_CGETSET_MAGIC_DEF("solidity", js_contour_get, NULL, PROP_SOLIDITY),
    JS_CGETSET_MAGIC_DEF("equivalentDiameter", js_contour_get, NULL, PROP_EQUIVALENT_DIAMETER),
    JS_CGETSET_MAGIC_DEF("orientation", js_contour_get, NULL, PROP_ORIENTATION),
    JS_CGETSET_MAGIC_DEF("boundingRect", js_contour_get, NULL, PROP_BOUNDING_RECT),

    JS_CFUNC_DEF("approxPolyDP", 1, js_contour_approxpolydp),
    JS_CFUNC_DEF("convexHull", 1, js_contour_convexhull),
    JS_CFUNC_DEF("getBoundingClientRect", 0, js_contour_boundingrect),
    JS_CFUNC_DEF("fitEllipse", 0, js_contour_fitellipse),
    JS_CFUNC_DEF("fitLine", 1, js_contour_fitline),
    JS_CFUNC_DEF("intersectConvex", 0, js_contour_intersectconvex),
    JS_CFUNC_DEF("isConvex", 0, js_contour_isconvex),
    JS_CFUNC_DEF("minAreaRect", 0, js_contour_minarearect),
    JS_CFUNC_DEF("minEnclosingCircle", 0, js_contour_minenclosingcircle),
    JS_CFUNC_DEF("minEnclosingTriangle", 0, js_contour_minenclosingtriangle),
    JS_CFUNC_DEF("pointPolygonTest", 0, js_contour_pointpolygontest),
    JS_CFUNC_DEF("rotatedRectangleIntersection", 0, js_contour_rotatedrectangleintersection),
    JS_CFUNC_DEF("arcLength", 0, js_contour_arclength),
    JS_CFUNC_DEF("rotatePoints", 1, js_contour_rotatepoints),
    JS_CFUNC_DEF("convexityDefects", 1, js_contour_convexitydefects),
    JS_CFUNC_MAGIC_DEF("simplifyReumannWitkam", 0, js_contour_psimpl, SIMPLIFY_REUMANN_WITKAM),
    JS_CFUNC_MAGIC_DEF("simplifyOpheim", 0, js_contour_psimpl, SIMPLIFY_OPHEIM),
    JS_CFUNC_MAGIC_DEF("simplifyLang", 0, js_contour_psimpl, SIMPLIFY_LANG),
    JS_CFUNC_MAGIC_DEF("simplifyDouglasPeucker", 0, js_contour_psimpl, SIMPLIFY_DOUGLAS_PEUCKER),
    JS_CFUNC_MAGIC_DEF("simplifyNthPoint", 0, js_contour_psimpl, SIMPLIFY_NTH_POINT),
    JS_CFUNC_MAGIC_DEF("simplifyRadialDistance", 0, js_contour_psimpl, SIMPLIFY_RADIAL_DISTANCE),
    JS_CFUNC_MAGIC_DEF("simplifyPerpendicularDistance", 0, js_contour_psimpl, SIMPLIFY_PERPENDICULAR_DISTANCE),
    JS_CFUNC_DEF("toArray", 0, js_contour_toarray),
    JS_CFUNC_MAGIC_DEF("toString", 0, js_contour_tostring, 0),
    JS_CFUNC_MAGIC_DEF("toSource", 0, js_contour_tosource, 1),
    JS_CFUNC_MAGIC_DEF("lines", 0, js_contour_iterator, NEXT_LINE),
    JS_CFUNC_MAGIC_DEF("points", 0, js_contour_iterator, NEXT_POINT),
    JS_ALIAS_DEF("[Symbol.iterator]", "points"),
    JS_ALIAS_DEF("size", "length"),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Contour", JS_PROP_CONFIGURABLE),

};
const JSCFunctionListEntry js_contour_static_funcs[] = {
    JS_CFUNC_DEF("fromRect", 1, js_contour_rect),
    JS_CFUNC_DEF("fromString", 1, js_contour_fromstr),
    JS_CFUNC_DEF("from", 1, js_contour_from),
    JS_CFUNC_DEF("match", 2, js_contour_match),
    JS_CFUNC_DEF("intersect", 2, js_contour_intersect),
    JS_PROP_INT32_DEF("FORMAT_XY", 0x00, 0),
    JS_PROP_INT32_DEF("FORMAT_01", 0x02, 0),
    JS_PROP_INT32_DEF("FORMAT_SPACE", 0x10, 0),
    JS_PROP_INT32_DEF("FORMAT_COMMA", 0x00, 0),
    JS_PROP_INT32_DEF("FORMAT_BRACKET", 0x00, 0),
    JS_PROP_INT32_DEF("FORMAT_NOBRACKET", 0x100, 0),
};

int
js_contour_init(JSContext* ctx, JSModuleDef* m) {

  if(js_contour_class_id == 0) {
    /* create the Contour class */
    JS_NewClassID(&js_contour_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_contour_class_id, &js_contour_class);

    contour_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, contour_proto, js_contour_proto_funcs, countof(js_contour_proto_funcs));
    JS_SetClassProto(ctx, js_contour_class_id, contour_proto);

    contour_class = JS_NewCFunction2(ctx, js_contour_constructor, "Contour", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, contour_class, contour_proto);
    JS_SetPropertyFunctionList(ctx, contour_class, js_contour_static_funcs, countof(js_contour_static_funcs));
  }

  // js_set_inspect_method(ctx, contour_proto, js_contour_inspect);

  {
    JSValue global = JS_GetGlobalObject(ctx);
    float64_array = JS_GetPropertyStr(ctx, global, "Float64Array");
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Contour", contour_class);
  /*  else
      JS_SetPropertyStr(ctx, *static_cast<JSValue*>(m), "Contour", contour_class);*/
  return 0;
}

extern "C" VISIBLE void
js_contour_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Contour");
}

#if defined(JS_CONTOUR_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_contour
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_contour_init);
  if(!m)
    return NULL;
  js_contour_export(ctx, m);
  return m;
}
}
