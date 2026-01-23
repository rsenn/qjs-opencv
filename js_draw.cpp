#include "cutils.h"
#include "geometry.hpp"
#include "js_cv.hpp"
#include "js_array.hpp"
#include "js_contour.hpp"
#include "js_line.hpp"
#include "js_mat.hpp"
#include "js_point.hpp"
#include "js_rect.hpp"
#include "js_size.hpp"
#include "js_umat.hpp"
#include "js_keypoint.hpp"
#include "jsbindings.hpp"
#include <opencv2/core/cvstd.inl.hpp>
#include <opencv2/core/cvstd_wrapper.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgproc.hpp>
#include <quickjs.h>
#include "util.hpp"
#include <climits>
#include <stddef.h>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

extern "C" int js_draw_init(JSContext*, JSModuleDef*);

#ifdef HAVE_OPENCV_FREETYPE
#include <opencv2/freetype.hpp>
cv::Ptr<cv::freetype::FreeType2> freetype2 = nullptr;
std::string freetype2_face;
#endif

extern "C" {

cv::Mat* dptr = 0;

static JSValue
js_draw_circle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  JSPointData<double> point;
  int32_t x, y;
  int radius = 0;
  JSColorData<double> color;
  bool antialias = true;
  int thickness = -1;
  int line_type = cv::LINE_AA;

  if(argc > i) {
    if(!js_is_noarray((dst = js_umat_or_mat(ctx, argv[i]))))
      i++;
  } else
    dst = *dptr;

  if(js_is_noarray(dst))
    return JS_EXCEPTION;

  if(js_point_read(ctx, argv[i], &point)) {
    x = point.x;
    y = point.y;
    i++;
  } else {
    JS_ToInt32(ctx, &x, argv[i]);
    JS_ToInt32(ctx, &y, argv[i + 1]);
    i += 2;
  }

  if(argc > i)
    js_value_to(ctx, argv[i++], radius);
  if(argc > i)
    js_color_read(ctx, argv[i++], &color);
  if(argc > i)
    js_value_to(ctx, argv[i++], thickness);

  /**/
  if(argc > i) {
    if(JS_IsBool(argv[i])) {
      js_value_to(ctx, argv[i++], antialias);
      line_type = antialias ? cv::LINE_AA : cv::LINE_8;

    } else if(JS_IsNumber(argv[i])) {
      js_value_to(ctx, argv[i++], line_type);
    }
  }

  try {
    cv::circle(dst, point, radius, cv::Scalar(color), thickness < 0 ? cv::FILLED : thickness, line_type);
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static JSValue
js_draw_ellipse(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  JSPointData<double> center;
  JSSizeData<double> axes;
  double angle = 0, start_angle = 0, end_angle = 0;
  JSColorData<double> color;
  bool antialias = true;
  int thickness = -1;
  int line_type = cv::LINE_AA;

  if(argc > i) {
    if(!js_is_noarray((dst = js_umat_or_mat(ctx, argv[i]))))
      i++;
  } else
    dst = *dptr;

  if(js_is_noarray(dst))
    return JS_EXCEPTION;

  if(js_rect_read(ctx, argv[i], &center, &axes)) {
    center.x += axes.width / 2;
    center.y += axes.height / 2;
    i++;
  } else {
    if(js_point_read(ctx, argv[i], &center)) {
      i++;
    } else {
      JS_ToFloat64(ctx, &center.x, argv[i]);
      JS_ToFloat64(ctx, &center.y, argv[i + 1]);
      i += 2;
    }
    if(js_size_read(ctx, argv[i], &axes)) {
      i++;
    } else {
      JS_ToFloat64(ctx, &axes.width, argv[i]);
      JS_ToFloat64(ctx, &axes.height, argv[i + 1]);
      i += 2;
    }
  }

  if(argc > i)
    js_value_to(ctx, argv[i++], angle);
  if(argc > i)
    js_value_to(ctx, argv[i++], start_angle);
  if(argc > i)
    js_value_to(ctx, argv[i++], end_angle);
  if(argc > i)
    js_color_read(ctx, argv[i++], &color);
  if(argc > i)
    js_value_to(ctx, argv[i++], thickness);
  if(argc > i) {
    if(JS_IsBool(argv[i])) {
      js_value_to(ctx, argv[i++], antialias);
      line_type = antialias ? cv::LINE_AA : cv::LINE_8;
    } else if(JS_IsNumber(argv[i])) {
      js_value_to(ctx, argv[i++], line_type);
    }
  }

  try {
    cv::ellipse(dst, center, axes, angle, start_angle, end_angle, cv::Scalar(color), thickness < 0 ? cv::FILLED : thickness, line_type);
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static JSValue
js_draw_contour(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  JSContoursData<int> contours;
  int32_t index = -1, line_type = cv::LINE_8;
  JSColorData<double> color;
  int thickness = 1;
  bool antialias = true;

  contours.resize(1);

  if(argc > i) {
    if(!js_is_noarray((dst = js_umat_or_mat(ctx, argv[i]))))
      i++;
  } else
    dst = *dptr;

  if(js_is_noarray(dst))
    return JS_EXCEPTION;

  /*if(i == argc || !js_is_array(ctx, argv[i]))
    return JS_EXCEPTION;*/

  if(argc > i)
    js_contour_read(ctx, argv[i++], &contours[0]);

  /* if(argc > i)
     JS_ToInt32(ctx, &index, argv[i++]);*/
  if(argc > i)
    js_color_read(ctx, argv[i++], &color);
  if(argc > i)
    js_value_to(ctx, argv[i++], thickness);
  if(argc > i)
    JS_ToInt32(ctx, &line_type, argv[i++]);

  std::cerr << "draw_contour() contours.length=" << contours.size() << " index=" << index << " thickness=" << thickness << std::endl;

  try {
    cv::drawContours(dst, contours, index, cv::Scalar(color), thickness, line_type);
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  std::cerr << "draw_contour() ret:" << ret << " color: " << cv::Scalar(color) << std::endl;
  return JS_UNDEFINED;
}

static JSValue
js_draw_contours(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  JSContoursData<int> contours;
  int32_t index = -1, line_type = cv::LINE_8;
  JSColorData<double> color;
  JSPointData<int> offset{0, 0};
  std::vector<cv::Vec4i> hier;
  int32_t thickness = 1, maxLevel = INT_MAX;
  bool antialias = true;

  if(js_is_noarray((dst = js_umat_or_mat(ctx, argv[0]))))
    return JS_EXCEPTION;

  js_array_to(ctx, argv[1], contours);
  js_value_to(ctx, argv[2], index);

  /* if(index >= 0 && index < contours.size()) {
     std::cerr << "contour #" << (index + 1) << ": " << contours[index].size() << std::endl;
   } else {
     for(size_t m = 0; m < contours.size(); ++m) {
       std::cerr << "contour #" << (m + 1) << "/" << (contours.size() - 1) << ": " << contours[m].size() << std::endl;
     }
   }*/

  js_color_read(ctx, argv[3], &color);

  if(argc > 4)
    js_value_to(ctx, argv[4], thickness);

  if(argc > 5)
    js_value_to(ctx, argv[5], line_type);

  if(argc > 6)
    js_array_to(ctx, argv[6], hier);

  if(argc > 7)
    js_value_to(ctx, argv[7], maxLevel);

  if(argc > 8)
    js_point_read<int>(ctx, argv[8], &offset);

  cv::Scalar scalar = cv::Scalar(color);
  // std::cerr << "draw_contours() contours.length=" << contours.size() << " index=" <<
  // index << " thickness="
  // << thickness << std::endl;

  try {
    if(dst.isMat()) {
      cv::Mat& mref = dst.getMatRef();
      cv::drawContours(mref, contours, index, scalar, thickness, line_type, argc > 6 ? JSInputOutputArray(hier) : cv::noArray(), maxLevel, offset);
    } else if(dst.isUMat()) {
      cv::UMat& umref = dst.getUMatRef();
      cv::drawContours(umref, contours, index, scalar, thickness, line_type, argc > 6 ? JSInputOutputArray(hier) : cv::noArray(), maxLevel, offset);
    } else {
      cv::drawContours(dst, contours, index, scalar, thickness, line_type, argc > 6 ? JSInputOutputArray(hier) : cv::noArray(), maxLevel, offset);
    }
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  // std::cerr << "draw_contours() ret:" << ret << " color: " <<
  // cv::Scalar(color) << std::endl;

  return JS_UNDEFINED;
}

static JSValue
js_draw_line(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSInputOutputArray dst;
  int i = 0, ret = -1, ia;
  JSLineData<int> line(0, 0, 0, 0);
  JSColorData<uint8_t> color;
  cv::Scalar scalar;
  int thickness = 1;
  bool antialias = true;

  if(argc > i) {
    if(!js_is_noarray((dst = js_umat_or_mat(ctx, argv[i]))))
      i++;
  } else
    dst = *dptr;

  if(js_is_noarray(dst))
    return JS_EXCEPTION;

  if(!(ia = js_line_arg(ctx, argc - i, argv + i, line)))
    return JS_ThrowTypeError(ctx, "argument 2 must be a line");

  i += ia;

  if(argc > i) {
    js_color_read(ctx, argv[i], &color);
    i++;

    scalar[0] = color.arr[0];
    scalar[1] = color.arr[1];
    scalar[2] = color.arr[2];
    scalar[3] = color.arr[3];
  }

  if(argc > i)
    js_value_to(ctx, argv[i++], thickness);

  if(argc > i) {
    if(JS_IsBool(argv[i])) {
      js_value_to(ctx, argv[i++], antialias);
    } else {
      int32_t type;
      JS_ToInt32(ctx, &type, argv[i++]);
      antialias = type == cv::LINE_AA;
    }
  }

#ifdef DEBUG_OUTPUT
  printf("cv::line %i|%i -> %i|%i [%.0lf,%.0lf,%.0lf] %i %s\n",
         points[0].x,
         points[0].y,
         points[1].x,
         points[1].y,
         scalar[0],
         scalar[1],
         scalar[2],
         thickness,
         antialias ? "true" : "false");
#endif

  try {
    cv::line(dst, line.a, line.b, scalar, thickness, antialias ? cv::LINE_AA : cv::LINE_8);
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static JSValue
js_draw_polygon(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  point_vector<int> points;
  cv::Scalar color;
  bool antialias = true;
  int thickness = -1;

  if(argc > i) {
    if(!js_is_noarray((dst = js_umat_or_mat(ctx, argv[i]))))
      i++;
  } else
    dst = *dptr;

  if(argc > i)
    js_array_to(ctx, argv[i++], points);

  if(argc > i && js_color_read(ctx, argv[i], &color))
    i++;

  if(argc > i)
    js_value_to(ctx, argv[i++], thickness);

  if(argc > i)
    js_value_to(ctx, argv[i++], antialias);

  if(dptr != nullptr) {
    const int size = points.size();
    int line_type = antialias ? cv::LINE_AA : cv::LINE_8;
    const JSPointData<int>* pts = points.data();

    std::cerr << "drawPolygon() points: " << (points) << " color: " << to_string(color) << std::endl;

    // cv::fillPoly(*dptr, points, color, antialias ? cv::LINE_AA : cv::LINE_8);

    try {
      (thickness <= 0 ? cv::fillPoly(dst, &pts, &size, 1, color, line_type) : cv::polylines(dst, &pts, &size, 1, true, color, thickness, line_type));
    } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

    return JS_UNDEFINED;
  }

  return JS_EXCEPTION;
}

static JSValue
js_draw_polylines(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  JSContoursData<int> points;
  JSColorData<double> color = {0, 0, 0};
  bool is_closed = false;
  int32_t thickness = -1, line_type = cv::LINE_AA;

  if(argc > i) {
    if(!js_is_noarray((dst = js_umat_or_mat(ctx, argv[i]))))
      i++;
  } else
    dst = *dptr;

  if(argc > i)
    js_array_to(ctx, argv[i++], points);

  if(argc > i)
    is_closed = JS_ToBool(ctx, argv[i++]);

  if(argc > i)
    js_array_to(ctx, argv[i++], color.arr);

  if(argc > i)
    JS_ToInt32(ctx, &thickness, argv[i++]);

  if(argc > i) {
    if(JS_IsBool(argv[i]))
      line_type = JS_ToBool(ctx, argv[i++]) ? cv::LINE_AA : cv::LINE_8;
    else
      JS_ToInt32(ctx, &line_type, argv[i++]);
  }

  // std::cerr << "polylines()" << " is_closed: " << is_closed << " color: " <<  (color) << " thickness: " << thickness << " line_type: " << line_type <<
  // std::endl;
  try {
    cv::polylines(dst, points, is_closed, *reinterpret_cast<cv::Scalar const*>(&color), thickness, line_type);
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static JSValue
js_draw_rectangle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  JSRectData<double> rect;
  JSPointData<double> points[2];
  JSColorData<uint8_t> color;
  cv::Scalar scalar;
  int32_t thickness = 1, line_type = cv::LINE_AA;

  if(argc > i) {
    if(!js_is_noarray((dst = js_umat_or_mat(ctx, argv[i]))))
      i++;
  } else
    dst = *dptr;

  if(js_is_noarray(dst))
    return JS_EXCEPTION;

  if(argc > i && js_rect_read(ctx, argv[i], &rect)) {
    i++;
  } else {
    cv::Point pt1, pt2;

    if(argc > i + 1 && js_point_read(ctx, argv[i], &pt1) && js_point_read(ctx, argv[i + 1], &pt2)) {
      rect.x = std::min(pt1.x, pt2.x);
      rect.y = std::min(pt1.y, pt2.y);
      rect.width = std::max(pt1.x, pt2.x) - rect.x;
      rect.height = std::max(pt1.y, pt2.y) - rect.y;

      i += 2;
    }
  }

  if(argc > i && js_color_read(ctx, argv[i], &color)) {
    i++;
    scalar[0] = color.arr[0];
    scalar[1] = color.arr[1];
    scalar[2] = color.arr[2];
    scalar[3] = color.arr[3];
  }

  if(argc > i)
    js_value_to(ctx, argv[i++], thickness);

  if(argc > i) {
    if(JS_IsBool(argv[i]))
      line_type = JS_ToBool(ctx, argv[i++]) ? cv::LINE_AA : cv::LINE_8;
    else
      JS_ToInt32(ctx, &line_type, argv[i++]);
  }

  points[0].x = rect.x;
  points[0].y = rect.y;
  points[1].x = rect.x + rect.width;
  points[1].y = rect.y + rect.height;

  // printf("cv::rectangle %lf,%lf %lfx%lf [%.0lf,%.0lf,%.0lf,%.0lf]\n", rect.x, rect.y, rect.width, rect.height, scalar[0], scalar[1], scalar[2], scalar[3]);
  try {
    cv::rectangle(dst, points[0], points[1], scalar, thickness, line_type);
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static JSValue
js_draw_keypoints(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  int i = 0, ret = -1;
  std::vector<JSKeyPointData> keypoints;
  JSInputOutputArray image, dst;
  JSColorData<double> color = {-1, -1, -1, -1};

  image = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[2]);

  js_array_to(ctx, argv[1], keypoints);

  if(argc)
    js_color_read(ctx, argv[3], &color);

  try {
    cv::drawKeypoints(image, keypoints, dst, *(cv::Scalar*)&color, cv::DrawMatchesFlags(0));
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static JSValue
js_put_text(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  cv::Mat* dst;
  int i = 0, ret = -1;
  JSColorData<double> color;
  JSPointData<double> point;
  std::string text, font_name;
  int32_t font_face = cv::FONT_HERSHEY_SIMPLEX, thickness = 1, line_type = cv::LINE_AA;
  double font_scale = 1;
  bool bottomLeftOrigin = false;

  if(argc > i && (dst = js_mat_data2(ctx, argv[i])))
    i++;
  else
    dst = dptr;

  if(dst == nullptr)
    return JS_EXCEPTION;

  js_value_to(ctx, argv[i++], text);

  if(js_point_read(ctx, argv[i], &point)) {
    i++;
  } else {
    int32_t x, y;
    JS_ToInt32(ctx, &x, argv[i]);
    JS_ToInt32(ctx, &y, argv[i + 1]);

    point.x = x;
    point.y = y;
    i += 2;
  }

  if(i < argc) {
    if(JS_IsNumber(argv[i]))
      JS_ToInt32(ctx, &font_face, argv[i]);
    else
      js_value_to(ctx, argv[i], font_name);

#ifndef HAVE_OPENCV_FREETYPE
    if(!font_name.empty())
      return JS_ThrowInternalError(ctx, "Got a font name, but not FreeType support");
#endif

    if(++i < argc) {
      JS_ToFloat64(ctx, &font_scale, argv[i]);

      if(++i < argc) {
        js_color_read(ctx, argv[i], &color);

        if(++i < argc) {
          JS_ToInt32(ctx, &thickness, argv[i]);

          if(++i < argc) {
            JS_ToInt32(ctx, &line_type, argv[i]);

            if(++i < argc)
              bottomLeftOrigin = JS_ToBool(ctx, argv[i]);
          }
        }
      }
    }
  }

  try {
#ifdef HAVE_OPENCV_FREETYPE
    if(freetype2 == nullptr)
      freetype2 = cv::freetype::createFreeType2();

    if(!font_name.empty() && font_name != freetype2_face) {
      try {
        freetype2->loadFontData(font_name, 0);
      } catch(const cv::Exception& e) {
        const char *errstr = e.what(), *err2;

        if((err2 = strchr(errstr, '!')))
          errstr = err2 + 1;

        return JS_ThrowInternalError(ctx, "freetype2 exception: %s\n", errstr);
      }
      freetype2_face = font_name;
    }

    if(!font_name.empty())
      freetype2->putText(*dst, text, point, font_scale, cv::Scalar(color), thickness, line_type, bottomLeftOrigin);
    else
#endif
      cv::putText(*dst, text, point, font_face, font_scale, cv::Scalar(color), thickness < 0 ? 0 : thickness, line_type, bottomLeftOrigin);
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static JSValue
js_get_text_size(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  int i = 0, baseline = 0;
  JSSizeData<double> size;
  const char* text;
  int32_t font_face = cv::FONT_HERSHEY_SIMPLEX, thickness = 1;
  std::string font_name;
  double font_scale = 1;
  JSValue baseline_value;
  std::array<int32_t, 2> dim;

  text = JS_ToCString(ctx, argv[i++]);

  if(argc > i) {
    if(JS_IsNumber(argv[i]))
      JS_ToInt32(ctx, &font_face, argv[i]);
    else
      js_value_to(ctx, argv[i], font_name);
    i++;
  }

  if(argc > i)
    JS_ToFloat64(ctx, &font_scale, argv[i++]);

  if(argc > i)
    JS_ToInt32(ctx, &thickness, argv[i++]);

  try {
#ifdef HAVE_OPENCV_FREETYPE
    if(freetype2 == nullptr)
      freetype2 = cv::freetype::createFreeType2();

    if(!font_name.empty() && font_name != freetype2_face) {
      freetype2->loadFontData(font_name, 0);
      freetype2_face = font_name;
    }

    if(!font_name.empty())
      size = freetype2->getTextSize(text, font_scale, thickness, &baseline);
    else
#endif
      size = cv::getTextSize(text, font_face, font_scale, thickness, &baseline);
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  baseline_value = JS_NewInt32(ctx, baseline);

  if(js_is_function(ctx, argv[i])) {
    JS_Call(ctx, argv[i], JS_UNDEFINED, 1, const_cast<JSValueConst*>(&baseline_value));
  } else if(js_is_array(ctx, argv[i])) {
    JS_SetPropertyUint32(ctx, argv[i], 0, baseline_value);
  } else if(JS_IsObject(argv[i])) {
    JS_SetPropertyStr(ctx, argv[i], "y", baseline_value);
  }
  dim[0] = size.width;
  dim[1] = size.height;

  return js_array<int32_t>::from_sequence(ctx, dim.cbegin(), dim.cend());
}

static JSValue
js_get_font_scale_from_height(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  int32_t font_face, pixel_height, thickness = 1;
  double font_scale;

  JS_ToInt32(ctx, &font_face, argv[0]);
  JS_ToInt32(ctx, &pixel_height, argv[1]);
  if(argc > 2)
    JS_ToInt32(ctx, &thickness, argv[2]);

  try {
    font_scale = cv::getFontScaleFromHeight(font_face, pixel_height, thickness);
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_NewFloat64(ctx, font_scale);
}

static JSValue
js_load_font(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  int32_t id = 0;
  std::string font_name;

  try {
#ifdef HAVE_OPENCV_FREETYPE
    if(freetype2 == nullptr)
      freetype2 = cv::freetype::createFreeType2();
#endif

    js_value_to(ctx, argv[0], font_name);
    if(argc > 1)
      js_value_to(ctx, argv[1], id);

#ifdef HAVE_OPENCV_FREETYPE
    freetype2->loadFontData(font_name, id);
#endif
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static JSValue
js_clip_line(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSSizeData<int> img_size;
  JSRectData<int> img_rect;
  JSLineData<double>* l;
  JSPointData<double>*a, *b;
  JSLineData<int> line = {0, 0, 0, 0};
  BOOL result = FALSE;

  if((a = js_point_data(argv[1])) && (b = js_point_data(argv[2]))) {
    line.a = *a;
    line.b = *b;
  } else if((l = js_line_data(argv[1]))) {
    line.a = l->a;
    line.b = l->b;
  } else {
    return JS_ThrowTypeError(ctx, "argument 2,3: expecting Point,Point or Line");
  }

  try {
    if(js_rect_read(ctx, argv[0], &img_rect))
      result = cv::clipLine(img_rect, line.a, line.b);
    else if(js_size_read(ctx, argv[0], &img_size))
      result = cv::clipLine(img_size, line.a, line.b);
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  if(a && b) {
    *a = line.a;
    *b = line.b;
  } else if(l) {
    l->a = line.a;
    l->b = line.b;
  }

  return JS_NewBool(ctx, result);
}

JSValue draw_proto = JS_UNDEFINED, draw_class = JS_UNDEFINED;
thread_local JSClassID js_draw_class_id = 0;

JSClassDef js_draw_class = {
    .class_name = "Draw",
    .finalizer = 0,
};

const JSCFunctionListEntry js_draw_proto_funcs[] = {
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Draw", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_draw_static_funcs[] = {
    JS_CFUNC_DEF("circle", 1, &js_draw_circle),
    JS_CFUNC_DEF("ellipse", 2, &js_draw_ellipse),
    JS_CFUNC_DEF("contour", 1, &js_draw_contour),
    JS_CFUNC_DEF("contours", 4, &js_draw_contours),
    JS_CFUNC_DEF("line", 1, &js_draw_line),
    JS_CFUNC_DEF("polygon", 1, &js_draw_polygon),
    JS_CFUNC_DEF("polylines", 1, &js_draw_polylines),
    JS_CFUNC_DEF("rectangle", 1, &js_draw_rectangle),
    JS_CFUNC_DEF("text", 2, &js_put_text),
    JS_CFUNC_DEF("textSize", 5, &js_get_text_size),
    JS_CFUNC_DEF("fontScaleFromHeight", 2, &js_get_font_scale_from_height),
    JS_CFUNC_DEF("loadFont", 1, &js_load_font),
    JS_CFUNC_DEF("clipLine", 3, &js_clip_line),
    JS_CFUNC_DEF("drawKeypoints", 3, &js_draw_keypoints),
};

const JSCFunctionListEntry js_draw_global_funcs[] = {
    JS_CFUNC_DEF("drawCircle", 1, &js_draw_circle),
    JS_CFUNC_DEF("drawEllipse", 2, &js_draw_ellipse),
    JS_CFUNC_DEF("drawContour", 1, &js_draw_contour),
    JS_CFUNC_DEF("drawContours", 4, &js_draw_contours),
    JS_CFUNC_DEF("drawLine", 1, &js_draw_line),
    JS_CFUNC_DEF("drawPolygon", 1, &js_draw_polygon),
    JS_CFUNC_DEF("drawPolylines", 1, &js_draw_polylines),
    JS_CFUNC_DEF("drawRect", 1, &js_draw_rectangle),
    JS_CFUNC_DEF("drawKeypoints", 3, &js_draw_keypoints),
    JS_CFUNC_DEF("putText", 2, &js_put_text),
    JS_CFUNC_DEF("getTextSize", 5, &js_get_text_size),
    JS_CFUNC_DEF("getFontScaleFromHeight", 2, &js_get_font_scale_from_height),
    JS_CFUNC_DEF("loadFont", 1, &js_load_font),
    JS_CFUNC_DEF("clipLine", 3, &js_clip_line),
};

static JSValue
js_draw_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSValue obj, proto;

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_draw_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  return obj;

fail:
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

int
js_draw_init(JSContext* ctx, JSModuleDef* m) {
  /* create the Draw class */
  JS_NewClassID(&js_draw_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_draw_class_id, &js_draw_class);

  draw_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, draw_proto, js_draw_proto_funcs, countof(js_draw_proto_funcs));
  JS_SetClassProto(ctx, js_draw_class_id, draw_proto);

  draw_class = JS_NewCFunction2(ctx, js_draw_constructor, "Draw", 2, JS_CFUNC_constructor, 0);

  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, draw_class, draw_proto);
  JS_SetPropertyFunctionList(ctx, draw_class, js_draw_static_funcs, countof(js_draw_static_funcs));

  JS_SetModuleExport(ctx, m, "Draw", draw_class);

  JS_SetModuleExportList(ctx, m, js_draw_global_funcs, countof(js_draw_global_funcs));

  return 0;
}

extern "C" void
js_draw_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Draw");
  JS_AddModuleExportList(ctx, m, js_draw_global_funcs, countof(js_draw_global_funcs));
}

#if defined(JS_DRAW_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_draw
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  if(!(m = JS_NewCModule(ctx, module_name, &js_draw_init)))
    return NULL;
  js_draw_export(ctx, m);
  return m;
}
}
