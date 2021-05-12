#include "quickjs/quickjs.h"
#include "util.hpp"

extern "C" int js_clahe_init(JSContext*, JSModuleDef*);
extern "C" int js_contour_init(JSContext*, JSModuleDef*);
extern "C" int js_cv_init(JSContext*, JSModuleDef*);
extern "C" int js_draw_init(JSContext*, JSModuleDef*);
extern "C" int js_line_init(JSContext*, JSModuleDef*);
extern "C" int js_mat_init(JSContext*, JSModuleDef*);
extern "C" int js_point_init(JSContext*, JSModuleDef*);
extern "C" int js_point_iterator_init(JSContext*, JSModuleDef*);
extern "C" int js_rect_init(JSContext*, JSModuleDef*);
extern "C" int js_size_init(JSContext*, JSModuleDef*);
extern "C" int js_slice_iterator_init(JSContext*, JSModuleDef*);
extern "C" int js_subdiv2d_init(JSContext*, JSModuleDef*);
extern "C" int js_umat_init(JSContext*, JSModuleDef*);
extern "C" int js_utility_init(JSContext*, JSModuleDef*);
extern "C" int js_video_capture_init(JSContext*, JSModuleDef*);

int
js_opencv_init(JSContext* ctx, JSModuleDef* m) {
  js_clahe_init(ctx, m);
  js_contour_init(ctx, m);
  js_cv_init(ctx, m);
  js_draw_init(ctx, m);
  js_line_init(ctx, m);
  js_mat_init(ctx, m);
  js_point_init(ctx, m);
  js_point_iterator_init(ctx, m);
  js_rect_init(ctx, m);
  js_size_init(ctx, m);
  js_slice_iterator_init(ctx, m);
  js_subdiv2d_init(ctx, m);
  js_umat_init(ctx, m);
  js_utility_init(ctx, m);
  js_video_capture_init(ctx, m);

  return 0;
}

extern "C" VISIBLE JSModuleDef*
js_init_module(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_opencv_init);
  if(!m)
    return NULL;
  
  JS_AddModuleExport(ctx, m, "CLAHE");
  JS_AddModuleExport(ctx, m, "Contour");
  JS_AddModuleExport(ctx, m, "Draw");
  JS_AddModuleExport(ctx, m, "Line");
  JS_AddModuleExport(ctx, m, "Mat");
  JS_AddModuleExport(ctx, m, "Point");
  JS_AddModuleExport(ctx, m, "PointIterator");
  JS_AddModuleExport(ctx, m, "Rect");
  JS_AddModuleExport(ctx, m, "Size");
  JS_AddModuleExport(ctx, m, "SliceIterator");
  JS_AddModuleExport(ctx, m, "Subdiv2D");
  JS_AddModuleExport(ctx, m, "UMat");
  JS_AddModuleExport(ctx, m, "TickMeter");
  JS_AddModuleExport(ctx, m, "VideoCapture");

  return m;
}
