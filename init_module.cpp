#include "quickjs.h"
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

extern "C" void js_clahe_export(JSContext*, JSModuleDef*);
extern "C" void js_contour_export(JSContext*, JSModuleDef*);
extern "C" void js_cv_export(JSContext*, JSModuleDef*);
extern "C" void js_draw_export(JSContext*, JSModuleDef*);
extern "C" void js_line_export(JSContext*, JSModuleDef*);
extern "C" void js_mat_export(JSContext*, JSModuleDef*);
extern "C" void js_point_export(JSContext*, JSModuleDef*);
extern "C" void js_point_iterator_export(JSContext*, JSModuleDef*);
extern "C" void js_rect_export(JSContext*, JSModuleDef*);
extern "C" void js_size_export(JSContext*, JSModuleDef*);
extern "C" void js_slice_iterator_export(JSContext*, JSModuleDef*);
extern "C" void js_subdiv2d_export(JSContext*, JSModuleDef*);
extern "C" void js_umat_export(JSContext*, JSModuleDef*);
extern "C" void js_utility_export(JSContext*, JSModuleDef*);
extern "C" void js_video_capture_export(JSContext*, JSModuleDef*);

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

#ifdef JS_OPENCV_MODULE
#define JS_INIT_MODULE js_init_module
#else
#define JS_INIT_MODULE js_init_module_opencv
#endif

extern "C" VISIBLE JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_opencv_init);
  if(!m)
    return NULL;

  js_clahe_export(ctx, m);
  js_contour_export(ctx, m);
  js_cv_export(ctx, m);
  js_draw_export(ctx, m);
  js_line_export(ctx, m);
  js_mat_export(ctx, m);
  js_point_export(ctx, m);
  js_point_iterator_export(ctx, m);
  js_rect_export(ctx, m);
  js_size_export(ctx, m);
  js_slice_iterator_export(ctx, m);
  js_subdiv2d_export(ctx, m);
  js_umat_export(ctx, m);
  js_utility_export(ctx, m);
  js_video_capture_export(ctx, m);

  return m;
}
