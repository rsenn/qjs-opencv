#include "cutils.h"
#include "geometry.hpp"
#include "js_array.hpp"
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

extern "C" VISIBLE int js_draw_init(JSContext*, JSModuleDef*);

#ifdef HAVE_OPENCV_FREETYPE
#include <opencv2/freetype.hpp>
cv::Ptr<cv::freetype::FreeType2> freetype2 = nullptr;
std::string freetype2_face;
#endif

extern "C" {

cv::Mat* dptr = 0;

static JSValue
js_draw_circle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  JSPointData<double> point;
  int32_t x, y;
  int radius = 0;
  JSColorData<double> color;
  bool antialias = true;
  int thickness = -1;
  int lineType = cv::LINE_AA;

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
      lineType = antialias ? cv::LINE_AA : cv::LINE_8;

    } else if(JS_IsNumber(argv[i])) {
      js_value_to(ctx, argv[i++], lineType);
    }
  }

  cv::circle(dst, point, radius, js_to_scalar(color), thickness < 0 ? cv::FILLED : thickness, lineType);
  return JS_UNDEFINED;
}

static JSValue
js_draw_ellipse(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  JSPointData<double> center;
  JSSizeData<double> axes;
  double angle = 0, startAngle = 0, endAngle = 0;
  JSColorData<double> color;
  bool antialias = true;
  int thickness = -1;
  int lineType = cv::LINE_AA;

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
    js_value_to(ctx, argv[i++], startAngle);
  if(argc > i)
    js_value_to(ctx, argv[i++], endAngle);
  if(argc > i)
    js_color_read(ctx, argv[i++], &color);
  if(argc > i)
    js_value_to(ctx, argv[i++], thickness);
  if(argc > i) {
    if(JS_IsBool(argv[i])) {
      js_value_to(ctx, argv[i++], antialias);
      lineType = antialias ? cv::LINE_AA : cv::LINE_8;
    } else if(JS_IsNumber(argv[i])) {
      js_value_to(ctx, argv[i++], lineType);
    }
  }

  cv::ellipse(dst, center, axes, angle, startAngle, endAngle, js_to_scalar(color), thickness < 0 ? cv::FILLED : thickness, lineType);

  return JS_UNDEFINED;
}

static JSValue
js_draw_contour(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  JSContoursData<int> contours;
  int32_t contourIdx = -1, lineType = cv::LINE_8;

  JSColorData<double> color;
  int thickness = 1;
  bool antialias = true;

  if(argc > i) {
    if(!js_is_noarray((dst = js_umat_or_mat(ctx, argv[i]))))
      i++;
  } else
    dst = *dptr;

  if(js_is_noarray(dst))
    return JS_EXCEPTION;

  if(i == argc || !js_is_array(ctx, argv[++i]))
    return JS_EXCEPTION;

  if(argc > i)
    JS_ToInt32(ctx, &contourIdx, argv[i]);
  if(argc > i)
    js_color_read(ctx, argv[i++], &color);
  if(argc > i)
    js_value_to(ctx, argv[i++], thickness);
  if(argc > i)
    JS_ToInt32(ctx, &lineType, argv[i]);

  std::cerr << "draw_contour() contours.length=" << contours.size() << " contourIdx=" << contourIdx << " thickness=" << thickness << std::endl;

  cv::drawContours(dst, contours, contourIdx, js_to_scalar(color), thickness, lineType);

  std::cerr << "draw_contour() ret:" << ret << " color: " << js_to_scalar(color) << std::endl;
  return JS_UNDEFINED;
}

