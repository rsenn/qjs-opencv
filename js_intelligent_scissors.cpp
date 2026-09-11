#include "js_alloc.hpp"
#include "js_point.hpp"
#include "js_cv.hpp"
#include "include/jsbindings.hpp"
#include "include/js_inputoutputarray.hpp"
#include <quickjs.h>
#include <stddef.h>
#include <new>
#ifdef HAVE_OPENCV2_PHOTO_SEGMENTATION_HPP
#include <opencv2/photo/segmentation.hpp>

/* cv::segmentation::IntelligentScissorsMB (opencv2/photo/segmentation.hpp)
 * is a plain default-constructible value type (no cv::Ptr/create*
 * factory, unlike CLAHE/BackgroundSubtractor), so it's stored directly by
 * value in the opaque block - same shape as js_dmatch.cpp's DMatch, not
 * js_clahe.cpp's cv::Ptr<CLAHE>. opencv.js flattens the `cv::segmentation`
 * namespace into the class name itself (`cv.segmentation_
 * IntelligentScissorsMB`, not `cv.segmentation.IntelligentScissorsMB`) -
 * see BUGS: opencvjs-intelligentscissors-missing. */
typedef cv::segmentation::IntelligentScissorsMB JSIntelligentScissorsMBData;

extern "C" {
thread_local JSValue intelligent_scissors_proto = JS_UNDEFINED, intelligent_scissors_class = JS_UNDEFINED;
thread_local JSClassID js_intelligent_scissors_class_id = 0;
}

extern "C" int js_intelligent_scissors_init(JSContext*, JSModuleDef*);

JSIntelligentScissorsMBData*
js_intelligent_scissors_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSIntelligentScissorsMBData*>(JS_GetOpaque2(ctx, val, js_intelligent_scissors_class_id));
}

static JSValue
js_intelligent_scissors_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSIntelligentScissorsMBData* s;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(s = js_allocate<JSIntelligentScissorsMBData>(ctx)))
    return JS_EXCEPTION;

  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_intelligent_scissors_class_id);
  JS_FreeValue(ctx, proto);
  if(JS_IsException(obj))
    goto fail;

  new(s) JSIntelligentScissorsMBData();
  JS_SetOpaque(obj, s);
  return obj;

fail:
  js_deallocate(ctx, s);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

void
js_intelligent_scissors_finalizer(JSRuntime* rt, JSValue val) {
  JSIntelligentScissorsMBData* s = static_cast<JSIntelligentScissorsMBData*>(JS_GetOpaque(val, js_intelligent_scissors_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  s->~JSIntelligentScissorsMBData();
  js_deallocate(rt, s);
}

enum {
  METHOD_SET_WEIGHTS = 0,
  METHOD_SET_GRADIENT_MAGNITUDE_MAX_LIMIT,
  METHOD_SET_EDGE_FEATURE_ZERO_CROSSING_PARAMETERS,
  METHOD_SET_EDGE_FEATURE_CANNY_PARAMETERS,
  METHOD_APPLY_IMAGE,
  METHOD_APPLY_IMAGE_FEATURES,
  METHOD_BUILD_MAP,
  METHOD_GET_CONTOUR,
};

/* setWeights()/setGradientMagnitudeMaxLimit()/etc. all return
 * `IntelligentScissorsMB&` (a reference to *this) in C++ for chaining -
 * mirrored here by returning `this_val` (duplicated) so `new
 * cv.segmentation_IntelligentScissorsMB().setWeights(...).applyImage(...)`
 * chains the same way in JS. */
static JSValue
js_intelligent_scissors_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSIntelligentScissorsMBData* s;
  JSValue ret = JS_UNDEFINED;

  if(!(s = js_intelligent_scissors_data2(ctx, this_val)))
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case METHOD_SET_WEIGHTS: {
        double non_edge = 0, gradient_direction = 0, gradient_magnitude = 0;
        JS_ToFloat64(ctx, &non_edge, argv[0]);
        JS_ToFloat64(ctx, &gradient_direction, argv[1]);
        JS_ToFloat64(ctx, &gradient_magnitude, argv[2]);
        s->setWeights(non_edge, gradient_direction, gradient_magnitude);
        ret = JS_DupValue(ctx, this_val);
        break;
      }

      case METHOD_SET_GRADIENT_MAGNITUDE_MAX_LIMIT: {
        double limit = 0;
        if(argc > 0)
          JS_ToFloat64(ctx, &limit, argv[0]);
        s->setGradientMagnitudeMaxLimit(limit);
        ret = JS_DupValue(ctx, this_val);
        break;
      }

      case METHOD_SET_EDGE_FEATURE_ZERO_CROSSING_PARAMETERS: {
        double gradient_magnitude_min_value = 0;
        if(argc > 0)
          JS_ToFloat64(ctx, &gradient_magnitude_min_value, argv[0]);
        s->setEdgeFeatureZeroCrossingParameters(gradient_magnitude_min_value);
        ret = JS_DupValue(ctx, this_val);
        break;
      }

      case METHOD_SET_EDGE_FEATURE_CANNY_PARAMETERS: {
        double threshold1 = 0, threshold2 = 0;
        int32_t apertureSize = 3;
        BOOL l2gradient = FALSE;
        JS_ToFloat64(ctx, &threshold1, argv[0]);
        JS_ToFloat64(ctx, &threshold2, argv[1]);
        if(argc > 2)
          JS_ToInt32(ctx, &apertureSize, argv[2]);
        if(argc > 3)
          l2gradient = JS_ToBool(ctx, argv[3]);
        s->setEdgeFeatureCannyParameters(threshold1, threshold2, apertureSize, l2gradient);
        ret = JS_DupValue(ctx, this_val);
        break;
      }

      case METHOD_APPLY_IMAGE: {
        JSInputArray image = js_cv_inputarray(ctx, argv[0]);
        s->applyImage(image);
        ret = JS_DupValue(ctx, this_val);
        break;
      }

      case METHOD_APPLY_IMAGE_FEATURES: {
        JSInputArray non_edge = js_cv_inputarray(ctx, argv[0]);
        JSInputArray gradient_direction = js_cv_inputarray(ctx, argv[1]);
        JSInputArray gradient_magnitude = js_cv_inputarray(ctx, argv[2]);
        JSInputArray image = argc > 3 ? js_cv_inputarray(ctx, argv[3]) : JSInputArray(cv::noArray());
        s->applyImageFeatures(non_edge, gradient_direction, gradient_magnitude, image);
        ret = JS_DupValue(ctx, this_val);
        break;
      }

      case METHOD_BUILD_MAP: {
        JSPointData<int> sourcePt;
        js_point_read(ctx, argv[0], &sourcePt);
        s->buildMap(cv::Point(sourcePt.x, sourcePt.y));
        break;
      }

      case METHOD_GET_CONTOUR: {
        JSPointData<int> targetPt;
        js_point_read(ctx, argv[0], &targetPt);
        JSOutputArray contour = js_cv_outputarray(ctx, argv[1]);
        BOOL backward = FALSE;
        if(argc > 2)
          backward = JS_ToBool(ctx, argv[2]);
        s->getContour(cv::Point(targetPt.x, targetPt.y), contour, backward);
        break;
      }
    }
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return ret;
}

