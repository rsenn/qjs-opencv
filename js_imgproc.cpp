#include "jsbindings.hpp"
#include "js_alloc.hpp"
#include "js_point.hpp"
#include "js_size.hpp"
#include "js_rect.hpp"
#include "js_array.hpp"
#include "js_umat.hpp"

#include <opencv2/imgproc.hpp>

enum { HIER_NEXT = 0, HIER_PREV, HIER_CHILD, HIER_PARENT };

static JSValue
js_cv_hough_lines(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray image;
  JSValueConst array;
  double rho, theta;
  int32_t threshold;
  double srn = 0, stn = 0, min_theta = 0, max_theta = CV_PI;
  std::vector<cv::Vec2f> lines;
  size_t i;

  double angle = 0, scale = 1;
  cv::Mat m;

  JSValue ret;
  if(argc < 5)
    return JS_EXCEPTION;

  image = js_umat_or_mat(ctx, argv[0]);

  if(js_is_noarray(image) || !js_is_array(ctx, argv[1]))
    return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");

  array = argv[1];
  JS_ToFloat64(ctx, &rho, argv[2]);
  JS_ToFloat64(ctx, &theta, argv[3]);
  JS_ToInt32(ctx, &threshold, argv[4]);

  if(argc >= 6)
    JS_ToFloat64(ctx, &srn, argv[5]);
  if(argc >= 7)
    JS_ToFloat64(ctx, &stn, argv[6]);

  if(argc >= 8)
    JS_ToFloat64(ctx, &min_theta, argv[7]);
  if(argc >= 9)
    JS_ToFloat64(ctx, &max_theta, argv[8]);

  cv::HoughLines(image, lines, rho, theta, threshold, srn, stn, min_theta, max_theta);

  i = 0;

  for(const cv::Vec2f& line : lines) {
    JSValue v = JS_NewArray(ctx);

    JS_SetPropertyUint32(ctx, v, 0, JS_NewFloat64(ctx, line[0]));
    JS_SetPropertyUint32(ctx, v, 1, JS_NewFloat64(ctx, line[1]));

    JS_SetPropertyUint32(ctx, argv[1], i++, v);
  }

  return JS_UNDEFINED;
}

static JSValue
js_cv_median_blur(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray src, dst;
  int32_t ksize;

  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  JS_ToInt32(ctx, &ksize, argv[2]);

  assert(ksize >= 1);
  assert(ksize % 2 == 1);

  cv::medianBlur(src, dst, ksize);
  return JS_UNDEFINED;
}