static JSValue
js_draw_contours(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  JSContoursData<int> contours;
  int32_t contourIdx = -1, lineType = cv::LINE_8;
  JSColorData<double> color;
  JSPointData<int> offset{0, 0};
  std::vector<cv::Vec4i> hier;
  int32_t thickness = 1, maxLevel = INT_MAX;
  bool antialias = true;

  if(js_is_noarray((dst = js_umat_or_mat(ctx, argv[0]))))
    return JS_EXCEPTION;

  js_array_to(ctx, argv[1], contours);

  js_value_to(ctx, argv[2], contourIdx);

  js_color_read(ctx, argv[3], &color);
  if(argc > 4) {
    js_value_to(ctx, argv[4], thickness);
    if(argc > 5) {
      js_value_to(ctx, argv[5], lineType);
      if(argc > 6) {
        js_array_to(ctx, argv[6], hier);
        if(argc > 7) {
          js_value_to(ctx, argv[7], maxLevel);
          if(argc > 8)
            js_point_read<int>(ctx, argv[8], &offset);
        }
      }
    }
  }

  // std::cerr << "draw_contours() contours.length=" << contours.size() << " contourIdx=" <<
  // contourIdx << " thickness="
  // << thickness << std::endl;

  cv::drawContours(dst, contours, contourIdx, js_to_scalar(color), thickness, lineType, argc > 6 ? JSInputOutputArray(hier) : cv::noArray(), maxLevel, offset);

  // std::cerr << "draw_contours() ret:" << ret << " color: " <<
  // js_to_scalar(color) << std::endl;

  return JS_UNDEFINED;
}

static JSValue
js_draw_line(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  JSPointData<int> points[2];
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

  if(argc > i && js_point_read(ctx, argv[i], &points[0]))
    i++;

  if(argc > i && js_point_read(ctx, argv[i], &points[1]))
    i++;

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

  if(argc > i && JS_IsBool(argv[i]))
    js_value_to(ctx, argv[i++], antialias);

#ifdef DEBUG_OUTPUT
  printf("cv::line %i|%i -> %i|%i [%.0lf,%.0lf,%.0lf] %i\n", points[0].x, points[0].y, points[1].x, points[1].y, scalar[0], scalar[1], scalar[2], thickness);
#endif

  cv::line(dst, points[0], points[1], scalar, thickness, antialias ? cv::LINE_AA : cv::LINE_8);
  return JS_UNDEFINED;
}

static JSValue
js_draw_polygon(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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
    int lineType = antialias ? cv::LINE_AA : cv::LINE_8;
    const JSPointData<int>* pts = points.data();

    std::cerr << "drawPolygon() points: " << (points) << " color: " << to_string(color) << std::endl;

    // cv::fillPoly(*dptr, points, color, antialias ? cv::LINE_AA : cv::LINE_8);
    (thickness <= 0 ? cv::fillPoly(dst, &pts, &size, 1, color, lineType) : cv::polylines(dst, &pts, &size, 1, true, color, thickness, lineType));

    return JS_UNDEFINED;
  }
  return JS_EXCEPTION;
}

static JSValue
js_draw_rectangle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSInputOutputArray dst;
  int i = 0, ret = -1;
  JSRectData<double> rect;
  JSPointData<double> points[2];
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
      js_value_to(ctx, argv[i], antialias);
    else {
      int32_t aa;
      JS_ToInt32(ctx, &aa, argv[i++]);
      antialias = (aa == cv::LINE_AA);
    }
    i++;
  }

  points[0].x = rect.x;
  points[0].y = rect.y;
  points[1].x = rect.x + rect.width;
  points[1].y = rect.y + rect.height;

  printf("cv::rectangle %lf,%lf %lfx%lf [%.0lf,%.0lf,%.0lf,%.0lf]\n", rect.x, rect.y, rect.width, rect.height, scalar[0], scalar[1], scalar[2], scalar[3]);
  cv::rectangle(dst, points[0], points[1], scalar, thickness, antialias ? cv::LINE_AA : cv::LINE_8);

  return JS_UNDEFINED;
}

static JSValue
js_draw_keypoints(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  int i = 0, ret = -1;
  std::vector<JSKeyPointData> keypoints;
  JSInputOutputArray image, outImage;
  JSColorData<double> color = {-1, -1, -1, -1};
  cv::Scalar scalar = cv::Scalar::all(-1);

  image = js_umat_or_mat(ctx, argv[0]);
  outImage = js_umat_or_mat(ctx, argv[2]);

  js_array_to(ctx, argv[1], keypoints);

  if(argc)
    js_color_read(ctx, argv[3], &color);

  cv::drawKeypoints(image, keypoints, outImage, *(cv::Scalar*)&color, cv::DrawMatchesFlags(0));

  return JS_UNDEFINED;
}

