#include "js_cv.hpp"
#include "js_umat.hpp"
#include "js_affine3.hpp"
#include "include/jsbindings.hpp"
#include "include/js_inputoutputarray.hpp"
#include "include/js_matvec.hpp"
#include <quickjs.h>

#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

enum {
  FISHEYE_CALIBRATE = 0,
  FISHEYE_DISTORT_POINTS,
  FISHEYE_ESTIMATE_NEW_CAMERA_MATRIX_FOR_UNDISTORT_RECTIFY,
  FISHEYE_INIT_UNDISTORT_RECTIFY_MAP,
  FISHEYE_PROJECT_POINTS,
  FISHEYE_STEREO_CALIBRATE,
  FISHEYE_STEREO_RECTIFY,
  FISHEYE_UNDISTORT_IMAGE,
  FISHEYE_UNDISTORT_POINTS,
};

static JSValue
js_fisheye_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
    switch(magic) {
      case FISHEYE_CALIBRATE: {
        /* (objectPoints, imagePoints, imageSize, K, D, rvecs, tvecs, flags = 0, criteria) -> RMS error.
         * objectPoints/imagePoints are JS arrays of Mat (one per view), rvecs/tvecs JS arrays filled in */
        if(argc < 7)
          return JS_ThrowTypeError(ctx, "calibrate(objectPoints, imagePoints, imageSize, K, D, rvecs, tvecs, flags, criteria)");

        JSMatVec objectPoints(ctx, argv[0], false, true), imagePoints(ctx, argv[1], false, true);
        JSSizeData<int> image_size;
        js_value_to(ctx, argv[2], image_size);

        JSInputOutputArray K = js_cv_inputoutputarray(ctx, argv[3]), D = js_cv_inputoutputarray(ctx, argv[4]);
        JSMatVec rvecs(ctx, argv[5], true), tvecs(ctx, argv[6], true);
        int32_t flags = 0;
        cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, DBL_EPSILON);

        if(argc > 7)
          JS_ToInt32(ctx, &flags, argv[7]);
        if(argc > 8 && JS_IsObject(argv[8])) {
          JSValue t = JS_GetPropertyStr(ctx, argv[8], "type"), c = JS_GetPropertyStr(ctx, argv[8], "maxCount"), e = JS_GetPropertyStr(ctx, argv[8], "epsilon");
          int32_t i;
          double d;

          if(JS_IsArray(ctx, argv[8])) {
            JS_FreeValue(ctx, t);
            JS_FreeValue(ctx, c);
            JS_FreeValue(ctx, e);
            t = JS_GetPropertyUint32(ctx, argv[8], 0);
            c = JS_GetPropertyUint32(ctx, argv[8], 1);
            e = JS_GetPropertyUint32(ctx, argv[8], 2);
          }
          if(!JS_IsUndefined(t) && !JS_ToInt32(ctx, &i, t))
            criteria.type = i;
          if(!JS_IsUndefined(c) && !JS_ToInt32(ctx, &i, c))
            criteria.maxCount = i;
          if(!JS_IsUndefined(e) && !JS_ToFloat64(ctx, &d, e))
            criteria.epsilon = d;
          JS_FreeValue(ctx, t);
          JS_FreeValue(ctx, c);
          JS_FreeValue(ctx, e);
        }

        ret = JS_NewFloat64(ctx, cv::fisheye::calibrate(objectPoints.in(), imagePoints.in(), image_size, K, D, rvecs.out(), tvecs.out(), flags, criteria));
        break;
      }

      case FISHEYE_DISTORT_POINTS: {
        JSInputArray undistorted = js_cv_inputarray(ctx, argv[0]);
        JSOutputArray distorted = js_cv_outputarray(ctx, argv[1]);
        JSInputArray K = js_cv_inputarray(ctx, argv[2]), D = js_cv_inputarray(ctx, argv[3]);
        double alpha = 0;

        if(argc > 4)
          JS_ToFloat64(ctx, &alpha, argv[4]);

        break;
      }

      case FISHEYE_ESTIMATE_NEW_CAMERA_MATRIX_FOR_UNDISTORT_RECTIFY: {
        JSInputArray K = js_cv_inputarray(ctx, argv[0]), D = js_cv_inputarray(ctx, argv[1]);
        JSSizeData<int> image_size;
        js_value_to(ctx, argv[2], image_size);
        JSInputArray R = js_cv_inputarray(ctx, argv[3]);
        JSOutputArray P = js_cv_outputarray(ctx, argv[4]);
        double balance = 0.0;

        if(argc > 5)
          JS_ToFloat64(ctx, &balance, argv[5]);

        JSSizeData<int> new_size;

        if(argc > 6)
          js_value_to(ctx, argv[6], new_size);

        double fov_scale = 1.0;

        if(argc > 7)
          JS_ToFloat64(ctx, &fov_scale, argv[7]);

        cv::fisheye::estimateNewCameraMatrixForUndistortRectify(K, D, image_size, R, P, balance, new_size, fov_scale);
        break;
      }

      case FISHEYE_INIT_UNDISTORT_RECTIFY_MAP: {
        JSInputArray K = js_cv_inputarray(ctx, argv[0]), D = js_cv_inputarray(ctx, argv[1]);
        JSInputArray R = js_cv_inputarray(ctx, argv[2]), P = js_cv_inputarray(ctx, argv[3]);
        JSSizeData<int> size;
        js_value_to(ctx, argv[4], size);
        int32_t m1type;
        js_value_to(ctx, argv[5], m1type);
        JSOutputArray map1 = js_cv_outputarray(ctx, argv[6]), map2 = js_cv_outputarray(ctx, argv[7]);

        cv::fisheye::initUndistortRectifyMap(K, D, R, P, size, m1type, map1, map2);
        break;
      }

      case FISHEYE_PROJECT_POINTS: {
        /* (objectPoints, imagePoints, rvec, tvec, K, D, alpha = 0, jacobian) like opencv.js,
         * or (objectPoints, imagePoints, affine, K, D, alpha = 0, jacobian) */
        if(argc < 5)
          return JS_ThrowTypeError(ctx, "projectPoints(objectPoints, imagePoints, rvec, tvec, K, D, alpha, jacobian)");

        JSInputArray objectPoints = js_cv_inputarray(ctx, argv[0]);
        JSOutputArray imagePoints = js_cv_outputarray(ctx, argv[1]);
        cv::Affine3<double>* affine = js_affine3_data(argv[2]);
        JSOutputArray jacobian = cv::noArray();
        double alpha = 0.0;

        if(affine) {
          JSInputArray K = js_cv_inputarray(ctx, argv[3]), D = js_cv_inputarray(ctx, argv[4]);

          if(argc > 5)
            JS_ToFloat64(ctx, &alpha, argv[5]);
          if(argc > 6)
            jacobian = js_cv_outputarray(ctx, argv[6]);

          cv::fisheye::projectPoints(objectPoints, imagePoints, *affine, K, D, alpha, jacobian);
        } else {
          if(argc < 6)
            return JS_ThrowTypeError(ctx, "projectPoints(objectPoints, imagePoints, rvec, tvec, K, D, alpha, jacobian)");

          JSInputArray rvec = js_cv_inputarray(ctx, argv[2]), tvec = js_cv_inputarray(ctx, argv[3]);
          JSInputArray K = js_cv_inputarray(ctx, argv[4]), D = js_cv_inputarray(ctx, argv[5]);

          if(argc > 6)
            JS_ToFloat64(ctx, &alpha, argv[6]);
          if(argc > 7)
            jacobian = js_cv_outputarray(ctx, argv[7]);

          cv::fisheye::projectPoints(objectPoints, imagePoints, rvec, tvec, K, D, alpha, jacobian);
        }

        break;
      }

      case FISHEYE_STEREO_CALIBRATE: {
        JSMatVec objectPointsv(ctx, argv[0], false, true), imagePoints1v(ctx, argv[1], false, true), imagePoints2v(ctx, argv[2], false, true);
        cv::_InputArray objectPoints = objectPointsv.in(), imagePoints1 = imagePoints1v.in(), imagePoints2 = imagePoints2v.in();
        JSInputOutputArray K1 = js_cv_inputoutputarray(ctx, argv[3]), D1 = js_cv_inputoutputarray(ctx, argv[4]), K2 = js_cv_inputoutputarray(ctx, argv[5]),
                           D2 = js_cv_inputoutputarray(ctx, argv[6]);
        JSSizeData<int> image_size;
        js_value_to(ctx, argv[7], image_size);
        JSOutputArray R = js_cv_outputarray(ctx, argv[8]), T = js_cv_outputarray(ctx, argv[9]);

        if(argc <= 10 || JS_IsNumber(argv[10])) {
          int32_t flags = cv::fisheye::CALIB_FIX_INTRINSIC;

          if(argc > 10)
            JS_ToInt32(ctx, &flags, argv[10]);
          cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, DBL_EPSILON);

          ret = JS_NewFloat64(ctx, cv::fisheye::stereoCalibrate(objectPoints, imagePoints1, imagePoints2, K1, D1, K2, D2, image_size, R, T, flags, criteria));
        } else {
          JSOutputArray rvecs = js_cv_outputarray(ctx, argv[10]), tvecs = js_cv_outputarray(ctx, argv[11]);
          int32_t flags = cv::fisheye::CALIB_FIX_INTRINSIC;

          if(argc > 12)
            JS_ToInt32(ctx, &flags, argv[12]);
          cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, DBL_EPSILON);

          ret = JS_NewFloat64(
              ctx, cv::fisheye::stereoCalibrate(objectPoints, imagePoints1, imagePoints2, K1, D1, K2, D2, image_size, R, T, rvecs, tvecs, flags, criteria));
        }

        break;
      }

      case FISHEYE_STEREO_RECTIFY: {
        JSInputArray K1 = js_cv_inputarray(ctx, argv[0]), D1 = js_cv_inputarray(ctx, argv[1]), K2 = js_cv_inputarray(ctx, argv[2]),
                     D2 = js_cv_inputarray(ctx, argv[3]);
        JSSizeData<int> image_size;
        js_value_to(ctx, argv[4], image_size);

        JSInputArray R = js_cv_inputarray(ctx, argv[5]), tvec = js_cv_inputarray(ctx, argv[6]);
        JSOutputArray R1 = js_cv_outputarray(ctx, argv[7]), R2 = js_cv_outputarray(ctx, argv[8]), P1 = js_cv_outputarray(ctx, argv[9]),
                      P2 = js_cv_outputarray(ctx, argv[10]), Q = js_cv_outputarray(ctx, argv[11]);
        int32_t flags = cv::fisheye::CALIB_ZERO_DISPARITY;

        if(argc > 12)
          JS_ToInt32(ctx, &flags, argv[12]);
        JSSizeData<int> new_image_size;

        if(argc > 13)
          js_value_to(ctx, argv[13], new_image_size);

        double balance = 0.0, fov_scale = 1.0;

        if(argc > 14)
          JS_ToFloat64(ctx, &balance, argv[14]);
        if(argc > 15)
          JS_ToFloat64(ctx, &fov_scale, argv[15]);

        cv::fisheye::stereoRectify(K1, D1, K2, D2, image_size, R, tvec, R1, R2, P1, P2, Q, flags, new_image_size, balance, fov_scale);

        break;
      }

      case FISHEYE_UNDISTORT_IMAGE: {
        JSInputArray distorted = js_cv_inputarray(ctx, argv[0]);
        JSOutputArray undistorted = js_cv_outputarray(ctx, argv[1]);
        JSInputArray K = js_cv_inputarray(ctx, argv[2]), D = js_cv_inputarray(ctx, argv[3]), Knew = cv::noArray();

        if(argc > 4)
          Knew = js_cv_inputarray(ctx, argv[4]);

        JSSizeData<int> new_size;

        if(argc > 5)
          js_value_to(ctx, argv[5], new_size);

        cv::fisheye::undistortImage(distorted, undistorted, K, D, Knew, new_size);
        break;
      }

      case FISHEYE_UNDISTORT_POINTS: {
        JSInputArray distorted = js_cv_inputarray(ctx, argv[0]);
        JSOutputArray undistorted = js_cv_outputarray(ctx, argv[1]);
        JSInputArray K = js_cv_inputarray(ctx, argv[2]), D = js_cv_inputarray(ctx, argv[3]), R = cv::noArray(), P = cv::noArray();

        if(argc > 4)
          R = js_cv_inputarray(ctx, argv[4]);
        if(argc > 5)
          P = js_cv_inputarray(ctx, argv[5]);

        cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, DBL_EPSILON);

        cv::fisheye::undistortPoints(distorted, undistorted, K, D, R, P, criteria);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

