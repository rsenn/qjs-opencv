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
#include <ext/alloc_traits.h>
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

  printf("js_contour_create   cid=%i this_val=%p\n", JS_GetClassID(this_val), JS_VALUE_GET_OBJ(this_val));

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
js_contour_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    return JS_ThrowTypeError(ctx, "new target requiring prototype");

  JSValue obj = js_contour_create(ctx, proto);
  JSContourData<double>*other, *contour = static_cast<JSContourData<double>*>(JS_GetOpaque(obj, js_contour_class_id));

  if(argc > 0) {
    if((other = static_cast<JSContourData<double>*>(JS_GetOpaque(argv[0], js_contour_class_id)))) {
      new(contour) JSContourData<double>(*other);
    } else if(js_is_array_like(ctx, argv[0])) {
      JSContourData<double> tmp;
      js_array_to(ctx, argv[0], tmp);
      new(contour) JSContourData<double>(std::move(tmp));
    }
  } else
    new(contour) JSContourData<double>();

  /* if(argc > 0) {
     int i;
     for(i = 0; i < argc; i++) {
       JSPointData<double> p;
       if(js_is_array(ctx, argv[i])) {
         if(js_array_length(ctx, argv[i]) > 0) {
           JSValue pt = JS_GetPropertyUint32(ctx, argv[i], 0);
           if(js_is_point(ctx, pt)) {
             js_array_to(ctx, argv[i], *contour);
             JS_FreeValue(ctx, pt);
             continue;
           }
           JS_FreeValue(ctx, pt);
         }
       }
       if(js_point_read(ctx, argv[i], &p)) {
         contour->push_back(p);
         continue;
       }
       goto fail;
     }
   }
 */
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

  JSValue* valptr = static_cast<JSValue*>(js_malloc(ctx, sizeof(JSValue)));
  *valptr = JS_DupValue(ctx, this_val);

  ret = JS_NewArrayBuffer(
      ctx,
      reinterpret_cast<uint8_t*>(contour->data()),
      contour->size() * sizeof(JSPointData<double>),
      [](JSRuntime* rt, void* opaque, void* ptr) {
        JS_FreeValueRT(rt, *(JSValue*)opaque);
        js_free_rt(rt, opaque);
      },
      valptr,
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
js_contour_approxpolydp(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSValue ret = JS_UNDEFINED;
  double epsilon;
  bool closed = false;
  std::vector<cv::Point> curve;
  JSContourData<float> approxCurve;
  JSContourData<double>*out, *v;

  v = js_contour_data2(ctx, this_val);

  if(!v)
    return JS_EXCEPTION;
  out = js_contour_data2(ctx, argv[0]);

  if(argc > 1) {
    JS_ToFloat64(ctx, &epsilon, argv[1]);

    if(argc > 2) {
      closed = !!JS_ToBool(ctx, argv[2]);
    }
  }

  std::copy(v->begin(), v->end(), std::back_inserter(curve));

  cv::approxPolyDP(curve, approxCurve, epsilon, closed);

  std::copy(approxCurve.begin(), approxCurve.end(), std::back_inserter(*out));

  return JS_UNDEFINED;
}

static JSValue
js_contour_arclength(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  JSPointData<float> pt;
  bool closed = false;
  JSContourData<float> contour;
  JSPointData<double>* point;
  double retval;

  v = js_contour_data2(ctx, this_val);
  if(!v)

    return JS_EXCEPTION;

  if(argc > 0) {
    closed = !!JS_ToBool(ctx, argv[0]);
  }

  std::copy(v->begin(), v->end(), std::back_inserter(contour));

  retval = cv::arcLength(contour, closed);

  ret = JS_NewFloat64(ctx, retval);

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

  std::copy(v->begin(), v->end(), std::back_inserter(contour));
  area = cv::contourArea(contour);
  ret = JS_NewFloat64(ctx, area);
  return ret;
}

static JSValue
js_contour_boundingrect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSValue ret = JS_UNDEFINED;
  cv::Rect2f rect;
  JSContourData<float> curve;
  JSContourData<double>* v;
  JSRectData<double> r;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  std::copy(v->begin(), v->end(), std::back_inserter(curve));

  rect = cv::boundingRect(curve);

  ret = js_rect_new(ctx, rect);

  return ret;
}