static JSValue
js_cv_hough_lines_p(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray src, dst;
  JSValueConst array;
  double rho, theta;
  int32_t threshold;
  double minLineLength = 0, maxLineGap = 0;

  std::vector<cv::Vec4i> lines;
  size_t i;

  double angle = 0, scale = 1;
  cv::Mat m;

  JSValue ret;
  if(argc < 5)
    return JS_EXCEPTION;

  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_cv_inputoutputarray(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  array = argv[1];
  JS_ToFloat64(ctx, &rho, argv[2]);
  JS_ToFloat64(ctx, &theta, argv[3]);
  JS_ToInt32(ctx, &threshold, argv[4]);

  if(argc >= 6)
    JS_ToFloat64(ctx, &minLineLength, argv[5]);
  if(argc >= 7)
    JS_ToFloat64(ctx, &maxLineGap, argv[6]);

  cv::HoughLinesP(src, lines, rho, theta, threshold, minLineLength, maxLineGap);

  cv::Mat(lines).copyTo(dst);
  /*
    i = 0;
    js_array_truncate(ctx, array, 0);

    for(const auto& line : lines) {
      JSValue v = js_line_new(ctx, line[0], line[1], line[2], line[3]);

      JS_SetPropertyUint32(ctx, array, i++, v);
    }
  */
  return JS_UNDEFINED;
}

static JSValue
js_cv_gaussian_blur(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSSizeData<double> size;
  double sigmaX, sigmaY = 0;
  JSInputOutputArray input, output;
  int32_t borderType = cv::BORDER_DEFAULT;

  input = js_umat_or_mat(ctx, argv[0]);
  output = js_umat_or_mat(ctx, argv[1]);

  if(js_is_noarray(input) || js_is_noarray(output))
    return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");

  size = js_size_get(ctx, argv[2]);

  JS_ToFloat64(ctx, &sigmaX, argv[3]);
  if(argc >= 5)
    JS_ToFloat64(ctx, &sigmaY, argv[4]);
  if(argc >= 6)
    JS_ToInt32(ctx, &borderType, argv[5]);

  // std::cerr << "cv::GaussianBlur size=" << size << " sigmaX=" << sigmaX << " sigmaY=" << sigmaY
  // << " borderType=" << borderType << std::endl;
  cv::GaussianBlur(input, output, size, sigmaX, sigmaY, borderType);

  return JS_UNDEFINED;
}

static JSValue
js_cv_hough_circles(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputArray image;
  JSValueConst array;
  int32_t method, minRadius = 0, maxRadius = 0;
  double dp, minDist, param1 = 100, param2 = 100;

  std::vector<cv::Vec<float, 4>> circles;
  size_t i;

  JSValue ret;
  if(argc < 5)
    return JS_EXCEPTION;

  image = js_umat_or_mat(ctx, argv[0]);

  if(js_is_noarray(image) || !js_is_array(ctx, argv[1]))
    return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");

  array = argv[1];
  JS_ToInt32(ctx, &method, argv[2]);
  JS_ToFloat64(ctx, &dp, argv[3]);
  JS_ToFloat64(ctx, &minDist, argv[4]);

  if(argc >= 6)
    JS_ToFloat64(ctx, &param1, argv[5]);
  if(argc >= 7)
    JS_ToFloat64(ctx, &param2, argv[6]);
  if(argc >= 8)
    JS_ToInt32(ctx, &minRadius, argv[7]);
  if(argc >= 9)
    JS_ToInt32(ctx, &maxRadius, argv[8]);

  cv::HoughCircles(image, circles, method, dp, minDist, param1, param2, minRadius, maxRadius);

  i = 0;
  js_array_truncate(ctx, array, 0);

  for(auto& circle : circles) {

    JSValue v = js_array_from(ctx, begin(circle), end(circle));

    JS_SetPropertyUint32(ctx, array, i++, v);
  }

  return JS_UNDEFINED;
}

static JSValue
js_cv_bounding_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputArray input;
  JSRectData<double> rect;

  input = js_cv_inputoutputarray(ctx, argv[0]);

  rect = cv::boundingRect(input);
  return js_rect_wrap(ctx, rect);
}

static JSValue
js_cv_corner_harris(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  cv::Mat* image;
  double k;
  JSInputOutputArray input, output;
  int32_t blockSize, ksize, borderType = cv::BORDER_DEFAULT;

  input = js_umat_or_mat(ctx, argv[0]);
  output = js_umat_or_mat(ctx, argv[1]);

  if(js_is_noarray(input) || js_is_noarray(output))
    return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");
  JS_ToInt32(ctx, &blockSize, argv[2]);
  JS_ToInt32(ctx, &ksize, argv[3]);

  JS_ToFloat64(ctx, &k, argv[4]);

  if(argc >= 6)
    JS_ToInt32(ctx, &borderType, argv[5]);

  cv::cornerHarris(input, output, blockSize, ksize, k, borderType);

  return JS_UNDEFINED;
}

