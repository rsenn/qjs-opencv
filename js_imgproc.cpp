#include "cutils.h"
#include "include/geometry.hpp"
#include "js_cv.hpp"
#include "include/js_array.hpp"
#include "js_contour.hpp"
#include "js_mat.hpp"
#include "js_object.hpp"
#include "js_point.hpp"
#include "js_rect.hpp"
#include "js_rotated_rect.hpp"
#include "js_size.hpp"
#include "js_typed_array.hpp"
#include "js_umat.hpp"
#include "jsbindings.hpp"
#include <quickjs.h>
#include "include/util.hpp"
#include <cassert>
#include <stddef.h>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <opencv2/imgproc.hpp>

#include <string>
#include <vector>
#include "lsd_opencv.hpp"

enum {
  HIER_NEXT = 0,
  HIER_PREV,
  HIER_CHILD,
  HIER_PARENT,
};

static JSValue
js_cv_blur(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
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
js_cv_bounding_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSRectData<double> rect;
  JSImageArgument input(ctx, argv[0]);

  rect = cv::boundingRect(input);
  return js_rect_wrap(ctx, rect);
}

static JSValue
js_cv_gaussian_blur(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSSizeData<double> size;
  double sigmaX, sigmaY = 0;
  int32_t borderType = cv::BORDER_DEFAULT;
  JSImageArgument input(ctx, argv[0]);
  JSImageArgument output(ctx, argv[1]);

  if(js_is_noarray(input) || js_is_noarray(output))
    return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");

  size = js_size_get(ctx, argv[2]);

  JS_ToFloat64(ctx, &sigmaX, argv[3]);
  if(argc > 4)
    JS_ToFloat64(ctx, &sigmaY, argv[4]);
  if(argc > 5)
    JS_ToInt32(ctx, &borderType, argv[5]);

  // std::cerr << "cv::GaussianBlur size=" << size << " sigmaX=" << sigmaX << " sigmaY=" <<
  // sigmaY
  // << " borderType=" << borderType << std::endl;
  cv::GaussianBlur(input, output, size, sigmaX, sigmaY, borderType);

  return JS_UNDEFINED;
}

static JSValue
js_cv_corner_harris(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  cv::Mat* image;
  double k;
  int32_t blockSize, ksize, borderType = cv::BORDER_DEFAULT;
  JSImageArgument input(ctx, argv[0]);
  JSImageArgument output(ctx, argv[1]);

  if(js_is_noarray(input) || js_is_noarray(output))
    return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");

  JS_ToInt32(ctx, &blockSize, argv[2]);
  JS_ToInt32(ctx, &ksize, argv[3]);

  JS_ToFloat64(ctx, &k, argv[4]);

  if(argc > 5)
    JS_ToInt32(ctx, &borderType, argv[5]);

  cv::cornerHarris(input, output, blockSize, ksize, k, borderType);

  return JS_UNDEFINED;
}

