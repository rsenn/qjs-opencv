#include "js_alloc.hpp"
#include "js_size.hpp"
#include "js_mat.hpp"
#include "js_umat.hpp"
#include "js_cv.hpp"
#include "include/js_inputoutputarray.hpp"
#include <opencv2/calib3d.hpp>
#include <opencv2/objdetect.hpp>
#include "include/js_array.hpp"
#include "include/js_matvec.hpp"

extern "C" int js_calib3d_init(JSContext*, JSModuleDef*);

enum {
  CALIBRATE_CAMERA,
  FIND_CHESSBOARD_CORNERS,
  FIND_CHESSBOARD_CORNERS_SB,
  ESTIMATE_AFFINE_2D,
  ESTIMATE_AFFINE_3D,
  ESTIMATE_AFFINE_PARTIAL_2D,
  FIND_HOMOGRAPHY,
  DRAW_CHESSBOARD_CORNERS,
  UNDISTORT,
  INIT_UNDISTORT_RECTIFY_MAP,
  GET_OPTIMAL_NEW_CAMERA_MATRIX,
  GET_DEFAULT_NEW_CAMERA_MATRIX,
  CORNER_SUB_PIX,
  UNDISTORT_POINTS,
  UNDISTORT_IMAGE_POINTS,
  PROJECT_POINTS,
  SOLVE_PNP,
  SOLVE_PNP_RANSAC,
  SOLVE_PNP_GENERIC,
  RODRIGUES,
  DRAW_FRAME_AXES,
  FIND_CIRCLES_GRID,
  INIT_CAMERA_MATRIX_2D,
  CALIBRATE_CAMERA_RO,
  STEREO_CALIBRATE,
};

/* optional argument: argv[i] if present and not undefined/null, else noArray() */
static cv::_InputArray
js_calib3d_opt_in(JSContext* ctx, int argc, JSValueConst argv[], int i) {
  if(argc > i && !JS_IsUndefined(argv[i]) && !JS_IsNull(argv[i]))
    return js_cv_inputarray(ctx, argv[i]);

  return cv::noArray();
}

static cv::_OutputArray
js_calib3d_opt_out(JSContext* ctx, int argc, JSValueConst argv[], int i) {
  if(argc > i && !JS_IsUndefined(argv[i]) && !JS_IsNull(argv[i]))
    return js_cv_outputarray(ctx, argv[i]);

  return cv::noArray();
}

/* termination criteria {type, maxCount, epsilon} object or [type, maxCount, epsilon] array */
static cv::TermCriteria
js_calib3d_criteria(JSContext* ctx, JSValueConst value, const cv::TermCriteria& dflt) {
  cv::TermCriteria tc = dflt;
  JSValue t, c, e;
  int32_t type;
  double eps;

  if(!JS_IsObject(value))
    return tc;

  if(JS_IsArray(ctx, value)) {
    t = JS_GetPropertyUint32(ctx, value, 0);
    c = JS_GetPropertyUint32(ctx, value, 1);
    e = JS_GetPropertyUint32(ctx, value, 2);
  } else {
    t = JS_GetPropertyStr(ctx, value, "type");
    c = JS_GetPropertyStr(ctx, value, "maxCount");
    e = JS_GetPropertyStr(ctx, value, "epsilon");
  }

  if(!JS_IsUndefined(t) && !JS_ToInt32(ctx, &type, t))
    tc.type = type;
  if(!JS_IsUndefined(c) && !JS_ToInt32(ctx, &type, c))
    tc.maxCount = type;
  if(!JS_IsUndefined(e) && !JS_ToFloat64(ctx, &eps, e))
    tc.epsilon = eps;

  JS_FreeValue(ctx, t);
  JS_FreeValue(ctx, c);
  JS_FreeValue(ctx, e);
  return tc;
}

static int32_t
js_calib3d_int(JSContext* ctx, int argc, JSValueConst argv[], int i, int32_t dflt) {
  int32_t v = dflt;

  if(argc > i && !JS_IsUndefined(argv[i]))
    JS_ToInt32(ctx, &v, argv[i]);

  return v;
}

static double
js_calib3d_double(JSContext* ctx, int argc, JSValueConst argv[], int i, double dflt) {
  double v = dflt;

  if(argc > i && !JS_IsUndefined(argv[i]))
    JS_ToFloat64(ctx, &v, argv[i]);

  return v;
}

#define JS_CALIB3D_NEED(n, usage) \
  if(argc < (n)) \
    return JS_ThrowTypeError(ctx, usage);