static JSValue
js_cv_draw_contours(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray mat;
  JSContoursData<int> contours;
  JSColorData<double> color;
  int32_t contourIdx = -1, thickness = 1, lineType = cv::LINE_8;

  mat = js_umat_or_mat(ctx, argv[0]);

  if(js_is_noarray(mat))
    return JS_ThrowInternalError(ctx, "mat not an array!");

  if(!js_is_array(ctx, argv[1]))
    return JS_ThrowInternalError(ctx, "argument 2 not an array!");

  js_array_to(ctx, argv[1], contours);

  /*printf("js_cv_draw_contours contours.size() = %zu\n", contours.size());
  if(contours.size())
    printf("js_cv_draw_contours contours.back().size() = %zu\n", contours.back().size());*/

  JS_ToInt32(ctx, &contourIdx, argv[2]);

  js_color_read(ctx, argv[3], &color);

  if(argc > 4)
    JS_ToInt32(ctx, &thickness, argv[4]);

  if(argc > 5)
    JS_ToInt32(ctx, &lineType, argv[5]);

  cv::drawContours(mat, contours, contourIdx, *reinterpret_cast<cv::Scalar*>(&color), thickness, lineType);
  return JS_UNDEFINED;
}

static JSValue
js_cv_equalize_hist(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray src, dst;
  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  cv::equalizeHist(src, dst);
  return JS_UNDEFINED;
}

static JSValue
js_cv_find_contours(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  cv::Mat* m = js_mat_data(ctx, argv[0]);
  JSValue array_buffer, ret = JS_UNDEFINED;
  int mode = cv::RETR_TREE;
  int approx = cv::CHAIN_APPROX_SIMPLE;
  bool hier_callback = JS_IsFunction(ctx, argv[2]);
  cv::Point offset(0, 0);

  JSContoursData<int> contours;
  std::vector<cv::Vec4i> hier;
  JSContoursData<double> poly;

  cv::findContours(*m, contours, hier, mode, approx, offset);

  /*printf("js_cv_find_contours contours.size() = %zu\n", contours.size());
  if(contours.size())
    printf("js_cv_find_contours contours.back().size() = %zu\n", contours.back().size());*/

  if(js_is_array(ctx, argv[1])) {
    js_array_truncate(ctx, argv[1], 0);
  }

  poly.resize(contours.size());

  transform_contours(contours.begin(), contours.end(), poly.begin());

  /*printf("js_cv_find_contours poly.size() = %zu\n", poly.size());
  if(poly.size())
    printf("js_cv_find_contours poly.back().size() = %zu\n", poly.back().size());*/

  array_buffer = js_arraybuffer_from(ctx, begin(hier), end(hier));

  {
    size_t i, length = contours.size();
    JSValue ctor = js_global_get(ctx, "Int32Array");

    for(i = 0; i < length; i++) {
      JSValue array = js_typedarray_new(ctx, array_buffer, i * sizeof(cv::Vec4i), 4, ctor);

      /*std::array<int32_t, 4> v;

      v[0] = hier[i][0];
      v[1] = hier[i][1];
      v[2] = hier[i][2];
      v[3] = hier[i][3];*/

      JS_SetPropertyUint32(ctx, argv[1], i, js_contour_new(ctx, poly[i]));

      if(!hier_callback) {
        JS_SetPropertyUint32(ctx, argv[2], i, array);
        // JS_SetPropertyUint32(ctx, argv[2], i, js_typedarray_from(ctx, v.cbegin(), v.cend()));
        // JS_SetPropertyUint32(ctx, argv[2], i, js_typedarray_from(ctx, begin(hier[i]),
        // end(hier[i])));
      }
    }
    JS_FreeValue(ctx, ctor);
  }

  if(hier_callback) {
    JSValueConst tarray;
    int32_t *hstart, *hend;
    hstart = reinterpret_cast<int32_t*>(&hier[0]);
    hend = reinterpret_cast<int32_t*>(&hier[hier.size()]);

    tarray = js_array_from(ctx, hstart, hend);

    JS_Call(ctx, argv[2], JS_NULL, 1, &tarray);
  }

  /*{
      JSValue hier_arr = js_vector_vec4i_to_array(ctx, hier);
      JSValue contours_obj = js_contours_new(ctx, poly);

      ret = JS_NewObject(ctx);

      JS_SetPropertyStr(ctx, ret, "hier", hier_arr);
      JS_SetPropertyStr(ctx, ret, "contours", contours_obj);
    }*/
  return ret;
}