JSClassDef js_intelligent_scissors_class = {
    .class_name = "segmentation_IntelligentScissorsMB",
    .finalizer = js_intelligent_scissors_finalizer,
};

const JSCFunctionListEntry js_intelligent_scissors_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("setWeights", 3, js_intelligent_scissors_method, METHOD_SET_WEIGHTS),
    JS_CFUNC_MAGIC_DEF("setGradientMagnitudeMaxLimit", 0, js_intelligent_scissors_method, METHOD_SET_GRADIENT_MAGNITUDE_MAX_LIMIT),
    JS_CFUNC_MAGIC_DEF(
        "setEdgeFeatureZeroCrossingParameters", 0, js_intelligent_scissors_method, METHOD_SET_EDGE_FEATURE_ZERO_CROSSING_PARAMETERS),
    JS_CFUNC_MAGIC_DEF("setEdgeFeatureCannyParameters", 2, js_intelligent_scissors_method, METHOD_SET_EDGE_FEATURE_CANNY_PARAMETERS),
    JS_CFUNC_MAGIC_DEF("applyImage", 1, js_intelligent_scissors_method, METHOD_APPLY_IMAGE),
    JS_CFUNC_MAGIC_DEF("applyImageFeatures", 3, js_intelligent_scissors_method, METHOD_APPLY_IMAGE_FEATURES),
    JS_CFUNC_MAGIC_DEF("buildMap", 1, js_intelligent_scissors_method, METHOD_BUILD_MAP),
    JS_CFUNC_MAGIC_DEF("getContour", 2, js_intelligent_scissors_method, METHOD_GET_CONTOUR),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "segmentation_IntelligentScissorsMB", JS_PROP_CONFIGURABLE),
};

extern "C" int
js_intelligent_scissors_init(JSContext* ctx, JSModuleDef* m) {
  if(js_intelligent_scissors_class_id == 0) {
    JS_NewClassID(&js_intelligent_scissors_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_intelligent_scissors_class_id, &js_intelligent_scissors_class);

    intelligent_scissors_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, intelligent_scissors_proto, js_intelligent_scissors_proto_funcs, countof(js_intelligent_scissors_proto_funcs));
    JS_SetClassProto(ctx, js_intelligent_scissors_class_id, intelligent_scissors_proto);

    intelligent_scissors_class =
        JS_NewCFunction2(ctx, js_intelligent_scissors_constructor, "segmentation_IntelligentScissorsMB", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, intelligent_scissors_class, intelligent_scissors_proto);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "segmentation_IntelligentScissorsMB", intelligent_scissors_class);

  return 0;
}

extern "C" void
js_intelligent_scissors_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "segmentation_IntelligentScissorsMB");
}

#endif /* defined(HAVE_OPENCV2_PHOTO_SEGMENTATION_HPP) */
