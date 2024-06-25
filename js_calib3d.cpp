#include "js_alloc.hpp"
#include "js_size.hpp"
#include "js_mat.hpp"
#include "js_umat.hpp"
#include <opencv2/calib3d.hpp>

extern "C" int js_calib3d_init(JSContext*, JSModuleDef*);

enum {
  FIND_CHESSBOARD_CORNERS,
  FIND_CHESSBOARD_CORNERS_SB,
  ESTIMATE_AFFINE_2D,
  ESTIMATE_AFFINE_3D,
  ESTIMATE_AFFINE_PARTIAL_2D,
  FIND_HOMOGRAPHY,
};

static JSValue
js_calib3d_functions(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  switch(magic) {
    case FIND_CHESSBOARD_CORNERS: {
      JSInputOutputArray image = js_umat_or_mat(ctx, argv[0]);
      cv::Size pattern_size = js_size_get(ctx, argv[1]);
      std::vector<cv::Point2f> corners;
      int32_t flags = cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE;

      if(argc > 3)
        JS_ToInt32(ctx, &flags, argv[3]);

      BOOL result = cv::findChessboardCorners(image, pattern_size, corners, flags);

      js_array_copy(ctx, argv[2], corners);

      ret = JS_NewBool(ctx, result);
      break;
    }

    case FIND_CHESSBOARD_CORNERS_SB: {
      JSInputOutputArray image = js_umat_or_mat(ctx, argv[0]);
      cv::Size pattern_size = js_size_get(ctx, argv[1]);
      std::vector<cv::Point2f> corners;
      int32_t flags = cv::CALIB_CB_EXHAUSTIVE | cv::CALIB_CB_NORMALIZE_IMAGE;
      JSOutputArray meta = cv::noArray();

      if(argc > 3)
        JS_ToInt32(ctx, &flags, argv[3]);
      if(argc > 4)
        meta = js_cv_outputarray(ctx, argv[4]);

      BOOL result =
          argc > 3 ? cv::findChessboardCornersSB(image, pattern_size, corners, flags, meta) : cv::findChessboardCornersSB(image, pattern_size, corners);

      js_array_copy(ctx, argv[2], corners);

      ret = JS_NewBool(ctx, result);
      break;
    }

    case ESTIMATE_AFFINE_2D: {
      JSInputArray from = js_input_array(ctx, argv[0]), to = js_input_array(ctx, argv[1]);
      JSOutputArray inliers = cv::noArray();
      int32_t method = cv::RANSAC;
      double ransacReprojThreshold = 3;
      uint32_t maxIters = 2000;
      double confidence = 0.99;
      uint32_t refineIters = 10;

      if(argc > 3)
        JS_ToInt32(ctx, &method, argv[3]);
      if(argc > 4)
        JS_ToFloat64(ctx, &ransacReprojThreshold, argv[4]);
      if(argc > 5)
        JS_ToUint32(ctx, &maxIters, argv[5]);
      if(argc > 6)
        JS_ToFloat64(ctx, &confidence, argv[6]);
      if(argc > 6)
        JS_ToUint32(ctx, &refineIters, argv[7]);

      cv::Mat mat = cv::estimateAffine2D(from, to, inliers, method, ransacReprojThreshold, maxIters, confidence, refineIters);

      ret = js_mat_wrap(ctx, mat);
      break;
    }

    case ESTIMATE_AFFINE_3D: {
      JSInputArray src = js_input_array(ctx, argv[0]), dst = js_input_array(ctx, argv[1]);
      JSOutputArray out = js_cv_outputarray(ctx, argv[2]), inliers = js_cv_outputarray(ctx, argv[3]);
      double ransacThreshold = 3;
      double confidence = 0.99;

      if(argc > 4)
        JS_ToFloat64(ctx, &ransacThreshold, argv[4]);

      if(argc > 5)
        JS_ToFloat64(ctx, &confidence, argv[5]);

      int result = cv::estimateAffine3D(src, dst, out, inliers, ransacThreshold, confidence);

      ret = JS_NewInt32(ctx, result);
      break;
    }

    case ESTIMATE_AFFINE_PARTIAL_2D: {
      JSInputArray from = js_input_array(ctx, argv[0]), to = js_input_array(ctx, argv[1]);
      JSOutputArray inliers = cv::noArray();
      int32_t method = cv::RANSAC;
      double ransacReprojThreshold = 3;
      uint32_t maxIters = 2000;
      double confidence = 0.99;
      uint32_t refineIters = 10;

      if(argc > 3)
        JS_ToInt32(ctx, &method, argv[3]);
      if(argc > 4)
        JS_ToFloat64(ctx, &ransacReprojThreshold, argv[4]);
      if(argc > 5)
        JS_ToUint32(ctx, &maxIters, argv[5]);
      if(argc > 6)
        JS_ToFloat64(ctx, &confidence, argv[6]);
      if(argc > 6)
        JS_ToUint32(ctx, &refineIters, argv[7]);

      cv::Mat mat = cv::estimateAffinePartial2D(from, to, inliers, method, ransacReprojThreshold, maxIters, confidence, refineIters);

      ret = js_mat_wrap(ctx, mat);
      break;
    }

    case FIND_HOMOGRAPHY: {
      JSInputArray src = js_input_array(ctx, argv[0]);
      JSInputArray dst = js_input_array(ctx, argv[1]);
      cv::Mat mat;
      JSOutputArray mask = cv::noArray();
      int32_t method = 0;
      double ransacReprojThreshold = 3;

      if(argc > 2 && !JS_IsNumber(argv[2])) {
        mask = js_cv_outputarray(ctx, argv[2]);

        if(argc > 3)
          JS_ToInt32(ctx, &method, argv[3]);

        if(argc > 4)
          JS_ToFloat64(ctx, &ransacReprojThreshold, argv[4]);

        mat = cv::findHomography(src, dst, mask, method, ransacReprojThreshold);

      } else {
        int32_t maxIters = 2000;
        double confidence = 0.995;

        JS_ToInt32(ctx, &method, argv[2]);

        if(argc > 3)
          JS_ToFloat64(ctx, &ransacReprojThreshold, argv[3]);

        if(argc > 4)
          mask = js_cv_outputarray(ctx, argv[4]);

        if(argc > 5)
          JS_ToInt32(ctx, &maxIters, argv[5]);
        if(argc > 6)
          JS_ToFloat64(ctx, &confidence, argv[6]);

        mat = cv::findHomography(src, dst, method, ransacReprojThreshold, mask, maxIters, confidence);
      }

      ret = js_mat_wrap(ctx, mat);
      break;
    }
  }

  return ret;
}