static JSValue
js_cv_morphology_ex(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {

  JSInputOutputArray src, dst, kernel;
  JSPointData<double> anchor = cv::Point(-1, -1);

  int32_t op, iterations = 1, borderType = cv::BORDER_CONSTANT;
  cv::Scalar borderValue = cv::morphologyDefaultBorderValue();

  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  JS_ToInt32(ctx, &op, argv[2]);
  kernel = js_umat_or_mat(ctx, argv[3]);

  if(js_is_noarray(src) || js_is_noarray(dst) || js_is_noarray(kernel) || argc < 3)
    return JS_ThrowInternalError(ctx, "src or dst or kernel not an array!");

  if(argc >= 5)
    if(!js_point_read(ctx, argv[4], &anchor))
      return JS_EXCEPTION;

  if(argc >= 6)
    JS_ToInt32(ctx, &iterations, argv[5]);

  if(argc >= 7)
    JS_ToInt32(ctx, &borderType, argv[6]);

  if(argc >= 8) {
    std::vector<double> value;
    if(!js_is_array(ctx, argv[7]))
      return JS_EXCEPTION;

    js_array_to(ctx, argv[7], value);
    borderValue = cv::Scalar(value[0], value[1], value[2], value[3]);
  }

  cv::morphologyEx(src, dst, op, kernel, anchor, iterations, borderType, borderValue);
  return JS_UNDEFINED;
}

static JSValue
js_cv_bilateral_filter(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray src, dst;
  double sigmaColor, sigmaSpace;
  int32_t d, borderType = cv::BORDER_DEFAULT;

  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  JS_ToInt32(ctx, &d, argv[2]);

  JS_ToFloat64(ctx, &sigmaColor, argv[3]);
  JS_ToFloat64(ctx, &sigmaSpace, argv[4]);

  if(argc >= 6)
    JS_ToInt32(ctx, &borderType, argv[5]);

  cv::bilateralFilter(src, dst, d, sigmaColor, sigmaSpace, borderType);
  return JS_UNDEFINED;
}

static JSValue
js_cv_point_polygon_test(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>* contour;
  JSPointData<float> point;
  bool measureDist = false;

  contour = js_contour_data(ctx, argv[0]);

  if(contour == nullptr)
    return JS_EXCEPTION;

  point = js_point_get(ctx, argv[1]);
  if(argc >= 3)
    measureDist = JS_ToBool(ctx, argv[2]);

  return JS_NewFloat64(ctx, cv::pointPolygonTest(*contour, point, measureDist));
}

static JSValue
js_cv_getaffinetransform(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  bool handleNested = true;
  JSContourData<float> a, b;
  cv::Mat matrix;

  v = js_contour_data(ctx, argv[0]);
  if(!v)
    return JS_EXCEPTION;

  if(argc > 1)
    other = static_cast<JSContourData<double>*>(JS_GetOpaque2(ctx, argv[1], js_contour_class_id));

  std::transform(v->begin(), v->end(), std::back_inserter(a), [](const JSPointData<double>& pt) -> JSPointData<float> {
    return JSPointData<float>(pt.x, pt.y);
  });
  std::transform(other->begin(), other->end(), std::back_inserter(b), [](const JSPointData<double>& pt) -> JSPointData<float> {
    return JSPointData<float>(pt.x, pt.y);
  });
  matrix = cv::getAffineTransform(a, b);

  ret = js_mat_wrap(ctx, matrix);
  return ret;
}

static JSValue
js_cv_good_features_to_track(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  cv::Mat *image, *corners, *mask = nullptr;
  int32_t maxCorners, blockSize = 3, gradientSize;
  double qualityLevel, minDistance, k = 0.04;
  bool useHarrisDetector = false;
  int argind;

  image = js_mat_data(ctx, argv[0]);
  corners = js_mat_data(ctx, argv[1]);

  if(image == nullptr || corners == nullptr || image->empty())
    return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");

  JS_ToInt32(ctx, &maxCorners, argv[2]);
  JS_ToFloat64(ctx, &qualityLevel, argv[3]);
  JS_ToFloat64(ctx, &minDistance, argv[4]);

  if(argc >= 6)
    mask = js_mat_data(ctx, argv[5]);

  if(argc >= 7)
    JS_ToInt32(ctx, &blockSize, argv[6]);

  argind = 7;

  if(argc > argind) {
    if(JS_IsNumber(argv[argind])) {
      JS_ToInt32(ctx, &gradientSize, argv[argind]);
      argind++;
    }
  }
  if(argc > argind) {
    useHarrisDetector = JS_ToBool(ctx, argv[argind]);
    argind++;
  }
  if(argc > argind)
    JS_ToFloat64(ctx, &k, argv[argind]);

  if(argind == 9)
    cv::goodFeaturesToTrack(*image,
                            *corners,
                            maxCorners,
                            qualityLevel,
                            minDistance,
                            mask ? *mask : cv::noArray(),
                            blockSize,
                            gradientSize,
                            useHarrisDetector,
                            k);
  else
    cv::goodFeaturesToTrack(
        *image, *corners, maxCorners, qualityLevel, minDistance, mask ? *mask : cv::noArray(), blockSize, useHarrisDetector, k);

  return JS_UNDEFINED;
}

static JSValue
js_cv_good_features_to_track(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  cv::Mat *image, *corners, *mask = nullptr;
  int32_t maxCorners, blockSize = 3, gradientSize;
  double qualityLevel, minDistance, k = 0.04;
  bool useHarrisDetector = false;
  int argind;

  image = js_mat_data(ctx, argv[0]);
  corners = js_mat_data(ctx, argv[1]);

  if(image == nullptr || corners == nullptr || image->empty())
    return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");

  JS_ToInt32(ctx, &maxCorners, argv[2]);
  JS_ToFloat64(ctx, &qualityLevel, argv[3]);
  JS_ToFloat64(ctx, &minDistance, argv[4]);

  if(argc >= 6)
    mask = js_mat_data(ctx, argv[5]);

  if(argc >= 7)
    JS_ToInt32(ctx, &blockSize, argv[6]);

  argind = 7;

  if(argc > argind) {
    if(JS_IsNumber(argv[argind])) {
      JS_ToInt32(ctx, &gradientSize, argv[argind]);
      argind++;
    }
  }
  if(argc > argind) {
    useHarrisDetector = JS_ToBool(ctx, argv[argind]);
    argind++;
  }
  if(argc > argind)
    JS_ToFloat64(ctx, &k, argv[argind]);

  if(argind == 9)
    cv::goodFeaturesToTrack(*image,
                            *corners,
                            maxCorners,
                            qualityLevel,
                            minDistance,
                            mask ? *mask : cv::noArray(),
                            blockSize,
                            gradientSize,
                            useHarrisDetector,
                            k);
  else
    cv::goodFeaturesToTrack(
        *image, *corners, maxCorners, qualityLevel, minDistance, mask ? *mask : cv::noArray(), blockSize, useHarrisDetector, k);

  return JS_UNDEFINED;
}

static JSValue
js_cv_get_structuring_element(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  int32_t shape;
  JSSizeData<double> ksize;
  JSPointData<double> anchor = cv::Point(-1, -1);

  if(argc < 2)
    return JS_EXCEPTION;

  if(JS_ToInt32(ctx, &shape, argv[0]) == -1)
    return JS_EXCEPTION;

  if(!js_size_read(ctx, argv[1], &ksize))
    return JS_EXCEPTION;
  if(argc >= 3)
    if(!js_point_read(ctx, argv[2], &anchor))
      return JS_EXCEPTION;

  return js_mat_wrap(ctx, cv::getStructuringElement(shape, ksize, anchor));
}

static JSValue
js_cv_getperspectivetransform(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSContourData<double>*v, *other = nullptr, *ptr;
  JSValue ret = JS_UNDEFINED;
  bool handleNested = true;
  JSContourData<float> a, b;
  cv::Mat matrix;
  int32_t solveMethod = cv::DECOMP_LU;

  v = js_contour_data(ctx, argv[0]);
  if(!v)
    return JS_EXCEPTION;

  if(argc > 1) {
    other = static_cast<JSContourData<double>*>(JS_GetOpaque2(ctx, argv[1], js_contour_class_id));

    if(argc > 2)
      JS_ToInt32(ctx, &solveMethod, argv[2]);
  }

  std::transform(v->begin(), v->end(), std::back_inserter(a), [](const JSPointData<double>& pt) -> JSPointData<float> {
    return JSPointData<float>(pt.x, pt.y);
  });
  std::transform(other->begin(), other->end(), std::back_inserter(b), [](const JSPointData<double>& pt) -> JSPointData<float> {
    return JSPointData<float>(pt.x, pt.y);
  });
  matrix = cv::getPerspectiveTransform(a, b /*, solveMethod*/);

  ret = js_mat_wrap(ctx, matrix);
  return ret;
}

static JSValue
js_cv_blur(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  cv::Mat* image;
  JSSizeData<int> size;
  JSPointData<int> anchor = {-1, -1};
  double sigmaX, sigmaY = 0;
  JSInputOutputArray input, output;
  int32_t borderType = cv::BORDER_DEFAULT;

  JSValue ret;

  if(js_is_noarray((input = js_umat_or_mat(ctx, argv[0]))))
    return JS_ThrowInternalError(ctx, "argument 1 not an array!");

  if(js_is_noarray((output = js_umat_or_mat(ctx, argv[1]))))
    return JS_ThrowInternalError(ctx, "argument 2 not an array!");

  if(argc < 3)
    return JS_EXCEPTION;

  size = js_size_get(ctx, argv[2]);

  if(argc > 3)
    js_point_read(ctx, argv[3], &anchor);

  cv::blur(input, output, size, anchor);

  return JS_UNDEFINED;
}

static JSValue
js_cv_canny(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray image, edges;
  double threshold1, threshold2;
  int32_t apertureSize = 3;
  bool L2gradient = false;

  image = js_umat_or_mat(ctx, argv[0]);
  edges = js_cv_inputoutputarray(ctx, argv[1]);

  if(js_is_noarray(image) || js_is_noarray(edges))
    return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");

  JS_ToFloat64(ctx, &threshold1, argv[2]);
  JS_ToFloat64(ctx, &threshold2, argv[3]);

  if(argc >= 5)
    JS_ToInt32(ctx, &apertureSize, argv[4]);

  if(argc >= 6)
    L2gradient = JS_ToBool(ctx, argv[5]);

  // std::cerr << "cv::Canny threshold1=" << threshold1 << " threshold2=" << threshold2 << "
  // apertureSize=" << apertureSize << " L2gradient=" << L2gradient << std::endl;

  cv::Canny(image, edges, threshold1, threshold2, apertureSize, L2gradient);

  return JS_UNDEFINED;
}

static JSValue
js_cv_resize(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray src, dst;
  double fx, fy;
  JSSizeData<double> dsize;
  int32_t interpolation;

  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  if(!js_size_read(ctx, argv[2], &dsize) || dsize.width == 0 || dsize.height == 0) {
    uint32_t w, h;

    w = dst.cols() > 0 ? dst.cols() : src.cols();
    h = dst.rows() > 0 ? dst.rows() : src.rows();

    dsize = JSSizeData<double>(w, h);
  }

  if(argc > 3)
    JS_ToFloat64(ctx, &fx, argv[3]);
  if(argc > 4)
    JS_ToFloat64(ctx, &fy, argv[4]);
  if(argc > 5)
    JS_ToInt32(ctx, &interpolation, argv[5]);

  cv::resize(src, dst, dsize, fx, fy, interpolation);

  return JS_UNDEFINED;
}

static JSValue
js_cv_calc_hist(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  std::vector<cv::Mat> images;
  std::vector<int> channels, histSize;
  std::vector<std::vector<float>> ranges;

  cv::Mat *mask, *hist;
  int32_t dims;
  bool uniform = true, accumulate = false;

  if(js_array_to(ctx, argv[0], images) == -1)
    return JS_EXCEPTION;

  if(js_array_to(ctx, argv[1], channels) == -1)
    return JS_EXCEPTION;
  mask = js_mat_data(ctx, argv[2]);
  hist = js_mat_data(ctx, argv[3]);

  if(mask == nullptr || hist == nullptr || argc < 8)
    return JS_EXCEPTION;
  JS_ToInt32(ctx, &dims, argv[4]);

  if(js_array_to(ctx, argv[5], histSize) == -1)
    return JS_EXCEPTION;

  if(js_array_to(ctx, argv[6], ranges) == -1)
    return JS_EXCEPTION;

  if(argc >= 8)
    uniform = JS_ToBool(ctx, argv[7]);
  if(argc >= 9)
    accumulate = JS_ToBool(ctx, argv[8]);

  {
    std::vector<const float*> rangePtr(ranges.size());

    for(size_t i = 0; i < ranges.size(); i++) {
      if(ranges[i].size() < 2)
        ranges[i].resize(2);

      rangePtr[i] = ranges[i].data();
    }

    cv::calcHist(const_cast<const cv::Mat*>(images.data()),
                 images.size(),
                 channels.data(),
                 *mask,
                 *hist,
                 dims,
                 histSize.data(),
                 rangePtr.data(),
                 uniform,
                 accumulate);
  }
  return JS_UNDEFINED;
}

static JSValue
js_cv_cvt_color(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {

  JSInputOutputArray src, dst;
  int code, dstCn = 0;
  /*int64_t before, after;
  before = cv::getTickCount();*/

  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  JS_ToInt32(ctx, &code, argv[2]);

  if(argc >= 4)
    JS_ToInt32(ctx, &dstCn, argv[3]);

  try {

    cv::cvtColor(src, dst, code, dstCn);

  } catch(const cv::Exception& e) {
    std::cerr << e.what() << std::endl;
    return JS_ThrowInternalError(ctx, "C++ exception: %s", e.what());
  }

  /*after = cv::getTickCount();
  double t = static_cast<double>(after - before) / cv::getTickFrequency();
  std::cerr << "cv::cvtColor duration = " << t << "s" << std::endl;*/

  return JS_UNDEFINED;
}

static JSValue
js_cv_threshold(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray src, dst;
  double thresh, maxval;
  int32_t type;

  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  JS_ToFloat64(ctx, &thresh, argv[2]);
  JS_ToFloat64(ctx, &maxval, argv[3]);
  JS_ToInt32(ctx, &type, argv[4]);

  cv::threshold(src, dst, thresh, maxval, type);
  return JS_UNDEFINED;
}

JSValue cv_proto = JS_UNDEFINED, cv_class = JS_UNDEFINED;
JSClassID js_imgproc_class_id = 0;

void
js_imgproc_finalizer(JSRuntime* rt, JSValue val) {

  // JS_FreeValueRT(rt, val);
  // JS_FreeValueRT(rt, cv_class);
}

JSClassDef js_imgproc_class = {.class_name = "cv", .finalizer = js_imgproc_finalizer};

typedef std::vector<JSCFunctionListEntry> js_function_list_t;

js_function_list_t js_imgproc_static_funcs{
    JS_CFUNC_DEF("blur", 3, js_cv_blur),
    JS_CFUNC_DEF("boundingRect", 1, js_cv_bounding_rect),
    JS_CFUNC_DEF("GaussianBlur", 4, js_cv_gaussian_blur),
    JS_CFUNC_DEF("HoughLines", 5, js_cv_hough_lines),
    JS_CFUNC_DEF("HoughLinesP", 5, js_cv_hough_lines_p),
    JS_CFUNC_DEF("HoughCircles", 5, js_cv_hough_circles),
    JS_CFUNC_DEF("Canny", 4, js_cv_canny),
    JS_CFUNC_DEF("goodFeaturesToTrack", 5, js_cv_good_features_to_track),
    JS_CFUNC_DEF("goodFeaturesToTrack", 5, js_cv_good_features_to_track),
    JS_CFUNC_DEF("cvtColor", 3, js_cv_cvt_color),
    JS_CFUNC_DEF("equalizeHist", 2, js_cv_equalize_hist),
    JS_CFUNC_DEF("threshold", 5, js_cv_threshold),
    JS_CFUNC_DEF("bilateralFilter", 5, js_cv_bilateral_filter),
    JS_CFUNC_DEF("getPerspectiveTransform", 2, js_cv_getperspectivetransform),
    JS_CFUNC_DEF("getAffineTransform", 2, js_cv_getaffinetransform),
    JS_CFUNC_DEF("findContours", 1, js_cv_find_contours),
    JS_CFUNC_DEF("drawContours", 4, js_cv_draw_contours),
    JS_CFUNC_DEF("pointPolygonTest", 2, js_cv_point_polygon_test),
    JS_CFUNC_DEF("cornerHarris", 5, js_cv_corner_harris),
    JS_CFUNC_DEF("calcHist", 8, js_cv_calc_hist),
    JS_CFUNC_DEF("morphologyEx", 4, js_cv_morphology_ex),
    JS_CFUNC_DEF("getStructuringElement", 2, js_cv_get_structuring_element),
    JS_CFUNC_DEF("medianBlur", 3, js_cv_median_blur),
    JS_CFUNC_DEF("resize", 3, js_cv_resize),

};

extern "C" int
js_imgproc_init(JSContext* ctx, JSModuleDef* m) {
  JSAtom atom;
  JSValue g = JS_GetGlobalObject(ctx);

  /* std::cerr << "js_imgproc_static_funcs:" << std::endl << js_imgproc_static_funcs;
   std::cerr << "js_imgproc_static_funcs.size() = " << js_imgproc_static_funcs.size() << std::endl;*/
  if(m) {
    JS_SetModuleExportList(ctx, m, js_imgproc_static_funcs.data(), js_imgproc_static_funcs.size());
  }
  atom = JS_NewAtom(ctx, "cv");

  if(JS_HasProperty(ctx, g, atom)) {
    cv_class = JS_GetProperty(ctx, g, atom);
  } else {
    cv_class = JS_NewObject(ctx);
  }
  JS_SetPropertyFunctionList(ctx, cv_class, js_imgproc_static_funcs.data(), js_imgproc_static_funcs.size());

  if(!JS_HasProperty(ctx, g, atom)) {
    JS_SetPropertyInternal(ctx, g, atom, cv_class, 0);
  }

  JS_SetModuleExport(ctx, m, "default", cv_class);

  JS_FreeValue(ctx, g);
  return 0;
}

extern "C" VISIBLE void
js_imgproc_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_imgproc_static_funcs.data(), js_imgproc_static_funcs.size());
  JS_AddModuleExport(ctx, m, "default");
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_imgproc
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_imgproc_init);
  if(!m)
    return NULL;
  js_imgproc_export(ctx, m);
  return m;
}