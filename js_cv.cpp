#include "jsbindings.hpp"
#include "js_size.hpp"
#include "js_point.hpp"
#include "js_rect.hpp"
#include "js_umat.hpp"
#include "js_contour.hpp"
#include "js_array.hpp"
#include "js_alloc.hpp"
#include "js_typed_array.hpp"
#include "js_cv.hpp"
//#include "js_consts.hpp"
#include "geometry.hpp"
#include "skeletonization.hpp"
#include "pixel_neighborhood.hpp"
#include "png_write.hpp"
#include "palette.hpp"
#include "util.hpp"
#include "cutils.h"

#include <array>
#include <cassert>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

enum { HIER_NEXT = 0, HIER_PREV, HIER_CHILD, HIER_PARENT };

#define JS_CONSTANT(name) JS_PROP_INT32_DEF(#name, name, 0)
#define JS_CV_CONSTANT(name) JS_PROP_INT32_DEF(#name, cv::name, 0)

enum { DISPLAY_OVERLAY };

static JSValue
js_cv_imread(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {

  const char* filename = JS_ToCString(ctx, argv[0]);

  cv::Mat mat = cv::imread(filename);

  return js_mat_wrap(ctx, mat);
}

static JSValue
js_cv_imwrite(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {

  const char* filename = JS_ToCString(ctx, argv[0]);
  JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[1]);

  if(image.empty())
    return JS_ThrowInternalError(ctx, "Empty image");

  if(argc > 2 && /*image.type() == CV_8UC1 &&*/ str_end(filename, ".png") && image.isMat()) {
    double max;

    std::vector<JSColorData<uint8_t>> palette;

    js_array_to(ctx, argv[2], palette);

    cv::minMaxLoc(image, nullptr, &max);

    if(palette.size() < size_t(max))
      palette.resize(size_t(max));

    printf("png++ write_mat '%s' [%zu]\n", filename, palette.size());

    write_mat(filename, image.getMatRef(), palette);

  } else {
    cv::imwrite(filename, image);
  }

  return JS_UNDEFINED;
}

static JSValue
js_cv_split(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {

  cv::Mat* src;
  std::vector<cv::Mat> dst;
  int code, dstCn = 0;
  int32_t length;

  src = js_mat_data(ctx, argv[0]);

  if(src == nullptr)
    return JS_ThrowInternalError(ctx, "src not an array!");

  length = js_array_length(ctx, argv[1]);

  for(int32_t i = 0; i < src->channels(); i++) { dst.push_back(cv::Mat(src->size(), src->type() & 0x7)); }

  // dst.resize(src->channels());

  if(dst.size() >= src->channels()) {

    /*  std::transform(js_begin(ctx, argv[1]), js_end(ctx, argv[1]), std::back_inserter(dst), [ctx,
      src](const JSValue& v) -> cv::Mat { cv::Mat* mat = js_mat_data(ctx, v); return mat == nullptr
      ? cv::Mat::zeros(src->rows, src->cols, src->type()) : *mat;
      });
  */

    cv::split(*src, dst.data());

    for(int32_t i = 0; i < src->channels(); i++) { JS_SetPropertyUint32(ctx, argv[1], i, js_mat_wrap(ctx, dst[i])); }

    return JS_UNDEFINED;
  }
  return JS_EXCEPTION;
}

static JSValue
js_cv_normalize(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {

  JSInputOutputArray src, dst;
  double alpha = 1, beta = 0;
  int32_t norm_type = cv::NORM_L2, dtype = -1;

  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  if(argc >= 3)
    JS_ToFloat64(ctx, &alpha, argv[2]);
  if(argc >= 4)
    JS_ToFloat64(ctx, &beta, argv[3]);
  if(argc >= 5)
    JS_ToInt32(ctx, &norm_type, argv[4]);
  if(argc >= 6)
    JS_ToInt32(ctx, &dtype, argv[5]);

  cv::normalize(src, dst, alpha, beta, norm_type, dtype);
  return JS_UNDEFINED;
}

static JSValue
js_cv_add_weighted(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray a1, a2, dst;

  double alpha, beta, gamma;
  int32_t dtype = -1;

  a1 = js_umat_or_mat(ctx, argv[0]);
  a2 = js_umat_or_mat(ctx, argv[2]);

  if(js_is_noarray(a1) || js_is_noarray(a2))
    return JS_ThrowInternalError(ctx, "a1 or a2 not an array!");

  /*
    src1 = js_mat_data(ctx, argv[0]);

    src2 = js_mat_data(ctx, argv[2]);*/
  if(argc >= 6)
    dst = js_umat_or_mat(ctx, argv[5]);

  if(js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "dst not an array!");

  if(argc >= 2)
    JS_ToFloat64(ctx, &alpha, argv[1]);
  if(argc >= 4)
    JS_ToFloat64(ctx, &beta, argv[3]);
  if(argc >= 5)
    JS_ToFloat64(ctx, &gamma, argv[4]);

  if(argc >= 7)
    JS_ToInt32(ctx, &dtype, argv[6]);

  cv::addWeighted(a1, alpha, a2, beta, gamma, dst, dtype);
  return JS_UNDEFINED;
}

static JSValue
js_cv_skeletonization(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray src, dst;
  cv::Mat output;
  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  output = skeletonization(src);
  output.copyTo(dst);

  return JS_UNDEFINED;
}

static JSValue
js_cv_pixel_neighborhood(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSMatData* src;
  JSOutputArray dst;
  cv::Mat output;
  int count;

  src = js_mat_data(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(src == nullptr || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  if(argc > 2) {
    int32_t count;

    JS_ToInt32(ctx, &count, argv[2]);
    output = magic ? pixel_neighborhood_cross_if(*src, count) : pixel_neighborhood_if(*src, count);
  } else {
    output = magic ? pixel_neighborhood_cross(*src) : pixel_neighborhood(*src);
  }

  dst.getMatRef() = output;
  //  output.copyTo();

  return JS_UNDEFINED;
}

static JSValue
js_cv_pixel_find_value(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSMatData* src;
  std::vector<JSPointData<int>> output;
  uint32_t value;

  src = js_mat_data(ctx, argv[0]);

  if(src == nullptr)
    return JS_ThrowInternalError(ctx, "src not an array!");

  JS_ToUint32(ctx, &value, argv[1]);
  output = pixel_find_value(*src, value);

  return js_array_from(ctx, output);
}

static JSValue
js_cv_palette_apply(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSMatData* src;
  JSOutputArray dst;
  std::array<uint32_t, 256> palette32;
  std::vector<cv::Vec3b> palette;

  src = js_mat_data(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(src == nullptr || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  {
    cv::Mat& output = dst.getMatRef();
    int channels = output.channels();

    std::cout << "output.channels() = " << channels << std::endl;
    /*if(js_is_typedarray(ctx, argv[2])) {
      return JS_ThrowInternalError(ctx, "typed array not handled");
    } else*/

    if(channels == 1)
      channels = 3;

    if(channels == 4) {
      std::vector<JSColorData<uint8_t>> palette;
      std::vector<cv::Vec4b> palette4b;
      std::vector<cv::Scalar> palettesc;

      js_array_to(ctx, argv[2], palette);

      for(auto& color : palette) {
        palette4b.push_back(cv::Vec4b(color.arr[0], color.arr[1], color.arr[2], color.arr[3]));
        palettesc.push_back(cv::Scalar(color.arr[0], color.arr[1], color.arr[2], color.arr[3]));
      }

      palette_apply<cv::Vec4b>(*src, dst, &palette4b[0]);

    } else if(channels == 3) {
      std::vector<JSColorData<uint8_t>> palette;
      std::vector<cv::Vec3b> palette3b;
      std::vector<cv::Scalar> palettesc;

      js_array_to(ctx, argv[2], palette);

      for(auto& color : palette) {
        palette3b.push_back(cv::Vec3b(color.arr[2], color.arr[1], color.arr[0]));
        palettesc.push_back(cv::Scalar(color.arr[2], color.arr[1], color.arr[0]));
      }

      palette_apply<cv::Vec3b>(*src, dst, &palette3b[0]);
    } else {
      return JS_ThrowInternalError(ctx, "output mat channels = %u", output.channels());
    }
  }
  return JS_UNDEFINED;
}

enum { MAT_COUNTNONZERO = 0, MAT_FINDNONZERO, MAT_HCONCAT, MAT_VCONCAT };

static JSValue
js_cv_mat_functions(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValue ret = JS_UNDEFINED;
  JSInputOutputArray mat;
  mat = js_umat_or_mat(ctx, argv[0]);

  if(js_is_noarray(mat))
    return JS_ThrowInternalError(ctx, "mat not an array!");

  switch(magic) {
    case MAT_COUNTNONZERO: {
      ret = JS_NewInt64(ctx, cv::countNonZero(mat));
      break;
    }
    case MAT_FINDNONZERO: {
      std::vector<JSPointData<int>> output;
      cv::findNonZero(mat, output);
      ret = js_array_from(ctx, output);
      break;
    }
    case MAT_HCONCAT: {
      std::vector<cv::Mat> a;
      JSInputOutputArray dst;
      if(js_is_noarray((dst = js_umat_or_mat(ctx, argv[1]))))
        return JS_ThrowInternalError(ctx, "dst not an array!");

      js_array_to(ctx, argv[0], a);
      cv::hconcat(a.data(), a.size(), dst);
      break;
    }
    case MAT_VCONCAT: {
      std::vector<cv::Mat> a;
      JSInputOutputArray dst;
      if(js_is_noarray((dst = js_umat_or_mat(ctx, argv[1]))))
        return JS_ThrowInternalError(ctx, "dst not an array!");

      js_array_to(ctx, argv[0], a);
      cv::vconcat(a.data(), a.size(), dst);
      break;
    }
  }
  return ret;
}

static JSValue
js_cv_convert_scale_abs(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {

  JSInputOutputArray src, dst;
  double alpha = 1, beta = 0;

  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  if(argc >= 3)
    JS_ToFloat64(ctx, &alpha, argv[2]);
  if(argc >= 4)
    JS_ToFloat64(ctx, &beta, argv[3]);

  cv::convertScaleAbs(src, dst, alpha, beta);
  return JS_UNDEFINED;
}

static JSValue
js_cv_morphology(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSInputOutputArray src, dst, kernel;
  JSPointData<double> anchor = cv::Point(-1, -1);
  int32_t iterations = 1, borderType = cv::BORDER_CONSTANT;
  cv::Scalar borderValue = cv::morphologyDefaultBorderValue();

  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);
  kernel = js_cv_inputoutputarray(ctx, argv[2]);

  if(js_is_noarray(src) || js_is_noarray(dst) || js_is_noarray(kernel) || argc < 3)
    return JS_ThrowInternalError(ctx, "src or dst or kernel not an array!");

  if(argc >= 4)
    if(!js_point_read(ctx, argv[3], &anchor))
      return JS_EXCEPTION;

  if(argc >= 5)
    JS_ToInt32(ctx, &iterations, argv[4]);

  if(argc >= 6)
    JS_ToInt32(ctx, &borderType, argv[5]);

  if(argc >= 7) {
    std::vector<double> value;
    if(!js_is_array(ctx, argv[6]))
      return JS_EXCEPTION;

    js_array_to(ctx, argv[6], value);
    borderValue = cv::Scalar(value[0], value[1], value[2], value[3]);
  }

  switch(magic) {
    case 0: cv::dilate(src, dst, kernel, anchor, iterations, borderType, borderValue); break;
    case 1: cv::erode(src, dst, kernel, anchor, iterations, borderType, borderValue); break;
  }

  return JS_UNDEFINED;
}

static JSValue
js_cv_merge(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  std::vector<cv::Mat> mv;
  cv::Mat* dst;

  if(js_array_to(ctx, argv[0], mv) == -1)
    return JS_EXCEPTION;

  dst = js_mat_data(ctx, argv[1]);

  if(dst == nullptr)
    return JS_EXCEPTION;

  cv::merge(const_cast<const cv::Mat*>(mv.data()), mv.size(), *dst);

  return JS_UNDEFINED;
}

static JSValue
js_cv_mix_channels(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  std::vector<cv::Mat> srcs, dsts;
  std::vector<int> fromTo;

  cv::Mat* dst;

  if(js_array_to(ctx, argv[0], srcs) == -1)
    return JS_EXCEPTION;
  if(js_array_to(ctx, argv[1], dsts) == -1)
    return JS_EXCEPTION;

  if(js_array_to(ctx, argv[2], fromTo) == -1)
    return JS_EXCEPTION;

  cv::mixChannels(
      const_cast<const cv::Mat*>(srcs.data()), srcs.size(), dsts.data(), dsts.size(), fromTo.data(), fromTo.size() >> 1);

  return JS_UNDEFINED;
}

static JSValue
js_cv_min_max_loc(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  cv::Mat *src, *mask = nullptr;
  double minVal, maxVal;
  cv::Point minLoc, maxLoc;
  JSValue ret;

  src = js_mat_data(ctx, argv[0]);

  if(src == nullptr)
    return JS_EXCEPTION;

  if(argc >= 2)
    if((mask = js_mat_data(ctx, argv[1])) == nullptr)
      return JS_EXCEPTION;

  cv::minMaxLoc(*src, &minVal, &maxVal, &minLoc, &maxLoc, mask == nullptr ? cv::noArray() : *mask);

  ret = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, ret, "minVal", JS_NewFloat64(ctx, minVal));
  JS_SetPropertyStr(ctx, ret, "maxVal", JS_NewFloat64(ctx, maxVal));
  JS_SetPropertyStr(ctx,
                    ret,
                    "minLoc",
                    js_array_from(ctx, std::array<int, 2>{minLoc.x, minLoc.y})); // js_point_wrap(ctx, minLoc));
  JS_SetPropertyStr(ctx,
                    ret,
                    "maxLoc",
                    js_object::from_map(ctx,
                                        std::map<std::string, int>{
                                            std::pair<std::string, int>{"x", maxLoc.x},
                                            std::pair<std::string, int>{"y", maxLoc.y}})); // js_point_wrap(ctx, maxLoc));

  return ret;
}

static JSValue
js_cv_getticks(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValue ret = JS_UNDEFINED;
  switch(magic) {
    case 0: ret = JS_NewInt64(ctx, cv::getTickCount()); break;
    case 1: ret = JS_NewFloat64(ctx, cv::getTickFrequency()); break;
    case 2: ret = JS_NewInt64(ctx, cv::getCPUTickCount());
    default: ret = JS_EXCEPTION;
  }
  return ret;
}

static JSValue
js_cv_bitwise(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {

  JSInputOutputArray src, dst;
  JSInputArray other, mask;

  src = js_umat_or_mat(ctx, argv[0]);
  other = js_input_array(ctx, argv[1]);
  dst = src;
  if(argc > 2)
    dst = js_umat_or_mat(ctx, argv[2]);

  mask = cv::noArray();
  if(argc > 3)

    mask = js_input_array(ctx, argv[3]);

  switch(magic) {
    case 0: cv::bitwise_and(src, other, dst, mask); break;
    case 1: cv::bitwise_or(src, other, dst, mask); break;
    case 2: cv::bitwise_xor(src, other, dst, mask); break;
    case 3: cv::bitwise_not(src, dst, mask); break;
    default: return JS_EXCEPTION;
  }
  return JS_UNDEFINED;
}

JSValue cv_proto = JS_UNDEFINED, cv_class = JS_UNDEFINED;
JSClassID js_cv_class_id = 0;

void
js_cv_finalizer(JSRuntime* rt, JSValue val) {

  // JS_FreeValueRT(rt, val);
  // JS_FreeValueRT(rt, cv_class);
}

JSClassDef js_cv_class = {.class_name = "cv", .finalizer = js_cv_finalizer};

typedef std::vector<JSCFunctionListEntry> js_function_list_t;

js_function_list_t js_cv_static_funcs{
    JS_CFUNC_DEF("imread", 1, js_cv_imread),
    JS_CFUNC_DEF("imwrite", 2, js_cv_imwrite),
    JS_CFUNC_DEF("split", 2, js_cv_split),
    JS_CFUNC_DEF("normalize", 2, js_cv_normalize),
    JS_CFUNC_DEF("convertScaleAbs", 2, js_cv_convert_scale_abs),
    JS_CFUNC_MAGIC_DEF("dilate", 3, js_cv_morphology, 0),
    JS_CFUNC_MAGIC_DEF("erode", 3, js_cv_morphology, 1),
    JS_CFUNC_DEF("merge", 2, js_cv_merge),
    JS_CFUNC_DEF("mixChannels", 3, js_cv_mix_channels),
    JS_CFUNC_DEF("minMaxLoc", 2, js_cv_min_max_loc),
    JS_CFUNC_DEF("addWeighted", 6, js_cv_add_weighted),
    JS_CFUNC_DEF("skeletonization", 1, js_cv_skeletonization),
    JS_CFUNC_MAGIC_DEF("pixelNeighborhood", 2, js_cv_pixel_neighborhood, 0),
    JS_CFUNC_MAGIC_DEF("pixelNeighborhoodCross", 2, js_cv_pixel_neighborhood, 1),
    JS_CFUNC_DEF("pixelFindValue", 2, js_cv_pixel_find_value),
    JS_CFUNC_DEF("paletteApply", 2, js_cv_palette_apply),
    JS_CFUNC_MAGIC_DEF("getTickCount", 0, js_cv_getticks, 0),
    JS_CFUNC_MAGIC_DEF("getTickFrequency", 0, js_cv_getticks, 1),
    JS_CFUNC_MAGIC_DEF("getCPUTickCount", 0, js_cv_getticks, 2),
    JS_CFUNC_MAGIC_DEF("bitwise_and", 3, js_cv_bitwise, 0),
    JS_CFUNC_MAGIC_DEF("bitwise_or", 3, js_cv_bitwise, 1),
    JS_CFUNC_MAGIC_DEF("bitwise_xor", 3, js_cv_bitwise, 2),
    JS_CFUNC_MAGIC_DEF("bitwise_not", 2, js_cv_bitwise, 3),
    JS_CFUNC_MAGIC_DEF("countNonZero", 1, js_cv_mat_functions, MAT_COUNTNONZERO),
    JS_CFUNC_MAGIC_DEF("findNonZero", 1, js_cv_mat_functions, MAT_FINDNONZERO),
    JS_CFUNC_MAGIC_DEF("hconcat", 2, js_cv_mat_functions, MAT_HCONCAT),
    JS_CFUNC_MAGIC_DEF("vconcat", 2, js_cv_mat_functions, MAT_VCONCAT),
    /*};
    const js_function_list_t js_cv_core_flags{*/
    JS_PROP_STRING_DEF("CV_VERSION_STATUS", CV_VERSION_STATUS, 0),
    JS_PROP_DOUBLE_DEF("CV_PI", CV_PI, 0),
    JS_PROP_DOUBLE_DEF("CV_2PI", CV_2PI, 0),
    JS_PROP_DOUBLE_DEF("CV_LOG2", CV_LOG2, 0),

    /*};
    const js_function_list_t js_cv_videocapture_flags{*/
    // JS_PROP_INT32_DEF("CAP_REALSENSE", cv::CAP_REALSENSE, 0),
    /*};
    const js_function_list_t js_cv_highgui_flags{*/

    // JS_PROP_INT32_DEF("HOUGH_GRADIENT_ALT", cv::HOUGH_GRADIENT_ALT, 0),

};
js_function_list_t js_cv_constants{
    JS_CONSTANT(CV_VERSION_MAJOR),
    JS_CONSTANT(CV_VERSION_MINOR),
    JS_CONSTANT(CV_VERSION_REVISION),
    JS_CONSTANT(CV_CMP_EQ),
    JS_CONSTANT(CV_CMP_GT),
    JS_CONSTANT(CV_CMP_GE),
    JS_CONSTANT(CV_CMP_LT),
    JS_CONSTANT(CV_CMP_LE),
    JS_CONSTANT(CV_CMP_NE),
    JS_CONSTANT(CV_8U),
    JS_CONSTANT(CV_8S),
    JS_CONSTANT(CV_16U),
    JS_CONSTANT(CV_16S),
    JS_CONSTANT(CV_32S),
    JS_CONSTANT(CV_32F),
    JS_CONSTANT(CV_64F),
    JS_CONSTANT(CV_8UC1),
    JS_CONSTANT(CV_8UC2),
    JS_CONSTANT(CV_8UC3),
    JS_CONSTANT(CV_8UC4),
    JS_CONSTANT(CV_8SC1),
    JS_CONSTANT(CV_8SC2),
    JS_CONSTANT(CV_8SC3),
    JS_CONSTANT(CV_8SC4),
    JS_CONSTANT(CV_16UC1),
    JS_CONSTANT(CV_16UC2),
    JS_CONSTANT(CV_16UC3),
    JS_CONSTANT(CV_16UC4),
    JS_CONSTANT(CV_16SC1),
    JS_CONSTANT(CV_16SC2),
    JS_CONSTANT(CV_16SC3),
    JS_CONSTANT(CV_16SC4),
    JS_CONSTANT(CV_32SC1),
    JS_CONSTANT(CV_32SC2),
    JS_CONSTANT(CV_32SC3),
    JS_CONSTANT(CV_32SC4),
    JS_CONSTANT(CV_32FC1),
    JS_CONSTANT(CV_32FC2),
    JS_CONSTANT(CV_32FC3),
    JS_CONSTANT(CV_32FC4),
    JS_CONSTANT(CV_64FC1),
    JS_CONSTANT(CV_64FC2),
    JS_CONSTANT(CV_64FC3),
    JS_CONSTANT(CV_64FC4),
    JS_CV_CONSTANT(NORM_HAMMING),
    JS_CV_CONSTANT(NORM_HAMMING2),
    JS_CV_CONSTANT(NORM_INF),
    JS_CV_CONSTANT(NORM_L1),
    JS_CV_CONSTANT(NORM_L2),
    JS_CV_CONSTANT(NORM_L2SQR),
    JS_CV_CONSTANT(NORM_MINMAX),
    JS_CV_CONSTANT(NORM_RELATIVE),
    JS_CV_CONSTANT(NORM_TYPE_MASK),
    JS_CV_CONSTANT(COLOR_BGR2BGRA),
    JS_CV_CONSTANT(COLOR_RGB2RGBA),
    JS_CV_CONSTANT(COLOR_BGRA2BGR),
    JS_CV_CONSTANT(COLOR_RGBA2RGB),
    JS_CV_CONSTANT(COLOR_BGR2RGBA),
    JS_CV_CONSTANT(COLOR_RGB2BGRA),
    JS_CV_CONSTANT(COLOR_RGBA2BGR),
    JS_CV_CONSTANT(COLOR_BGRA2RGB),
    JS_CV_CONSTANT(COLOR_BGR2RGB),
    JS_CV_CONSTANT(COLOR_RGB2BGR),
    JS_CV_CONSTANT(COLOR_BGRA2RGBA),
    JS_CV_CONSTANT(COLOR_RGBA2BGRA),
    JS_CV_CONSTANT(COLOR_BGR2GRAY),
    JS_CV_CONSTANT(COLOR_RGB2GRAY),
    JS_CV_CONSTANT(COLOR_GRAY2BGR),
    JS_CV_CONSTANT(COLOR_GRAY2RGB),
    JS_CV_CONSTANT(COLOR_GRAY2BGRA),
    JS_CV_CONSTANT(COLOR_GRAY2RGBA),
    JS_CV_CONSTANT(COLOR_BGRA2GRAY),
    JS_CV_CONSTANT(COLOR_RGBA2GRAY),
    JS_CV_CONSTANT(COLOR_BGR2BGR565),
    JS_CV_CONSTANT(COLOR_RGB2BGR565),
    JS_CV_CONSTANT(COLOR_BGR5652BGR),
    JS_CV_CONSTANT(COLOR_BGR5652RGB),
    JS_CV_CONSTANT(COLOR_BGRA2BGR565),
    JS_CV_CONSTANT(COLOR_RGBA2BGR565),
    JS_CV_CONSTANT(COLOR_BGR5652BGRA),
    JS_CV_CONSTANT(COLOR_BGR5652RGBA),
    JS_CV_CONSTANT(COLOR_GRAY2BGR565),
    JS_CV_CONSTANT(COLOR_BGR5652GRAY),
    JS_CV_CONSTANT(COLOR_BGR2BGR555),
    JS_CV_CONSTANT(COLOR_RGB2BGR555),
    JS_CV_CONSTANT(COLOR_BGR5552BGR),
    JS_CV_CONSTANT(COLOR_BGR5552RGB),
    JS_CV_CONSTANT(COLOR_BGRA2BGR555),
    JS_CV_CONSTANT(COLOR_RGBA2BGR555),
    JS_CV_CONSTANT(COLOR_BGR5552BGRA),
    JS_CV_CONSTANT(COLOR_BGR5552RGBA),
    JS_CV_CONSTANT(COLOR_GRAY2BGR555),
    JS_CV_CONSTANT(COLOR_BGR5552GRAY),
    JS_CV_CONSTANT(COLOR_BGR2XYZ),
    JS_CV_CONSTANT(COLOR_RGB2XYZ),
    JS_CV_CONSTANT(COLOR_XYZ2BGR),
    JS_CV_CONSTANT(COLOR_XYZ2RGB),
    JS_CV_CONSTANT(COLOR_BGR2YCrCb),
    JS_CV_CONSTANT(COLOR_RGB2YCrCb),
    JS_CV_CONSTANT(COLOR_YCrCb2BGR),
    JS_CV_CONSTANT(COLOR_YCrCb2RGB),
    JS_CV_CONSTANT(COLOR_BGR2HSV),
    JS_CV_CONSTANT(COLOR_RGB2HSV),
    JS_CV_CONSTANT(COLOR_BGR2Lab),
    JS_CV_CONSTANT(COLOR_RGB2Lab),
    JS_CV_CONSTANT(COLOR_BGR2Luv),
    JS_CV_CONSTANT(COLOR_RGB2Luv),
    JS_CV_CONSTANT(COLOR_BGR2HLS),
    JS_CV_CONSTANT(COLOR_RGB2HLS),
    JS_CV_CONSTANT(COLOR_HSV2BGR),
    JS_CV_CONSTANT(COLOR_HSV2RGB),
    JS_CV_CONSTANT(COLOR_Lab2BGR),
    JS_CV_CONSTANT(COLOR_Lab2RGB),
    JS_CV_CONSTANT(COLOR_Luv2BGR),
    JS_CV_CONSTANT(COLOR_Luv2RGB),
    JS_CV_CONSTANT(COLOR_HLS2BGR),
    JS_CV_CONSTANT(COLOR_HLS2RGB),
    JS_CV_CONSTANT(COLOR_BGR2HSV_FULL),
    JS_CV_CONSTANT(COLOR_RGB2HSV_FULL),
    JS_CV_CONSTANT(COLOR_BGR2HLS_FULL),
    JS_CV_CONSTANT(COLOR_RGB2HLS_FULL),
    JS_CV_CONSTANT(COLOR_HSV2BGR_FULL),
    JS_CV_CONSTANT(COLOR_HSV2RGB_FULL),
    JS_CV_CONSTANT(COLOR_HLS2BGR_FULL),
    JS_CV_CONSTANT(COLOR_HLS2RGB_FULL),
    JS_CV_CONSTANT(COLOR_LBGR2Lab),
    JS_CV_CONSTANT(COLOR_LRGB2Lab),
    JS_CV_CONSTANT(COLOR_LBGR2Luv),
    JS_CV_CONSTANT(COLOR_LRGB2Luv),
    JS_CV_CONSTANT(COLOR_Lab2LBGR),
    JS_CV_CONSTANT(COLOR_Lab2LRGB),
    JS_CV_CONSTANT(COLOR_Luv2LBGR),
    JS_CV_CONSTANT(COLOR_Luv2LRGB),
    JS_CV_CONSTANT(COLOR_BGR2YUV),
    JS_CV_CONSTANT(COLOR_RGB2YUV),
    JS_CV_CONSTANT(COLOR_YUV2BGR),
    JS_CV_CONSTANT(COLOR_YUV2RGB),
    JS_CV_CONSTANT(COLOR_YUV2RGB_NV12),
    JS_CV_CONSTANT(COLOR_YUV2BGR_NV12),
    JS_CV_CONSTANT(COLOR_YUV2RGB_NV21),
    JS_CV_CONSTANT(COLOR_YUV2BGR_NV21),
    JS_CV_CONSTANT(COLOR_YUV420sp2RGB),
    JS_CV_CONSTANT(COLOR_YUV420sp2BGR),
    JS_CV_CONSTANT(COLOR_YUV2RGBA_NV12),
    JS_CV_CONSTANT(COLOR_YUV2BGRA_NV12),
    JS_CV_CONSTANT(COLOR_YUV2RGBA_NV21),
    JS_CV_CONSTANT(COLOR_YUV2BGRA_NV21),
    JS_CV_CONSTANT(COLOR_YUV420sp2RGBA),
    JS_CV_CONSTANT(COLOR_YUV420sp2BGRA),
    JS_CV_CONSTANT(COLOR_YUV2RGB_YV12),
    JS_CV_CONSTANT(COLOR_YUV2BGR_YV12),
    JS_CV_CONSTANT(COLOR_YUV2RGB_IYUV),
    JS_CV_CONSTANT(COLOR_YUV2BGR_IYUV),
    JS_CV_CONSTANT(COLOR_YUV2RGB_I420),
    JS_CV_CONSTANT(COLOR_YUV2BGR_I420),
    JS_CV_CONSTANT(COLOR_YUV420p2RGB),
    JS_CV_CONSTANT(COLOR_YUV420p2BGR),
    JS_CV_CONSTANT(COLOR_YUV2RGBA_YV12),
    JS_CV_CONSTANT(COLOR_YUV2BGRA_YV12),
    JS_CV_CONSTANT(COLOR_YUV2RGBA_IYUV),
    JS_CV_CONSTANT(COLOR_YUV2BGRA_IYUV),
    JS_CV_CONSTANT(COLOR_YUV2RGBA_I420),
    JS_CV_CONSTANT(COLOR_YUV2BGRA_I420),
    JS_CV_CONSTANT(COLOR_YUV420p2RGBA),
    JS_CV_CONSTANT(COLOR_YUV420p2BGRA),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_420),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_NV21),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_NV12),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_YV12),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_IYUV),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_I420),
    JS_CV_CONSTANT(COLOR_YUV420sp2GRAY),
    JS_CV_CONSTANT(COLOR_YUV420p2GRAY),
    JS_CV_CONSTANT(COLOR_YUV2RGB_UYVY),
    JS_CV_CONSTANT(COLOR_YUV2BGR_UYVY),
    JS_CV_CONSTANT(COLOR_YUV2RGB_Y422),
    JS_CV_CONSTANT(COLOR_YUV2BGR_Y422),
    JS_CV_CONSTANT(COLOR_YUV2RGB_UYNV),
    JS_CV_CONSTANT(COLOR_YUV2BGR_UYNV),
    JS_CV_CONSTANT(COLOR_YUV2RGBA_UYVY),
    JS_CV_CONSTANT(COLOR_YUV2BGRA_UYVY),
    JS_CV_CONSTANT(COLOR_YUV2RGBA_Y422),
    JS_CV_CONSTANT(COLOR_YUV2BGRA_Y422),
    JS_CV_CONSTANT(COLOR_YUV2RGBA_UYNV),
    JS_CV_CONSTANT(COLOR_YUV2BGRA_UYNV),
    JS_CV_CONSTANT(COLOR_YUV2RGB_YUY2),
    JS_CV_CONSTANT(COLOR_YUV2BGR_YUY2),
    JS_CV_CONSTANT(COLOR_YUV2RGB_YVYU),
    JS_CV_CONSTANT(COLOR_YUV2BGR_YVYU),
    JS_CV_CONSTANT(COLOR_YUV2RGB_YUYV),
    JS_CV_CONSTANT(COLOR_YUV2BGR_YUYV),
    JS_CV_CONSTANT(COLOR_YUV2RGB_YUNV),
    JS_CV_CONSTANT(COLOR_YUV2BGR_YUNV),
    JS_CV_CONSTANT(COLOR_YUV2RGBA_YUY2),
    JS_CV_CONSTANT(COLOR_YUV2BGRA_YUY2),
    JS_CV_CONSTANT(COLOR_YUV2RGBA_YVYU),
    JS_CV_CONSTANT(COLOR_YUV2BGRA_YVYU),
    JS_CV_CONSTANT(COLOR_YUV2RGBA_YUYV),
    JS_CV_CONSTANT(COLOR_YUV2BGRA_YUYV),
    JS_CV_CONSTANT(COLOR_YUV2RGBA_YUNV),
    JS_CV_CONSTANT(COLOR_YUV2BGRA_YUNV),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_UYVY),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_YUY2),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_Y422),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_UYNV),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_YVYU),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_YUYV),
    JS_CV_CONSTANT(COLOR_YUV2GRAY_YUNV),
    JS_CV_CONSTANT(COLOR_RGBA2mRGBA),
    JS_CV_CONSTANT(COLOR_mRGBA2RGBA),
    JS_CV_CONSTANT(COLOR_RGB2YUV_I420),
    JS_CV_CONSTANT(COLOR_BGR2YUV_I420),
    JS_CV_CONSTANT(COLOR_RGB2YUV_IYUV),
    JS_CV_CONSTANT(COLOR_BGR2YUV_IYUV),
    JS_CV_CONSTANT(COLOR_RGBA2YUV_I420),
    JS_CV_CONSTANT(COLOR_BGRA2YUV_I420),
    JS_CV_CONSTANT(COLOR_RGBA2YUV_IYUV),
    JS_CV_CONSTANT(COLOR_BGRA2YUV_IYUV),
    JS_CV_CONSTANT(COLOR_RGB2YUV_YV12),
    JS_CV_CONSTANT(COLOR_BGR2YUV_YV12),
    JS_CV_CONSTANT(COLOR_RGBA2YUV_YV12),
    JS_CV_CONSTANT(COLOR_BGRA2YUV_YV12),
    JS_CV_CONSTANT(COLOR_BayerBG2BGR),
    JS_CV_CONSTANT(COLOR_BayerGB2BGR),
    JS_CV_CONSTANT(COLOR_BayerRG2BGR),
    JS_CV_CONSTANT(COLOR_BayerGR2BGR),
    JS_CV_CONSTANT(COLOR_BayerBG2RGB),
    JS_CV_CONSTANT(COLOR_BayerGB2RGB),
    JS_CV_CONSTANT(COLOR_BayerRG2RGB),
    JS_CV_CONSTANT(COLOR_BayerGR2RGB),
    JS_CV_CONSTANT(COLOR_BayerBG2GRAY),
    JS_CV_CONSTANT(COLOR_BayerGB2GRAY),
    JS_CV_CONSTANT(COLOR_BayerRG2GRAY),
    JS_CV_CONSTANT(COLOR_BayerGR2GRAY),
    JS_CV_CONSTANT(COLOR_BayerBG2BGR_VNG),
    JS_CV_CONSTANT(COLOR_BayerGB2BGR_VNG),
    JS_CV_CONSTANT(COLOR_BayerRG2BGR_VNG),
    JS_CV_CONSTANT(COLOR_BayerGR2BGR_VNG),
    JS_CV_CONSTANT(COLOR_BayerBG2RGB_VNG),
    JS_CV_CONSTANT(COLOR_BayerGB2RGB_VNG),
    JS_CV_CONSTANT(COLOR_BayerRG2RGB_VNG),
    JS_CV_CONSTANT(COLOR_BayerGR2RGB_VNG),
    JS_CV_CONSTANT(COLOR_BayerBG2BGR_EA),
    JS_CV_CONSTANT(COLOR_BayerGB2BGR_EA),
    JS_CV_CONSTANT(COLOR_BayerRG2BGR_EA),
    JS_CV_CONSTANT(COLOR_BayerGR2BGR_EA),
    JS_CV_CONSTANT(COLOR_BayerBG2RGB_EA),
    JS_CV_CONSTANT(COLOR_BayerGB2RGB_EA),
    JS_CV_CONSTANT(COLOR_BayerRG2RGB_EA),
    JS_CV_CONSTANT(COLOR_BayerGR2RGB_EA),
    JS_CV_CONSTANT(COLOR_BayerBG2BGRA),
    JS_CV_CONSTANT(COLOR_BayerGB2BGRA),
    JS_CV_CONSTANT(COLOR_BayerRG2BGRA),
    JS_CV_CONSTANT(COLOR_BayerGR2BGRA),
    JS_CV_CONSTANT(COLOR_BayerBG2RGBA),
    JS_CV_CONSTANT(COLOR_BayerGB2RGBA),
    JS_CV_CONSTANT(COLOR_BayerRG2RGBA),
    JS_CV_CONSTANT(COLOR_BayerGR2RGBA),
    JS_CV_CONSTANT(RETR_EXTERNAL),
    JS_CV_CONSTANT(RETR_LIST),
    JS_CV_CONSTANT(RETR_CCOMP),
    JS_CV_CONSTANT(RETR_TREE),
    JS_CV_CONSTANT(RETR_FLOODFILL),
    JS_CV_CONSTANT(CHAIN_APPROX_NONE),
    JS_CV_CONSTANT(CHAIN_APPROX_SIMPLE),
    JS_CV_CONSTANT(CHAIN_APPROX_TC89_L1),
    JS_CV_CONSTANT(CHAIN_APPROX_TC89_KCOS),
    JS_CV_CONSTANT(BORDER_CONSTANT),
    JS_CV_CONSTANT(BORDER_REPLICATE),
    JS_CV_CONSTANT(BORDER_REFLECT),
    JS_CV_CONSTANT(BORDER_WRAP),
    JS_CV_CONSTANT(BORDER_REFLECT_101),
    JS_CV_CONSTANT(BORDER_TRANSPARENT),
    JS_CV_CONSTANT(BORDER_REFLECT101),
    JS_CV_CONSTANT(BORDER_DEFAULT),
    JS_CV_CONSTANT(BORDER_ISOLATED),
    JS_CV_CONSTANT(THRESH_BINARY),
    JS_CV_CONSTANT(THRESH_BINARY_INV),
    JS_CV_CONSTANT(THRESH_TRUNC),
    JS_CV_CONSTANT(THRESH_TOZERO),
    JS_CV_CONSTANT(THRESH_TOZERO_INV),
    JS_CV_CONSTANT(THRESH_MASK),
    JS_CV_CONSTANT(THRESH_OTSU),
    JS_CV_CONSTANT(THRESH_TRIANGLE),
    JS_CV_CONSTANT(MORPH_RECT),
    JS_CV_CONSTANT(MORPH_CROSS),
    JS_CV_CONSTANT(MORPH_ELLIPSE),
    JS_CV_CONSTANT(CAP_ANY),
    JS_CV_CONSTANT(CAP_VFW),
    JS_CV_CONSTANT(CAP_V4L),
    JS_CV_CONSTANT(CAP_V4L2),
    JS_CV_CONSTANT(CAP_FIREWIRE),
    JS_CV_CONSTANT(CAP_FIREWARE),
    JS_CV_CONSTANT(CAP_IEEE1394),
    JS_CV_CONSTANT(CAP_DC1394),
    JS_CV_CONSTANT(CAP_CMU1394),
    JS_CV_CONSTANT(CAP_QT),
    JS_CV_CONSTANT(CAP_UNICAP),
    JS_CV_CONSTANT(CAP_DSHOW),
    JS_CV_CONSTANT(CAP_PVAPI),
    JS_CV_CONSTANT(CAP_OPENNI),
    JS_CV_CONSTANT(CAP_OPENNI_ASUS),
    JS_CV_CONSTANT(CAP_ANDROID),
    JS_CV_CONSTANT(CAP_XIAPI),
    JS_CV_CONSTANT(CAP_AVFOUNDATION),
    JS_CV_CONSTANT(CAP_GIGANETIX),
    JS_CV_CONSTANT(CAP_MSMF),
    JS_CV_CONSTANT(CAP_WINRT),
    JS_CV_CONSTANT(CAP_INTELPERC),
    JS_CV_CONSTANT(CAP_OPENNI2),
    JS_CV_CONSTANT(CAP_OPENNI2_ASUS),
    JS_CV_CONSTANT(CAP_GPHOTO2),
    JS_CV_CONSTANT(CAP_GSTREAMER),
    JS_CV_CONSTANT(CAP_FFMPEG),
    JS_CV_CONSTANT(CAP_IMAGES),
    JS_CV_CONSTANT(CAP_ARAVIS),
    JS_CV_CONSTANT(CAP_OPENCV_MJPEG),
    JS_CV_CONSTANT(CAP_INTEL_MFX),
    JS_CV_CONSTANT(CAP_XINE),
    JS_CV_CONSTANT(CAP_PROP_POS_MSEC),
    JS_CV_CONSTANT(CAP_PROP_POS_FRAMES),
    JS_CV_CONSTANT(CAP_PROP_POS_AVI_RATIO),
    JS_CV_CONSTANT(CAP_PROP_FRAME_WIDTH),
    JS_CV_CONSTANT(CAP_PROP_FRAME_HEIGHT),
    JS_CV_CONSTANT(CAP_PROP_FPS),
    JS_CV_CONSTANT(CAP_PROP_FOURCC),
    JS_CV_CONSTANT(CAP_PROP_FRAME_COUNT),
    JS_CV_CONSTANT(CAP_PROP_FORMAT),
    JS_CV_CONSTANT(CAP_PROP_MODE),
    JS_CV_CONSTANT(CAP_PROP_BRIGHTNESS),
    JS_CV_CONSTANT(CAP_PROP_CONTRAST),
    JS_CV_CONSTANT(CAP_PROP_SATURATION),
    JS_CV_CONSTANT(CAP_PROP_HUE),
    JS_CV_CONSTANT(CAP_PROP_GAIN),
    JS_CV_CONSTANT(CAP_PROP_EXPOSURE),
    JS_CV_CONSTANT(CAP_PROP_CONVERT_RGB),
    JS_CV_CONSTANT(CAP_PROP_WHITE_BALANCE_BLUE_U),
    JS_CV_CONSTANT(CAP_PROP_RECTIFICATION),
    JS_CV_CONSTANT(CAP_PROP_MONOCHROME),
    JS_CV_CONSTANT(CAP_PROP_SHARPNESS),
    JS_CV_CONSTANT(CAP_PROP_AUTO_EXPOSURE),
    JS_CV_CONSTANT(CAP_PROP_GAMMA),
    JS_CV_CONSTANT(CAP_PROP_TEMPERATURE),
    JS_CV_CONSTANT(CAP_PROP_TRIGGER),
    JS_CV_CONSTANT(CAP_PROP_TRIGGER_DELAY),
    JS_CV_CONSTANT(CAP_PROP_WHITE_BALANCE_RED_V),
    JS_CV_CONSTANT(CAP_PROP_ZOOM),
    JS_CV_CONSTANT(CAP_PROP_FOCUS),
    JS_CV_CONSTANT(CAP_PROP_GUID),
    JS_CV_CONSTANT(CAP_PROP_ISO_SPEED),
    JS_CV_CONSTANT(CAP_PROP_BACKLIGHT),
    JS_CV_CONSTANT(CAP_PROP_PAN),
    JS_CV_CONSTANT(CAP_PROP_TILT),
    JS_CV_CONSTANT(CAP_PROP_ROLL),
    JS_CV_CONSTANT(CAP_PROP_IRIS),
    JS_CV_CONSTANT(CAP_PROP_SETTINGS),
    JS_CV_CONSTANT(CAP_PROP_BUFFERSIZE),
    JS_CV_CONSTANT(CAP_PROP_AUTOFOCUS),
    JS_CV_CONSTANT(CAP_PROP_SAR_NUM),
    JS_CV_CONSTANT(CAP_PROP_SAR_DEN),
    JS_CV_CONSTANT(CAP_PROP_BACKEND),
    JS_CV_CONSTANT(CAP_PROP_CHANNEL),
    JS_CV_CONSTANT(CAP_PROP_AUTO_WB),
    JS_CV_CONSTANT(CAP_PROP_WB_TEMPERATURE),
    JS_CV_CONSTANT(CAP_PROP_CODEC_PIXEL_FORMAT),
    JS_CV_CONSTANT(FONT_HERSHEY_SIMPLEX),
    JS_CV_CONSTANT(FONT_HERSHEY_PLAIN),
    JS_CV_CONSTANT(FONT_HERSHEY_DUPLEX),
    JS_CV_CONSTANT(FONT_HERSHEY_COMPLEX),
    JS_CV_CONSTANT(FONT_HERSHEY_TRIPLEX),
    JS_CV_CONSTANT(FONT_HERSHEY_COMPLEX_SMALL),
    JS_CV_CONSTANT(FONT_HERSHEY_SCRIPT_SIMPLEX),
    JS_CV_CONSTANT(FONT_HERSHEY_SCRIPT_COMPLEX),
    JS_CV_CONSTANT(FONT_ITALIC),
    JS_CONSTANT(HIER_NEXT),
    JS_CONSTANT(HIER_PREV),
    JS_CONSTANT(HIER_CHILD),
    JS_CONSTANT(HIER_PARENT),
    JS_CV_CONSTANT(HOUGH_STANDARD),
    JS_CV_CONSTANT(HOUGH_PROBABILISTIC),
    JS_CV_CONSTANT(HOUGH_MULTI_SCALE),
    JS_CV_CONSTANT(HOUGH_GRADIENT),
    JS_CV_CONSTANT(INTER_NEAREST),
    JS_CV_CONSTANT(INTER_LINEAR),
    JS_CV_CONSTANT(INTER_CUBIC),
    JS_CV_CONSTANT(INTER_AREA),
    JS_CV_CONSTANT(INTER_LANCZOS4),
    JS_CV_CONSTANT(INTER_LINEAR_EXACT),
    JS_CV_CONSTANT(INTER_MAX),
    JS_CV_CONSTANT(CONTOURS_MATCH_I1),
    JS_CV_CONSTANT(CONTOURS_MATCH_I2),
    JS_CV_CONSTANT(CONTOURS_MATCH_I3),
    JS_CV_CONSTANT(ACCESS_READ),
    JS_CV_CONSTANT(ACCESS_WRITE),
    JS_CV_CONSTANT(ACCESS_RW),
    JS_CV_CONSTANT(ACCESS_MASK),
    JS_CV_CONSTANT(ACCESS_FAST),
    JS_CV_CONSTANT(USAGE_DEFAULT),
    JS_CV_CONSTANT(USAGE_ALLOCATE_HOST_MEMORY),
    JS_CV_CONSTANT(USAGE_ALLOCATE_DEVICE_MEMORY),
    JS_CV_CONSTANT(USAGE_ALLOCATE_SHARED_MEMORY),
    JS_CV_CONSTANT(IMWRITE_JPEG_QUALITY),
    JS_CV_CONSTANT(IMWRITE_JPEG_PROGRESSIVE),
    JS_CV_CONSTANT(IMWRITE_JPEG_OPTIMIZE),
    JS_CV_CONSTANT(IMWRITE_JPEG_RST_INTERVAL),
    JS_CV_CONSTANT(IMWRITE_JPEG_LUMA_QUALITY),
    JS_CV_CONSTANT(IMWRITE_JPEG_CHROMA_QUALITY),
    JS_CV_CONSTANT(IMWRITE_PNG_COMPRESSION),
    JS_CV_CONSTANT(IMWRITE_PNG_STRATEGY),
    JS_CV_CONSTANT(IMWRITE_PNG_BILEVEL),
    JS_CV_CONSTANT(IMWRITE_PXM_BINARY),
    JS_CV_CONSTANT(IMWRITE_EXR_TYPE),
    JS_CV_CONSTANT(IMWRITE_WEBP_QUALITY),
    JS_CV_CONSTANT(IMWRITE_PAM_TUPLETYPE),
    JS_CV_CONSTANT(IMWRITE_TIFF_RESUNIT),
    JS_CV_CONSTANT(IMWRITE_TIFF_XDPI),
    JS_CV_CONSTANT(IMWRITE_TIFF_YDPI),
    JS_CV_CONSTANT(IMWRITE_TIFF_COMPRESSION),
    JS_CV_CONSTANT(IMWRITE_JPEG2000_COMPRESSION_X1000),
    JS_CV_CONSTANT(IMWRITE_EXR_TYPE_HALF),
    JS_CV_CONSTANT(IMWRITE_EXR_TYPE_FLOAT),
    JS_CV_CONSTANT(IMWRITE_PNG_STRATEGY_DEFAULT),
    JS_CV_CONSTANT(IMWRITE_PNG_STRATEGY_FILTERED),
    JS_CV_CONSTANT(IMWRITE_PNG_STRATEGY_HUFFMAN_ONLY),
    JS_CV_CONSTANT(IMWRITE_PNG_STRATEGY_RLE),
    JS_CV_CONSTANT(IMWRITE_PNG_STRATEGY_FIXED),
    JS_CV_CONSTANT(IMWRITE_PAM_FORMAT_NULL),
    JS_CV_CONSTANT(IMWRITE_PAM_FORMAT_BLACKANDWHITE),
    JS_CV_CONSTANT(IMWRITE_PAM_FORMAT_GRAYSCALE),
    JS_CV_CONSTANT(IMWRITE_PAM_FORMAT_GRAYSCALE_ALPHA),
    JS_CV_CONSTANT(IMWRITE_PAM_FORMAT_RGB),
    JS_CV_CONSTANT(IMWRITE_PAM_FORMAT_RGB_ALPHA),
    JS_CV_CONSTANT(FILLED),
    JS_CV_CONSTANT(LINE_8),
    JS_CV_CONSTANT(LINE_AA),
    JS_CV_CONSTANT(LSD_REFINE_NONE),
    JS_CV_CONSTANT(LSD_REFINE_STD),
    JS_CV_CONSTANT(LSD_REFINE_ADV),
};
std::string
js_prop_flags(int flags) {
  std::vector<const char*> names;
  if(flags & JS_PROP_CONFIGURABLE)
    names.push_back("CONFIGURABLE");
  if(flags & JS_PROP_WRITABLE)
    names.push_back("WRITABLE");
  if(flags & JS_PROP_ENUMERABLE)
    names.push_back("ENUMERABLE");
  if(flags & JS_PROP_NORMAL)
    names.push_back("NORMAL");
  if(flags & JS_PROP_GETSET)
    names.push_back("GETSET");
  if(flags & JS_PROP_VARREF)
    names.push_back("VARREF");
  if(flags & JS_PROP_AUTOINIT)
    names.push_back("AUTOINIT");
  return join(names.cbegin(), names.cend(), "|");
}