static JSValue
js_cv_hough_lines(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
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

  JSImageArgument image(ctx, argv[0]);

  if(js_is_noarray(image) || !js_is_array(ctx, argv[1]))
    return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");

  array = argv[1];
  JS_ToFloat64(ctx, &rho, argv[2]);
  JS_ToFloat64(ctx, &theta, argv[3]);
  JS_ToInt32(ctx, &threshold, argv[4]);

  if(argc > 5)
    JS_ToFloat64(ctx, &srn, argv[5]);

  if(argc > 6)
    JS_ToFloat64(ctx, &stn, argv[6]);

  if(argc > 7)
    JS_ToFloat64(ctx, &min_theta, argv[7]);

  if(argc > 8)
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
js_cv_hough_lines_p(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
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

  JSImageArgument src(ctx, argv[0]);
  JSImageArgument dst(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  array = argv[1];
  JS_ToFloat64(ctx, &rho, argv[2]);
  JS_ToFloat64(ctx, &theta, argv[3]);
  JS_ToInt32(ctx, &threshold, argv[4]);

  if(argc > 5)
    JS_ToFloat64(ctx, &minLineLength, argv[5]);
  if(argc > 6)
    JS_ToFloat64(ctx, &maxLineGap, argv[6]);

  cv::HoughLinesP(src, lines, rho, theta, threshold, minLineLength, maxLineGap);

  cv::Mat(lines).copyTo(dst);

  /* i = 0;
    js_array_truncate(ctx, array, 0);

    for(const auto& line : lines) {
      JSValue v = js_line_new(ctx, line[0], line[1], line[2], line[3]);

      JS_SetPropertyUint32(ctx, array, i++, v);
    }*/

  return JS_UNDEFINED;
}

static JSValue
js_cv_hough_circles(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValueConst array;
  int32_t method, minRadius = 0, maxRadius = 0;
  double dp, minDist, param1 = 100, param2 = 100;
  size_t i;
  JSValue ret;

  if(argc < 5)
    return JS_EXCEPTION;

  JSImageArgument image(ctx, argv[0]);
  JSOutputArgument circles(ctx, argv[1]);

  /*  if(js_is_noarray(image) || !js_is_array(ctx, argv[1]))
      return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");*/

  array = argv[1];
  JS_ToInt32(ctx, &method, argv[2]);
  JS_ToFloat64(ctx, &dp, argv[3]);
  JS_ToFloat64(ctx, &minDist, argv[4]);

  if(argc > 5)
    JS_ToFloat64(ctx, &param1, argv[5]);
  if(argc > 6)
    JS_ToFloat64(ctx, &param2, argv[6]);
  if(argc > 7)
    JS_ToInt32(ctx, &minRadius, argv[7]);
  if(argc > 8)
    JS_ToInt32(ctx, &maxRadius, argv[8]);

  cv::HoughCircles(image, circles, method, dp, minDist, param1, param2, minRadius, maxRadius);

  /* i = 0;
   js_array_truncate(ctx, array, 0);

   for(auto& circle : circles) {

     JSValue v = js_array_from(ctx, begin(circle), end(circle));

     JS_SetPropertyUint32(ctx, array, i++, v);
   }*/

  return JS_UNDEFINED;
}

static JSValue
js_cv_canny(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  double threshold1, threshold2;
  int32_t apertureSize = 3;
  bool L2gradient = false;

  JSImageArgument image(ctx, argv[0]);
  JSImageArgument edges(ctx, argv[1]);

  if(js_is_noarray(image) || js_is_noarray(edges))
    return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");

  JS_ToFloat64(ctx, &threshold1, argv[2]);
  JS_ToFloat64(ctx, &threshold2, argv[3]);

  if(argc > 4)
    JS_ToInt32(ctx, &apertureSize, argv[4]);

  if(argc > 5)
    L2gradient = JS_ToBool(ctx, argv[5]);

  // std::cerr << "cv::Canny threshold1=" << threshold1 << " threshold2=" << threshold2 << "
  // apertureSize=" << apertureSize << " L2gradient=" << L2gradient << std::endl;

  cv::Canny(image, edges, threshold1, threshold2, apertureSize, L2gradient);

  return JS_UNDEFINED;
}

static JSValue
js_cv_good_features_to_track(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  cv::Mat *image, *corners, *mask = nullptr;
  int32_t maxCorners, blockSize = 3, gradientSize;
  double qualityLevel, minDistance, k = 0.04;
  bool useHarrisDetector = false;
  int argind;

  image = js_mat_data2(ctx, argv[0]);
  corners = js_mat_data2(ctx, argv[1]);

  if(image == nullptr || corners == nullptr || image->empty())
    return JS_ThrowInternalError(ctx, "argument 1 or argument 2 not an array!");

  JS_ToInt32(ctx, &maxCorners, argv[2]);
  JS_ToFloat64(ctx, &qualityLevel, argv[3]);
  JS_ToFloat64(ctx, &minDistance, argv[4]);

  if(argc > 5)
    mask = js_mat_data2(ctx, argv[5]);

  if(argc > 6)
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
    cv::goodFeaturesToTrack(
        *image, *corners, maxCorners, qualityLevel, minDistance, mask ? *mask : cv::noArray(), blockSize, gradientSize, useHarrisDetector, k);
  else
    cv::goodFeaturesToTrack(*image, *corners, maxCorners, qualityLevel, minDistance, mask ? *mask : cv::noArray(), blockSize, useHarrisDetector, k);

  return JS_UNDEFINED;
}

enum {
  CONVERT_COLOR,
  CONVERT_COLOR_TWOPLANE,
  CONVERT_DEMOSAICING,
};

static JSValue
js_cv_cvt_color(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  int32_t code = -1, dstCn = 0;

  JSImageArgument src(ctx, argv[0]);
  JSImageArgument dst(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  try {
    switch(magic) {
      case CONVERT_COLOR: {

        JS_ToInt32(ctx, &code, argv[2]);

        if(argc > 3)
          JS_ToInt32(ctx, &dstCn, argv[3]);

        cv::cvtColor(src, dst, code, dstCn);

        break;
      }

      case CONVERT_COLOR_TWOPLANE: {
        JSInputOutputArray dst2 = js_umat_or_mat(ctx, argv[2]);

        JS_ToInt32(ctx, &code, argv[3]);

        cv::cvtColorTwoPlane(src, dst, dst2, code);
        break;
      }

      case CONVERT_DEMOSAICING: {
        JS_ToInt32(ctx, &code, argv[2]);

        if(argc > 3)
          JS_ToInt32(ctx, &dstCn, argv[3]);

        cv::demosaicing(src, dst, code, dstCn);
        break;
      }
    }
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  /*after = cv::getTickCount();
  double t = static_cast<double>(after - before) / cv::getTickFrequency();
  std::cerr << "cv::cvtColor duration = " << t << "s" << std::endl;*/

  return JS_UNDEFINED;
}

static JSValue
js_cv_equalize_hist(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSImageArgument src(ctx, argv[0]);
  JSImageArgument dst(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  cv::equalizeHist(src, dst);
  return JS_UNDEFINED;
}

static JSValue
js_cv_threshold(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  double thresh, maxval;
  int32_t type;

  JSImageArgument src(ctx, argv[0]);
  JSImageArgument dst(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  JS_ToFloat64(ctx, &thresh, argv[2]);
  JS_ToFloat64(ctx, &maxval, argv[3]);
  JS_ToInt32(ctx, &type, argv[4]);

  cv::threshold(src, dst, thresh, maxval, type);
  return JS_UNDEFINED;
}

static JSValue
js_cv_bilateral_filter(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  double sigmaColor, sigmaSpace;
  int32_t d, borderType = cv::BORDER_DEFAULT;
  JSImageArgument src(ctx, argv[0]);
  JSImageArgument dst(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  JS_ToInt32(ctx, &d, argv[2]);

  JS_ToFloat64(ctx, &sigmaColor, argv[3]);
  JS_ToFloat64(ctx, &sigmaSpace, argv[4]);

  if(argc > 5)
    JS_ToInt32(ctx, &borderType, argv[5]);

  cv::bilateralFilter(src, dst, d, sigmaColor, sigmaSpace, borderType);
  return JS_UNDEFINED;
}

static JSValue
js_cv_calc_hist(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
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

  mask = js_mat_data2(ctx, argv[2]);
  hist = js_mat_data2(ctx, argv[3]);

  if(mask == nullptr || hist == nullptr || argc < 8)
    return JS_EXCEPTION;

  JS_ToInt32(ctx, &dims, argv[4]);

  if(js_array_to(ctx, argv[5], histSize) == -1)
    return JS_EXCEPTION;

  if(js_array_to(ctx, argv[6], ranges) == -1)
    return JS_EXCEPTION;

  if(argc > 7)
    uniform = JS_ToBool(ctx, argv[7]);

  if(argc > 8)
    accumulate = JS_ToBool(ctx, argv[8]);

  std::vector<const float*> rangePtr(ranges.size());

  for(size_t i = 0; i < ranges.size(); i++) {
    if(ranges[i].size() < 2)
      ranges[i].resize(2);

    rangePtr[i] = ranges[i].data();
  }

  cv::calcHist(
      const_cast<const cv::Mat*>(images.data()), images.size(), channels.data(), *mask, *hist, dims, histSize.data(), rangePtr.data(), uniform, accumulate);

  return JS_UNDEFINED;
}

static JSValue
js_cv_morphology(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSPointData<double> anchor = cv::Point(-1, -1);
  int32_t iterations = 1, borderType = cv::BORDER_CONSTANT;
  cv::Scalar borderValue = cv::morphologyDefaultBorderValue();
  JSImageArgument src(ctx, argv[0]);
  JSImageArgument dst(ctx, argv[1]);
  JSImageArgument kernel(ctx, argv[2]);

  if(js_is_noarray(src) || js_is_noarray(dst) || js_is_noarray(kernel) || argc < 3)
    return JS_ThrowInternalError(ctx, "src or dst or kernel not an array!");

  if(argc > 3)
    if(!js_point_read(ctx, argv[3], &anchor))
      return JS_EXCEPTION;

  if(argc > 4)
    JS_ToInt32(ctx, &iterations, argv[4]);

  if(argc > 5)
    JS_ToInt32(ctx, &borderType, argv[5]);

  if(argc > 6) {
    std::vector<double> value;
    if(!js_is_array(ctx, argv[6]))
      return JS_EXCEPTION;

    js_array_to(ctx, argv[6], value);
    borderValue = cv::Scalar(value[0], value[1], value[2], value[3]);
  }

  try {
    switch(magic) {
      case 0: cv::dilate(src, dst, kernel, anchor, iterations, borderType, borderValue); break;
      case 1: cv::erode(src, dst, kernel, anchor, iterations, borderType, borderValue); break;
    }
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static JSValue
js_cv_morphology_ex(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSPointData<double> anchor = cv::Point(-1, -1);

  int32_t op, iterations = 1, borderType = cv::BORDER_CONSTANT;
  cv::Scalar borderValue = cv::morphologyDefaultBorderValue();

  JSImageArgument src(ctx, argv[0]);
  JSImageArgument dst(ctx, argv[1]);

  JS_ToInt32(ctx, &op, argv[2]);
  JSImageArgument kernel(ctx, argv[3]);

  if(js_is_noarray(src) || js_is_noarray(dst) || js_is_noarray(kernel) || argc < 3)
    return JS_ThrowInternalError(ctx, "src or dst or kernel not an array!");

  if(argc > 4)
    if(!js_point_read(ctx, argv[4], &anchor))
      return JS_EXCEPTION;

  if(argc > 5)
    JS_ToInt32(ctx, &iterations, argv[5]);

  if(argc > 6)
    JS_ToInt32(ctx, &borderType, argv[6]);

  if(argc > 7) {
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
js_cv_median_blur(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  int32_t ksize;
  JSImageArgument src(ctx, argv[0]);
  JSImageArgument dst(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  JS_ToInt32(ctx, &ksize, argv[2]);

  assert(ksize >= 1);
  assert(ksize % 2 == 1);

  cv::medianBlur(src, dst, ksize);
  return JS_UNDEFINED;
}

static JSValue
js_cv_lsd(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  std::vector<cv::Vec4i> lines;
  std::vector<double> width, prec, nfa;
  JSInputArray src = js_umat_or_mat(ctx, argv[0]);

  if(src.empty())
    return JS_ThrowInternalError(ctx, "argument 1 must be Mat or UMat");

  // lines = js_cv_inputoutputarray(ctx, argv[1]);

  cv::Ptr<cv::LSD> ls = cv::createLSDPtr(::LSD_REFINE_ADV);

  ls->detect(src, lines, width, prec, nfa);

  if(!js_is_array(ctx, argv[1]))
    return JS_ThrowTypeError(ctx, "argument 2 must be line array");

  js_array_copy(ctx, argv[1], reinterpret_cast<JSLineData<int>*>(lines.data()), reinterpret_cast<JSLineData<int>*>(lines.data() + lines.size()));

  if(argc > 2) {
    if(!js_is_array(ctx, argv[2]))
      return JS_ThrowTypeError(ctx, "argument 3 must be array");
    js_array_copy(ctx, argv[2], width);

    if(argc > 3) {
      if(!js_is_array(ctx, argv[3]))
        return JS_ThrowTypeError(ctx, "argument 4 must be array");
      js_array_copy(ctx, argv[3], prec);

      if(argc > 4) {
        if(!js_is_array(ctx, argv[4]))
          return JS_ThrowTypeError(ctx, "argument 5 must be array");
        js_array_copy(ctx, argv[4], nfa);
      }
    }
  }

  return JS_UNDEFINED;
}

static JSValue
js_cv_find_contours(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  cv::Mat* m = js_mat_data2(ctx, argv[0]);
  JSValue array_buffer, ret = JS_UNDEFINED;
  int32_t mode = cv::RETR_TREE;
  int32_t approx = cv::CHAIN_APPROX_SIMPLE;
  bool hier_callback, contours_array, hier_array, hier_mat;
  cv::Point offset(0, 0);

  JSContoursData<int> contours;
  std::vector<cv::Vec4i> vec4i;
  JSContoursData<double> poly;
  JSInputOutputArray hier(vec4i);

  hier_mat = /*hier.isUMat() || hier.isMat() || */ js_mat_data_nothrow(argv[2]);
  hier_callback = !hier_mat && js_is_function(ctx, argv[2]);
  contours_array = JS_IsArray(ctx, argv[1]);
  hier_array = js_is_array(ctx, argv[2]);

  if(hier_mat)
    hier = js_umat_or_mat(ctx, argv[2]);
  /*else
    hier = JSInputOutputArray(vec4i);*/

  if(argc > 3)
    JS_ToInt32(ctx, &mode, argv[3]);
  if(argc > 4)
    JS_ToInt32(ctx, &approx, argv[4]);

  cv::findContours(*m, contours, hier, mode, approx, offset);

  if(contours_array)
    js_array_truncate(ctx, argv[1], 0);

  poly.resize(contours.size());
  transform_contours(contours.begin(), contours.end(), poly.begin());

  array_buffer = js_arraybuffer_from(ctx, begin(vec4i), end(vec4i));

  /* if(contours_array)
     js_array_copy<JSContoursData<double>>(ctx, argv[1], poly);
 */
  {
    size_t i, length = poly.size();
    JSValue ctor = js_global_get(ctx, "Int32Array");
    for(i = 0; i < length; i++) {
      if(contours_array) {
        JSValue contour = js_contour_move(ctx, std::move(poly[i]));
        JS_SetPropertyUint32(ctx, argv[1], i, contour);
      }
      if(hier_array) {
        JSValue array = js_typedarray_new(ctx, array_buffer, i * sizeof(cv::Vec4i), 4, ctor);
        JS_SetPropertyUint32(ctx, argv[2], i, array);
      }
    }
    JS_FreeValue(ctx, ctor);
  }

  if(hier_callback) {
    JSValueConst tarray;
    int32_t *hstart, *hend;
    hstart = reinterpret_cast<int32_t*>(&vec4i[0]);
    hend = reinterpret_cast<int32_t*>(&vec4i[vec4i.size()]);
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
js_cv_draw_contours(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContoursData<int> contours;
  JSColorData<double> color;
  int32_t index = -1, thickness = 1, lineType = cv::LINE_8;

  JSImageArgument mat(ctx, argv[0]);

  if(js_is_noarray(mat))
    return JS_ThrowInternalError(ctx, "mat not an array!");

  if(!js_is_array(ctx, argv[1]))
    return JS_ThrowInternalError(ctx, "argument 2 not an array!");

  js_array_to(ctx, argv[1], contours);

  JS_ToInt32(ctx, &index, argv[2]);

  js_color_read(ctx, argv[3], &color);

  if(argc > 4)
    JS_ToInt32(ctx, &thickness, argv[4]);

  if(argc > 5)
    JS_ToInt32(ctx, &lineType, argv[5]);

  cv::Scalar scalar = cv::Scalar(color);

  if(mat.isMat()) {
    cv::Mat& mref = mat.getMatRef();

    cv::drawContours(mref, contours, index, scalar, thickness, lineType);
  } else {
    cv::drawContours(mat, contours, index, scalar, thickness, lineType);
  }

  return JS_UNDEFINED;
}

static JSValue
js_cv_point_polygon_test(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContourData<double>* contour;
  JSPointData<float> point;
  bool measureDist = false;

  contour = js_contour_data2(ctx, argv[0]);

  if(contour == nullptr)
    return JS_EXCEPTION;

  point = js_point_get(ctx, argv[1]);
  if(argc > 2)
    measureDist = JS_ToBool(ctx, argv[2]);

  return JS_NewFloat64(ctx, cv::pointPolygonTest(*contour, point, measureDist));
}

enum {
  MOTION_ACCUMULATE = 0,
  MOTION_ACCUMULATE_PRODUCT,
  MOTION_ACCUMULATE_SQUARE,
  MOTION_ACCUMULATE_WEIGHTED,
  MOTION_CREATE_HANNING_WINDOW,
  MOTION_DIV_SPECTRUMS,
  MOTION_PHASE_CORRELATE,
};

static JSValue
js_imgproc_motion(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSInputOutputArray src;
  JSValue ret = JS_UNDEFINED;

  src = argc >= 1 ? js_umat_or_mat(ctx, argv[0]) : cv::noArray();

  try {
    switch(magic) {
      case MOTION_ACCUMULATE: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        JSInputArray mask = cv::noArray();
        if(argc > 2)
          mask = js_umat_or_mat(ctx, argv[2]);
        cv::accumulate(src, dst, mask);
        break;
      }

      case MOTION_ACCUMULATE_PRODUCT: {
        JSInputArray src2 = js_umat_or_mat(ctx, argv[1]);
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[2]);
        JSInputArray mask = cv::noArray();
        if(argc > 3)
          mask = js_umat_or_mat(ctx, argv[3]);
        cv::accumulateProduct(src, src2, dst, mask);
        break;
      }

      case MOTION_ACCUMULATE_SQUARE: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        JSInputArray mask = cv::noArray();
        if(argc > 2)
          mask = js_umat_or_mat(ctx, argv[2]);
        cv::accumulateSquare(src, dst, mask);
        break;
      }

      case MOTION_ACCUMULATE_WEIGHTED: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        double alpha;
        JSInputArray mask = cv::noArray();
        JS_ToFloat64(ctx, &alpha, argv[2]);
        if(argc > 3)
          mask = js_umat_or_mat(ctx, argv[3]);
        cv::accumulateWeighted(src, dst, alpha, mask);
        break;
      }

      case MOTION_CREATE_HANNING_WINDOW: {
        JSSizeData<int> winSize;
        int32_t type;
        js_size_read(ctx, argv[1], &winSize);
        JS_ToInt32(ctx, &type, argv[2]);
        cv::createHanningWindow(src, winSize, type);
        break;
      }

      case MOTION_DIV_SPECTRUMS: {
        JSInputOutputArray b = js_umat_or_mat(ctx, argv[1]);
        JSInputOutputArray c = js_umat_or_mat(ctx, argv[2]);
        int32_t flags;
        BOOL conjB = FALSE;
        JS_ToInt32(ctx, &flags, argv[3]);

        if(argc > 4)
          conjB = JS_ToBool(ctx, argv[4]);

        cv::divSpectrums(src, b, c, flags, conjB);
        break;
      }

      case MOTION_PHASE_CORRELATE: {
        JSInputArray src2 = js_umat_or_mat(ctx, argv[1]), window = cv::noArray();
        double response = 0;
        JSPointData<double> result;
        if(argc > 2)
          window = js_umat_or_mat(ctx, argv[2]);

        result = cv::phaseCorrelate(src, src2, window, &response);
        ret = js_point_new(ctx, result);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

enum {
  MISC_ADAPTIVE_THRESHOLD = 0,
  MISC_BLEND_LINEAR,
  MISC_DISTANCE_TRANSFORM,
  MISC_FLOOD_FILL,
  MISC_GRAB_CUT,
  MISC_INTEGRAL,
  MISC_WATERSHED,
  MISC_APPLY_COLORMAP,
  MISC_MOMENTS,
};

static JSValue
js_imgproc_misc(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSInputOutputArray src;
  JSValue ret = JS_UNDEFINED;

  src = argc >= 1 ? js_umat_or_mat(ctx, argv[0]) : cv::noArray();

  try {
    switch(magic) {
      case MISC_ADAPTIVE_THRESHOLD: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        double maxValue, C;
        int32_t adaptiveMethod, thresholdType, blockSize;
        JS_ToFloat64(ctx, &maxValue, argv[2]);
        JS_ToInt32(ctx, &adaptiveMethod, argv[3]);
        JS_ToInt32(ctx, &thresholdType, argv[4]);
        JS_ToInt32(ctx, &blockSize, argv[5]);
        JS_ToFloat64(ctx, &C, argv[6]);

        cv::adaptiveThreshold(src, dst, maxValue, adaptiveMethod, thresholdType, blockSize, C);
        break;
      }

      case MISC_BLEND_LINEAR: {
        JSInputArray src2 = js_umat_or_mat(ctx, argv[1]);
        JSInputArray weights1 = js_umat_or_mat(ctx, argv[2]);
        JSInputArray weights2 = js_umat_or_mat(ctx, argv[3]);
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[4]);
        cv::blendLinear(src, src2, weights1, weights2, dst);

        break;
      }

      case MISC_DISTANCE_TRANSFORM: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        JSInputOutputArray labels = js_umat_or_mat(ctx, argv[2]);
        int32_t distanceType, maskSize, labelType = cv::DIST_LABEL_CCOMP;
        JS_ToInt32(ctx, &distanceType, argv[3]);
        JS_ToInt32(ctx, &maskSize, argv[4]);
        if(argc > 5)
          JS_ToInt32(ctx, &labelType, argv[5]);
        // XXX: overload
        cv::distanceTransform(src, dst, labels, distanceType, maskSize, labelType);
        break;
      }

      case MISC_FLOOD_FILL: {
        JSPointData<int> seedPoint;
        JSColorData<double> newVal;
        JSRectData<int> rect, *rectPtr = 0;
        cv::Scalar loDiff = cv::Scalar(), upDiff = cv::Scalar();
        int32_t flags = 4;

        js_point_read(ctx, argv[1], &seedPoint);
        js_color_read(ctx, argv[2], &newVal);
        if(js_rect_read(ctx, argv[3], &rect)) {
          rectPtr = &rect;
        }
        js_array_to(ctx, argv[4], loDiff);
        js_array_to(ctx, argv[5], upDiff);

        if(argc > 6)
          JS_ToInt32(ctx, &flags, argv[6]);
        // XXX: overload
        ret = JS_NewInt32(ctx, cv::floodFill(src, seedPoint, cv::Scalar(newVal), rectPtr, loDiff, upDiff, flags));
        break;
      }

      case MISC_GRAB_CUT: {
        JSInputOutputArray mask = js_umat_or_mat(ctx, argv[1]);
        JSRectData<int> rect;
        JSInputOutputArray bgdModel, fgdModel;
        int32_t iterCount, mode = cv::GC_EVAL;
        js_rect_read(ctx, argv[2], &rect);
        bgdModel = js_umat_or_mat(ctx, argv[3]);
        fgdModel = js_umat_or_mat(ctx, argv[4]);
        JS_ToInt32(ctx, &iterCount, argv[5]);
        if(argc > 6)
          JS_ToInt32(ctx, &mode, argv[6]);
        cv::grabCut(src, mask, rect, bgdModel, fgdModel, iterCount, mode);
        break;
      }

      case MISC_INTEGRAL: {
        JSInputOutputArray sum = js_umat_or_mat(ctx, argv[1]);
        int32_t sdepth = -1;
        if(argc > 2)
          JS_ToInt32(ctx, &sdepth, argv[2]);
        // XXX: overload
        cv::integral(src, sum, sdepth);
        break;
      }

      case MISC_WATERSHED: {
        JSInputOutputArray markers = js_umat_or_mat(ctx, argv[1]);
        cv::watershed(src, markers);
        break;
      }

      case MISC_APPLY_COLORMAP: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);

        if(JS_IsNumber(argv[2])) {
          int32_t colormap;
          JS_ToInt32(ctx, &colormap, argv[2]);
          cv::applyColorMap(src, dst, colormap);
        } else {
          JSInputArray colormap = js_umat_or_mat(ctx, argv[2]);
          cv::applyColorMap(src, dst, colormap);
        }
        break;
      }

      case MISC_MOMENTS: {
        BOOL binaryImage = false;
        cv::Moments moments;
        std::map<std::string, double> moments_map;

        if(argc >= 2)
          binaryImage = JS_ToBool(ctx, argv[1]);

        if(!binaryImage) {
          std::vector<JSPointData<float>> polygon;
          js_array_to(ctx, argv[0], polygon);
          moments = cv::moments(polygon, binaryImage);
        } else {
          moments = cv::moments(src, true);
        }

        moments_map["m00"] = moments.m00;
        moments_map["m10"] = moments.m10;
        moments_map["m01"] = moments.m01;
        moments_map["m20"] = moments.m20;
        moments_map["m11"] = moments.m11;
        moments_map["m02"] = moments.m02;
        moments_map["m30"] = moments.m30;
        moments_map["m21"] = moments.m21;
        moments_map["m12"] = moments.m12;
        moments_map["m03"] = moments.m03;
        moments_map["mu20"] = moments.mu20;
        moments_map["mu11"] = moments.mu11;
        moments_map["mu02"] = moments.mu02;
        moments_map["mu30"] = moments.mu30;
        moments_map["mu21"] = moments.mu21;
        moments_map["mu12"] = moments.mu12;
        moments_map["mu03"] = moments.mu03;
        moments_map["nu20"] = moments.nu20;
        moments_map["nu11"] = moments.nu11;
        moments_map["nu02"] = moments.nu02;
        moments_map["nu30"] = moments.nu30;
        moments_map["nu21"] = moments.nu21;
        moments_map["nu12"] = moments.nu12;
        moments_map["nu03"] = moments.nu03;

        ret = js_object_from(ctx, moments_map);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}
enum {
  TRANSFORM_CONVERT_MAPS = 0,
  TRANSFORM_GET_AFFINE_TRANSFORM,
  TRANSFORM_GET_PERSPECTIVE_TRANSFORM,
  TRANSFORM_GET_RECT_SUB_PIX,
  TRANSFORM_GET_ROTATION_MATRIX2_D,
  TRANSFORM_GET_ROTATION_MATRIX2D_,
  TRANSFORM_INVERT_AFFINE_TRANSFORM,
  TRANSFORM_LINEAR_POLAR,
  TRANSFORM_LOG_POLAR,
  TRANSFORM_REMAP,
  TRANSFORM_RESIZE,
  TRANSFORM_WARP_AFFINE,
  TRANSFORM_WARP_PERSPECTIVE,
  TRANSFORM_WARP_POLAR,
};

static JSValue
js_imgproc_transform(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSInputArray src;
  JSValue ret = JS_UNDEFINED;

  if(magic != TRANSFORM_GET_ROTATION_MATRIX2_D && magic != TRANSFORM_GET_ROTATION_MATRIX2D_)
    src = argc >= 1 ? js_input_array(ctx, argv[0]) : JSInputArray();

  try {
    switch(magic) {
      case TRANSFORM_CONVERT_MAPS: {
        JSInputArray map2 = js_umat_or_mat(ctx, argv[1]);
        JSInputOutputArray dstmap1 = js_umat_or_mat(ctx, argv[2]);
        JSInputOutputArray dstmap2 = js_umat_or_mat(ctx, argv[3]);
        int32_t dstmap1type;
        BOOL nninterpolation = FALSE;

        JS_ToInt32(ctx, &dstmap1type, argv[4]);

        if(argc > 5)
          nninterpolation = JS_ToBool(ctx, argv[5]);

        cv::convertMaps(src, map2, dstmap1, dstmap2, dstmap1type, nninterpolation);
        break;
      }

      case TRANSFORM_GET_AFFINE_TRANSFORM: {
        JSInputArray dst = js_input_array(ctx, argv[1]);
        JSContourData<float> sc, dc;

        if(js_contour_read(ctx, argv[0], &sc))
          src = JSInputArray(sc);

        if(js_contour_read(ctx, argv[1], &dc))
          dst = JSInputArray(dc);

        JSMatData mat = cv::getAffineTransform(src, dst);
        ret = js_mat_wrap(ctx, mat);
        break;
      }

      case TRANSFORM_GET_PERSPECTIVE_TRANSFORM: {
        int32_t solveMethod = cv::DECOMP_LU;
        JSMatData mat;

        if(argc > 2)
          JS_ToInt32(ctx, &solveMethod, argv[2]);

        if(JS_IsArray(ctx, argv[0])) {
          std::vector<JSPointData<float>> src, dst;

          js_array_to(ctx, argv[0], src);
          js_array_to(ctx, argv[1], dst);

          mat = cv::getPerspectiveTransform(src, dst, solveMethod);
        } else {
          JSInputArray dst = js_umat_or_mat(ctx, argv[1]);
          mat = cv::getPerspectiveTransform(src, dst, solveMethod);
        }

        ret = js_mat_wrap(ctx, mat);
        break;
      }

      case TRANSFORM_GET_RECT_SUB_PIX: {
        JSSizeData<int> patchSize;
        JSPointData<float> center;
        JSOutputArray patch;
        int32_t patchType = -1;

        js_size_read(ctx, argv[1], &patchSize);
        js_point_read(ctx, argv[2], &center);
        patch = js_umat_or_mat(ctx, argv[3]);

        if(argc > 4)
          JS_ToInt32(ctx, &patchType, argv[4]);

        cv::getRectSubPix(src, patchSize, center, patch, patchType);
        break;
      }

      case TRANSFORM_GET_ROTATION_MATRIX2_D: {
        JSPointData<float> center;
        double angle, scale;
        js_point_read(ctx, argv[0], &center);
        JS_ToFloat64(ctx, &angle, argv[1]);
        JS_ToFloat64(ctx, &scale, argv[2]);

        JSMatData mat = cv::getRotationMatrix2D(center, angle, scale);
        ret = js_mat_wrap(ctx, mat);
        break;
      }

      case TRANSFORM_GET_ROTATION_MATRIX2D_: {
        break;
      }

      case TRANSFORM_INVERT_AFFINE_TRANSFORM: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);

        cv::invertAffineTransform(src, dst);
        break;
      }

      case TRANSFORM_LINEAR_POLAR: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        JSPointData<float> center;
        double maxRadius;
        int32_t flags;

        js_point_read(ctx, argv[2], &center);
        JS_ToFloat64(ctx, &maxRadius, argv[3]);
        JS_ToInt32(ctx, &flags, argv[4]);

        cv::linearPolar(src, dst, center, maxRadius, flags);
        break;
      }

      case TRANSFORM_LOG_POLAR: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        JSPointData<float> center;
        double M;
        int32_t flags;

        js_point_read(ctx, argv[2], &center);
        JS_ToFloat64(ctx, &M, argv[3]);
        JS_ToInt32(ctx, &flags, argv[4]);

        cv::logPolar(src, dst, center, M, flags);
        break;
      }

      case TRANSFORM_REMAP: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        JSInputArray map1 = js_umat_or_mat(ctx, argv[2]);
        JSInputArray map2 = js_umat_or_mat(ctx, argv[3]);
        int32_t interpolation, borderMode = cv::BORDER_CONSTANT;
        JSColorData<double> borderValue;

        JS_ToInt32(ctx, &interpolation, argv[4]);

        if(argc > 5)
          JS_ToInt32(ctx, &borderMode, argv[5]);

        if(argc > 6)
          js_color_read(ctx, argv[6], &borderValue);

        cv::remap(src, dst, map1, map2, interpolation, borderMode, cv::Scalar(borderValue));
        break;
      }

      case TRANSFORM_RESIZE: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        JSSizeData<int> dsize;
        double fx = 0, fy = 0;
        int32_t interpolation = cv::INTER_LINEAR;

        js_size_read(ctx, argv[2], &dsize);

        if(argc > 3)
          JS_ToFloat64(ctx, &fx, argv[3]);

        if(argc > 4)
          JS_ToFloat64(ctx, &fy, argv[4]);

        /* if(!js_size_read(ctx, argv[2], &dsize) && fx > 0 && fy > 0) {
           dsize.width = fx * src->cols;
           dsize.height = fy * src->rows;
         }*/

        if(argc > 5)
          JS_ToInt32(ctx, &interpolation, argv[5]);

        cv::resize(src, dst, dsize, fx, fy, interpolation);
        break;
      }

      case TRANSFORM_WARP_AFFINE: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        JSInputArray M = js_umat_or_mat(ctx, argv[2]);
        JSSizeData<int> dsize;
        int32_t flags = cv::INTER_LINEAR, borderMode = cv::BORDER_CONSTANT;
        JSColorData<double> borderValue;

        js_size_read(ctx, argv[3], &dsize);
        if(argc > 4)
          JS_ToInt32(ctx, &flags, argv[4]);
        if(argc > 5)
          JS_ToInt32(ctx, &borderMode, argv[5]);
        if(argc > 6)
          js_color_read(ctx, argv[6], &borderValue);

        cv::warpAffine(src, dst, M, dsize, flags, borderMode, cv::Scalar(borderValue));
        break;
      }

      case TRANSFORM_WARP_PERSPECTIVE: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        JSInputArray M = js_umat_or_mat(ctx, argv[2]);
        JSSizeData<int> dsize;
        int32_t flags = cv::INTER_LINEAR, borderMode = cv::BORDER_CONSTANT;
        JSColorData<double> borderValue;

        js_size_read(ctx, argv[3], &dsize);
        if(argc > 4)
          JS_ToInt32(ctx, &flags, argv[4]);
        if(argc > 5)
          JS_ToInt32(ctx, &borderMode, argv[5]);
        if(argc > 6)
          js_color_read(ctx, argv[6], &borderValue);

        cv::warpPerspective(src, dst, M, dsize, flags, borderMode, cv::Scalar(borderValue));
        break;
      }

      case TRANSFORM_WARP_POLAR: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        JSSizeData<int> dsize;
        JSPointData<float> center;
        double maxRadius;
        int32_t flags;

        js_size_read(ctx, argv[2], &dsize);
        js_point_read(ctx, argv[3], &center);
        JS_ToFloat64(ctx, &maxRadius, argv[4]);
        JS_ToInt32(ctx, &flags, argv[5]);

        cv::warpPolar(src, dst, dsize, center, maxRadius, flags);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

enum {
  FILTER_BOX_FILTER = 0,
  FILTER_BUILD_PYRAMID,
  FILTER_FILTER2_D,
  FILTER_GAUSSIAN_BLUR,
  FILTER_GET_DERIV_KERNELS,
  FILTER_GET_GABOR_KERNEL,
  FILTER_GET_GAUSSIAN_KERNEL,
  FILTER_GET_STRUCTURING_ELEMENT,
  FILTER_LAPLACIAN,
  FILTER_MEDIAN_BLUR,
  FILTER_MORPHOLOGY_DEFAULT_BORDER_VALUE,
  FILTER_MORPHOLOGY_EX,
  FILTER_PYR_DOWN,
  FILTER_PYR_MEAN_SHIFT_FILTERING,
  FILTER_PYR_UP,
  FILTER_SCHARR,
  FILTER_SEP_FILTER2_D,
  FILTER_SOBEL,
  FILTER_SPATIAL_GRADIENT,
  FILTER_SQR_BOX_FILTER,
  FILTER_STACK_BLUR,
};

static JSValue
js_imgproc_filter(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSInputOutputArray src;
  JSValue ret = JS_UNDEFINED;

  if(magic != FILTER_GET_GABOR_KERNEL && magic != FILTER_GET_GAUSSIAN_KERNEL && magic != FILTER_GET_STRUCTURING_ELEMENT)
    src = argc >= 1 ? js_umat_or_mat(ctx, argv[0]) : cv::noArray();

  try {
    switch(magic) {

      case FILTER_BOX_FILTER: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        int32_t ddepth;
        JSSizeData<int> ksize;
        JSPointData<int> anchor{-1, -1};
        BOOL normalize = TRUE;
        int32_t borderType = cv::BORDER_DEFAULT;

        JS_ToInt32(ctx, &ddepth, argv[2]);
        js_size_read(ctx, argv[3], &ksize);

        if(!(argc >= 5 && js_point_read(ctx, argv[4], &anchor)))
          anchor = JSPointData<int>(-1, -1);
        if(argc > 5)
          normalize = JS_ToBool(ctx, argv[5]);
        if(argc > 6)
          JS_ToInt32(ctx, &borderType, argv[6]);
        cv::boxFilter(src, dst, ddepth, ksize, anchor, normalize, borderType);
        break;
      }

      case FILTER_BUILD_PYRAMID: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        int32_t maxlevel, borderType = cv::BORDER_DEFAULT;

        JS_ToInt32(ctx, &maxlevel, argv[2]);
        if(argc > 3)
          JS_ToInt32(ctx, &borderType, argv[3]);
        cv::buildPyramid(src, dst, maxlevel, borderType);
        break;
      }

        /*      case FILTER_DILATE: {
                JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
                JSInputArray kernel = js_umat_or_mat(ctx, argv[2]);
                JSPointData<int> anchor;
                int32_t iterations = 1, borderType = cv::BORDER_CONSTANT;
                JSColorData<double> borderValue;
                if(!(argc >= 4 && js_point_read(ctx, argv[3], &anchor)))
                  anchor = JSPointData<int>(-1, -1);
                if(argc > 4)
                  JS_ToInt32(ctx, &iterations, argv[4]);
                if(argc > 5)
                  JS_ToInt32(ctx, &borderType, argv[5]);
                if(argc > 6)
                  js_color_read(ctx, argv[6], &borderValue);

                cv::dilate(src, dst, kernel, anchor, iterations, borderType,
           cv::Scalar(borderValue)); break;
              }*/

        /*  case FILTER_ERODE: {
            JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
            JSInputArray kernel = js_umat_or_mat(ctx, argv[2]);
            JSPointData<int> anchor;
            int32_t iterations = 1, borderType = cv::BORDER_CONSTANT;
            JSColorData<double> borderValue;
            if(!(argc >= 4 && js_point_read(ctx, argv[3], &anchor)))
              anchor = JSPointData<int>(-1, -1);
            if(argc > 4)
              JS_ToInt32(ctx, &iterations, argv[4]);
            if(argc > 5)
              JS_ToInt32(ctx, &borderType, argv[5]);
            if(argc > 6)
              js_color_read(ctx, argv[6], &borderValue);

            cv::erode(src, dst, kernel, anchor, iterations, borderType,
          cv::Scalar(borderValue)); break;
          }
    */
      case FILTER_FILTER2_D: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        int32_t ddepth;
        JSInputArray kernel = js_umat_or_mat(ctx, argv[3]);
        JSPointData<int> anchor;
        double delta = 0;
        int32_t borderType = cv::BORDER_CONSTANT;
        JS_ToInt32(ctx, &ddepth, argv[2]);
        if(!(argc >= 5 && js_point_read(ctx, argv[4], &anchor)))
          anchor = JSPointData<int>(-1, -1);
        if(argc > 5)
          JS_ToFloat64(ctx, &delta, argv[5]);
        if(argc > 6)
          JS_ToInt32(ctx, &borderType, argv[6]);
        cv::filter2D(src, dst, ddepth, kernel, anchor, delta, borderType);
        break;
      }

      case FILTER_GAUSSIAN_BLUR: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        JSSizeData<int> ksize;
        double sigmaX, sigmaY = 0;
        int32_t borderType = cv::BORDER_DEFAULT;
        js_size_read(ctx, argv[2], &ksize);
        JS_ToFloat64(ctx, &sigmaX, argv[3]);
        if(argc > 4)
          JS_ToFloat64(ctx, &sigmaY, argv[4]);
        if(argc > 5)
          JS_ToInt32(ctx, &borderType, argv[5]);
        cv::GaussianBlur(src, dst, ksize, sigmaX, sigmaY, borderType);
        break;
      }

      case FILTER_GET_DERIV_KERNELS: {
        JSInputOutputArray ky = js_umat_or_mat(ctx, argv[1]);
        int32_t dx, dy, ksize, ktype = CV_32F;
        BOOL normalize = FALSE;

        JS_ToInt32(ctx, &dx, argv[2]);
        JS_ToInt32(ctx, &dy, argv[3]);

        JS_ToInt32(ctx, &ksize, argv[4]);
        if(argc > 5)
          normalize = JS_ToBool(ctx, argv[5]);

        if(argc > 6)
          JS_ToInt32(ctx, &ktype, argv[6]);
        cv::getDerivKernels(src, ky, dx, dy, ksize, normalize, ktype);
        break;
      }

      case FILTER_GET_GABOR_KERNEL: {
        JSSizeData<int> ksize;
        double sigma, theta, lambd, gamma, psi = CV_PI * 0.5;
        int32_t ktype = CV_64F;
        js_size_read(ctx, argv[0], &ksize);
        JS_ToFloat64(ctx, &sigma, argv[1]);
        JS_ToFloat64(ctx, &theta, argv[2]);
        JS_ToFloat64(ctx, &lambd, argv[3]);
        JS_ToFloat64(ctx, &gamma, argv[4]);
        if(argc > 5)
          JS_ToFloat64(ctx, &psi, argv[5]);

        if(argc > 6)
          JS_ToInt32(ctx, &ktype, argv[6]);
        ret = js_mat_wrap(ctx, cv::getGaborKernel(ksize, sigma, theta, lambd, gamma, psi, ktype));
        break;
      }

      case FILTER_GET_GAUSSIAN_KERNEL: {
        int32_t ksize, ktype = CV_64F;
        double sigma;
        JS_ToInt32(ctx, &ksize, argv[0]);
        JS_ToFloat64(ctx, &sigma, argv[1]);
        if(argc > 2)
          JS_ToInt32(ctx, &ktype, argv[2]);
        ret = js_mat_wrap(ctx, cv::getGaussianKernel(ksize, sigma, ktype));
        break;
      }

      case FILTER_GET_STRUCTURING_ELEMENT: {
        int32_t shape;
        JSSizeData<int> ksize;
        JSPointData<int> anchor;
        JS_ToInt32(ctx, &shape, argv[0]);
        js_size_read(ctx, argv[1], &ksize);
        if(!(argc >= 3 && js_point_read(ctx, argv[2], &anchor)))
          anchor = JSPointData<int>(-1, -1);
        ret = js_mat_wrap(ctx, cv::getStructuringElement(shape, ksize, anchor));

        break;
      }

      case FILTER_LAPLACIAN: {
        break;
      }

      case FILTER_MEDIAN_BLUR: {
        break;
      }

      case FILTER_MORPHOLOGY_DEFAULT_BORDER_VALUE: {
        break;
      }

      case FILTER_MORPHOLOGY_EX: {
        break;
      }

      case FILTER_PYR_DOWN: {
        break;
      }

      case FILTER_PYR_MEAN_SHIFT_FILTERING: {
        break;
      }

      case FILTER_PYR_UP: {
        break;
      }

      case FILTER_SCHARR: {
        break;
      }

      case FILTER_SEP_FILTER2_D: {
        break;
      }

      case FILTER_SOBEL: {
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);
        int32_t ddepth, dx, dy, ksize = 3, borderType = cv::BORDER_DEFAULT;

        JS_ToInt32(ctx, &ddepth, argv[2]);
        JS_ToInt32(ctx, &dx, argv[3]);
        JS_ToInt32(ctx, &dy, argv[4]);

        if(argc > 5)
          JS_ToInt32(ctx, &ksize, argv[5]);
        if(argc > 6)
          JS_ToInt32(ctx, &borderType, argv[6]);
        cv::Sobel(src, dst, ddepth, dx, dy, ksize, borderType);
        break;
      }

      case FILTER_SPATIAL_GRADIENT: {
        int32_t ksize = 3, borderType = cv::BORDER_DEFAULT;
        JSInputOutputArray dx = js_umat_or_mat(ctx, argv[1]);
        JSInputOutputArray dy = js_umat_or_mat(ctx, argv[2]);

        if(argc > 3)
          JS_ToInt32(ctx, &ksize, argv[3]);

        if(argc > 4)
          JS_ToInt32(ctx, &borderType, argv[4]);

        cv::spatialGradient(src, dx, dy, ksize, borderType);
        break;
      }

      case FILTER_SQR_BOX_FILTER: {
        int32_t ddepth = 3, borderType = cv::BORDER_DEFAULT;
        cv::Size ksize;
        cv::Point anchor(-1, -1);
        BOOL normalize = TRUE;
        JSInputOutputArray dst = js_umat_or_mat(ctx, argv[1]);

        if(argc > 2)
          JS_ToInt32(ctx, &ddepth, argv[2]);

        if(argc > 3)
          js_size_read(ctx, argv[3], &ksize);

        if(argc > 4)
          js_point_read(ctx, argv[4], &anchor);

        if(argc > 5)
          normalize = JS_ToBool(ctx, argv[5]);

        if(argc > 6)
          JS_ToInt32(ctx, &borderType, argv[6]);

        cv::sqrBoxFilter(src, dst, ddepth, ksize, anchor, normalize, borderType);
        break;
      }

      case FILTER_STACK_BLUR: {
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}
enum {
  SHAPE_APPROX_POLY_DP = 0,
  SHAPE_ARC_LENGTH,
  SHAPE_BOX_POINTS,
  SHAPE_CONNECTED_COMPONENTS,
  SHAPE_CONNECTED_COMPONENTS_WITH_STATS,
  SHAPE_CONTOUR_AREA,
  SHAPE_CONVEX_HULL,
  SHAPE_CONVEXITY_DEFECTS,

  SHAPE_FIT_ELLIPSE,
  SHAPE_FIT_ELLIPSE_AMS,
  SHAPE_FIT_ELLIPSE_DIRECT,
  SHAPE_FIT_LINE,
  SHAPE_HU_MOMENTS,
  SHAPE_INTERSECT_CONVEX_CONVEX,
  SHAPE_IS_CONTOUR_CONVEX,
  SHAPE_MATCH_SHAPES,
  SHAPE_MIN_AREA_RECT,
  SHAPE_MIN_ENCLOSING_CIRCLE,
  SHAPE_MIN_ENCLOSING_TRIANGLE,
  SHAPE_ROTATED_RECTANGLE_INTERSECTION,
};

static JSValue
js_imgproc_shape(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSInputOutputArray src;
  JSValue ret = JS_UNDEFINED;
  JSContourData<double>* contour;
  cv::Mat mat;

  if((contour = js_contour_data(argv[0]))) {
    JSContourData<float> fcontour;
    fcontour.resize(contour->size());
    std::copy(contour->begin(), contour->end(), fcontour.begin());

    mat = contour_getmat(fcontour);
    src = mat;

  } else if(magic != SHAPE_BOX_POINTS && magic != SHAPE_HU_MOMENTS)
    src = argc >= 1 ? js_umat_or_mat(ctx, argv[0]) : cv::noArray();

  try {
    switch(magic) {
      case SHAPE_APPROX_POLY_DP: {
        double epsilon;
        BOOL closed = argc > 2 ? JS_ToBool(ctx, argv[3]) : FALSE;
        JS_ToFloat64(ctx, &epsilon, argv[2]);
        JSContourData<double>*srcContour, *pContour;
        // std::cerr << "epsilon = " << epsilon << std::endl;

        if((srcContour = js_contour_data(argv[0])) && (pContour = js_contour_data(argv[1]))) {
          JSContourData<float> src, dst;

          src.resize(srcContour->size());
          std::copy(srcContour->begin(), srcContour->end(), src.begin());
          cv::approxPolyDP(src, dst, epsilon, closed);
          pContour->resize(dst.size());
          std::copy(dst.begin(), dst.end(), pContour->begin());

        } else {
          JSInputOutputArray approxCurve = js_umat_or_mat(ctx, argv[1]);

          cv::approxPolyDP(src, approxCurve, epsilon, closed);
        }
        break;
      }

      case SHAPE_ARC_LENGTH: {
        BOOL closed = JS_ToBool(ctx, argv[1]);
        double length = cv::arcLength(src, closed);

        ret = JS_NewFloat64(ctx, length);
        break;
      }

      case SHAPE_BOX_POINTS: {
        JSRotatedRectData* rr = js_rotated_rect_data2(ctx, argv[0]);
        JSContourData<double>* points = js_contour_data2(ctx, argv[1]);

        cv::boxPoints(*rr, *points);
        break;
      }

      case SHAPE_CONNECTED_COMPONENTS: {
        JSInputOutputArray labels = js_umat_or_mat(ctx, argv[1]);
        int32_t connectivity, ltype, ccltype;

        JS_ToInt32(ctx, &connectivity, argv[2]);
        JS_ToInt32(ctx, &ltype, argv[3]);
        JS_ToInt32(ctx, &ccltype, argv[4]);

        // XXX: overload
        ret = JS_NewInt32(ctx, cv::connectedComponents(src, labels, connectivity, ltype, ccltype));
        break;
      }

      case SHAPE_CONNECTED_COMPONENTS_WITH_STATS: {
        JSInputOutputArray labels = js_umat_or_mat(ctx, argv[1]);
        JSInputOutputArray stats = js_umat_or_mat(ctx, argv[2]);
        JSInputOutputArray centroids = js_umat_or_mat(ctx, argv[3]);
        int32_t connectivity, ltype, ccltype;

        JS_ToInt32(ctx, &connectivity, argv[4]);
        JS_ToInt32(ctx, &ltype, argv[5]);
        JS_ToInt32(ctx, &ccltype, argv[6]);

        // XXX: overload
        ret = JS_NewInt32(ctx, cv::connectedComponentsWithStats(src, labels, stats, centroids, connectivity, ltype, ccltype));
        break;
      }

      case SHAPE_CONTOUR_AREA: {
        BOOL oriented;
        oriented = argc > 1 ? JS_ToBool(ctx, argv[1]) : FALSE;
        cv::Mat mat = src.getMat(0);
        double area = 0;
        JSContourData<double>* contour;

        if((contour = js_contour_data(argv[0]))) {
          JSContourData<float> fcontour;
          fcontour.resize(contour->size());
          std::copy(contour->begin(), contour->end(), fcontour.begin());

          mat = contour_getmat(fcontour);
          area = cv::contourArea(mat, oriented);

        } else if(mat.rows > 0 && mat.cols > 0) {
          area = cv::contourArea(src, oriented);
        } else {
          JSInputArray in = js_input_array(ctx, argv[0]);

          area = cv::contourArea(in, oriented);
        }

        ret = JS_NewFloat64(ctx, area);
        break;
      }

      case SHAPE_CONVEX_HULL: {
        JSInputOutputArray hull = js_umat_or_mat(ctx, argv[1]);
        BOOL clockwise = FALSE, returnPoints = TRUE;

        if(argc > 2)
          clockwise = JS_ToBool(ctx, argv[2]);

        if(argc > 3)
          returnPoints = JS_ToBool(ctx, argv[3]);

        cv::convexHull(src, hull, clockwise, returnPoints);
        break;
      }

      case SHAPE_CONVEXITY_DEFECTS: {
        JSInputArray convexhull = js_umat_or_mat(ctx, argv[1]);
        JSInputOutputArray convexityDefects = js_umat_or_mat(ctx, argv[2]);

        cv::convexityDefects(src, convexhull, convexityDefects);
        break;
      }

      case SHAPE_FIT_ELLIPSE: {
        JSRotatedRectData rr = cv::fitEllipse(src);
        ret = js_rotated_rect_new(ctx, rr);
        break;
      }

      case SHAPE_FIT_ELLIPSE_AMS: {
        JSRotatedRectData rr = cv::fitEllipseAMS(src);
        ret = js_rotated_rect_new(ctx, rr);
        break;
      }

      case SHAPE_FIT_ELLIPSE_DIRECT: {
        JSRotatedRectData rr = cv::fitEllipseDirect(src);
        ret = js_rotated_rect_new(ctx, rr);
        break;
      }

      case SHAPE_FIT_LINE: {
        JSInputOutputArray line = js_umat_or_mat(ctx, argv[1]);
        int32_t dist_type;
        double param, reps, aeps;
        JS_ToInt32(ctx, &dist_type, argv[2]);
        JS_ToFloat64(ctx, &param, argv[3]);
        JS_ToFloat64(ctx, &reps, argv[4]);
        JS_ToFloat64(ctx, &aeps, argv[5]);
        cv::fitLine(src, line, dist_type, param, reps, aeps);
        break;
      }

      case SHAPE_HU_MOMENTS: {
        std::map<std::string, double> moments_map;
        std::array<double, 7> hu;
        js_object_to(ctx, argv[0], moments_map);

        cv::Moments moments(moments_map["m00"],
                            moments_map["m10"],
                            moments_map["m01"],
                            moments_map["m20"],
                            moments_map["m11"],
                            moments_map["m02"],
                            moments_map["m30"],
                            moments_map["m21"],
                            moments_map["m12"],
                            moments_map["m03"]);

        cv::HuMoments(moments, &hu[0]);

        if(JS_IsArray(ctx, argv[1])) {
          js_array_copy(ctx, argv[1], hu.begin(), hu.end());
        }

        break;
      }

      case SHAPE_INTERSECT_CONVEX_CONVEX: {
        JSInputArray p2 = js_input_array(ctx, argv[1]);
        JSInputOutputArray p12 = js_umat_or_mat(ctx, argv[2]);
        BOOL handleNested = TRUE;

        if(argc > 3)
          handleNested = JS_ToBool(ctx, argv[3]);

        ret = JS_NewFloat64(ctx, cv::intersectConvexConvex(src, p2, p12, handleNested));
        break;
      }

      case SHAPE_IS_CONTOUR_CONVEX: {
        ret = JS_NewBool(ctx, cv::isContourConvex(src));
        break;
      }

      case SHAPE_MATCH_SHAPES: {
        JSInputArray c2 = js_input_array(ctx, argv[1]);
        int32_t method = -1;
        double parameter;

        JS_ToInt32(ctx, &method, argv[2]);
        JS_ToFloat64(ctx, &parameter, argv[3]);

        ret = JS_NewFloat64(ctx, cv::matchShapes(src, c2, method, parameter));
        break;
      }

      case SHAPE_MIN_AREA_RECT: {
        ret = js_rotated_rect_new(ctx, cv::minAreaRect(src));
        break;
      }

      case SHAPE_MIN_ENCLOSING_CIRCLE: {
        JSPointData<float> center;
        float radius;
        cv::minEnclosingCircle(src, center, radius);

        if(js_is_function(ctx, argv[1])) {
          JSValue point = js_point_new(ctx, center);
          JS_Call(ctx, argv[1], JS_NULL, 1, &point);
          JS_FreeValue(ctx, point);
        }

        if(js_is_function(ctx, argv[2])) {
          JSValue r = JS_NewFloat64(ctx, radius);
          JS_Call(ctx, argv[2], JS_NULL, 1, &r);
          JS_FreeValue(ctx, r);
        }

        break;
      }

      case SHAPE_MIN_ENCLOSING_TRIANGLE: {
        JSInputOutputArray triangle = js_umat_or_mat(ctx, argv[1]);

        ret = JS_NewFloat64(ctx, cv::minEnclosingTriangle(src, triangle));
        break;
      }

      case SHAPE_ROTATED_RECTANGLE_INTERSECTION: {
        cv::RotatedRect *r1, *r2;
        JSInputOutputArray intersectingRegion = js_umat_or_mat(ctx, argv[2]);

        if(!(r1 = js_rotated_rect_data(argv[0])))
          return JS_ThrowTypeError(ctx, "argument 1 must be RotateRect");

        if(!(r2 = js_rotated_rect_data(argv[1])))
          return JS_ThrowTypeError(ctx, "argument 2 must be RotateRect");

        ret = JS_NewInt32(ctx, cv::rotatedRectangleIntersection(*r1, *r2, intersectingRegion));
        break;
      }
    }
  }

  catch(const cv::Exception& e) {
    ret = js_cv_throw(ctx, e);
  }

  return ret;
}

thread_local JSClassID js_imgproc_class_id = 0;

void
js_imgproc_finalizer(JSRuntime* rt, JSValue val) {

  // JS_FreeValueRT(rt, val);
  // JS_FreeValueRT(rt, cv_class);
}

JSClassDef js_imgproc_class = {
    .class_name = "cv",
    .finalizer = js_imgproc_finalizer,
};

js_function_list_t js_imgproc_static_funcs{
    JS_CFUNC_DEF("HoughLines", 5, js_cv_hough_lines),
    JS_CFUNC_DEF("HoughLinesP", 5, js_cv_hough_lines_p),
    JS_CFUNC_DEF("HoughCircles", 5, js_cv_hough_circles),

    /* Feature Detection */
    JS_CFUNC_DEF("Canny", 4, js_cv_canny),
    JS_CFUNC_DEF("cornerHarris", 5, js_cv_corner_harris),
    JS_CFUNC_DEF("goodFeaturesToTrack", 5, js_cv_good_features_to_track),
    // JS_CFUNC_DEF("drawContours", 4, js_cv_draw_contours),
    JS_CFUNC_DEF("lineSegmentDetector", 2, js_cv_lsd),

    /* Histograms */
    JS_CFUNC_DEF("calcHist", 8, js_cv_calc_hist),
    JS_CFUNC_DEF("equalizeHist", 2, js_cv_equalize_hist),

    /* Color Space Conversions */
    JS_CFUNC_MAGIC_DEF("cvtColor", 3, js_cv_cvt_color, CONVERT_COLOR),
    JS_CFUNC_MAGIC_DEF("cvtColorTwoPlane", 4, js_cv_cvt_color, CONVERT_COLOR_TWOPLANE),
    JS_CFUNC_MAGIC_DEF("demosaicing", 3, js_cv_cvt_color, CONVERT_DEMOSAICING),

    /*Motion Analysis and Object Tracking */
    JS_CFUNC_MAGIC_DEF("accumulate", 2, js_imgproc_motion, MOTION_ACCUMULATE),
    JS_CFUNC_MAGIC_DEF("accumulateProduct", 3, js_imgproc_motion, MOTION_ACCUMULATE_PRODUCT),
    JS_CFUNC_MAGIC_DEF("accumulateSquare", 2, js_imgproc_motion, MOTION_ACCUMULATE_SQUARE),
    JS_CFUNC_MAGIC_DEF("accumulateWeighted", 3, js_imgproc_motion, MOTION_ACCUMULATE_WEIGHTED),
    JS_CFUNC_MAGIC_DEF("createHanningWindow", 3, js_imgproc_motion, MOTION_CREATE_HANNING_WINDOW),
    JS_CFUNC_MAGIC_DEF("divSpectrums", 4, js_imgproc_motion, MOTION_DIV_SPECTRUMS),
    JS_CFUNC_MAGIC_DEF("phaseCorrelate", 2, js_imgproc_motion, MOTION_PHASE_CORRELATE),

    /* Miscellaneous Image Transformations */
    JS_CFUNC_MAGIC_DEF("adaptiveThreshold", 7, js_imgproc_misc, MISC_ADAPTIVE_THRESHOLD),
    JS_CFUNC_MAGIC_DEF("blendLinear", 5, js_imgproc_misc, MISC_BLEND_LINEAR),
    JS_CFUNC_MAGIC_DEF("distanceTransform", 5, js_imgproc_misc, MISC_DISTANCE_TRANSFORM),
    JS_CFUNC_MAGIC_DEF("floodFill", 3, js_imgproc_misc, MISC_FLOOD_FILL),
    JS_CFUNC_MAGIC_DEF("integral", 2, js_imgproc_misc, MISC_INTEGRAL),
    JS_CFUNC_DEF("threshold", 5, js_cv_threshold),

    /* Image Segmentation */
    JS_CFUNC_MAGIC_DEF("grabCut", 5, js_imgproc_misc, MISC_GRAB_CUT),
    JS_CFUNC_MAGIC_DEF("watershed", 2, js_imgproc_misc, MISC_WATERSHED),

    /* ColorMaps */
    JS_CFUNC_MAGIC_DEF("applyColorMap", 3, js_imgproc_misc, MISC_APPLY_COLORMAP),

    /* Geometric Image Transformations */
    JS_CFUNC_MAGIC_DEF("convertMaps", 5, js_imgproc_transform, TRANSFORM_CONVERT_MAPS),
    JS_CFUNC_MAGIC_DEF("getAffineTransform", 2, js_imgproc_transform, TRANSFORM_GET_AFFINE_TRANSFORM),
    JS_CFUNC_MAGIC_DEF("getPerspectiveTransform", 2, js_imgproc_transform, TRANSFORM_GET_PERSPECTIVE_TRANSFORM),
    JS_CFUNC_MAGIC_DEF("getRectSubPix", 4, js_imgproc_transform, TRANSFORM_GET_RECT_SUB_PIX),
    JS_CFUNC_MAGIC_DEF("getRotationMatrix2D", 3, js_imgproc_transform, TRANSFORM_GET_ROTATION_MATRIX2_D),
    // JS_CFUNC_MAGIC_DEF("getRotationMatrix2D_", 3, js_imgproc_transform,
    // TRANSFORM_GET_ROTATION_MATRIX2D_),
    JS_CFUNC_MAGIC_DEF("invertAffineTransform", 2, js_imgproc_transform, TRANSFORM_INVERT_AFFINE_TRANSFORM),
    JS_CFUNC_MAGIC_DEF("linearPolar", 5, js_imgproc_transform, TRANSFORM_LINEAR_POLAR),
    JS_CFUNC_MAGIC_DEF("logPolar", 5, js_imgproc_transform, TRANSFORM_LOG_POLAR),
    JS_CFUNC_MAGIC_DEF("remap", 5, js_imgproc_transform, TRANSFORM_REMAP),
    JS_CFUNC_MAGIC_DEF("resize", 3, js_imgproc_transform, TRANSFORM_RESIZE),
    JS_CFUNC_MAGIC_DEF("warpAffine", 4, js_imgproc_transform, TRANSFORM_WARP_AFFINE),
    JS_CFUNC_MAGIC_DEF("warpPerspective", 4, js_imgproc_transform, TRANSFORM_WARP_PERSPECTIVE),
    JS_CFUNC_MAGIC_DEF("warpPolar", 6, js_imgproc_transform, TRANSFORM_WARP_POLAR),

    /* Image Filtering */
    JS_CFUNC_DEF("bilateralFilter", 5, js_cv_bilateral_filter),
    JS_CFUNC_DEF("blur", 3, js_cv_blur),
    JS_CFUNC_MAGIC_DEF("boxFilter", 1, js_imgproc_filter, FILTER_BOX_FILTER),
    JS_CFUNC_MAGIC_DEF("buildPyramid", 1, js_imgproc_filter, FILTER_BUILD_PYRAMID),
    JS_CFUNC_MAGIC_DEF("dilate", 3, js_cv_morphology, 0),
    JS_CFUNC_MAGIC_DEF("erode", 3, js_cv_morphology, 1),
    JS_CFUNC_MAGIC_DEF("filter2D", 1, js_imgproc_filter, FILTER_FILTER2_D),
    JS_CFUNC_DEF("GaussianBlur", 4, js_cv_gaussian_blur),
    JS_CFUNC_MAGIC_DEF("getDerivKernels", 1, js_imgproc_filter, FILTER_GET_DERIV_KERNELS),
    JS_CFUNC_MAGIC_DEF("getGaborKernel", 1, js_imgproc_filter, FILTER_GET_GABOR_KERNEL),
    JS_CFUNC_MAGIC_DEF("getGaussianKernel", 1, js_imgproc_filter, FILTER_GET_GAUSSIAN_KERNEL),
    JS_CFUNC_MAGIC_DEF("getStructuringElement", 1, js_imgproc_filter, FILTER_GET_STRUCTURING_ELEMENT),
    JS_CFUNC_MAGIC_DEF("Laplacian", 1, js_imgproc_filter, FILTER_LAPLACIAN),
    JS_CFUNC_DEF("medianBlur", 3, js_cv_median_blur),
    JS_CFUNC_MAGIC_DEF("morphologyDefaultBorderValue", 1, js_imgproc_filter, FILTER_MORPHOLOGY_DEFAULT_BORDER_VALUE),
    JS_CFUNC_DEF("morphologyEx", 4, js_cv_morphology_ex),
    JS_CFUNC_MAGIC_DEF("pyrDown", 1, js_imgproc_filter, FILTER_PYR_DOWN),
    JS_CFUNC_MAGIC_DEF("pyrMeanShiftFiltering", 1, js_imgproc_filter, FILTER_PYR_MEAN_SHIFT_FILTERING),
    JS_CFUNC_MAGIC_DEF("pyrUp", 1, js_imgproc_filter, FILTER_PYR_UP),
    JS_CFUNC_MAGIC_DEF("Scharr", 1, js_imgproc_filter, FILTER_SCHARR),
    JS_CFUNC_MAGIC_DEF("sepFilter2D", 1, js_imgproc_filter, FILTER_SEP_FILTER2_D),
    JS_CFUNC_MAGIC_DEF("Sobel", 1, js_imgproc_filter, FILTER_SOBEL),
    JS_CFUNC_MAGIC_DEF("spatialGradient", 1, js_imgproc_filter, FILTER_SPATIAL_GRADIENT),
    JS_CFUNC_MAGIC_DEF("sqrBoxFilter", 1, js_imgproc_filter, FILTER_SQR_BOX_FILTER),
    JS_CFUNC_MAGIC_DEF("stackBlur", 3, js_imgproc_filter, FILTER_STACK_BLUR),

    /* Structural Analysis and Shape Descriptors */
    JS_CFUNC_MAGIC_DEF("approxPolyDP", 1, js_imgproc_shape, SHAPE_APPROX_POLY_DP),
    JS_CFUNC_MAGIC_DEF("arcLength", 1, js_imgproc_shape, SHAPE_ARC_LENGTH),
    JS_CFUNC_DEF("boundingRect", 1, js_cv_bounding_rect),
    JS_CFUNC_MAGIC_DEF("boxPoints", 1, js_imgproc_shape, SHAPE_BOX_POINTS),
    JS_CFUNC_MAGIC_DEF("connectedComponents", 1, js_imgproc_shape, SHAPE_CONNECTED_COMPONENTS),
    JS_CFUNC_MAGIC_DEF("connectedComponentsWithStats", 1, js_imgproc_shape, SHAPE_CONNECTED_COMPONENTS_WITH_STATS),
    JS_CFUNC_MAGIC_DEF("contourArea", 1, js_imgproc_shape, SHAPE_CONTOUR_AREA),
    JS_CFUNC_MAGIC_DEF("convexHull", 1, js_imgproc_shape, SHAPE_CONVEX_HULL),
    JS_CFUNC_MAGIC_DEF("convexityDefects", 1, js_imgproc_shape, SHAPE_CONVEXITY_DEFECTS),
    // JS_CFUNC_MAGIC_DEF("createGeneralizedHoughBallard", 1, js_imgproc_shape,
    // SHAPE_CREATE_GENERALIZED_HOUGH_BALLARD), JS_CFUNC_MAGIC_DEF("createGeneralizedHoughGuil",
    // 1, js_imgproc_shape, SHAPE_CREATE_GENERALIZED_HOUGH_GUIL),
    JS_CFUNC_DEF("findContours", 1, js_cv_find_contours),
    JS_CFUNC_MAGIC_DEF("fitEllipse", 1, js_imgproc_shape, SHAPE_FIT_ELLIPSE),
    JS_CFUNC_MAGIC_DEF("fitEllipseAMS", 1, js_imgproc_shape, SHAPE_FIT_ELLIPSE_AMS),
    JS_CFUNC_MAGIC_DEF("fitEllipseDirect", 1, js_imgproc_shape, SHAPE_FIT_ELLIPSE_DIRECT),
    JS_CFUNC_MAGIC_DEF("fitLine", 1, js_imgproc_shape, SHAPE_FIT_LINE),
    JS_CFUNC_MAGIC_DEF("HuMoments", 1, js_imgproc_shape, SHAPE_HU_MOMENTS),
    JS_CFUNC_MAGIC_DEF("intersectConvexConvex", 1, js_imgproc_shape, SHAPE_INTERSECT_CONVEX_CONVEX),
    JS_CFUNC_MAGIC_DEF("isContourConvex", 1, js_imgproc_shape, SHAPE_IS_CONTOUR_CONVEX),
    JS_CFUNC_MAGIC_DEF("matchShapes", 1, js_imgproc_shape, SHAPE_MATCH_SHAPES),
    JS_CFUNC_MAGIC_DEF("minAreaRect", 1, js_imgproc_shape, SHAPE_MIN_AREA_RECT),
    JS_CFUNC_MAGIC_DEF("minEnclosingCircle", 1, js_imgproc_shape, SHAPE_MIN_ENCLOSING_CIRCLE),
    JS_CFUNC_MAGIC_DEF("minEnclosingTriangle", 1, js_imgproc_shape, SHAPE_MIN_ENCLOSING_TRIANGLE),
    JS_CFUNC_MAGIC_DEF("moments", 1, js_imgproc_misc, MISC_MOMENTS),
    JS_CFUNC_DEF("pointPolygonTest", 2, js_cv_point_polygon_test),
    JS_CFUNC_MAGIC_DEF("rotatedRectangleIntersection", 1, js_imgproc_shape, SHAPE_ROTATED_RECTANGLE_INTERSECTION),

    /* Constants */
    JS_CV_CONSTANT(MORPH_RECT),
    JS_CV_CONSTANT(MORPH_CROSS),
    JS_CV_CONSTANT(MORPH_ELLIPSE),
    JS_CV_CONSTANT(MORPH_ERODE),
    JS_CV_CONSTANT(MORPH_DILATE),
    JS_CV_CONSTANT(MORPH_OPEN),
    JS_CV_CONSTANT(MORPH_CLOSE),
    JS_CV_CONSTANT(MORPH_GRADIENT),
    JS_CV_CONSTANT(MORPH_TOPHAT),
    JS_CV_CONSTANT(MORPH_BLACKHAT),
    JS_CV_CONSTANT(MORPH_HITMISS),

};

extern "C" int
js_imgproc_init(JSContext* ctx, JSModuleDef* m) {
  if(m)
    JS_SetModuleExportList(ctx, m, js_imgproc_static_funcs.data(), js_imgproc_static_funcs.size());

  return 0;
}

extern "C" void
js_imgproc_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_imgproc_static_funcs.data(), js_imgproc_static_funcs.size());
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_imgproc
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_imgproc_init)))
    return NULL;

  js_imgproc_export(ctx, m);
  return m;
}