static JSValue
js_calib3d_functions(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
  switch(magic) {
    case CALIBRATE_CAMERA: {
      /* (objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs,
       *  [stdDeviationsIntrinsics, stdDeviationsExtrinsics, perViewErrors,] flags = 0, criteria) -> RMS.
       * objectPoints/imagePoints are JS arrays of Mat (one per view), rvecs/tvecs JS arrays that are filled in */
      JS_CALIB3D_NEED(7, "calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs, ...)");
      JSMatVec objectPoints(ctx, argv[0], false, true), imagePoints(ctx, argv[1], false, true);
      JSSizeData<int> image_size;
      js_value_to(ctx, argv[2], image_size);

      JSInputOutputArray cameraMatrix = js_cv_inputoutputarray(ctx, argv[3]), distCoeffs = js_cv_inputoutputarray(ctx, argv[4]);
      JSMatVec rvecs(ctx, argv[5], true), tvecs(ctx, argv[6], true);
      cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, DBL_EPSILON);

      if(argc >= 10 && !JS_IsNumber(argv[7])) {
        JSOutputArray stdDeviationsIntrinsics = js_cv_outputarray(ctx, argv[7]), stdDeviationsExtrinsics = js_cv_outputarray(ctx, argv[8]),
                      perViewErrors = js_cv_outputarray(ctx, argv[9]);

        ret = JS_NewFloat64(ctx,
                            cv::calibrateCamera(objectPoints.in(),
                                                imagePoints.in(),
                                                image_size,
                                                cameraMatrix,
                                                distCoeffs,
                                                rvecs.out(),
                                                tvecs.out(),
                                                stdDeviationsIntrinsics,
                                                stdDeviationsExtrinsics,
                                                perViewErrors,
                                                js_calib3d_int(ctx, argc, argv, 10, 0),
                                                js_calib3d_criteria(ctx, argc > 11 ? argv[11] : JS_UNDEFINED, criteria)));
      } else
        ret = JS_NewFloat64(ctx,
                            cv::calibrateCamera(objectPoints.in(),
                                                imagePoints.in(),
                                                image_size,
                                                cameraMatrix,
                                                distCoeffs,
                                                rvecs.out(),
                                                tvecs.out(),
                                                js_calib3d_int(ctx, argc, argv, 7, 0),
                                                js_calib3d_criteria(ctx, argc > 8 ? argv[8] : JS_UNDEFINED, criteria)));

      break;
    }

    case FIND_CHESSBOARD_CORNERS: {
      JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[0]);
      cv::Size pattern_size = js_size_get(ctx, argv[1]);
      JSOutputArrayOf<cv::Point2f> corners(ctx, argv[2]);
      int32_t flags = cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE;

      if(argc > 3)
        JS_ToInt32(ctx, &flags, argv[3]);

      BOOL result = cv::findChessboardCorners(image, pattern_size, corners, flags);

      ret = JS_NewBool(ctx, result);
      break;
    }

    case FIND_CHESSBOARD_CORNERS_SB: {
      JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[0]);
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

      js_array_clear(ctx, argv[2]);
      js_array_copy(ctx, argv[2], corners);

      ret = JS_NewBool(ctx, result);
      break;
    }

    case ESTIMATE_AFFINE_2D: {
      JSInputArray from = js_cv_inputarray(ctx, argv[0]), to = js_cv_inputarray(ctx, argv[1]);
      /* argv[2] is opencv.js's `inliers` OutputArray position - was
       * hardcoded to cv::noArray() and never read, silently discarding
       * whatever the caller passed. See BUGS:
       * estimateaffine2d-inliers-output-discarded. */
      JSOutputArray inliers = cv::noArray();

      if(argc > 2)
        inliers = js_cv_outputarray(ctx, argv[2]);

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
      if(argc > 7)
        JS_ToUint32(ctx, &refineIters, argv[7]);

      cv::Mat mat = cv::estimateAffine2D(from, to, inliers, method, ransacReprojThreshold, maxIters, confidence, refineIters);

      ret = js_mat_wrap(ctx, mat);
      break;
    }

    case ESTIMATE_AFFINE_3D: {
      JSInputArray src = js_cv_inputarray(ctx, argv[0]), dst = js_cv_inputarray(ctx, argv[1]);
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
      JSInputArray from = js_cv_inputarray(ctx, argv[0]), to = js_cv_inputarray(ctx, argv[1]);
      /* See the identical fix/comment in ESTIMATE_AFFINE_2D above - same
       * bug, same fix. BUGS: estimateaffine2d-inliers-output-discarded. */
      JSOutputArray inliers = cv::noArray();

      if(argc > 2)
        inliers = js_cv_outputarray(ctx, argv[2]);

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
      if(argc > 7)
        JS_ToUint32(ctx, &refineIters, argv[7]);

      cv::Mat mat = cv::estimateAffinePartial2D(from, to, inliers, method, ransacReprojThreshold, maxIters, confidence, refineIters);

      ret = js_mat_wrap(ctx, mat);
      break;
    }

    case FIND_HOMOGRAPHY: {
      JSInputArray src = js_cv_inputarray(ctx, argv[0]);
      JSInputArray dst = js_cv_inputarray(ctx, argv[1]);

      if(src.type() == JSInputArray::NONE)
        return JS_ThrowTypeError(ctx, "argument 1 must be a PointVector or Mat of type CV_32FC2");

      if(dst.type() == JSInputArray::NONE)
        return JS_ThrowTypeError(ctx, "argument 1 must be a PointVector or Mat of type CV_32FC2");

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

    case DRAW_CHESSBOARD_CORNERS: {
      JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[0]);
      JSSizeData<int> patternSize;
      js_value_to(ctx, argv[1], patternSize);
      JSInputArray corners = js_cv_inputarray(ctx, argv[2]);
      BOOL patternWasFound = js_value_to<BOOL>(ctx, argv[3]);

      cv::drawChessboardCorners(image, patternSize, corners, patternWasFound);
      break;
    }

    case UNDISTORT: {
      /* (src, dst, cameraMatrix, distCoeffs, newCameraMatrix = cameraMatrix) */
      JS_CALIB3D_NEED(4, "undistort(src, dst, cameraMatrix, distCoeffs, newCameraMatrix)");
      JSInputArray src = js_cv_inputarray(ctx, argv[0]);
      JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
      JSInputArray K = js_cv_inputarray(ctx, argv[2]), D = js_cv_inputarray(ctx, argv[3]);

      cv::undistort(src, dst, K, D, js_calib3d_opt_in(ctx, argc, argv, 4));
      break;
    }

    case INIT_UNDISTORT_RECTIFY_MAP: {
      /* (cameraMatrix, distCoeffs, R, newCameraMatrix, size, m1type, map1, map2) */
      JS_CALIB3D_NEED(8, "initUndistortRectifyMap(cameraMatrix, distCoeffs, R, newCameraMatrix, size, m1type, map1, map2)");
      JSInputArray K = js_cv_inputarray(ctx, argv[0]), D = js_cv_inputarray(ctx, argv[1]);
      cv::_InputArray R = js_calib3d_opt_in(ctx, argc, argv, 2), P = js_calib3d_opt_in(ctx, argc, argv, 3);
      cv::Size size = js_size_get(ctx, argv[4]);
      int32_t m1type = js_calib3d_int(ctx, argc, argv, 5, CV_16SC2);
      JSOutputArray map1 = js_cv_outputarray(ctx, argv[6]), map2 = js_cv_outputarray(ctx, argv[7]);

      cv::initUndistortRectifyMap(K, D, R, P, size, m1type, map1, map2);
      break;
    }

    case GET_OPTIMAL_NEW_CAMERA_MATRIX: {
      /* (cameraMatrix, distCoeffs, imageSize, alpha, newImgSize = imageSize, validPixROI, centerPrincipalPoint = false) */
      JS_CALIB3D_NEED(4, "getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imageSize, alpha, newImgSize, validPixROI, centerPrincipalPoint)");
      JSInputArray K = js_cv_inputarray(ctx, argv[0]), D = js_cv_inputarray(ctx, argv[1]);
      cv::Size size = js_size_get(ctx, argv[2]), newsize;
      double alpha = js_calib3d_double(ctx, argc, argv, 3, 0);
      cv::Rect roi;
      bool center = argc > 6 && JS_ToBool(ctx, argv[6]);

      if(argc > 4 && JS_IsObject(argv[4]))
        newsize = js_size_get(ctx, argv[4]);

      cv::Mat m = cv::getOptimalNewCameraMatrix(K, D, size, alpha, newsize, &roi, center);

      /* the valid pixel ROI goes into the caller's Rect object (opencv.js out parameter) */
      if(argc > 5 && JS_IsObject(argv[5])) {
        JS_SetPropertyStr(ctx, argv[5], "x", JS_NewInt32(ctx, roi.x));
        JS_SetPropertyStr(ctx, argv[5], "y", JS_NewInt32(ctx, roi.y));
        JS_SetPropertyStr(ctx, argv[5], "width", JS_NewInt32(ctx, roi.width));
        JS_SetPropertyStr(ctx, argv[5], "height", JS_NewInt32(ctx, roi.height));
      }

      ret = js_mat_wrap(ctx, m);
      break;
    }

    case GET_DEFAULT_NEW_CAMERA_MATRIX: {
      /* (cameraMatrix, imgsize = Size(), centerPrincipalPoint = false) */
      JS_CALIB3D_NEED(1, "getDefaultNewCameraMatrix(cameraMatrix, imgsize, centerPrincipalPoint)");
      JSInputArray K = js_cv_inputarray(ctx, argv[0]);
      cv::Size size;

      if(argc > 1 && JS_IsObject(argv[1]))
        size = js_size_get(ctx, argv[1]);

      ret = js_mat_wrap(ctx, cv::getDefaultNewCameraMatrix(K, size, argc > 2 && JS_ToBool(ctx, argv[2])));
      break;
    }

    case CORNER_SUB_PIX: {
      /* (image, corners, winSize, zeroZone, criteria): refines corners in place (Mat or array of points) */
      JS_CALIB3D_NEED(4, "cornerSubPix(image, corners, winSize, zeroZone, criteria)");
      JSInputArray image = js_cv_inputarray(ctx, argv[0]);
      cv::Size win = js_size_get(ctx, argv[2]), zero = js_size_get(ctx, argv[3]);
      cv::TermCriteria criteria = js_calib3d_criteria(ctx, argc > 4 ? argv[4] : JS_UNDEFINED, cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.001));
      cv::Mat* mat = js_mat_data_nothrow(argv[1]);

      if(mat)
        cv::cornerSubPix(image, *mat, win, zero, criteria);
      else {
        std::vector<cv::Point2f> corners;

        js_array_to(ctx, argv[1], corners);
        cv::cornerSubPix(image, corners, win, zero, criteria);
        js_array_clear(ctx, argv[1]);
        js_array_copy(ctx, argv[1], corners);
      }
      break;
    }

    case UNDISTORT_POINTS: {
      /* (src, dst, cameraMatrix, distCoeffs, R = noArray, P = noArray, criteria) */
      JS_CALIB3D_NEED(4, "undistortPoints(src, dst, cameraMatrix, distCoeffs, R, P, criteria)");
      JSInputArray src = js_cv_inputarray(ctx, argv[0]);
      JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
      JSInputArray K = js_cv_inputarray(ctx, argv[2]), D = js_cv_inputarray(ctx, argv[3]);

      cv::undistortPoints(src,
                          dst,
                          K,
                          D,
                          js_calib3d_opt_in(ctx, argc, argv, 4),
                          js_calib3d_opt_in(ctx, argc, argv, 5),
                          js_calib3d_criteria(ctx, argc > 6 ? argv[6] : JS_UNDEFINED, cv::TermCriteria(cv::TermCriteria::MAX_ITER, 5, 0.01)));
      break;
    }

    case UNDISTORT_IMAGE_POINTS: {
#if CV_VERSION_MAJOR >= 5 || CV_VERSION_MINOR >= 7
      /* (src, dst, cameraMatrix, distCoeffs, criteria) */
      JS_CALIB3D_NEED(4, "undistortImagePoints(src, dst, cameraMatrix, distCoeffs, criteria)");
      JSInputArray src = js_cv_inputarray(ctx, argv[0]);
      JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
      JSInputArray K = js_cv_inputarray(ctx, argv[2]), D = js_cv_inputarray(ctx, argv[3]);

      cv::undistortImagePoints(src, dst, K, D, js_calib3d_criteria(ctx, argc > 4 ? argv[4] : JS_UNDEFINED, cv::TermCriteria(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 5, 0.01)));
#else
      ret = JS_ThrowInternalError(ctx, "undistortImagePoints needs OpenCV >= 4.7");
#endif
      break;
    }

    case PROJECT_POINTS: {
      /* (objectPoints, rvec, tvec, cameraMatrix, distCoeffs, imagePoints, jacobian = noArray, aspectRatio = 0) */
      JS_CALIB3D_NEED(6, "projectPoints(objectPoints, rvec, tvec, cameraMatrix, distCoeffs, imagePoints, jacobian, aspectRatio)");
      JSInputArray obj = js_cv_inputarray(ctx, argv[0]), rvec = js_cv_inputarray(ctx, argv[1]), tvec = js_cv_inputarray(ctx, argv[2]);
      JSInputArray K = js_cv_inputarray(ctx, argv[3]), D = js_cv_inputarray(ctx, argv[4]);
      JSOutputArray img = js_cv_outputarray(ctx, argv[5]);

      cv::projectPoints(obj, rvec, tvec, K, D, img, js_calib3d_opt_out(ctx, argc, argv, 6), js_calib3d_double(ctx, argc, argv, 7, 0));
      break;
    }

    case SOLVE_PNP: {
      /* (objectPoints, imagePoints, cameraMatrix, distCoeffs, rvec, tvec, useExtrinsicGuess = false, flags = SOLVEPNP_ITERATIVE) */
      JS_CALIB3D_NEED(6, "solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs, rvec, tvec, useExtrinsicGuess, flags)");
      JSInputArray obj = js_cv_inputarray(ctx, argv[0]), img = js_cv_inputarray(ctx, argv[1]);
      JSInputArray K = js_cv_inputarray(ctx, argv[2]), D = js_cv_inputarray(ctx, argv[3]);
      JSInputOutputArray rvec = js_cv_inputoutputarray(ctx, argv[4]), tvec = js_cv_inputoutputarray(ctx, argv[5]);

      ret = JS_NewBool(ctx, cv::solvePnP(obj, img, K, D, rvec, tvec, argc > 6 && JS_ToBool(ctx, argv[6]), js_calib3d_int(ctx, argc, argv, 7, cv::SOLVEPNP_ITERATIVE)));
      break;
    }

    case SOLVE_PNP_RANSAC: {
      /* (objectPoints, imagePoints, cameraMatrix, distCoeffs, rvec, tvec, useExtrinsicGuess = false, iterationsCount = 100,
       *  reprojectionError = 8, confidence = 0.99, inliers = noArray, flags = SOLVEPNP_ITERATIVE) */
      JS_CALIB3D_NEED(6, "solvePnPRansac(objectPoints, imagePoints, cameraMatrix, distCoeffs, rvec, tvec, ...)");
      JSInputArray obj = js_cv_inputarray(ctx, argv[0]), img = js_cv_inputarray(ctx, argv[1]);
      JSInputArray K = js_cv_inputarray(ctx, argv[2]), D = js_cv_inputarray(ctx, argv[3]);
      JSInputOutputArray rvec = js_cv_inputoutputarray(ctx, argv[4]), tvec = js_cv_inputoutputarray(ctx, argv[5]);

      ret = JS_NewBool(ctx,
                       cv::solvePnPRansac(obj,
                                          img,
                                          K,
                                          D,
                                          rvec,
                                          tvec,
                                          argc > 6 && JS_ToBool(ctx, argv[6]),
                                          js_calib3d_int(ctx, argc, argv, 7, 100),
                                          float(js_calib3d_double(ctx, argc, argv, 8, 8.0)),
                                          js_calib3d_double(ctx, argc, argv, 9, 0.99),
                                          js_calib3d_opt_out(ctx, argc, argv, 10),
                                          js_calib3d_int(ctx, argc, argv, 11, cv::SOLVEPNP_ITERATIVE)));
      break;
    }

    case SOLVE_PNP_GENERIC: {
      /* (objectPoints, imagePoints, cameraMatrix, distCoeffs, rvecs, tvecs, useExtrinsicGuess = false, flags = SOLVEPNP_ITERATIVE,
       *  rvec = noArray, tvec = noArray, reprojectionError = noArray) -> number of solutions */
      JS_CALIB3D_NEED(6, "solvePnPGeneric(objectPoints, imagePoints, cameraMatrix, distCoeffs, rvecs, tvecs, ...)");
      JSInputArray obj = js_cv_inputarray(ctx, argv[0]), img = js_cv_inputarray(ctx, argv[1]);
      JSInputArray K = js_cv_inputarray(ctx, argv[2]), D = js_cv_inputarray(ctx, argv[3]);
      JSMatVec rvecsv(ctx, argv[4], true), tvecsv(ctx, argv[5], true);
      cv::_OutputArray rvecs = rvecsv.out(), tvecs = tvecsv.out();

      ret = JS_NewInt32(ctx,
                        cv::solvePnPGeneric(obj,
                                            img,
                                            K,
                                            D,
                                            rvecs,
                                            tvecs,
                                            argc > 6 && JS_ToBool(ctx, argv[6]),
                                            cv::SolvePnPMethod(js_calib3d_int(ctx, argc, argv, 7, cv::SOLVEPNP_ITERATIVE)),
                                            js_calib3d_opt_in(ctx, argc, argv, 8),
                                            js_calib3d_opt_in(ctx, argc, argv, 9),
                                            js_calib3d_opt_out(ctx, argc, argv, 10)));
      break;
    }

    case RODRIGUES: {
      /* (src, dst, jacobian = noArray) */
      JS_CALIB3D_NEED(2, "Rodrigues(src, dst, jacobian)");
      JSInputArray src = js_cv_inputarray(ctx, argv[0]);
      JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);

      cv::Rodrigues(src, dst, js_calib3d_opt_out(ctx, argc, argv, 2));
      break;
    }

    case DRAW_FRAME_AXES: {
      /* (image, cameraMatrix, distCoeffs, rvec, tvec, length, thickness = 3) */
      JS_CALIB3D_NEED(6, "drawFrameAxes(image, cameraMatrix, distCoeffs, rvec, tvec, length, thickness)");
      JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[0]);
      JSInputArray K = js_cv_inputarray(ctx, argv[1]), D = js_cv_inputarray(ctx, argv[2]);
      JSInputArray rvec = js_cv_inputarray(ctx, argv[3]), tvec = js_cv_inputarray(ctx, argv[4]);

      cv::drawFrameAxes(image, K, D, rvec, tvec, float(js_calib3d_double(ctx, argc, argv, 5, 1)), js_calib3d_int(ctx, argc, argv, 6, 3));
      break;
    }

    case FIND_CIRCLES_GRID: {
      /* (image, patternSize, centers, flags = CALIB_CB_SYMMETRIC_GRID) */
      JS_CALIB3D_NEED(3, "findCirclesGrid(image, patternSize, centers, flags)");
      JSInputArray image = js_cv_inputarray(ctx, argv[0]);
      cv::Size pattern = js_size_get(ctx, argv[1]);
      JSOutputArrayOf<cv::Point2f> centers(ctx, argv[2]);

      ret = JS_NewBool(ctx, cv::findCirclesGrid(image, pattern, centers, js_calib3d_int(ctx, argc, argv, 3, cv::CALIB_CB_SYMMETRIC_GRID)));
      break;
    }

    case INIT_CAMERA_MATRIX_2D: {
      /* (objectPoints, imagePoints, imageSize, aspectRatio = 1) */
      JS_CALIB3D_NEED(3, "initCameraMatrix2D(objectPoints, imagePoints, imageSize, aspectRatio)");
      JSInputArray obj = js_cv_inputarray(ctx, argv[0]), img = js_cv_inputarray(ctx, argv[1]);

      ret = js_mat_wrap(ctx, cv::initCameraMatrix2D(obj, img, js_size_get(ctx, argv[2]), js_calib3d_double(ctx, argc, argv, 3, 1.0)));
      break;
    }

    case CALIBRATE_CAMERA_RO: {
      /* (objectPoints, imagePoints, imageSize, iFixedPoint, cameraMatrix, distCoeffs, rvecs, tvecs, newObjPoints,
       *  [stdDeviationsIntrinsics, stdDeviationsExtrinsics, stdDeviationsObjPoints, perViewErrors,] flags = 0, criteria) -> RMS */
      JS_CALIB3D_NEED(9, "calibrateCameraRO(objectPoints, imagePoints, imageSize, iFixedPoint, cameraMatrix, distCoeffs, rvecs, tvecs, newObjPoints, ...)");
      JSMatVec objv(ctx, argv[0], false, true), imgv(ctx, argv[1], false, true);
      cv::_InputArray obj = objv.in(), img = imgv.in();
      cv::Size size = js_size_get(ctx, argv[2]);
      int32_t fixed = js_calib3d_int(ctx, argc, argv, 3, -1);
      JSInputOutputArray K = js_cv_inputoutputarray(ctx, argv[4]), D = js_cv_inputoutputarray(ctx, argv[5]);
      JSMatVec rvecsv(ctx, argv[6], true), tvecsv(ctx, argv[7], true);
      cv::_OutputArray rvecs = rvecsv.out(), tvecs = tvecsv.out();
      JSOutputArray newobj = js_cv_outputarray(ctx, argv[8]);
      cv::TermCriteria dflt(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, DBL_EPSILON);

      if(argc >= 13 && !JS_IsNumber(argv[9])) {
        JSOutputArray sdi = js_cv_outputarray(ctx, argv[9]), sde = js_cv_outputarray(ctx, argv[10]), sdo = js_cv_outputarray(ctx, argv[11]),
                      pve = js_cv_outputarray(ctx, argv[12]);

        ret = JS_NewFloat64(ctx,
                            cv::calibrateCameraRO(obj, img, size, fixed, K, D, rvecs, tvecs, newobj, sdi, sde, sdo, pve, js_calib3d_int(ctx, argc, argv, 13, 0), js_calib3d_criteria(ctx, argc > 14 ? argv[14] : JS_UNDEFINED, dflt)));
      } else
        ret = JS_NewFloat64(ctx,
                            cv::calibrateCameraRO(obj, img, size, fixed, K, D, rvecs, tvecs, newobj, js_calib3d_int(ctx, argc, argv, 9, 0), js_calib3d_criteria(ctx, argc > 10 ? argv[10] : JS_UNDEFINED, dflt)));
      break;
    }

    case STEREO_CALIBRATE: {
      /* (objectPoints, imagePoints1, imagePoints2, cameraMatrix1, distCoeffs1, cameraMatrix2, distCoeffs2, imageSize, R, T, E, F,
       *  flags = CALIB_FIX_INTRINSIC, criteria) -> RMS */
      JS_CALIB3D_NEED(12, "stereoCalibrate(objectPoints, imagePoints1, imagePoints2, K1, D1, K2, D2, imageSize, R, T, E, F, flags, criteria)");
      JSMatVec objv(ctx, argv[0], false, true), img1v(ctx, argv[1], false, true), img2v(ctx, argv[2], false, true);
      cv::_InputArray obj = objv.in(), img1 = img1v.in(), img2 = img2v.in();
      JSInputOutputArray K1 = js_cv_inputoutputarray(ctx, argv[3]), D1 = js_cv_inputoutputarray(ctx, argv[4]), K2 = js_cv_inputoutputarray(ctx, argv[5]),
                         D2 = js_cv_inputoutputarray(ctx, argv[6]);
      cv::Size size = js_size_get(ctx, argv[7]);
      JSInputOutputArray R = js_cv_inputoutputarray(ctx, argv[8]), T = js_cv_inputoutputarray(ctx, argv[9]);
      JSOutputArray E = js_cv_outputarray(ctx, argv[10]), F = js_cv_outputarray(ctx, argv[11]);

      ret = JS_NewFloat64(ctx,
                          cv::stereoCalibrate(obj,
                                              img1,
                                              img2,
                                              K1,
                                              D1,
                                              K2,
                                              D2,
                                              size,
                                              R,
                                              T,
                                              E,
                                              F,
                                              js_calib3d_int(ctx, argc, argv, 12, cv::CALIB_FIX_INTRINSIC),
                                              js_calib3d_criteria(ctx, argc > 13 ? argv[13] : JS_UNDEFINED, cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, 1e-6))));
      break;
    }
  }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

