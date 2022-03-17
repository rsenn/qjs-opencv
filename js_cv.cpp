#include "js_cv.hpp"
#include "cutils.h"
#include "js_array.hpp"
#include "js_mat.hpp"
#include "js_point.hpp"
#include "js_umat.hpp"
#include "jsbindings.hpp"
#include "palette.hpp"
#include "png_write.hpp"
#include "gif_write.hpp"
#include <quickjs.h>
#include "util.hpp"
#include <exception>
#include <float.h>
#include <opencv2/core/core_c.h>
#include <opencv2/core/cvdef.h>
#include <opencv2/core/hal/interface.h>
#include <png.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <opencv2/core.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/core/version.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <string>
#include <vector>

enum { HIER_NEXT = 0, HIER_PREV, HIER_CHILD, HIER_PARENT };

#define JS_CONSTANT(name) JS_PROP_INT32_DEF(#name, name, 0)
#define JS_CV_CONSTANT(name) JS_PROP_INT32_DEF(#name, cv::name, JS_PROP_ENUMERABLE)

enum { DISPLAY_OVERLAY };

extern "C" {
JSValue cv_proto = JS_UNDEFINED, cv_class = JS_UNDEFINED;
thread_local VISIBLE JSClassID js_cv_class_id = 0;
}

static JSValue
js_cv_imdecode(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSInputOutputArray buf = js_cv_inputoutputarray(ctx, argv[0]);
  int32_t flags = 0;
  cv::Mat image, *dst = nullptr;

  if(argc >= 2)
    JS_ToInt32(ctx, &flags, argv[1]);

  if(argc >= 3)
    dst = js_mat_data2(ctx, argv[2]);

  image = dst ? cv::imdecode(buf, flags, dst) : cv::imdecode(buf, flags);

  return js_mat_wrap(ctx, image);
}

static JSValue
js_cv_imencode(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  const char* ext = JS_ToCString(ctx, argv[0]);
  JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[1]);
  JSValue ret = JS_UNDEFINED;
  int32_t transparent = -1;

  if(argc >= 3 && str_end(ext, "png") && image.isMat()) {
    double max;
    std::string data;
    std::vector<JSColorData<uint8_t>> palette;
    palette_read(ctx, argv[2], palette);
    cv::minMaxLoc(image, nullptr, &max);
    if(palette.size() < size_t(max))
      palette.resize(size_t(max));
    if(argc >= 4)
      JS_ToInt32(ctx, &transparent, argv[3]);

    // printf("png_write '%s' [%zu] (transparent: %i)\n", ext, palette.size(), transparent);
    data = png_write(image.getMatRef(), palette);
    ret = JS_NewArrayBufferCopy(ctx, (const uint8_t*)data.data(), data.size());
  } else {
    std::vector<uchar> buf;
    std::vector<int> params;
    if(argc >= 4)
      js_array_to(ctx, argv[3], params);
    cv::imencode(ext, image, buf, params);
    ret = JS_NewArrayBufferCopy(ctx, buf.data(), buf.size());
  }

  return ret;
}

static JSValue
js_cv_imread(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  const char* filename = JS_ToCString(ctx, argv[0]);
  cv::Mat mat = cv::imread(filename);
  return js_mat_wrap(ctx, mat);
}

