#include "js_mat.hpp"
#include "js_contour.hpp"
#include "js_point.hpp"
#include "include/js_array.hpp"
#include "include/psimpl.hpp"
#include <quickjs.h>
#include <opencv2/core.hpp>
#include <vector>

extern "C" {
extern int js_psimpl_init(JSContext* ctx, JSModuleDef* m);
extern void js_psimpl_export(JSContext* ctx, JSModuleDef* m);
}

enum {
  PSIMPL_REUMANN_WITKAM = 0,
  PSIMPL_OPHEIM,
  PSIMPL_LANG,
  PSIMPL_DOUGLAS_PEUCKER,
  PSIMPL_NTH_POINT,
  PSIMPL_RADIAL_DISTANCE,
  PSIMPL_PERPENDICULAR_DISTANCE
};

static JSValue
js_psimpl_simplify(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSMatData* mat = nullptr;
  JSContourData<double>* contour = nullptr;
  std::vector<cv::Point> points;
  double arg1 = 0, arg2 = 0;
  
  // Parse first argument: Mat or array
  if ((mat = js_mat_data2(ctx, argv[0]))) {
    // Convert Mat CV_32SC2 to vector<Point>
    if (mat->type() != CV_32SC2) {
      return JS_ThrowTypeError(ctx, "Expected Mat CV_32SC2");
    }
    
    int n = mat->rows * mat->cols;
    cv::Point* data = mat->ptr<cv::Point>();
    points.assign(data, data + n);
  } else if (js_contour_data2(ctx, argv[0])) {
    // Backward compatibility with Contour
    contour = js_contour_data2(ctx, argv[0]);
    points.reserve(contour->size());
    for (const auto& p : *contour) {
      points.emplace_back(static_cast<int>(p.x), static_cast<int>(p.y));
    }
  } else if (JS_IsArray(ctx, argv[0])) {
    // JS array of points
    js_array_to(ctx, argv[0], points);
  } else {
    return JS_ThrowTypeError(ctx, "Expected Mat, Contour, or array");
  }
  
  // Parse tolerance arguments
  if (argc > 1) {
    JS_ToFloat64(ctx, &arg1, argv[1]);
  }
  if (argc > 2) {
    JS_ToFloat64(ctx, &arg2, argv[2]);
  }
  
  // Perform simplification
  std::vector<cv::Point> result;
  result.resize(points.size()); // Reserve space
  
  double* start = reinterpret_cast<double*>(points.data());
  double* end = start + points.size() * 2;
  double* out_ptr = reinterpret_cast<double*>(result.data());
  
  switch (magic) {
    case PSIMPL_REUMANN_WITKAM:
      if (arg1 == 0) arg1 = 2;
      out_ptr = psimpl::simplify_reumann_witkam<2>(start, end, arg1, out_ptr);
      break;
      
    case PSIMPL_OPHEIM:
      if (arg1 == 0) arg1 = 2;
      if (arg2 == 0) arg2 = 10;
      out_ptr = psimpl::simplify_opheim<2>(start, end, arg1, arg2, out_ptr);
      break;
      
    case PSIMPL_LANG:
      if (arg1 == 0) arg1 = 2;
      if (arg2 == 0) arg2 = 10;
      out_ptr = psimpl::simplify_lang<2>(start, end, arg1, arg2, out_ptr);
      break;
      
    case PSIMPL_DOUGLAS_PEUCKER:
      if (arg1 == 0) arg1 = 2;
      out_ptr = psimpl::simplify_douglas_peucker<2>(start, end, arg1, out_ptr);
      break;
      
    case PSIMPL_NTH_POINT:
      if (arg1 == 0) arg1 = 2;
      out_ptr = psimpl::simplify_nth_point<2>(start, end, arg1, out_ptr);
      break;
      
    case PSIMPL_RADIAL_DISTANCE:
      if (arg1 == 0) arg1 = 2;
      out_ptr = psimpl::simplify_radial_distance<2>(start, end, arg1, out_ptr);
      break;
      
    case PSIMPL_PERPENDICULAR_DISTANCE:
      if (arg1 == 0) arg1 = 2;
      if (arg2 == 0) arg2 = 10;
      out_ptr = psimpl::simplify_perpendicular_distance<2>(start, end, arg1, arg2, out_ptr);
      break;
  }
  
  // Calculate actual size
  size_t result_size = (out_ptr - reinterpret_cast<double*>(result.data())) / 2;
  result.resize(result_size);
  
  // Return as Mat CV_32SC2
  cv::Mat out(result_size, 1, CV_32SC2);
  cv::Point* out_data = out.ptr<cv::Point>();
  std::copy(result.begin(), result.end(), out_data);
  
  return js_mat_wrap(ctx, out);
}

static const JSCFunctionListEntry psimpl_funcs[] = {
  JS_CFUNC_MAGIC_DEF("reumannWitkam", 1, js_psimpl_simplify, PSIMPL_REUMANN_WITKAM),
  JS_CFUNC_MAGIC_DEF("opheim", 1, js_psimpl_simplify, PSIMPL_OPHEIM),
  JS_CFUNC_MAGIC_DEF("lang", 1, js_psimpl_simplify, PSIMPL_LANG),
  JS_CFUNC_MAGIC_DEF("douglasPeucker", 1, js_psimpl_simplify, PSIMPL_DOUGLAS_PEUCKER),
  JS_CFUNC_MAGIC_DEF("nthPoint", 1, js_psimpl_simplify, PSIMPL_NTH_POINT),
  JS_CFUNC_MAGIC_DEF("radialDistance", 1, js_psimpl_simplify, PSIMPL_RADIAL_DISTANCE),
  JS_CFUNC_MAGIC_DEF("perpendicularDistance", 1, js_psimpl_simplify, PSIMPL_PERPENDICULAR_DISTANCE),
};

extern "C" int
js_psimpl_init(JSContext* ctx, JSModuleDef* m) {
  JSValue psimpl_object = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, psimpl_object, psimpl_funcs, countof(psimpl_funcs));
  
  if (m) {
    JS_SetModuleExport(ctx, m, "psimpl", psimpl_object);
  }
  
  return 0;
}

extern "C" void
js_psimpl_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "psimpl");
}