static JSValue
js_put_text(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  cv::Mat* dst;
  int i = 0, ret = -1;
  JSColorData<double> color;
  JSPointData<double> point;
  std::string text, fontName;
  int32_t fontFace = cv::FONT_HERSHEY_SIMPLEX, thickness = 1, lineType = cv::LINE_AA;
  double fontScale = 1;
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
      JS_ToInt32(ctx, &fontFace, argv[i]);
    else
      js_value_to(ctx, argv[i], fontName);

    if(++i < argc) {
      JS_ToFloat64(ctx, &fontScale, argv[i]);

      if(++i < argc) {
        js_color_read(ctx, argv[i], &color);

        if(++i < argc) {
          JS_ToInt32(ctx, &thickness, argv[i]);

          if(++i < argc) {
            JS_ToInt32(ctx, &lineType, argv[i]);

            if(++i < argc)
              bottomLeftOrigin = JS_ToBool(ctx, argv[i]);
          }
        }
      }
    }
  }

#ifdef HAVE_OPENCV_FREETYPE
  if(freetype2 == nullptr)
    freetype2 = cv::freetype::createFreeType2();

  if(!fontName.empty() && fontName != freetype2_face) {
    try {
      freetype2->loadFontData(fontName, 0);
    } catch(const cv::Exception& e) {
      const char *errstr = e.what(), *err2;

      if((err2 = strchr(errstr, '!')))
        errstr = err2 + 1;

      return JS_ThrowInternalError(ctx, "freetype2 exception: %s\n", errstr);
    }
    freetype2_face = fontName;
  }

  if(!fontName.empty())
    freetype2->putText(*dst, text, point, fontScale, js_to_scalar(color), thickness, lineType, bottomLeftOrigin);
  else
#endif
    cv::putText(*dst, text, point, fontFace, fontScale, js_to_scalar(color), thickness, lineType, bottomLeftOrigin);

  return JS_UNDEFINED;
}

static JSValue
js_get_text_size(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  int i = 0, baseline = 0;
  JSSizeData<double> size;
  const char* text;
  int32_t fontFace = cv::FONT_HERSHEY_SIMPLEX, thickness = 1;
  std::string fontName;
  double fontScale = 1;
  JSValue baselineVal;
  std::array<int32_t, 2> dim;

  text = JS_ToCString(ctx, argv[i++]);

  if(argc > i) {
    if(JS_IsNumber(argv[i]))
      JS_ToInt32(ctx, &fontFace, argv[i]);
    else
      js_value_to(ctx, argv[i], fontName);
    i++;
  }

  if(argc > i)
    JS_ToFloat64(ctx, &fontScale, argv[i++]);

  if(argc > i)
    JS_ToInt32(ctx, &thickness, argv[i++]);

#ifdef HAVE_OPENCV_FREETYPE
  if(freetype2 == nullptr)
    freetype2 = cv::freetype::createFreeType2();

  if(!fontName.empty() && fontName != freetype2_face) {
    freetype2->loadFontData(fontName, 0);
    freetype2_face = fontName;
  }

  if(!fontName.empty())
    size = freetype2->getTextSize(text, fontScale, thickness, &baseline);
  else
#endif
    size = cv::getTextSize(text, fontFace, fontScale, thickness, &baseline);

  baselineVal = JS_NewInt32(ctx, baseline);

  if(JS_IsFunction(ctx, argv[i])) {
    JS_Call(ctx, argv[i], JS_UNDEFINED, 1, const_cast<JSValueConst*>(&baselineVal));
  } else if(js_is_array(ctx, argv[i])) {
    JS_SetPropertyUint32(ctx, argv[i], 0, baselineVal);
  } else if(JS_IsObject(argv[i])) {
    JS_SetPropertyStr(ctx, argv[i], "y", baselineVal);
  }
  dim[0] = size.width;
  dim[1] = size.height;

  return js_array<int32_t>::from_sequence(ctx, dim.cbegin(), dim.cend());
}

