#include <quickjs.h>
#include "util.hpp"
#include <stddef.h>

extern "C" int js_clahe_init(JSContext*, JSModuleDef*);
extern "C" int js_contour_init(JSContext*, JSModuleDef*);
extern "C" int js_cv_init(JSContext*, JSModuleDef*);
extern "C" int js_draw_init(JSContext*, JSModuleDef*);
extern "C" int js_line_init(JSContext*, JSModuleDef*);
extern "C" int js_mat_init(JSContext*, JSModuleDef*);
extern "C" int js_affine3_init(JSContext*, JSModuleDef*);
extern "C" int js_point_init(JSContext*, JSModuleDef*);
extern "C" int js_point_iterator_init(JSContext*, JSModuleDef*);
extern "C" int js_rect_init(JSContext*, JSModuleDef*);
extern "C" int js_rotated_rect_init(JSContext*, JSModuleDef*);
extern "C" int js_size_init(JSContext*, JSModuleDef*);
extern "C" int js_slice_iterator_init(JSContext*, JSModuleDef*);
extern "C" int js_subdiv2d_init(JSContext*, JSModuleDef*);
extern "C" int js_umat_init(JSContext*, JSModuleDef*);
extern "C" int js_utility_init(JSContext*, JSModuleDef*);
extern "C" int js_video_capture_init(JSContext*, JSModuleDef*);
extern "C" int js_line_segment_detector_init(JSContext*, JSModuleDef*);
extern "C" int js_fast_line_detector_init(JSContext*, JSModuleDef*);
extern "C" int js_highgui_init(JSContext*, JSModuleDef*);
extern "C" int js_imgproc_init(JSContext*, JSModuleDef*);
extern "C" int js_video_writer_init(JSContext*, JSModuleDef*);
extern "C" int js_keypoint_init(JSContext*, JSModuleDef*);
extern "C" int js_feature2d_init(JSContext*, JSModuleDef*);
extern "C" int js_libcamera_app_init(JSContext*, JSModuleDef*);
extern "C" int js_raspi_cam_init(JSContext*, JSModuleDef*);
extern "C" int js_bg_subtractor_init(JSContext*, JSModuleDef*);
extern "C" int js_white_balancer_init(JSContext*, JSModuleDef*);
extern "C" int js_barcode_detector_init(JSContext*, JSModuleDef*);
extern "C" int js_calib3d_init(JSContext*, JSModuleDef*);
extern "C" int js_algorithms_init(JSContext*, JSModuleDef*);
extern "C" int js_ximgproc_init(JSContext*, JSModuleDef*);
extern "C" int js_filestorage_init(JSContext*, JSModuleDef*);
extern "C" int js_filenode_init(JSContext*, JSModuleDef*);
extern "C" int js_line_iterator_init(JSContext*, JSModuleDef*);
extern "C" int js_fisheye_init(JSContext*, JSModuleDef*);
extern "C" int js_matx_init(JSContext*, JSModuleDef*);
extern "C" int js_aruco_init(JSContext*, JSModuleDef*);

extern "C" void js_clahe_export(JSContext*, JSModuleDef*);
extern "C" void js_contour_export(JSContext*, JSModuleDef*);
extern "C" void js_cv_export(JSContext*, JSModuleDef*);
extern "C" void js_draw_export(JSContext*, JSModuleDef*);
extern "C" void js_line_export(JSContext*, JSModuleDef*);
extern "C" void js_mat_export(JSContext*, JSModuleDef*);
extern "C" void js_affine3_export(JSContext*, JSModuleDef*);
extern "C" void js_point_export(JSContext*, JSModuleDef*);
extern "C" void js_point_iterator_export(JSContext*, JSModuleDef*);
extern "C" void js_rect_export(JSContext*, JSModuleDef*);
extern "C" void js_rotated_rect_export(JSContext*, JSModuleDef*);
extern "C" void js_size_export(JSContext*, JSModuleDef*);
extern "C" void js_slice_iterator_export(JSContext*, JSModuleDef*);
extern "C" void js_subdiv2d_export(JSContext*, JSModuleDef*);
extern "C" void js_umat_export(JSContext*, JSModuleDef*);
extern "C" void js_utility_export(JSContext*, JSModuleDef*);
extern "C" void js_video_capture_export(JSContext*, JSModuleDef*);
extern "C" void js_line_segment_detector_export(JSContext*, JSModuleDef*);
extern "C" void js_fast_line_detector_export(JSContext*, JSModuleDef*);
extern "C" void js_highgui_export(JSContext*, JSModuleDef*);
extern "C" void js_imgproc_export(JSContext*, JSModuleDef*);
extern "C" void js_video_writer_export(JSContext*, JSModuleDef*);
extern "C" void js_keypoint_export(JSContext*, JSModuleDef*);
extern "C" void js_feature2d_export(JSContext*, JSModuleDef*);
extern "C" void js_libcamera_app_export(JSContext*, JSModuleDef*);
extern "C" void js_raspi_cam_export(JSContext*, JSModuleDef*);
extern "C" void js_bg_subtractor_export(JSContext*, JSModuleDef*);
extern "C" void js_white_balancer_export(JSContext*, JSModuleDef*);
extern "C" void js_barcode_detector_export(JSContext*, JSModuleDef*);
extern "C" void js_calib3d_export(JSContext*, JSModuleDef*);
extern "C" void js_algorithms_export(JSContext*, JSModuleDef*);
extern "C" void js_ximgproc_export(JSContext*, JSModuleDef*);
extern "C" void js_filestorage_export(JSContext*, JSModuleDef*);
extern "C" void js_filenode_export(JSContext*, JSModuleDef*);
extern "C" void js_line_iterator_export(JSContext*, JSModuleDef*);
extern "C" void js_fisheye_export(JSContext*, JSModuleDef*);
extern "C" void js_matx_export(JSContext*, JSModuleDef*);
extern "C" void js_aruco_export(JSContext*, JSModuleDef*);

