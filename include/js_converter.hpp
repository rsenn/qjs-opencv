#ifndef JS_CONVERTER_HPP
#define JS_CONVERTER_HPP

#include <quickjs.h>
#include <opencv2/core.hpp>

#include "js_typed_array.hpp"

// Forward declarations for OpenCV type converters
#include "js_mat.hpp"
#include "js_point.hpp"
#include "js_rect.hpp"
#include "js_converter.hpp"

// Forward declarations for DMatch
typedef cv::DMatch JSDMatchData;
JSDMatchData* js_dmatch_data(JSValueConst val);
JSValue js_dmatch_new(JSContext* ctx, const JSDMatchData& dm);

/**
 * @brief Common template infrastructure for OpenCV.js vector container types
 *
 * This header provides a generic JSVector template class that factors out
 * common code for all 16 vector container types (MatVector, PointVector,
 * KeyPointVector, DMatchVector, RectVector, etc.).
 *
 * Each vector type needs a JSConverter specialization that handles
 * JS ↔ C++ type conversion.
 */

// Forward declarations
template<typename T> struct JSConverter;

template<typename T> class JSVector;

/**
 * @brief JSConverter specializations for primitive types
 */
template<> struct JSConverter<int> {
  static int fromJS(JSContext* ctx, JSValueConst val) {
    int result;
    JS_ToInt32(ctx, &result, val);
    return result;
  }

  static JSValue toJS(JSContext* ctx, int val) { return JS_NewInt32(ctx, val); }
};

template<> struct JSConverter<float> {
  static float fromJS(JSContext* ctx, JSValueConst val) {
    double result;
    JS_ToFloat64(ctx, &result, val);
    return static_cast<float>(result);
  }

  static JSValue toJS(JSContext* ctx, float val) { return JS_NewFloat64(ctx, val); }
};

template<> struct JSConverter<double> {
  static double fromJS(JSContext* ctx, JSValueConst val) {
    double result;
    JS_ToFloat64(ctx, &result, val);
    return result;
  }

  static JSValue toJS(JSContext* ctx, double val) { return JS_NewFloat64(ctx, val); }
};

template<> struct JSConverter<char> {
  static char fromJS(JSContext* ctx, JSValueConst val) {
    int result;
    JS_ToInt32(ctx, &result, val);
    return static_cast<char>(result);
  }

  static JSValue toJS(JSContext* ctx, char val) { return JS_NewInt32(ctx, val); }
};

template<> struct JSConverter<std::string> {
  static std::string fromJS(JSContext* ctx, JSValueConst val) {
    const char* str = JS_ToCString(ctx, val);
    std::string result(str ? str : "");

    if(str)
      JS_FreeCString(ctx, str);

    return result;
  }

  static JSValue toJS(JSContext* ctx, const std::string& val) { return JS_NewStringLen(ctx, val.c_str(), val.size()); }
};

/**
 * @brief JSConverter specialization for cv::Mat
 *
 * Mat objects are reference-counted, so we just wrap/unwrap the existing Mat.
 * When getting from vector, we return a new JS wrapper pointing to the same Mat data.
 * When setting into vector, we extract the Mat from the JS wrapper.
 */
template<> struct JSConverter<cv::Mat> {
  static cv::Mat fromJS(JSContext* ctx, JSValueConst val) {
    JSMatData* mat;

    if(!(mat = js_mat_data(val)))
      return cv::Mat();

    return *mat;
  }

  static JSValue toJS(JSContext* ctx, const cv::Mat& val) { return js_mat_wrap(ctx, val); }
};

/**
 * @brief JSConverter specialization for cv::Point
 *
 * Point objects are value types, so we copy them.
 */
template<> struct JSConverter<cv::Point> {
  static cv::Point fromJS(JSContext* ctx, JSValueConst val) {
    JSPointData<double>* point;

    if(!(point = js_point_data(val))) {
      cv::Point result(0, 0);
      js_point_read(ctx, val, &result);
      return result;
    }

    return cv::Point(static_cast<int>(point->x), static_cast<int>(point->y));
  }

  static JSValue toJS(JSContext* ctx, const cv::Point& val) { return js_point_new(ctx, point_proto, val.x, val.y); }
};