template<class Stream>
Stream&
operator<<(Stream& s, const JSCFunctionListEntry& entry) {
  std::string name(entry.name);
  s << name << std::setw(30 - name.size()) << ' ';
  s << "type = "
    << (std::vector<const char*>{"CFUNC",
                                 "CGETSET",
                                 "CGETSET_MAGIC",
                                 "PROP_STRING",
                                 "PROP_INT32",
                                 "PROP_INT64",
                                 "PROP_DOUBLE",
                                 "PROP_UNDEFINED",
                                 "OBJECT",
                                 "ALIAS"})[entry.def_type]
    << ", ";
  switch(entry.def_type) {
    case JS_DEF_CGETSET_MAGIC: s << "magic = " << (unsigned int)entry.magic << ", "; break;
    case JS_DEF_PROP_INT32: s << "value = " << std::setw(9) << entry.u.i32 << ", "; break;
    case JS_DEF_PROP_INT64: s << "value = " << std::setw(9) << entry.u.i64 << ", "; break;
    case JS_DEF_PROP_DOUBLE: s << "value = " << std::setw(9) << entry.u.f64 << ", "; break;
    case JS_DEF_PROP_UNDEFINED:
      s << "value = " << std::setw(9) << "undefined"
        << ", ";
      break;
    case JS_DEF_PROP_STRING: s << "value = " << std::setw(9) << entry.u.str << ", "; break;
  }
  s << "flags = " << js_prop_flags(entry.prop_flags) << std::endl;
  return s;
}