static JSValue
js_contour_center(JSContext* ctx, JSValueConst this_val) {
  JSContourData<double>* v;
  JSValue ret = JS_UNDEFINED;
  double area;
  v = js_contour_data2(ctx, this_val);
  if(!v)
    return JS_EXCEPTION;
  {
    std::vector<cv::Point> points;
    points.resize(v->size());
    std::copy(v->begin(), v->end(), points.begin());
    cv::Moments mu = cv::moments(points);
    cv::Point centroid = cv::Point(mu.m10 / mu.m00, mu.m01 / mu.m00);

    ret = js_point_new(ctx, point_proto, centroid.x, centroid.y);
  }

  return ret;
}

static JSValue
js_contour_convexhull(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSValue ret = JS_UNDEFINED;
  bool clockwise = false, returnPoints = true;
  JSContourData<float> curve, hull;
  std::vector<int> hullIndices;
  JSContourData<double>*out, *v;

  v = js_contour_data2(ctx, this_val);

  if(!v)
    return JS_EXCEPTION;

  if(argc > 0) {
    clockwise = !!JS_ToBool(ctx, argv[0]);

    if(argc > 1) {
      returnPoints = !!JS_ToBool(ctx, argv[1]);
    }
  }

  std::copy(v->begin(), v->end(), std::back_inserter(curve));

  if(returnPoints)
    cv::convexHull(curve, hull, clockwise, true);
  else
    cv::convexHull(curve, hullIndices, clockwise, false);

  if(returnPoints) {
    ret = js_contour_new(ctx, hull);
  } else {
    uint32_t i, size = hullIndices.size();

    ret = JS_NewArray(ctx);

    for(i = 0; i < size; i++) { JS_SetPropertyUint32(ctx, ret, i, JS_NewInt32(ctx, hullIndices[i])); }
  }

  return ret;
}

static JSValue
js_contour_convexitydefects(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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
js_contour_fitellipse(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>* v;
  JSRotatedRectData rr;
  JSValue ret = JS_UNDEFINED;
  JSContourData<float> contour;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  std::copy(v->begin(), v->end(), std::back_inserter(contour));

  rr = cv::fitEllipse(contour);

  return js_rotated_rect_new(ctx, rr);
}

static JSValue
js_contour_fitline(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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
js_contour_getmat(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>* v;
  JSMatDimensions size;
  int32_t type;
  cv::Mat mat;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  size.rows = 1;
  size.cols = v->size();
  type = CV_64FC2;

  mat = cv::Mat(cv::Size(size), type, static_cast<void*>(v->data()));

  return js_mat_wrap(ctx, mat);
}

static JSValue
js_contour_intersectconvex(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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

  std::copy(v->begin(), v->end(), std::back_inserter(a));

  std::copy(other->begin(), other->end(), std::back_inserter(b));

  cv::intersectConvexConvex(a, b, intersection, handleNested);

  ret = js_contour_new(ctx, intersection);
  return ret;
}

static JSValue
js_contour_isconvex(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  bool isConvex;
  JSContourData<float> contour;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  std::copy(v->begin(), v->end(), std::back_inserter(contour));

  isConvex = cv::isContourConvex(contour);

  ret = JS_NewBool(ctx, isConvex);

  return ret;
}

static JSValue
js_contour_length(JSContext* ctx, JSValueConst this_val) {
  JSContourData<double>* v;
  JSValue ret;
  if(!(v = static_cast<JSContourData<double>*>(JS_GetOpaque(this_val, js_contour_class_id))))
    return JS_UNDEFINED;

  ret = JS_NewInt64(ctx, v->size());
  return ret;
}

static JSValue
js_contour_minarearect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  cv::RotatedRect rr;

  JSContourData<float> contour, minarea;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  std::copy(v->begin(), v->end(), std::back_inserter(contour));

  rr = cv::minAreaRect(contour);
  minarea.resize(5);
  rr.points(minarea.data());

  ret = js_contour_new(ctx, minarea);
  return ret;
}

static JSValue
js_contour_minenclosingcircle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  cv::RotatedRect rr;
  JSContourData<float> contour;
  // JSPointData<float> center;
  float radius;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  std::copy(v->begin(), v->end(), std::back_inserter(contour));

  cv::minEnclosingCircle(contour, rr.center, radius);

  rr.size.height = rr.size.width = radius * 2;

  ret = JS_NewObject(ctx);

  JS_SetPropertyStr(ctx, ret, "center", js_point_new(ctx, rr.center));
  JS_SetPropertyStr(ctx, ret, "radius", JS_NewFloat64(ctx, radius));

  return ret;
}