js_function_list_t js_fisheye_fisheye_funcs{
    JS_CFUNC_MAGIC_DEF("calibrate", 7, js_fisheye_func, FISHEYE_CALIBRATE),
    JS_CFUNC_MAGIC_DEF("distortPoints", 4, js_fisheye_func, FISHEYE_DISTORT_POINTS),
    JS_CFUNC_MAGIC_DEF("estimateNewCameraMatrixForUndistortRectify", 5, js_fisheye_func, FISHEYE_ESTIMATE_NEW_CAMERA_MATRIX_FOR_UNDISTORT_RECTIFY),
    JS_CFUNC_MAGIC_DEF("initUndistortRectifyMap", 8, js_fisheye_func, FISHEYE_INIT_UNDISTORT_RECTIFY_MAP),
    JS_CFUNC_MAGIC_DEF("projectPoints", 5, js_fisheye_func, FISHEYE_PROJECT_POINTS),
    JS_CFUNC_MAGIC_DEF("stereoCalibrate", 10, js_fisheye_func, FISHEYE_STEREO_CALIBRATE),
    JS_CFUNC_MAGIC_DEF("stereoRectify", 13, js_fisheye_func, FISHEYE_STEREO_RECTIFY),
    JS_CFUNC_MAGIC_DEF("undistortImage", 4, js_fisheye_func, FISHEYE_UNDISTORT_IMAGE),
    JS_CFUNC_MAGIC_DEF("undistortPoints", 4, js_fisheye_func, FISHEYE_UNDISTORT_POINTS),
    JS_PROP_INT32_DEF("CALIB_CHECK_COND", cv::fisheye::CALIB_CHECK_COND, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("CALIB_FIX_FOCAL_LENGTH", cv::fisheye::CALIB_FIX_FOCAL_LENGTH, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("CALIB_FIX_INTRINSIC", cv::fisheye::CALIB_FIX_INTRINSIC, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("CALIB_FIX_K1", cv::fisheye::CALIB_FIX_K1, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("CALIB_FIX_K2", cv::fisheye::CALIB_FIX_K2, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("CALIB_FIX_K3", cv::fisheye::CALIB_FIX_K3, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("CALIB_FIX_K4", cv::fisheye::CALIB_FIX_K4, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("CALIB_FIX_PRINCIPAL_POINT", cv::fisheye::CALIB_FIX_PRINCIPAL_POINT, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("CALIB_FIX_SKEW", cv::fisheye::CALIB_FIX_SKEW, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("CALIB_RECOMPUTE_EXTRINSIC", cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("CALIB_USE_INTRINSIC_GUESS", cv::fisheye::CALIB_USE_INTRINSIC_GUESS, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("CALIB_ZERO_DISPARITY", cv::fisheye::CALIB_ZERO_DISPARITY, JS_PROP_CONFIGURABLE),
};

js_function_list_t js_fisheye_static_funcs{
    JS_OBJECT_DEF("fisheye", js_fisheye_fisheye_funcs.data(), int(js_fisheye_fisheye_funcs.size()), JS_PROP_C_W_E),
};

extern "C" int
js_fisheye_init(JSContext* ctx, JSModuleDef* m) {

  if(m) {
    JS_SetModuleExportList(ctx, m, js_fisheye_static_funcs.data(), js_fisheye_static_funcs.size());
  }

  return 0;
}

extern "C" void
js_fisheye_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_fisheye_static_funcs.data(), js_fisheye_static_funcs.size());
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_fisheye
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_fisheye_init)))
    return NULL;

  js_fisheye_export(ctx, m);
  return m;
}