template<class Stream, class Item>
Stream&
operator<<(Stream& s, const std::vector<Item>& vector) {
  size_t i = 0;
  for(auto entry : vector) {

    s << "#" << i << " ";
    s << entry;
    i++;
  }
  return s;
}

extern "C" int
js_cv_init(JSContext* ctx, JSModuleDef* m) {
  JSAtom atom;
  JSValue g = JS_GetGlobalObject(ctx);

  /* std::cerr << "js_cv_static_funcs:" << std::endl << js_cv_static_funcs;
   std::cerr << "js_cv_static_funcs.size() = " << js_cv_static_funcs.size() << std::endl;*/
  if(m) {
    JS_SetModuleExportList(ctx, m, js_cv_static_funcs.data(), js_cv_static_funcs.size());
    JS_SetModuleExportList(ctx, m, js_cv_constants.data(), js_cv_constants.size());
  }
  atom = JS_NewAtom(ctx, "cv");

  if(JS_HasProperty(ctx, g, atom)) {
    cv_class = JS_GetProperty(ctx, g, atom);
  } else {
    cv_class = JS_NewObject(ctx);
  }
  JS_SetPropertyFunctionList(ctx, cv_class, js_cv_static_funcs.data(), js_cv_static_funcs.size());
  JS_SetPropertyFunctionList(ctx, cv_class, js_cv_constants.data(), js_cv_constants.size());

  if(!JS_HasProperty(ctx, g, atom)) {
    JS_SetPropertyInternal(ctx, g, atom, cv_class, 0);
  }

  JS_SetModuleExport(ctx, m, "default", cv_class);

  JS_FreeValue(ctx, g);
  return 0;
}

extern "C" VISIBLE void
js_cv_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_cv_static_funcs.data(), js_cv_static_funcs.size());
  JS_AddModuleExportList(ctx, m, js_cv_constants.data(), js_cv_constants.size());
  JS_AddModuleExport(ctx, m, "default");
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_cv
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_cv_init);
  if(!m)
    return NULL;
  js_cv_export(ctx, m);
  return m;
}