const JSCFunctionListEntry js_calib3d_static_funcs[] = {
    JS_CFUNC_MAGIC_DEF("calibrateCamera", 10, js_calib3d_functions, CALIBRATE_CAMERA),
    JS_CFUNC_MAGIC_DEF("findChessboardCorners", 3, js_calib3d_functions, FIND_CHESSBOARD_CORNERS),
    JS_CFUNC_MAGIC_DEF("findChessboardCornersSB", 5, js_calib3d_functions, FIND_CHESSBOARD_CORNERS_SB),
    JS_CFUNC_MAGIC_DEF("estimateAffine2D", 2, js_calib3d_functions, ESTIMATE_AFFINE_2D),
    JS_CFUNC_MAGIC_DEF("estimateAffine3D", 4, js_calib3d_functions, ESTIMATE_AFFINE_3D),
    JS_CFUNC_MAGIC_DEF("estimateAffinePartial2D", 2, js_calib3d_functions, ESTIMATE_AFFINE_PARTIAL_2D),
    JS_CFUNC_MAGIC_DEF("findHomography", 2, js_calib3d_functions, FIND_HOMOGRAPHY),
    JS_CFUNC_MAGIC_DEF("drawChessboardCorners", 4, js_calib3d_functions, DRAW_CHESSBOARD_CORNERS),
    JS_CFUNC_MAGIC_DEF("undistort", 4, js_calib3d_functions, UNDISTORT),
    JS_CFUNC_MAGIC_DEF("initUndistortRectifyMap", 8, js_calib3d_functions, INIT_UNDISTORT_RECTIFY_MAP),
    JS_CFUNC_MAGIC_DEF("getOptimalNewCameraMatrix", 4, js_calib3d_functions, GET_OPTIMAL_NEW_CAMERA_MATRIX),
    JS_CFUNC_MAGIC_DEF("getDefaultNewCameraMatrix", 1, js_calib3d_functions, GET_DEFAULT_NEW_CAMERA_MATRIX),
    JS_CFUNC_MAGIC_DEF("cornerSubPix", 4, js_calib3d_functions, CORNER_SUB_PIX),
    JS_CFUNC_MAGIC_DEF("undistortPoints", 4, js_calib3d_functions, UNDISTORT_POINTS),
    JS_CFUNC_MAGIC_DEF("undistortImagePoints", 4, js_calib3d_functions, UNDISTORT_IMAGE_POINTS),
    JS_CFUNC_MAGIC_DEF("projectPoints", 6, js_calib3d_functions, PROJECT_POINTS),
    JS_CFUNC_MAGIC_DEF("solvePnP", 6, js_calib3d_functions, SOLVE_PNP),
    JS_CFUNC_MAGIC_DEF("solvePnPRansac", 6, js_calib3d_functions, SOLVE_PNP_RANSAC),
    JS_CFUNC_MAGIC_DEF("solvePnPGeneric", 6, js_calib3d_functions, SOLVE_PNP_GENERIC),
    JS_CFUNC_MAGIC_DEF("Rodrigues", 2, js_calib3d_functions, RODRIGUES),
    JS_CFUNC_MAGIC_DEF("drawFrameAxes", 6, js_calib3d_functions, DRAW_FRAME_AXES),
    JS_CFUNC_MAGIC_DEF("findCirclesGrid", 3, js_calib3d_functions, FIND_CIRCLES_GRID),
    JS_CFUNC_MAGIC_DEF("initCameraMatrix2D", 3, js_calib3d_functions, INIT_CAMERA_MATRIX_2D),
    JS_CFUNC_MAGIC_DEF("calibrateCameraRO", 9, js_calib3d_functions, CALIBRATE_CAMERA_RO),
    JS_CFUNC_MAGIC_DEF("stereoCalibrate", 12, js_calib3d_functions, STEREO_CALIBRATE),
    JS_PROP_INT32_DEF("CALIB_CB_ADAPTIVE_THRESH", cv::CALIB_CB_ADAPTIVE_THRESH, 0),
    JS_PROP_INT32_DEF("CALIB_CB_NORMALIZE_IMAGE", cv::CALIB_CB_NORMALIZE_IMAGE, 0),
    JS_PROP_INT32_DEF("CALIB_CB_FILTER_QUADS", cv::CALIB_CB_FILTER_QUADS, 0),
    JS_PROP_INT32_DEF("CALIB_CB_FAST_CHECK", cv::CALIB_CB_FAST_CHECK, 0),
    JS_PROP_INT32_DEF("CALIB_CB_EXHAUSTIVE", cv::CALIB_CB_EXHAUSTIVE, 0),
    JS_PROP_INT32_DEF("CALIB_CB_ACCURACY", cv::CALIB_CB_ACCURACY, 0),
    JS_PROP_INT32_DEF("CALIB_CB_LARGER", cv::CALIB_CB_LARGER, 0),
    JS_PROP_INT32_DEF("CALIB_CB_MARKER", cv::CALIB_CB_MARKER, 0),
    JS_PROP_INT32_DEF("CALIB_USE_INTRINSIC_GUESS", int(cv::CALIB_USE_INTRINSIC_GUESS), 0),
    JS_PROP_INT32_DEF("CALIB_FIX_ASPECT_RATIO", int(cv::CALIB_FIX_ASPECT_RATIO), 0),
    JS_PROP_INT32_DEF("CALIB_FIX_PRINCIPAL_POINT", int(cv::CALIB_FIX_PRINCIPAL_POINT), 0),
    JS_PROP_INT32_DEF("CALIB_ZERO_TANGENT_DIST", int(cv::CALIB_ZERO_TANGENT_DIST), 0),
    JS_PROP_INT32_DEF("CALIB_FIX_FOCAL_LENGTH", int(cv::CALIB_FIX_FOCAL_LENGTH), 0),
    JS_PROP_INT32_DEF("CALIB_FIX_K1", int(cv::CALIB_FIX_K1), 0),
    JS_PROP_INT32_DEF("CALIB_FIX_K2", int(cv::CALIB_FIX_K2), 0),
    JS_PROP_INT32_DEF("CALIB_FIX_K3", int(cv::CALIB_FIX_K3), 0),
    JS_PROP_INT32_DEF("CALIB_FIX_K4", int(cv::CALIB_FIX_K4), 0),
    JS_PROP_INT32_DEF("CALIB_FIX_K5", int(cv::CALIB_FIX_K5), 0),
    JS_PROP_INT32_DEF("CALIB_FIX_K6", int(cv::CALIB_FIX_K6), 0),
    JS_PROP_INT32_DEF("CALIB_RATIONAL_MODEL", int(cv::CALIB_RATIONAL_MODEL), 0),
    JS_PROP_INT32_DEF("CALIB_THIN_PRISM_MODEL", int(cv::CALIB_THIN_PRISM_MODEL), 0),
    JS_PROP_INT32_DEF("CALIB_FIX_S1_S2_S3_S4", int(cv::CALIB_FIX_S1_S2_S3_S4), 0),
    JS_PROP_INT32_DEF("CALIB_TILTED_MODEL", int(cv::CALIB_TILTED_MODEL), 0),
    JS_PROP_INT32_DEF("CALIB_FIX_TAUX_TAUY", int(cv::CALIB_FIX_TAUX_TAUY), 0),
    JS_PROP_INT32_DEF("CALIB_USE_QR", int(cv::CALIB_USE_QR), 0),
    JS_PROP_INT32_DEF("CALIB_USE_LU", int(cv::CALIB_USE_LU), 0),
    JS_PROP_INT32_DEF("CALIB_USE_EXTRINSIC_GUESS", int(cv::CALIB_USE_EXTRINSIC_GUESS), 0),
    JS_PROP_INT32_DEF("CALIB_FIX_INTRINSIC", int(cv::CALIB_FIX_INTRINSIC), 0),
    JS_PROP_INT32_DEF("CALIB_SAME_FOCAL_LENGTH", int(cv::CALIB_SAME_FOCAL_LENGTH), 0),
    JS_PROP_INT32_DEF("CALIB_ZERO_DISPARITY", int(cv::CALIB_ZERO_DISPARITY), 0),
    JS_PROP_INT32_DEF("CALIB_CB_SYMMETRIC_GRID", int(cv::CALIB_CB_SYMMETRIC_GRID), 0),
    JS_PROP_INT32_DEF("CALIB_CB_ASYMMETRIC_GRID", int(cv::CALIB_CB_ASYMMETRIC_GRID), 0),
    JS_PROP_INT32_DEF("CALIB_CB_CLUSTERING", int(cv::CALIB_CB_CLUSTERING), 0),
    JS_PROP_INT32_DEF("SOLVEPNP_ITERATIVE", int(cv::SOLVEPNP_ITERATIVE), 0),
    JS_PROP_INT32_DEF("SOLVEPNP_EPNP", int(cv::SOLVEPNP_EPNP), 0),
    JS_PROP_INT32_DEF("SOLVEPNP_P3P", int(cv::SOLVEPNP_P3P), 0),
#if CV_VERSION_MAJOR < 5
    JS_PROP_INT32_DEF("SOLVEPNP_DLS", int(cv::SOLVEPNP_DLS), 0),
    JS_PROP_INT32_DEF("SOLVEPNP_UPNP", int(cv::SOLVEPNP_UPNP), 0),
#endif
    JS_PROP_INT32_DEF("SOLVEPNP_AP3P", int(cv::SOLVEPNP_AP3P), 0),
    JS_PROP_INT32_DEF("SOLVEPNP_IPPE", int(cv::SOLVEPNP_IPPE), 0),
    JS_PROP_INT32_DEF("SOLVEPNP_IPPE_SQUARE", int(cv::SOLVEPNP_IPPE_SQUARE), 0),
    JS_PROP_INT32_DEF("SOLVEPNP_SQPNP", int(cv::SOLVEPNP_SQPNP), 0),
    JS_PROP_INT32_DEF("SOLVEPNP_MAX_COUNT", int(cv::SOLVEPNP_MAX_COUNT), 0),
    JS_PROP_INT32_DEF("LMEDS", int(cv::LMEDS), 0),
    JS_PROP_INT32_DEF("RHO", int(cv::RHO), 0),
};

extern "C" int
js_calib3d_init(JSContext* ctx, JSModuleDef* m) {
  if(m)
    JS_SetModuleExportList(ctx, m, js_calib3d_static_funcs, countof(js_calib3d_static_funcs));

  return 0;
}

#ifdef JS_Calib3D_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_calib3d
#endif

extern "C" void
js_calib3d_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_calib3d_static_funcs, countof(js_calib3d_static_funcs));
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