static JSValue
js_contour_minenclosingtriangle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  cv::RotatedRect rr;

  JSContourData<float> contour, triangle;
  JSPointData<float> center;
  float radius;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  std::copy(v->begin(), v->end(), std::back_inserter(contour));

  cv::minEnclosingTriangle(contour, triangle);

  ret = js_contour_new(ctx, triangle);

  return ret;
}

static JSValue
js_contour_pointpolygontest(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  cv::RotatedRect rr;
  JSPointData<float> pt;
  bool measureDist = false;
  JSContourData<float> contour, triangle;
  JSPointData<double> point;
  double retval;

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

  std::copy(v->begin(), v->end(), std::back_inserter(contour));

  retval = cv::pointPolygonTest(contour, pt, measureDist);

  ret = JS_NewFloat64(ctx, retval);

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
js_contour_psimpl(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
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
js_contour_push(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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

static JSValue
js_contour_pop(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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

static JSValue
js_contour_unshift(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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

static JSValue
js_contour_shift(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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

static JSValue
js_contour_concat(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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
  std::copy(other->cbegin(), other->cend(), std::back_inserter(*r));
  return ret;
}

static JSValue
js_contour_rotatedrectangleintersection(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  bool handleNested = true;
  JSContourData<float> a, b, intersection;

  if(!(v = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(argc > 0) {
    other = js_contour_data2(ctx, argv[0]);
  }

  std::copy(v->begin(), v->end(), std::back_inserter(a));
  std::copy(other->begin(), other->end(), std::back_inserter(b));

  {
    cv::RotatedRect rra(a[0], a[1], a[2]);
    cv::RotatedRect rrb(b[0], b[1], b[2]);

    cv::rotatedRectangleIntersection(rra, rrb, intersection);

    ret = js_contour_new(ctx, intersection);
  }
  return ret;
}

static JSValue
js_contour_rotatepoints(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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

static JSValue
js_contour_toarray(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>* s;
  uint32_t i, size;
  JSValue ret = JS_UNDEFINED;

  if(!(s = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;
  size = s->size();
  ret = JS_NewArray(ctx);

  for(i = 0; i < size; i++) { JS_SetPropertyUint32(ctx, ret, i, js_point_clone(ctx, (*s)[i])); }

  return ret;
}

static JSValue
js_contour_tostring(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
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

static JSValue
js_contour_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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

enum { PROP_ASPECT_RATIO = 0, PROP_EXTENT, PROP_SOLIDITY, PROP_EQUIVALENT_DIAMETER, PROP_ORIENTATION };

static JSValue
js_contour_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSContourData<double>* contour;
  JSValue ret = JS_UNDEFINED;

  if(!(contour = static_cast<JSContourData<double>*>(JS_GetOpaque(this_val, js_contour_class_id)))) {
    printf("js_contour_get   cid=%i this_val=%p contour=%p\n", JS_GetClassID(this_val), JS_VALUE_GET_OBJ(this_val), contour);
    return JS_UNDEFINED;
  }

  switch(magic) {
    case PROP_ASPECT_RATIO: {
      JSContourData<float> points;
      JSRectData<float> rect;

      points.resize(contour->size());

      std::copy(contour->begin(), contour->end(), points.begin());

      rect = cv::boundingRect(points);

      ret = JS_NewFloat64(ctx, (double)rect.width / rect.height);
      break;
    }
    case PROP_EXTENT: {
      double area = cv::contourArea(*contour);
      JSRectData<double> rect = cv::boundingRect(*contour);

      ret = JS_NewFloat64(ctx, area / (rect.width * rect.height));
      break;
    }
    case PROP_SOLIDITY: {
      double area = cv::contourArea(*contour);
      JSContourData<double> hull;
      cv::convexHull(*contour, hull);
      double hullArea = cv::contourArea(hull);

      ret = JS_NewFloat64(ctx, area / cv::contourArea(hull));
      break;
    }
    case PROP_EQUIVALENT_DIAMETER: {
      double area = cv::contourArea(*contour);

      ret = JS_NewFloat64(ctx, std::sqrt(4 * area / 3.14159265358979323846));
      break;
    }
    case PROP_ORIENTATION: {
      cv::RotatedRect rect = cv::fitEllipse(*contour);

      ret = JS_NewFloat64(ctx, rect.angle);
      break;
    }
  }
  return ret;
}

static JSValue
js_contour_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>* contour;

  if(!(contour = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;
  assert(js_contour_class_id);
  JSValue obj = JS_NewObjectClass(ctx, js_contour_class_id);

  JS_DefinePropertyValueStr(ctx, obj, "length", JS_NewUint32(ctx, contour->size()), JS_PROP_ENUMERABLE);
  return obj;
}

void
js_contour_finalizer(JSRuntime* rt, JSValue this_val) {
  JSContourData<double>* contour;

  assert(js_contour_class_id);
  contour = static_cast<JSContourData<double>*>(JS_GetOpaque(this_val, js_contour_class_id));
  assert(contour);
  // printf("js_contour_finalizer  cid=%i this_val=%p contour=%p\n", JS_GetClassID(this_val), JS_VALUE_GET_OBJ(this_val), contour);

  contour_deallocate(rt, contour);
  JS_FreeValueRT(rt, this_val);
}

extern "C" {

static int
js_contour_get_own_property(JSContext* ctx, JSPropertyDescriptor* pdesc, JSValueConst obj, JSAtom prop) {
  JSContourData<double>* contour;
  JSValue value = JS_UNDEFINED;
  uint32_t index;

  if(!(contour = js_contour_data(obj)))
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
    if(JS_IsObject(proto))
      value = JS_GetProperty(ctx, proto, prop);
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
js_contour_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSContourData<double>* s;

  if(!(s = js_contour_data2(ctx, this_val)))
    return JS_EXCEPTION;

  return js_point_iterator_new(ctx, s->data(), s->data() + s->size(), magic);
}

const JSCFunctionListEntry js_contour_proto_funcs[] = {
    JS_CFUNC_DEF("push", 1, js_contour_push),
    JS_CFUNC_DEF("pop", 0, js_contour_pop),
    JS_CFUNC_DEF("unshift", 1, js_contour_unshift),
    JS_CFUNC_DEF("shift", 0, js_contour_shift),
    JS_CFUNC_DEF("concat", 1, js_contour_concat),
    JS_CFUNC_DEF("getMat", 0, js_contour_getmat),
    JS_CGETSET_DEF("length", js_contour_length, NULL),
    JS_CGETSET_DEF("area", js_contour_area, NULL),
    JS_CGETSET_DEF("buffer", js_contour_buffer, NULL),
    JS_CGETSET_DEF("array", js_contour_array, NULL),
    JS_CGETSET_MAGIC_DEF("aspectRatio", js_contour_get, NULL, PROP_ASPECT_RATIO),
    JS_CGETSET_MAGIC_DEF("extent", js_contour_get, NULL, PROP_EXTENT),
    JS_CGETSET_MAGIC_DEF("solidity", js_contour_get, NULL, PROP_SOLIDITY),
    JS_CGETSET_MAGIC_DEF("equivalentDiameter", js_contour_get, NULL, PROP_EQUIVALENT_DIAMETER),
    JS_CGETSET_MAGIC_DEF("orientation", js_contour_get, NULL, PROP_ORIENTATION),

    JS_CFUNC_DEF("approxPolyDP", 1, js_contour_approxpolydp),
    JS_CFUNC_DEF("convexHull", 1, js_contour_convexhull),
    JS_CFUNC_DEF("boundingRect", 0, js_contour_boundingrect),
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
    JS_CFUNC_MAGIC_DEF("toSource", 0, js_contour_tostring, 1),
    JS_CFUNC_MAGIC_DEF("lines", 0, js_contour_iterator, NEXT_LINE),
    JS_CFUNC_MAGIC_DEF("points", 0, js_contour_iterator, NEXT_POINT),
    JS_ALIAS_DEF("[Symbol.iterator]", "points"),
    JS_ALIAS_DEF("size", "length"),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Contour", JS_PROP_CONFIGURABLE),

};
const JSCFunctionListEntry js_contour_static_funcs[] = {
    JS_CFUNC_DEF("fromRect", 1, js_contour_rect),
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
  js_set_inspect_method(ctx, contour_proto, js_contour_inspect);

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