/**
 * @brief JSConverter specialization for cv::Point2f
 */
template<> struct JSConverter<cv::Point2f> {
  static cv::Point2f fromJS(JSContext* ctx, JSValueConst val) {
    JSPointData<double>* point;

    if(!(point = js_point_data(val))) {
      cv::Point2f result(0.0f, 0.0f);
      js_point_read(ctx, val, &result);
      return result;
    }

    return cv::Point2f(static_cast<float>(point->x), static_cast<float>(point->y));
  }

  static JSValue toJS(JSContext* ctx, const cv::Point2f& val) { return js_point_new(ctx, point_proto, val.x, val.y); }
};

/**
 * @brief JSConverter specialization for cv::Point3f
 */
template<> struct JSConverter<cv::Point3f> {
  static cv::Point3f fromJS(JSContext* ctx, JSValueConst val) {
    cv::Point3f result(0.0f, 0.0f, 0.0f);
    js_point3_read(ctx, val, &result);
    return result;
  }

  static JSValue toJS(JSContext* ctx, const cv::Point3f& val) {
    cv::Vec<float, 3> vec(val);
    return js_typedarray_from(ctx, &vec[0], &vec[3]);
  }
};

/**
 * @brief JSConverter specialization for cv::Rect
 */
template<> struct JSConverter<cv::Rect> {
  static cv::Rect fromJS(JSContext* ctx, JSValueConst val) {
    JSRectData<double>* rect;

    if(!(rect = js_rect_data(val))) {
      cv::Rect result(0, 0, 0, 0);
      js_rect_read(ctx, val, &result);
      return result;
    }

    return cv::Rect(static_cast<int>(rect->x), static_cast<int>(rect->y), static_cast<int>(rect->width), static_cast<int>(rect->height));
  }

  static JSValue toJS(JSContext* ctx, const cv::Rect& val) { return js_rect_new(ctx, rect_proto, val.x, val.y, val.width, val.height); }
};

/**
 * @brief JSConverter specialization for cv::DMatch
 *
 * DMatch doesn't have JS bindings yet, so we use a simple object representation.
 * TODO: Create proper DMatch bindings when needed.
 */
template<> struct JSConverter<cv::DMatch> {
  static cv::DMatch fromJS(JSContext* ctx, JSValueConst val) {
    // Try to extract from DMatch instance first
    JSDMatchData* dm = js_dmatch_data(val);
    if(dm) {
      return *dm;
    }

    // Fall back to plain object
    cv::DMatch match;

    JSValue queryIdx = JS_GetPropertyStr(ctx, val, "queryIdx");
    JSValue trainIdx = JS_GetPropertyStr(ctx, val, "trainIdx");
    JSValue imgIdx = JS_GetPropertyStr(ctx, val, "imgIdx");
    JSValue distance = JS_GetPropertyStr(ctx, val, "distance");

    if(!JS_IsUndefined(queryIdx)) {
      int idx;
      JS_ToInt32(ctx, &idx, queryIdx);
      match.queryIdx = idx;
    }
    if(!JS_IsUndefined(trainIdx)) {
      int idx;
      JS_ToInt32(ctx, &idx, trainIdx);
      match.trainIdx = idx;
    }
    if(!JS_IsUndefined(imgIdx)) {
      int idx;
      JS_ToInt32(ctx, &idx, imgIdx);
      match.imgIdx = idx;
    }
    if(!JS_IsUndefined(distance)) {
      double dist;
      JS_ToFloat64(ctx, &dist, distance);
      match.distance = static_cast<float>(dist);
    }

    JS_FreeValue(ctx, queryIdx);
    JS_FreeValue(ctx, trainIdx);
    JS_FreeValue(ctx, imgIdx);
    JS_FreeValue(ctx, distance);

    return match;
  }

  static JSValue toJS(JSContext* ctx, const cv::DMatch& val) {
    // Return a DMatch instance instead of a plain object
    return js_dmatch_new(ctx, val);
  }
};

#endif // JS_CONVERTER_HPP