int
js_opencv_init(JSContext* ctx, JSModuleDef* m) {
  js_cv_init(ctx, m);
  js_highgui_init(ctx, m);
  js_imgproc_init(ctx, m);
  js_clahe_init(ctx, m);
  js_contour_init(ctx, m);
  js_draw_init(ctx, m);
  js_line_init(ctx, m);
  js_line_iterator_init(ctx, m);
  js_mat_init(ctx, m);
  js_affine3_init(ctx, m);
  js_point_init(ctx, m);
  js_point_iterator_init(ctx, m);
  js_rect_init(ctx, m);
  js_rotated_rect_init(ctx, m);
  js_size_init(ctx, m);
  js_slice_iterator_init(ctx, m);
  js_subdiv2d_init(ctx, m);
  js_umat_init(ctx, m);
  js_utility_init(ctx, m);
  js_video_capture_init(ctx, m);
  js_video_writer_init(ctx, m);
  js_line_segment_detector_init(ctx, m);
  js_fast_line_detector_init(ctx, m);
  js_keypoint_init(ctx, m);
#ifdef USE_FEATURE2D
  js_feature2d_init(ctx, m);
#endif
#ifdef USE_LIBCAMERA
  js_libcamera_app_init(ctx, m);
#endif
#ifdef USE_LCCV
  js_raspi_cam_init(ctx, m);
#endif
  js_bg_subtractor_init(ctx, m);
  js_white_balancer_init(ctx, m);
#ifdef USE_BARCODE
  js_barcode_detector_init(ctx, m);
#endif
  js_calib3d_init(ctx, m);
  js_algorithms_init(ctx, m);
#ifdef HAVE_OPENCV2_XIMGPROC_HPP
  js_ximgproc_init(ctx, m);
#endif
  js_filestorage_init(ctx, m);
  js_filenode_init(ctx, m);
  js_fisheye_init(ctx, m);
  // js_matx_init(ctx, m);
  js_aruco_init(ctx, m);

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

  if(!(m = JS_NewCModule(ctx, module_name, &js_opencv_init)))
    return NULL;

  js_cv_export(ctx, m);
  js_highgui_export(ctx, m);
  js_imgproc_export(ctx, m);
  js_clahe_export(ctx, m);
  js_contour_export(ctx, m);
  js_draw_export(ctx, m);
  js_line_export(ctx, m);
  js_line_iterator_export(ctx, m);
  js_mat_export(ctx, m);
  js_affine3_export(ctx, m);
  js_point_export(ctx, m);
  js_point_iterator_export(ctx, m);
  js_rect_export(ctx, m);
  js_rotated_rect_export(ctx, m);
  js_size_export(ctx, m);
  js_slice_iterator_export(ctx, m);
  js_subdiv2d_export(ctx, m);
  js_umat_export(ctx, m);
  js_utility_export(ctx, m);
  js_video_capture_export(ctx, m);
  js_video_writer_export(ctx, m);
  js_line_segment_detector_export(ctx, m);
  js_fast_line_detector_export(ctx, m);
  js_keypoint_export(ctx, m);
#ifdef USE_FEATURE2D
  js_feature2d_export(ctx, m);
#endif
#ifdef USE_LIBCAMERA
  js_libcamera_app_export(ctx, m);
#endif
#ifdef USE_LCCV
  js_raspi_cam_export(ctx, m);
#endif
  js_bg_subtractor_export(ctx, m);
  js_white_balancer_export(ctx, m);
#ifdef USE_BARCODE
  js_barcode_detector_export(ctx, m);
#endif
  js_calib3d_export(ctx, m);
  js_algorithms_export(ctx, m);
#ifdef HAVE_OPENCV2_XIMGPROC_HPP
  js_ximgproc_export(ctx, m);
#endif
  js_filestorage_export(ctx, m);
  js_filenode_export(ctx, m);
  js_fisheye_export(ctx, m);
  // js_matx_export(ctx, m);
  js_aruco_export(ctx, m);

  return m;
}
