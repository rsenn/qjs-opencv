#include "js_mat.hpp"
#include "js_vector.hpp"
#include "include/js_array.hpp"
#include "include/jsbindings.hpp"
#include "include/psimpl.hpp"
#include <quickjs.h>
#include <opencv2/core.hpp>
#include <vector>

using cv::Point;
using cv::Point2f;

enum {
  SIMPLIFY_REUMANN_WITKAM = 0,
  SIMPLIFY_OPHEIM,
  SIMPLIFY_LANG,
  SIMPLIFY_DOUGLAS_PEUCKER,
  SIMPLIFY_NTH_POINT,
  SIMPLIFY_RADIAL_DISTANCE,
  SIMPLIFY_PERPENDICULAR_DISTANCE
};

/**
 * @brief Run one of the 7 psimpl algorithms directly from a caller-owned
 * [first, first + coord_count) source buffer into a caller-owned
 * `out_first` destination buffer (assumed sized for the worst case, i.e.
 * >= coord_count) - no intermediate buffer or copy on either side. `T` is
 * the coordinate's own storage type (int32_t for CV_32SC2/PointVector,
 * double for CV_64FC2, float for Point2fVector), so integer sources run
 * the algorithms with integer tolerances, matching their native
 * precision. Returns the number of coordinates actually written.
 */
template<typename T>
static size_t
js_psimpl_run(const T* first, size_t coord_count, T* out_first, double arg1, double arg2, int magic) {
  const T* end = first + coord_count;
  T* out_ptr = out_first;

  switch(magic) {
    case SIMPLIFY_REUMANN_WITKAM: {
      T tol = arg1 != 0 ? T(arg1) : T(2);
      out_ptr = psimpl::simplify_reumann_witkam<2>(first, end, tol, out_ptr);
      break;
    }

    case SIMPLIFY_OPHEIM: {
      T minTol = arg1 != 0 ? T(arg1) : T(2);
      T maxTol = arg2 != 0 ? T(arg2) : T(10);
      out_ptr = psimpl::simplify_opheim<2>(first, end, minTol, maxTol, out_ptr);
      break;
    }

    case SIMPLIFY_LANG: {
      T tol = arg1 != 0 ? T(arg1) : T(2);
      unsigned lookAhead = arg2 != 0 ? unsigned(arg2) : 10;
      out_ptr = psimpl::simplify_lang<2>(first, end, tol, lookAhead, out_ptr);
      break;
    }

    case SIMPLIFY_DOUGLAS_PEUCKER: {
      T tol = arg1 != 0 ? T(arg1) : T(2);
      out_ptr = psimpl::simplify_douglas_peucker<2>(first, end, tol, out_ptr);
      break;
    }

    case SIMPLIFY_NTH_POINT: {
      unsigned n = arg1 != 0 ? unsigned(arg1) : 2;
      out_ptr = psimpl::simplify_nth_point<2>(first, end, n, out_ptr);
      break;
    }

    case SIMPLIFY_RADIAL_DISTANCE: {
      T tol = arg1 != 0 ? T(arg1) : T(2);
      out_ptr = psimpl::simplify_radial_distance<2>(first, end, tol, out_ptr);
      break;
    }

    case SIMPLIFY_PERPENDICULAR_DISTANCE: {
      T tol = arg1 != 0 ? T(arg1) : T(2);
      unsigned repeat = arg2 != 0 ? unsigned(arg2) : 1;
      out_ptr = psimpl::simplify_perpendicular_distance<2>(first, end, tol, repeat, out_ptr);
      break;
    }
  }

  return size_t(out_ptr - out_first);
}

// Run into a freshly allocated 2-channel Mat, sized to the worst case
// up front (simplification only ever removes points) and then trimmed to
// the actual result count - cv::Mat::resize() only touches the row count
// metadata when shrinking, so this is not a copy.
template<typename T>
static JSValue
js_psimpl_run_to_mat(JSContext* ctx, const T* first, size_t coord_count, double arg1, double arg2, int magic, int matType) {
  cv::Mat out(int(coord_count / 2), 1, matType);
  size_t result_coords = js_psimpl_run<T>(first, coord_count, out.ptr<T>(), arg1, arg2, magic);
  out.resize(int(result_coords / 2));
  return js_mat_wrap(ctx, out);
}

// Run into a freshly allocated JSVector<T>, same worst-case-then-trim
// approach as js_psimpl_run_to_mat() - std::vector<T>::resize() to a
// smaller size doesn't reallocate either.
template<typename PointT, typename CoordT>
static JSValue
js_psimpl_run_to_vector(JSContext* ctx, const CoordT* first, size_t coord_count, double arg1, double arg2, int magic) {
  JSVector<PointT>* v = new JSVector<PointT>();
  v->vec->resize(coord_count / 2);
  CoordT* out_first = reinterpret_cast<CoordT*>(v->vec->data());
  size_t result_coords = js_psimpl_run<CoordT>(first, coord_count, out_first, arg1, arg2, magic);
  v->vec->resize(result_coords / 2);
  return v->toJS(ctx);
}