const JSCFunctionListEntry js_calib3d_static_funcs[] = {
    JS_CFUNC_MAGIC_DEF("findChessboardCorners", 3, js_calib3d_functions, FIND_CHESSBOARD_CORNERS),
    JS_CFUNC_MAGIC_DEF("findChessboardCornersSB", 5, js_calib3d_functions, FIND_CHESSBOARD_CORNERS_SB),
    JS_CFUNC_MAGIC_DEF("estimateAffine2D", 2, js_calib3d_functions, ESTIMATE_AFFINE_2D),
    JS_CFUNC_MAGIC_DEF("estimateAffine3D", 4, js_calib3d_functions, ESTIMATE_AFFINE_3D),
    JS_CFUNC_MAGIC_DEF("estimateAffinePartial2D", 2, js_calib3d_functions, ESTIMATE_AFFINE_PARTIAL_2D),
    JS_CFUNC_MAGIC_DEF("findHomography", 2, js_calib3d_functions, FIND_HOMOGRAPHY),
    JS_PROP_INT32_DEF("CALIB_CB_ADAPTIVE_THRESH", cv::CALIB_CB_ADAPTIVE_THRESH, 0),
    JS_PROP_INT32_DEF("CALIB_CB_NORMALIZE_IMAGE", cv::CALIB_CB_NORMALIZE_IMAGE, 0),
    JS_PROP_INT32_DEF("CALIB_CB_FILTER_QUADS", cv::CALIB_CB_FILTER_QUADS, 0),
    JS_PROP_INT32_DEF("CALIB_CB_FAST_CHECK", cv::CALIB_CB_FAST_CHECK, 0),
    JS_PROP_INT32_DEF("CALIB_CB_EXHAUSTIVE", cv::CALIB_CB_EXHAUSTIVE, 0),
    JS_PROP_INT32_DEF("CALIB_CB_ACCURACY", cv::CALIB_CB_ACCURACY, 0),
    JS_PROP_INT32_DEF("CALIB_CB_LARGER", cv::CALIB_CB_LARGER, 0),
    JS_PROP_INT32_DEF("CALIB_CB_MARKER", cv::CALIB_CB_MARKER, 0),
};

extern "C" int
js_calib3d_init(JSContext* ctx, JSModuleDef* bd) {
  if(bd)
    JS_SetModuleExportList(ctx, bd, js_calib3d_static_funcs, countof(js_calib3d_static_funcs));

  return 0;
}

#ifdef JS_Calib3D_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_calib3d
#endif

extern "C" void
js_calib3d_export(JSContext* ctx, JSModuleDef* bd) {
  JS_AddModuleExportList(ctx, bd, js_calib3d_static_funcs, countof(js_calib3d_static_funcs));
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* bd;
  bd = JS_NewCModule(ctx, module_name, &js_calib3d_init);

  if(!bd)
    return NULL;

  js_calib3d_export(ctx, bd);
  return bd;
}