static JSValue
js_cv_imwrite(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {

  const char* filename = JS_ToCString(ctx, argv[0]);
  JSInputOutputArray image;
  int32_t transparent = -1;

  if(argc >= 3 && str_end(filename, "gif") && js_is_array_like(ctx, argv[1])) {
    std::vector<JSColorData<uint8_t>> palette;
    std::vector<cv::Mat> mats;
    std::vector<int> delays;
    int32_t loop = 0;

    js_array_to(ctx, argv[1], mats);
    palette_read(ctx, argv[2], palette);

    if(argc >= 4 && js_is_array(ctx, argv[3])) {
      js_array_to(ctx, argv[3], delays);
    } else {
      int32_t delay = 100;
      if(argc >= 4)
        JS_ToInt32(ctx, &delay, argv[3]);
      delays.resize(mats.size());
      std::fill(delays.begin(), delays.end(), delay);
    }

    if(argc >= 5)
      JS_ToInt32(ctx, &transparent, argv[4]);
    if(argc >= 6)
      JS_ToInt32(ctx, &loop, argv[5]);

    // printf("gif_write '%s' (%zu) [%zu] (transparent: %i)\n", filename, mats.size(), palette.size(), transparent);
    gif_write(filename, mats, delays, palette, transparent, loop);
    return JS_UNDEFINED;
  }

  image = js_cv_inputoutputarray(ctx, argv[1]);

  if(image.empty())
    return JS_ThrowInternalError(ctx, "Empty image");

  if(argc >= 3 && /*image.type() == CV_8UC1 &&*/ str_end(filename, ".png") && image.isMat()) {
    double max;
    std::vector<JSColorData<uint8_t>> palette;

    palette_read(ctx, argv[2], palette);

    if(argc >= 4)
      JS_ToInt32(ctx, &transparent, argv[3]);

    cv::minMaxLoc(image, nullptr, &max);
    if(palette.size() < size_t(max))
      palette.resize(size_t(max));
    // printf("png++ write_mat '%s' [%zu] (transparent: %i)\n", filename, palette.size(), transparent);

    png_write(filename, image.getMatRef(), palette, transparent);

  } else {
    cv::imwrite(filename, image);
  }

  return JS_UNDEFINED;
}

static JSValue
js_cv_split(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  cv::Mat* src;
  std::vector<cv::Mat> dst;
  int code, dstCn = 0;
  int32_t length;

  if(!(src = js_mat_data2(ctx, argv[0])))
    return JS_ThrowInternalError(ctx, "src not an array!");

  length = js_array_length(ctx, argv[1]);

  for(int32_t i = 0; i < src->channels(); i++) { dst.push_back(cv::Mat(src->size(), src->type() & 0x7)); }

  if(dst.size() >= src->channels()) {
    cv::split(*src, dst.data());
    for(int32_t i = 0; i < src->channels(); i++) { JS_SetPropertyUint32(ctx, argv[1], i, js_mat_wrap(ctx, dst[i])); }
    return JS_UNDEFINED;
  }

  return JS_EXCEPTION;
}

static JSValue
js_cv_normalize(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {

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
js_cv_add_weighted(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSInputOutputArray a1, a2, dst;

  double alpha, beta, gamma;
  int32_t dtype = -1;

  a1 = js_umat_or_mat(ctx, argv[0]);
  a2 = js_umat_or_mat(ctx, argv[2]);

  if(js_is_noarray(a1) || js_is_noarray(a2))
    return JS_ThrowInternalError(ctx, "a1 or a2 not an array!");

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

enum { MAT_COUNTNONZERO = 0, MAT_FINDNONZERO, MAT_HCONCAT, MAT_VCONCAT };

static JSValue
js_cv_mat_functions(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
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
js_cv_merge(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  std::vector<cv::Mat> mv;
  cv::Mat* dst;

  if(js_array_to(ctx, argv[0], mv) == -1)
    return JS_EXCEPTION;

  dst = js_mat_data2(ctx, argv[1]);

  if(dst == nullptr)
    return JS_EXCEPTION;

  cv::merge(const_cast<const cv::Mat*>(mv.data()), mv.size(), *dst);

  return JS_UNDEFINED;
}

static JSValue
js_cv_mix_channels(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  std::vector<cv::Mat> srcs, dsts;
  std::vector<int> fromTo;

  cv::Mat* dst;

  if(js_array_to(ctx, argv[0], srcs) == -1)
    return JS_EXCEPTION;
  if(js_array_to(ctx, argv[1], dsts) == -1)
    return JS_EXCEPTION;

  if(js_array_to(ctx, argv[2], fromTo) == -1)
    return JS_EXCEPTION;

  cv::mixChannels(const_cast<const cv::Mat*>(srcs.data()), srcs.size(), dsts.data(), dsts.size(), fromTo.data(), fromTo.size() >> 1);

  return JS_UNDEFINED;
}

/*static JSValue
js_cv_min_max_loc(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  cv::Mat *src, *mask = nullptr;
  double minVal, maxVal;
  cv::Point minLoc, maxLoc;
  JSValue ret;

  src = js_mat_data2(ctx, argv[0]);

  if(src == nullptr)
    return JS_EXCEPTION;

  if(argc >= 2)
    if((mask = js_mat_data2(ctx, argv[1])) == nullptr)
      return JS_EXCEPTION;

  cv::minMaxLoc(*src, &minVal, &maxVal, &minLoc, &maxLoc, mask == nullptr ? cv::noArray() : *mask);

  ret = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, ret, "minVal", JS_NewFloat64(ctx, minVal));
  JS_SetPropertyStr(ctx, ret, "maxVal", JS_NewFloat64(ctx, maxVal));
  JS_SetPropertyStr(ctx,
                    ret,
                    "minLoc",
                    js_array_from(ctx, std::array<int, 2>{minLoc.x, minLoc.y})); // js_point_new(ctx, minLoc));
  JS_SetPropertyStr(ctx,
                    ret,
                    "maxLoc",
                    js_object::from_map(ctx,
                                        std::map<std::string, int>{
                                            std::pair<std::string, int>{"x", maxLoc.x},
                                            std::pair<std::string, int>{"y", maxLoc.y}})); // js_point_new(ctx, maxLoc));

  return ret;
}*/

static JSValue
js_cv_getticks(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
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
js_cv_bitwise(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {

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

  std::string s_str, o_str, d_str;

  s_str = dump(src);
  o_str = dump(other);
  d_str = dump(dst);

  // printf("js_cv_bitwise %s src=%s other=%s dst=%s\n", ((const char*[]){"cv::bitwise_and", "cv::bitwise_or", "cv::bitwise_xor", "cv::bitwise_not"})[magic],
  // s_str.c_str(), o_str.c_str(), d_str.c_str());

  switch(magic) {
    case 0: cv::bitwise_and(src, other, dst, mask); break;
    case 1: cv::bitwise_or(src, other, dst, mask); break;
    case 2: cv::bitwise_xor(src, other, dst, mask); break;
    case 3: cv::bitwise_not(src, dst, mask); break;
    default: return JS_EXCEPTION;
  }
  return JS_UNDEFINED;
}

enum { MATH_ABSDIFF = 0, MATH_ADD, MATH_COMPARE, MATH_DIVIDE, MATH_GEMM, MATH_MAX, MATH_MIN, MATH_MULTIPLY, MATH_SOLVE, MATH_SUBTRACT };

static JSValue
js_cv_math(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {

  JSOutputArray dst;
  JSInputArray src1, src2;
  JSValue ret = JS_UNDEFINED;

  src1 = argc >= 1 ? js_umat_or_mat(ctx, argv[0]) : cv::noArray();
  src2 = argc >= 2 ? js_umat_or_mat(ctx, argv[1]) : cv::noArray();
  dst = argc >= 3 ? js_umat_or_mat(ctx, argv[2]) : cv::noArray();

  switch(magic) {
    case MATH_ABSDIFF: {
      cv::absdiff(src1, src2, dst);
      break;
    }

    case MATH_ADD: {
      JSInputArray mask;
      int32_t dtype = -1;
      if(argc >= 4)
        mask = js_umat_or_mat(ctx, argv[3]);
      if(argc >= 5)
        mask = JS_ToInt32(ctx, &dtype, argv[4]);
      cv::add(src1, src2, dst, mask, dtype);
      break;
    }

    case MATH_COMPARE: {
      int32_t cmpop = 0;
      JS_ToInt32(ctx, &cmpop, argv[3]);
      cv::compare(src1, src2, dst, cmpop);
      break;
    }

    case MATH_DIVIDE: {
      double scale = 1;
      int dtype = -1;
      if(argc >= 4)
        JS_ToFloat64(ctx, &scale, argv[3]);
      if(argc >= 5)
        JS_ToInt32(ctx, &dtype, argv[4]);

      cv::divide(src1, src2, dst, scale, dtype);
      break;
    }

    case MATH_MAX: {
      cv::max(src1, src2, dst);
      break;
    }

    case MATH_MIN: {
      cv::min(src1, src2, dst);
      break;
    }

    case MATH_MULTIPLY: {
      double scale = 1;
      int dtype = -1;
      if(argc >= 4)
        JS_ToFloat64(ctx, &scale, argv[3]);
      if(argc >= 5)
        JS_ToInt32(ctx, &dtype, argv[4]);
      cv::multiply(src1, src2, dst, scale, dtype);
      break;
    }

    case MATH_SOLVE: {
      int flags = cv::DECOMP_LU;
      if(argc >= 4)
        JS_ToInt32(ctx, &flags, argv[3]);
      cv::solve(src1, src2, dst, flags);
      break;
    }

    case MATH_SUBTRACT: {
      JSInputArray mask;
      int32_t dtype = -1;
      if(argc >= 4)
        mask = js_umat_or_mat(ctx, argv[3]);
      if(argc >= 5)
        mask = JS_ToInt32(ctx, &dtype, argv[4]);
      cv::subtract(src1, src2, dst, mask, dtype);
      break;
    }
  }
  return ret;
}

enum {
  CORE_CONVERTFP16 = 0,
  CORE_CONVERTSCALEABS,
  CORE_COPYMAKEBORDER,
  CORE_COPYTO,
  CORE_DCT,
  CORE_DFT,
  CORE_EXP,
  CORE_EXTRACTCHANNEL,
  CORE_FLIP,
  CORE_IDCT,
  CORE_IDFT,
  CORE_INVERT,
  CORE_LOG,
  CORE_MULTRANSPOSED,
  CORE_PERSPECTIVETRANSFORM,
  CORE_REDUCE,
  CORE_ROTATE,
  CORE_SORT,
  CORE_SORTIDX,
  CORE_SQRT,
  CORE_TRANSFORM,
  CORE_TRANSPOSE
};

static JSValue
js_cv_core(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {

  JSOutputArray dst;
  JSInputArray src;
  JSValue ret = JS_UNDEFINED;

  if(argc >= 1)
    src = js_input_array(ctx, argv[0]);

  dst = argc >= 2 ? js_umat_or_mat(ctx, argv[1]) : cv::noArray();

  switch(magic) {
    case CORE_CONVERTFP16: {
      cv::convertFp16(src, dst);
      break;
    }
    case CORE_CONVERTSCALEABS: {
      double alpha = 1, beta = 0;
      if(argc >= 3)
        JS_ToFloat64(ctx, &alpha, argv[2]);
      if(argc >= 4)
        JS_ToFloat64(ctx, &beta, argv[3]);
      cv::convertScaleAbs(src, dst, alpha, beta);
      break;
    }
    case CORE_COPYMAKEBORDER: {
      int32_t top, bottom, left, right, borderType;
      cv::Scalar value = cv::Scalar();
      if(argc >= 3)
        JS_ToInt32(ctx, &top, argv[2]);
      if(argc >= 4)
        JS_ToInt32(ctx, &bottom, argv[3]);
      if(argc >= 5)
        JS_ToInt32(ctx, &left, argv[4]);
      if(argc >= 6)
        JS_ToInt32(ctx, &right, argv[5]);
      if(argc >= 7)
        JS_ToInt32(ctx, &borderType, argv[6]);
      if(argc >= 8)
        js_color_read(ctx, argv[7], &value);

      cv::copyMakeBorder(src, dst, top, bottom, left, right, borderType, value);

      break;
    }
    case CORE_COPYTO: {
      JSInputArray mask = cv::noArray();
      if(argc >= 3)
        mask = js_umat_or_mat(ctx, argv[2]);
      cv::copyTo(src, dst, mask);
      break;
    }
    case CORE_DCT: {
      int32_t flags = 0;
      if(argc >= 3)
        JS_ToInt32(ctx, &flags, argv[2]);
      cv::dct(src, dst, flags);
      break;
    }
    case CORE_DFT: {
      int32_t flags = 0, nonZeroRows = 0;
      if(argc >= 3)
        JS_ToInt32(ctx, &flags, argv[2]);
      if(argc >= 4)
        JS_ToInt32(ctx, &nonZeroRows, argv[3]);
      cv::dft(src, dst, flags, nonZeroRows);
      break;
    }
    case CORE_EXP: {
      cv::exp(src, dst);
      break;
    }
    case CORE_EXTRACTCHANNEL: {
      int32_t coi = 0;
      if(argc >= 3)
        JS_ToInt32(ctx, &coi, argv[2]);
      cv::extractChannel(src, dst, coi);
      break;
    }
    case CORE_FLIP: {
      int32_t flipCode = 0;
      if(argc >= 3)
        JS_ToInt32(ctx, &flipCode, argv[2]);
      cv::flip(src, dst, flipCode);
      break;
    }
    case CORE_IDCT: {
      int32_t flags = 0;
      if(argc >= 3)
        JS_ToInt32(ctx, &flags, argv[2]);
      cv::idct(src, dst, flags);
      break;
    }
    case CORE_IDFT: {
      int32_t flags = 0, nonZeroRows = 0;
      if(argc >= 3)
        JS_ToInt32(ctx, &flags, argv[2]);
      if(argc >= 4)
        JS_ToInt32(ctx, &nonZeroRows, argv[3]);
      cv::idft(src, dst, flags, nonZeroRows);
      break;
    }
    case CORE_INVERT: {
      int flags = cv::DECOMP_LU;
      if(argc >= 3)
        JS_ToInt32(ctx, &flags, argv[2]);
      cv::invert(src, dst, flags);
      break;
    }
    case CORE_LOG: {
      cv::log(src, dst);
      break;
    }
    case CORE_MULTRANSPOSED: {
      BOOL aTa;
      JSInputArray delta = cv::noArray();
      double scale = 1;
      int32_t dtype = -1;

      if(argc >= 3)
        aTa = JS_ToBool(ctx, argv[2]);
      if(argc >= 4)
        delta = js_umat_or_mat(ctx, argv[3]);
      if(argc >= 5)
        JS_ToFloat64(ctx, &scale, argv[4]);
      if(argc >= 6)
        JS_ToInt32(ctx, &dtype, argv[5]);
      cv::mulTransposed(src, dst, aTa, delta, scale, dtype);

      break;
    }
    case CORE_PERSPECTIVETRANSFORM: {
      JSInputArray m = cv::noArray();
      if(argc >= 3)
        m = js_umat_or_mat(ctx, argv[2]);

      cv::perspectiveTransform(src, dst, m);
      break;
    }
    case CORE_REDUCE: {
      int32_t dim, rtype, dtype = -1;
      if(argc >= 3)
        JS_ToInt32(ctx, &dim, argv[2]);
      if(argc >= 4)
        JS_ToInt32(ctx, &rtype, argv[3]);
      if(argc >= 5)
        JS_ToInt32(ctx, &dtype, argv[4]);
      cv::reduce(src, dst, dim, rtype, dtype);
      break;
    }
    case CORE_ROTATE: {
      int32_t rotateCode = 0;
      if(argc >= 3)
        JS_ToInt32(ctx, &rotateCode, argv[2]);
      cv::rotate(src, dst, rotateCode);
      break;
    }
    case CORE_SORT: {
      int32_t flags;
      if(argc >= 3)
        JS_ToInt32(ctx, &flags, argv[2]);
      cv::sort(src, dst, flags);
      break;
    }
    case CORE_SORTIDX: {
      int32_t flags;
      if(argc >= 3)
        JS_ToInt32(ctx, &flags, argv[2]);
      cv::sortIdx(src, dst, flags);
      break;
      break;
    }
    case CORE_SQRT: {
      cv::sqrt(src, dst);
      break;
    }
    case CORE_TRANSFORM: {
      JSInputArray m = cv::noArray();
      if(argc >= 3)
        m = js_umat_or_mat(ctx, argv[2]);

      cv::transform(src, dst, m);
      break;
    }
    case CORE_TRANSPOSE: {
      cv::transpose(src, dst);
      break;
    }
  }

  return ret;
}

enum {
  OTHER_CALC_COVAR_MATRIX = 0,
  OTHER_CART_TO_POLAR,
  OTHER_DETERMINANT,
  OTHER_EIGEN,
  OTHER_EIGEN_NON_SYMMETRIC,
  OTHER_CHECK_RANGE,
  OTHER_IN_RANGE,
  OTHER_INSERT_CHANNEL,
  OTHER_LUT,
  OTHER_MAGNITUDE,
  OTHER_MAHALANOBIS,
  OTHER_MEAN,
  OTHER_MEAN_STD_DEV,
  OTHER_MIN_MAX_IDX,
  OTHER_MIN_MAX_LOC,
  OTHER_MUL_SPECTRUMS,
  OTHER_NORM,
  OTHER_PATCH_NANS,
  OTHER_PHASE,
  OTHER_POLAR_TO_CART,
  OTHER_POW,
  OTHER_RANDN,
  OTHER_RAND_SHUFFLE,
  OTHER_RANDU,
  OTHER_REPEAT,
  OTHER_SCALE_ADD,
  OTHER_SET_IDENTITY,
  OTHER_SOLVE_CUBIC,
  OTHER_SOLVE_POLY,
  OTHER_SUM,
  OTHER_TRACE,
  OTHER_RGB,
};

static JSValue
js_cv_other(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSInputOutputArray src;
  JSValue ret = JS_UNDEFINED;

  src = argc >= 1 ? js_umat_or_mat(ctx, argv[0]) : cv::noArray();

  switch(magic) {

    case OTHER_MEAN: {
      cv::Scalar mean;
      JSInputArray mask = cv::noArray();
      if(argc >= 2)
        mask = js_umat_or_mat(ctx, argv[1]);
      mean = cv::mean(src, mask);
      ret = js_color_new(ctx, mean);
      break;
    }
    case OTHER_CALC_COVAR_MATRIX: {
      JSOutputArray covar = cv::noArray();
      JSInputOutputArray mean = cv::noArray();
      int32_t flags, ctype = CV_64F;

      if(argc >= 2)
        covar = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        mean = js_umat_or_mat(ctx, argv[2]);
      if(argc >= 4)
        JS_ToInt32(ctx, &flags, argv[3]);
      if(argc >= 5)
        JS_ToInt32(ctx, &ctype, argv[4]);
      cv::calcCovarMatrix(src, covar, mean, flags, ctype);
      break;
    }
    case OTHER_CART_TO_POLAR: {
      JSInputArray y = cv::noArray();
      JSOutputArray magnitude = cv::noArray(), angle = cv::noArray();
      BOOL angleInDegrees = FALSE;

      if(argc >= 2)
        y = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        magnitude = js_umat_or_mat(ctx, argv[2]);
      if(argc >= 4)
        angle = js_umat_or_mat(ctx, argv[3]);
      if(argc >= 5)
        angleInDegrees = JS_ToBool(ctx, argv[4]);
      cv::cartToPolar(src, y, magnitude, angle, angleInDegrees);
      break;
    }
    case OTHER_DETERMINANT: {
      double d;
      d = cv::determinant(src);
      ret = JS_NewFloat64(ctx, d);
      break;
    }
    case OTHER_EIGEN: {
      JSOutputArray eigenvalues = cv::noArray(), eigenvectors = cv::noArray();
      if(argc >= 2)
        eigenvalues = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        eigenvectors = js_umat_or_mat(ctx, argv[2]);
      ret = JS_NewBool(ctx, cv::eigen(src, eigenvalues, eigenvectors));
      break;
    }
    case OTHER_EIGEN_NON_SYMMETRIC: {
      JSOutputArray eigenvalues = cv::noArray(), eigenvectors = cv::noArray();
      if(argc >= 2)
        eigenvalues = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        eigenvectors = js_umat_or_mat(ctx, argv[2]);
      cv::eigenNonSymmetric(src, eigenvalues, eigenvectors);

      break;
    }
    case OTHER_CHECK_RANGE: {
      BOOL result, quiet = TRUE;
      JSPointData<int> position, *pos = 0;
      double minVal = -DBL_MAX, maxVal = DBL_MAX;
      if(argc >= 2)
        quiet = JS_ToBool(ctx, argv[1]);
      if(argc >= 3) {
        js_point_read(ctx, argv[2], &position);
        pos = &position;
      }
      if(argc >= 4)
        JS_ToFloat64(ctx, &minVal, argv[3]);
      if(argc >= 5)
        JS_ToFloat64(ctx, &maxVal, argv[4]);
      result = cv::checkRange(src, quiet, pos, minVal, maxVal);
      ret = JS_NewBool(ctx, result);
      break;
    }
    case OTHER_IN_RANGE: {
      JSInputArray lowerb = cv::noArray(), upperb = cv::noArray();
      JSOutputArray dst = cv::noArray();

      if(argc >= 2)
        lowerb = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        upperb = js_umat_or_mat(ctx, argv[2]);
      if(argc >= 4)
        dst = js_umat_or_mat(ctx, argv[3]);
      cv::inRange(src, lowerb, upperb, dst);
      break;
    }
    case OTHER_INSERT_CHANNEL: {
      JSInputOutputArray dst = cv::noArray();
      int32_t coi;
      if(argc >= 2)
        dst = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        JS_ToInt32(ctx, &coi, argv[2]);
      cv::insertChannel(src, dst, coi);
      break;
    }
    case OTHER_LUT: {
      JSInputArray lut = cv::noArray();
      JSOutputArray dst = cv::noArray();
      if(argc >= 2)
        lut = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        dst = js_umat_or_mat(ctx, argv[2]);
      cv::LUT(src, lut, dst);
      break;
    }
    case OTHER_MAGNITUDE: {
      JSInputArray y = cv::noArray();
      JSOutputArray magnitude = cv::noArray();
      if(argc >= 2)
        y = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        magnitude = js_umat_or_mat(ctx, argv[2]);
      cv::magnitude(src, y, magnitude);
      break;
    }
    case OTHER_MAHALANOBIS: {
      JSInputArray v2 = cv::noArray(), icovar = cv::noArray();
      if(argc >= 2)
        v2 = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        icovar = js_umat_or_mat(ctx, argv[2]);
      ret = JS_NewFloat64(ctx, cv::Mahalanobis(src, v2, icovar));
      break;
    }
    case OTHER_MEAN_STD_DEV: {
      JSOutputArray mean = cv::noArray(), stdDev = cv::noArray();
      JSInputArray mask = cv::noArray();
      if(argc >= 2)
        mean = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        stdDev = js_umat_or_mat(ctx, argv[2]);
      if(argc >= 4)
        mask = js_umat_or_mat(ctx, argv[3]);
      cv::meanStdDev(src, mean, stdDev, mask);

      break;
    }
    case OTHER_MIN_MAX_IDX: {
      JSInputArray mask = cv::noArray();
      double minVal, maxVal;
      int32_t minIdx, maxIdx;
      JSValueConst results[4];
      // std::array<JSValue, 4> results;
      if(argc >= 6)
        mask = js_umat_or_mat(ctx, argv[5]);
      cv::minMaxIdx(src, &minVal, &maxVal, &minIdx, &maxIdx, mask);
      results[0] = JS_NewFloat64(ctx, minVal);
      results[1] = JS_NewFloat64(ctx, maxVal);
      results[2] = JS_NewInt32(ctx, minIdx);
      results[3] = JS_NewInt32(ctx, maxIdx);
      for(size_t i = 0; i < 4; i++)
        if(JS_IsFunction(ctx, argv[i + 1]))
          JS_Call(ctx, argv[i + 1], JS_NULL, 1, results + i);
      ret = js_array<JSValue>::from_sequence(ctx, const_cast<JSValue*>(&results[0]), const_cast<JSValue*>(&results[4]));
      break;
    }
    case OTHER_MIN_MAX_LOC: {
      JSInputArray mask = cv::noArray();
      double minVal, maxVal;
      JSPointData<int> minLoc, maxLoc;
      std::array<JSValueConst, 4> results;
      if(argc >= 6)
        mask = js_umat_or_mat(ctx, argv[5]);
      cv::minMaxLoc(src, &minVal, &maxVal, &minLoc, &maxLoc, mask);
      results[0] = JS_NewFloat64(ctx, minVal);
      results[1] = JS_NewFloat64(ctx, maxVal);
      results[2] = js_point_new(ctx, minLoc);
      results[3] = js_point_new(ctx, maxLoc);
      for(size_t i = 0; i < 4; i++)
        if(JS_IsFunction(ctx, argv[i + 1]))
          JS_Call(ctx, argv[i + 1], JS_NULL, 1, &results[i]);
      ret = js_array<JSValue>::from_sequence(ctx, const_cast<JSValue*>(&results[0]), const_cast<JSValue*>(&results[4]));
      break;
    }
    case OTHER_MUL_SPECTRUMS: {
      JSInputArray b = cv::noArray();
      JSOutputArray c = cv::noArray();
      int32_t flags = 0;
      BOOL conjB = FALSE;

      if(argc >= 2)
        b = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        c = js_umat_or_mat(ctx, argv[2]);
      if(argc >= 4)
        JS_ToInt32(ctx, &flags, argv[3]);
      if(argc >= 5)
        conjB = JS_ToBool(ctx, argv[4]);
      cv::mulSpectrums(src, b, c, flags, conjB);
      break;
    }
    case OTHER_NORM: {
      int32_t normType = cv::NORM_L2;
      JSInputArray mask = cv::noArray();

      if(argc >= 2)
        JS_ToInt32(ctx, &normType, argv[1]);
      if(argc >= 3)
        mask = js_umat_or_mat(ctx, argv[2]);
      ret = JS_NewFloat64(ctx, cv::norm(src, normType, mask));
      break;
    }
    case OTHER_PATCH_NANS: {
      double val = 0;

      if(argc >= 2)
        JS_ToFloat64(ctx, &val, argv[1]);
      cv::patchNaNs(src, val);
      break;
    }
    case OTHER_PHASE: {
      JSInputArray y = cv::noArray();
      JSOutputArray angle = cv::noArray();
      BOOL angleInDegrees = FALSE;

      if(argc >= 2)
        y = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        angle = js_umat_or_mat(ctx, argv[2]);
      if(argc >= 4)
        angleInDegrees = JS_ToBool(ctx, argv[3]);
      cv::phase(src, y, angle, angleInDegrees);
      break;
    }
    case OTHER_POLAR_TO_CART: {
      JSInputArray angle = cv::noArray();
      JSOutputArray x = cv::noArray(), y = cv::noArray();
      BOOL angleInDegrees = FALSE;

      if(argc >= 2)
        angle = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        x = js_umat_or_mat(ctx, argv[2]);
      if(argc >= 4)
        y = js_umat_or_mat(ctx, argv[3]);
      if(argc >= 5)
        angleInDegrees = JS_ToBool(ctx, argv[4]);
      cv::polarToCart(src, angle, x, y, angleInDegrees);
      break;
    }
    case OTHER_POW: {
      double power;
      JSOutputArray dst = cv::noArray();

      if(argc >= 2)
        JS_ToFloat64(ctx, &power, argv[1]);
      if(argc >= 3)
        dst = js_umat_or_mat(ctx, argv[2]);
      cv::pow(src, power, dst);
      break;
    }
    case OTHER_RANDN: {
      JSInputArray mean = cv::noArray(), stddev = cv::noArray();
      if(argc >= 2)
        mean = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        stddev = js_umat_or_mat(ctx, argv[2]);
      cv::randn(src, mean, stddev);
      break;
    }
    case OTHER_RAND_SHUFFLE: {
      double iterFactor = 1;
      if(argc >= 2)
        JS_ToFloat64(ctx, &iterFactor, argv[1]);

      cv::randShuffle(src, iterFactor);
      break;
    }
    case OTHER_RANDU: {
      JSInputArray low = cv::noArray(), high = cv::noArray();
      if(argc >= 2)
        low = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        high = js_umat_or_mat(ctx, argv[2]);
      cv::randu(src, low, high);
      break;
    }
    case OTHER_REPEAT: {
      int32_t nx, ny;
      JSOutputArray dst = cv::noArray();

      if(argc >= 2)
        JS_ToInt32(ctx, &nx, argv[1]);
      if(argc >= 3)
        JS_ToInt32(ctx, &ny, argv[2]);
      if(argc >= 4)
        dst = js_umat_or_mat(ctx, argv[3]);
      cv::repeat(src, nx, ny, dst);
      break;
    }
    case OTHER_SCALE_ADD: {
      double alpha;
      JSInputArray src2 = cv::noArray();
      JSOutputArray dst = cv::noArray();

      if(argc >= 2)
        JS_ToFloat64(ctx, &alpha, argv[1]);
      if(argc >= 3)
        src2 = js_umat_or_mat(ctx, argv[2]);

      if(argc >= 4)
        dst = js_umat_or_mat(ctx, argv[3]);

      cv::scaleAdd(src, alpha, src2, dst);
      break;
    }
    case OTHER_SET_IDENTITY: {
      cv::Scalar s = cv::Scalar(1);

      if(argc >= 2)
        js_color_read(ctx, argv[1], &s);

      cv::setIdentity(src, s);
      break;
    }
    case OTHER_SOLVE_CUBIC: {
      JSOutputArray dst = cv::noArray();
      if(argc >= 2)
        dst = js_umat_or_mat(ctx, argv[1]);
      ret = JS_NewInt32(ctx, cv::solveCubic(src, dst));
      break;
    }
    case OTHER_SOLVE_POLY: {
      JSOutputArray dst = cv::noArray();
      int32_t maxIters = 300;

      if(argc >= 2)
        dst = js_umat_or_mat(ctx, argv[1]);
      if(argc >= 3)
        JS_ToInt32(ctx, &maxIters, argv[2]);
      ret = JS_NewFloat64(ctx, cv::solvePoly(src, dst, maxIters));
      break;
    }
    case OTHER_SUM: {
      cv::Scalar r = cv::sum(src);
      ret = js_color_new(ctx, r);
      break;
    }
    case OTHER_TRACE: {
      cv::Scalar r = cv::trace(src);
      ret = js_color_new(ctx, r);
      break;
    }
    case OTHER_RGB: {
      cv::Scalar color;
      JS_ToFloat64(ctx, &color[2], argv[0]);
      JS_ToFloat64(ctx, &color[1], argv[1]);
      JS_ToFloat64(ctx, &color[0], argv[2]);
      ret = js_color_new(ctx, color);
      break;
    }
  }
  return ret;
}

void
js_cv_finalizer(JSRuntime* rt, JSValue val) {
}

JSClassDef js_cv_class = {
    .class_name = "cv",
    .finalizer = js_cv_finalizer,
};

typedef std::vector<JSCFunctionListEntry> js_function_list_t;

js_function_list_t js_cv_static_funcs{
    JS_CFUNC_DEF("imdecode", 1, js_cv_imdecode),
    JS_CFUNC_DEF("imencode", 1, js_cv_imencode),
    JS_CFUNC_DEF("imread", 1, js_cv_imread),
    JS_CFUNC_DEF("imwrite", 2, js_cv_imwrite),
    JS_CFUNC_DEF("split", 2, js_cv_split),
    JS_CFUNC_DEF("normalize", 2, js_cv_normalize),
    JS_CFUNC_DEF("merge", 2, js_cv_merge),
    JS_CFUNC_DEF("mixChannels", 3, js_cv_mix_channels),
    JS_CFUNC_DEF("addWeighted", 6, js_cv_add_weighted),
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

    JS_PROP_STRING_DEF("CV_VERSION_STATUS", CV_VERSION_STATUS, 0),
    JS_PROP_DOUBLE_DEF("CV_PI", CV_PI, 0),
    JS_PROP_DOUBLE_DEF("CV_2PI", CV_2PI, 0),
    JS_PROP_DOUBLE_DEF("CV_LOG2", CV_LOG2, 0),

    JS_CFUNC_MAGIC_DEF("absdiff", 3, js_cv_math, MATH_ABSDIFF),
    JS_CFUNC_MAGIC_DEF("add", 3, js_cv_math, MATH_ADD),
    JS_CFUNC_MAGIC_DEF("compare", 3, js_cv_math, MATH_COMPARE),
    JS_CFUNC_MAGIC_DEF("divide", 3, js_cv_math, MATH_DIVIDE),
    JS_CFUNC_MAGIC_DEF("max", 3, js_cv_math, MATH_MAX),
    JS_CFUNC_MAGIC_DEF("min", 3, js_cv_math, MATH_MIN),
    JS_CFUNC_MAGIC_DEF("multiply", 3, js_cv_math, MATH_MULTIPLY),
    JS_CFUNC_MAGIC_DEF("solve", 3, js_cv_math, MATH_SOLVE),
    JS_CFUNC_MAGIC_DEF("subtract", 3, js_cv_math, MATH_SUBTRACT),

    JS_CFUNC_MAGIC_DEF("convertFp16", 2, js_cv_core, CORE_CONVERTFP16),
    JS_CFUNC_MAGIC_DEF("convertScaleAbs", 2, js_cv_core, CORE_CONVERTSCALEABS),
    JS_CFUNC_MAGIC_DEF("copyMakeBorder", 7, js_cv_core, CORE_COPYMAKEBORDER),
    JS_CFUNC_MAGIC_DEF("copyTo", 2, js_cv_core, CORE_COPYTO),
    JS_CFUNC_MAGIC_DEF("dct", 2, js_cv_core, CORE_DCT),
    JS_CFUNC_MAGIC_DEF("dft", 2, js_cv_core, CORE_DFT),
    JS_CFUNC_MAGIC_DEF("exp", 2, js_cv_core, CORE_EXP),
    JS_CFUNC_MAGIC_DEF("extractChannel", 3, js_cv_core, CORE_EXTRACTCHANNEL),
    JS_CFUNC_MAGIC_DEF("flip", 3, js_cv_core, CORE_FLIP),
    JS_CFUNC_MAGIC_DEF("idct", 2, js_cv_core, CORE_IDCT),
    JS_CFUNC_MAGIC_DEF("idft", 2, js_cv_core, CORE_IDFT),
    JS_CFUNC_MAGIC_DEF("invert", 2, js_cv_core, CORE_INVERT),
    JS_CFUNC_MAGIC_DEF("log", 2, js_cv_core, CORE_LOG),
    JS_CFUNC_MAGIC_DEF("mulTransposed", 3, js_cv_core, CORE_MULTRANSPOSED),
    JS_CFUNC_MAGIC_DEF("perspectiveTransform", 2, js_cv_core, CORE_PERSPECTIVETRANSFORM),
    JS_CFUNC_MAGIC_DEF("reduce", 4, js_cv_core, CORE_REDUCE),
    JS_CFUNC_MAGIC_DEF("rotate", 3, js_cv_core, CORE_ROTATE),
    JS_CFUNC_MAGIC_DEF("sort", 3, js_cv_core, CORE_SORT),
    JS_CFUNC_MAGIC_DEF("sortIdx", 3, js_cv_core, CORE_SORTIDX),
    JS_CFUNC_MAGIC_DEF("sqrt", 2, js_cv_core, CORE_SQRT),
    JS_CFUNC_MAGIC_DEF("transform", 3, js_cv_core, CORE_TRANSFORM),
    JS_CFUNC_MAGIC_DEF("transpose", 2, js_cv_core, CORE_TRANSPOSE),

    JS_CFUNC_MAGIC_DEF("calcCovarMatrix", 4, js_cv_other, OTHER_CALC_COVAR_MATRIX),
    JS_CFUNC_MAGIC_DEF("cartToPolar", 4, js_cv_other, OTHER_CART_TO_POLAR),
    JS_CFUNC_MAGIC_DEF("checkRange", 1, js_cv_other, OTHER_CHECK_RANGE),
    JS_CFUNC_MAGIC_DEF("determinant", 1, js_cv_other, OTHER_DETERMINANT),
    JS_CFUNC_MAGIC_DEF("eigen", 2, js_cv_other, OTHER_EIGEN),
    JS_CFUNC_MAGIC_DEF("eigenNonSymmetric", 3, js_cv_other, OTHER_EIGEN_NON_SYMMETRIC),
    JS_CFUNC_MAGIC_DEF("inRange", 4, js_cv_other, OTHER_IN_RANGE),
    JS_CFUNC_MAGIC_DEF("insertChannel", 3, js_cv_other, OTHER_INSERT_CHANNEL),
    JS_CFUNC_MAGIC_DEF("LUT", 3, js_cv_other, OTHER_LUT),
    JS_CFUNC_MAGIC_DEF("magnitude", 3, js_cv_other, OTHER_MAGNITUDE),
    JS_CFUNC_MAGIC_DEF("Mahalanobis", 3, js_cv_other, OTHER_MAHALANOBIS),
    JS_CFUNC_MAGIC_DEF("mean", 1, js_cv_other, OTHER_MEAN),
    JS_CFUNC_MAGIC_DEF("meanStdDev", 3, js_cv_other, OTHER_MEAN_STD_DEV),
    JS_CFUNC_MAGIC_DEF("minMaxIdx", 2, js_cv_other, OTHER_MIN_MAX_IDX),
    JS_CFUNC_MAGIC_DEF("minMaxLoc", 2, js_cv_other, OTHER_MIN_MAX_LOC),
    JS_CFUNC_MAGIC_DEF("mulSpectrums", 4, js_cv_other, OTHER_MUL_SPECTRUMS),
    JS_CFUNC_MAGIC_DEF("norm", 2, js_cv_other, OTHER_NORM),
    JS_CFUNC_MAGIC_DEF("patchNaNs", 1, js_cv_other, OTHER_PATCH_NANS),
    JS_CFUNC_MAGIC_DEF("phase", 3, js_cv_other, OTHER_PHASE),
    JS_CFUNC_MAGIC_DEF("polarToCart", 4, js_cv_other, OTHER_POLAR_TO_CART),
    JS_CFUNC_MAGIC_DEF("pow", 3, js_cv_other, OTHER_POW),
    JS_CFUNC_MAGIC_DEF("randn", 3, js_cv_other, OTHER_RANDN),
    JS_CFUNC_MAGIC_DEF("randShuffle", 1, js_cv_other, OTHER_RAND_SHUFFLE),
    JS_CFUNC_MAGIC_DEF("randu", 3, js_cv_other, OTHER_RANDU),
    JS_CFUNC_MAGIC_DEF("repeat", 4, js_cv_other, OTHER_REPEAT),
    JS_CFUNC_MAGIC_DEF("scaleAdd", 4, js_cv_other, OTHER_SCALE_ADD),
    JS_CFUNC_MAGIC_DEF("setIdentity", 1, js_cv_other, OTHER_SET_IDENTITY),
    JS_CFUNC_MAGIC_DEF("solveCubic", 2, js_cv_other, OTHER_SOLVE_CUBIC),
    JS_CFUNC_MAGIC_DEF("solvePoly", 2, js_cv_other, OTHER_SOLVE_POLY),
    JS_CFUNC_MAGIC_DEF("sum", 1, js_cv_other, OTHER_SUM),
    JS_CFUNC_MAGIC_DEF("trace", 1, js_cv_other, OTHER_TRACE),
    JS_CFUNC_MAGIC_DEF("CV_RGB", 3, js_cv_other, OTHER_RGB),
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

    JS_CV_CONSTANT(VIDEOWRITER_PROP_QUALITY),
    JS_CV_CONSTANT(VIDEOWRITER_PROP_FRAMEBYTES),
    JS_CV_CONSTANT(VIDEOWRITER_PROP_NSTRIPES),
    // JS_CV_CONSTANT(VIDEOWRITER_PROP_IS_COLOR),

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
    JS_CV_CONSTANT(IMREAD_UNCHANGED),
    JS_CV_CONSTANT(IMREAD_GRAYSCALE),
    JS_CV_CONSTANT(IMREAD_COLOR),
    JS_CV_CONSTANT(IMREAD_ANYDEPTH),
    JS_CV_CONSTANT(IMREAD_ANYCOLOR),
    JS_CV_CONSTANT(IMREAD_LOAD_GDAL),
    JS_CV_CONSTANT(IMREAD_REDUCED_GRAYSCALE_2),
    JS_CV_CONSTANT(IMREAD_REDUCED_COLOR_2),
    JS_CV_CONSTANT(IMREAD_REDUCED_GRAYSCALE_4),
    JS_CV_CONSTANT(IMREAD_REDUCED_COLOR_4),
    JS_CV_CONSTANT(IMREAD_REDUCED_GRAYSCALE_8),
    JS_CV_CONSTANT(IMREAD_REDUCED_COLOR_8),
    JS_CV_CONSTANT(IMREAD_IGNORE_ORIENTATION),

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

    JS_CV_CONSTANT(COLORMAP_AUTUMN),
    JS_CV_CONSTANT(COLORMAP_BONE),
    JS_CV_CONSTANT(COLORMAP_JET),
    JS_CV_CONSTANT(COLORMAP_WINTER),
    JS_CV_CONSTANT(COLORMAP_RAINBOW),
    JS_CV_CONSTANT(COLORMAP_OCEAN),
    JS_CV_CONSTANT(COLORMAP_SUMMER),
    JS_CV_CONSTANT(COLORMAP_SPRING),
    JS_CV_CONSTANT(COLORMAP_COOL),
    JS_CV_CONSTANT(COLORMAP_HSV),
    JS_CV_CONSTANT(COLORMAP_PINK),
    JS_CV_CONSTANT(COLORMAP_HOT),
    JS_CV_CONSTANT(COLORMAP_PARULA),
    JS_CV_CONSTANT(COLORMAP_MAGMA),
    JS_CV_CONSTANT(COLORMAP_INFERNO),
    JS_CV_CONSTANT(COLORMAP_PLASMA),
    JS_CV_CONSTANT(COLORMAP_VIRIDIS),
    JS_CV_CONSTANT(COLORMAP_CIVIDIS),
    JS_CV_CONSTANT(COLORMAP_TWILIGHT),
    JS_CV_CONSTANT(COLORMAP_TWILIGHT_SHIFTED),
    JS_CV_CONSTANT(COLORMAP_TURBO),

    JS_PROP_INT32_DEF("DEFAULT", 0, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DRAW_OVER_OUTIMG", 1, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("NOT_DRAW_SINGLE_POINTS", 2, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DRAW_RICH_KEYPOINTS", 4, JS_PROP_ENUMERABLE),

    JS_CONSTANT(ALIGN_CENTER),
    JS_CONSTANT(ALIGN_LEFT),
    JS_CONSTANT(ALIGN_RIGHT),
    JS_CONSTANT(ALIGN_HORIZONTAL),
    JS_CONSTANT(ALIGN_MIDDLE),
    JS_CONSTANT(ALIGN_TOP),
    JS_CONSTANT(ALIGN_BOTTOM),
    JS_CONSTANT(ALIGN_VERTICAL),

    // JS_CV_CONSTANT(COLORMAP_DEEPGREEN),
};

extern "C" int
js_cv_init(JSContext* ctx, JSModuleDef* m) {
  JSAtom atom;
  JSValue cvObj, g = JS_GetGlobalObject(ctx);
  if(m) {
    JS_SetModuleExportList(ctx, m, js_cv_static_funcs.data(), js_cv_static_funcs.size());
    JS_SetModuleExportList(ctx, m, js_cv_constants.data(), js_cv_constants.size());
  }

  cv_class = JS_NewObject(ctx);
  /* JS_SetPropertyFunctionList(ctx, cv_class, js_cv_static_funcs.data(), js_cv_static_funcs.size());
  JS_SetPropertyFunctionList(ctx, cv_class, js_cv_constants.data(), js_cv_constants.size());
  JS_SetModuleExport(ctx, m, "default", cv_class);*/

  atom = JS_NewAtom(ctx, "cv");

  /* if(JS_HasProperty(ctx, g, atom)) {
     cvObj = JS_GetProperty(ctx, g, atom);
   } else {
     cvObj = JS_NewObject(ctx);
 }
   JS_SetPropertyFunctionList(ctx, cvObj, js_cv_static_funcs.data(), js_cv_static_funcs.size());
   JS_SetPropertyFunctionList(ctx, cvObj, js_cv_constants.data(), js_cv_constants.size());

     if(!JS_HasProperty(ctx, g, atom)) {
       JS_SetProperty(ctx, g, atom, cvObj);
     }

   JS_SetModuleExport(ctx, m, "default", cvObj);
 */
  JS_FreeValue(ctx, g);
  //  JS_FreeValue(ctx, cvObj);
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