/**
 * cv.psimpl.* - polyline simplification, opencv.js-compatible namespace.
 *
 * Accepts, each read zero-copy (no per-point conversion of the input
 * polyline) and returned as the same type it was given:
 *   - Mat CV_32SC2 or Mat CV_64FC2 (must be continuous) -> same Mat type
 *   - cv.PointVector   (std::vector<cv::Point>, binary-compatible with
 *                        interleaved int32 pairs) -> cv.PointVector
 *   - cv.Point2fVector (std::vector<cv::Point2f>, binary-compatible with
 *                        interleaved float pairs) -> cv.Point2fVector
 *   - a plain JS array of points (no backing buffer to share, so this path
 *     does copy on read) -> Mat CV_32SC2
 *
 * The simplification itself writes straight into the final output
 * container (Mat / PointVector / Point2fVector), sized to the worst case
 * up front and trimmed down afterwards - no intermediate buffer.
 */
static JSValue
js_psimpl_simplify(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  if(argc < 1)
    return JS_ThrowTypeError(ctx, "Expected at least 1 argument");

  double arg1 = 0, arg2 = 0;
  if(argc > 1) {
    JS_ToFloat64(ctx, &arg1, argv[1]);
    if(argc > 2)
      JS_ToFloat64(ctx, &arg2, argv[2]);
  }

  JSMatData* mat;
  JSVector<Point>* pointVec;
  JSVector<Point2f>* point2fVec;

  if((mat = js_mat_data_nothrow(argv[0]))) {
    if(!mat->isContinuous())
      return JS_ThrowTypeError(ctx, "Expected a continuous Mat");

    if(mat->type() == CV_32SC2)
      return js_psimpl_run_to_mat<int32_t>(ctx, mat->ptr<int32_t>(), size_t(mat->total()) * 2, arg1, arg2, magic, CV_32SC2);

    if(mat->type() == CV_64FC2)
      return js_psimpl_run_to_mat<double>(ctx, mat->ptr<double>(), size_t(mat->total()) * 2, arg1, arg2, magic, CV_64FC2);

    return JS_ThrowTypeError(ctx, "Expected Mat CV_32SC2 or CV_64FC2");
  }

  if((pointVec = JSVector<Point>::fromJS(argv[0])))
    return js_psimpl_run_to_vector<Point>(ctx, reinterpret_cast<const int32_t*>(pointVec->vec->data()), pointVec->vec->size() * 2, arg1, arg2, magic);

  if((point2fVec = JSVector<Point2f>::fromJS(argv[0])))
    return js_psimpl_run_to_vector<Point2f>(ctx, reinterpret_cast<const float*>(point2fVec->vec->data()), point2fVec->vec->size() * 2, arg1, arg2, magic);

  if(js_is_array(ctx, argv[0])) {
    std::vector<cv::Point> points;
    js_array_to(ctx, argv[0], points);
    return js_psimpl_run_to_mat<int32_t>(ctx, reinterpret_cast<const int32_t*>(points.data()), points.size() * 2, arg1, arg2, magic, CV_32SC2);
  }

  return JS_ThrowTypeError(ctx, "Expected Mat, PointVector, Point2fVector, or array");
}

static const JSCFunctionListEntry js_psimpl_funcs[] = {
    JS_CFUNC_MAGIC_DEF("reumannWitkam", 1, js_psimpl_simplify, SIMPLIFY_REUMANN_WITKAM),
    JS_CFUNC_MAGIC_DEF("opheim", 1, js_psimpl_simplify, SIMPLIFY_OPHEIM),
    JS_CFUNC_MAGIC_DEF("lang", 1, js_psimpl_simplify, SIMPLIFY_LANG),
    JS_CFUNC_MAGIC_DEF("douglasPeucker", 1, js_psimpl_simplify, SIMPLIFY_DOUGLAS_PEUCKER),
    JS_CFUNC_MAGIC_DEF("nthPoint", 1, js_psimpl_simplify, SIMPLIFY_NTH_POINT),
    JS_CFUNC_MAGIC_DEF("radialDistance", 1, js_psimpl_simplify, SIMPLIFY_RADIAL_DISTANCE),
    JS_CFUNC_MAGIC_DEF("perpendicularDistance", 1, js_psimpl_simplify, SIMPLIFY_PERPENDICULAR_DISTANCE),
};

static const JSCFunctionListEntry js_psimpl_static_funcs[] = {
    JS_OBJECT_DEF("psimpl", js_psimpl_funcs, countof(js_psimpl_funcs), JS_PROP_C_W_E),
};

extern "C" int
js_psimpl_init(JSContext* ctx, JSModuleDef* m) {
  if(m)
    JS_SetModuleExportList(ctx, m, js_psimpl_static_funcs, countof(js_psimpl_static_funcs));

  return 0;
}

extern "C" void
js_psimpl_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_psimpl_static_funcs, countof(js_psimpl_static_funcs));
}