static JSValue
js_get_font_scale_from_height(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  int32_t fontFace, pixelHeight, thickness = 1;
  double fontScale;

  JS_ToInt32(ctx, &fontFace, argv[0]);
  JS_ToInt32(ctx, &pixelHeight, argv[1]);
  if(argc > 2)
    JS_ToInt32(ctx, &thickness, argv[2]);

  fontScale = cv::getFontScaleFromHeight(fontFace, pixelHeight, thickness);

  return JS_NewFloat64(ctx, fontScale);
}

static JSValue
js_load_font(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  int32_t id = 0;
  std::string fontName;

#ifdef HAVE_OPENCV_FREETYPE
  if(freetype2 == nullptr)
    freetype2 = cv::freetype::createFreeType2();
#endif

  js_value_to(ctx, argv[0], fontName);
  if(argc > 1)
    js_value_to(ctx, argv[1], id);

#ifdef HAVE_OPENCV_FREETYPE
  freetype2->loadFontData(fontName, id);
#endif
  return JS_UNDEFINED;
}

static JSValue
js_clip_line(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSSizeData<int> imgSize;
  JSRectData<int> imgRect;
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

  if(js_rect_read(ctx, argv[0], &imgRect)) {
    result = cv::clipLine(imgRect, line.a, line.b);
  } else if(js_size_read(ctx, argv[0], &imgSize)) {
    result = cv::clipLine(imgSize, line.a, line.b);
  }

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
thread_local VISIBLE JSClassID js_draw_class_id = 0;

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
    JS_CFUNC_DEF("drawLine", 1, &js_draw_line),
    JS_CFUNC_DEF("drawPolygon", 1, &js_draw_polygon),
    JS_CFUNC_DEF("drawRect", 1, &js_draw_rectangle),
    JS_CFUNC_DEF("drawKeypoints", 3, &js_draw_keypoints),
    JS_CFUNC_DEF("putText", 2, &js_put_text),
    JS_CFUNC_DEF("getTextSize", 5, &js_get_text_size),
    JS_CFUNC_DEF("getFontScaleFromHeight", 2, &js_get_font_scale_from_height),
    JS_CFUNC_DEF("loadFont", 1, &js_load_font),
    JS_CFUNC_DEF("clipLine", 3, &js_clip_line),
};

static JSValue
js_draw_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  JSValue obj = JS_UNDEFINED;
  JSValue proto;

  /* using new_target to get the prototype is necessary when the
     class is extended. */
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

  if(js_draw_class_id == 0) {
    /* create the Draw class */
    JS_NewClassID(&js_draw_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_draw_class_id, &js_draw_class);

    draw_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, draw_proto, js_draw_proto_funcs, countof(js_draw_proto_funcs));
    JS_SetClassProto(ctx, js_draw_class_id, draw_proto);

    draw_class = JS_NewCFunction2(ctx, js_draw_ctor, "Draw", 2, JS_CFUNC_constructor, 0);

    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, draw_class, draw_proto);
    JS_SetPropertyFunctionList(ctx, draw_class, js_draw_static_funcs, countof(js_draw_static_funcs));
  }

  if(m) {
    JS_SetModuleExportList(ctx, m, js_draw_static_funcs, countof(js_draw_static_funcs));
    JS_SetModuleExport(ctx, m, "Draw", draw_class);
  }
  /*  else
      JS_SetPropertyStr(ctx, *static_cast<JSValue*>(m), "Draw", draw_class);*/
  return 0;
}

extern "C" VISIBLE void
js_draw_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Draw");
  JS_AddModuleExportList(ctx, m, js_draw_static_funcs, countof(js_draw_static_funcs));
}

#if defined(JS_DRAW_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_draw
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_draw_init);
  if(!m)
    return NULL;
  js_draw_export(ctx, m);
  return m;
}
void
js_draw_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(draw_class))
    js_draw_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "Draw", draw_class);
}
}
