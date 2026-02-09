#include "js_cv.hpp"
#include "js_umat.hpp"
#include "js_contour.hpp"
#include "jsbindings.hpp"
#include <quickjs.h>

#ifdef HAVE_OPENCV2_XIMGPROC_HPP
#include <opencv2/ximgproc.hpp>
#ifdef HAVE_OPENCV2_XIMGPROC_FIND_ELLIPSES_HPP
#include <opencv2/ximgproc/find_ellipses.hpp>
#endif
#include <opencv2/ximgproc/segmentation.hpp>

/**
 * Wrapping https://docs.opencv.org/4.7.0/d9/d29/namespacecv_1_1ximgproc.html
 *
 */

typedef cv::Ptr<cv::ximgproc::EdgeDrawing> JSEdgeDrawingData;

extern "C" {
thread_local JSValue edge_drawing_proto = JS_UNDEFINED, edge_drawing_class = JS_UNDEFINED, edge_drawing_params_proto = JS_UNDEFINED,
                     edge_drawing_params_class = JS_UNDEFINED;
thread_local JSClassID js_edge_drawing_class_id = 0;
}

JSEdgeDrawingData*
js_edge_drawing_data(JSValueConst val) {
  return static_cast<JSEdgeDrawingData*>(JS_GetOpaque(val, js_edge_drawing_class_id));
}

JSEdgeDrawingData*
js_edge_drawing_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSEdgeDrawingData*>(JS_GetOpaque2(ctx, val, js_edge_drawing_class_id));
}

static JSValue
js_edge_drawing_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSEdgeDrawingData* ed;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(ed = js_allocate<JSEdgeDrawingData>(ctx)))
    return JS_EXCEPTION;

  *ed = cv::ximgproc::createEdgeDrawing();

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_edge_drawing_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, ed);

  return obj;

fail:
  js_deallocate(ctx, ed);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

void
js_edge_drawing_finalizer(JSRuntime* rt, JSValue val) {
  JSEdgeDrawingData* ed;

  if((ed = js_edge_drawing_data(val))) {
    cv::ximgproc::EdgeDrawing* ptr = ed->get();

    ptr->~EdgeDrawing();

    js_deallocate(rt, ed);
  }
}

enum {
  PARAM_ANCHORTHRESHOLDVALUE,
  PARAM_EDGEDETECTIONOPERATOR,
  PARAM_GRADIENTTHRESHOLDVALUE,
  PARAM_LINEFITERRORTHRESHOLD,
  PARAM_MAXDISTANCEBETWEENTWOLINES,
  PARAM_MAXERRORTHRESHOLD,
  PARAM_MINLINELENGTH,
  PARAM_MINPATHLENGTH,
  PARAM_NFAVALIDATION,
  PARAM_PFMODE,
  PARAM_SCANINTERVAL,
  PARAM_SIGMA,
  PARAM_SUMFLAG,
};

static JSValue
js_edge_drawing_params_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSEdgeDrawingData* ed;
  JSValue ret = JS_UNDEFINED;

  if(!(ed = js_edge_drawing_data2(ctx, this_val)))
    return JS_EXCEPTION;

  auto const& params = ed->get()->params;

  switch(magic) {
    case PARAM_ANCHORTHRESHOLDVALUE: {
      ret = js_value_from(ctx, params.AnchorThresholdValue);
      break;
    }

    case PARAM_EDGEDETECTIONOPERATOR: {
      ret = js_value_from(ctx, params.EdgeDetectionOperator);
      break;
    }

    case PARAM_GRADIENTTHRESHOLDVALUE: {
      ret = js_value_from(ctx, params.GradientThresholdValue);
      break;
    }

    case PARAM_LINEFITERRORTHRESHOLD: {
      ret = js_value_from(ctx, params.LineFitErrorThreshold);
      break;
    }

    case PARAM_MAXDISTANCEBETWEENTWOLINES: {
      ret = js_value_from(ctx, params.MaxDistanceBetweenTwoLines);
      break;
    }

    case PARAM_MAXERRORTHRESHOLD: {
      ret = js_value_from(ctx, params.MaxErrorThreshold);
      break;
    }

    case PARAM_MINLINELENGTH: {
      ret = js_value_from(ctx, params.MinLineLength);
      break;
    }

    case PARAM_MINPATHLENGTH: {
      ret = js_value_from(ctx, params.MinPathLength);
      break;
    }

    case PARAM_NFAVALIDATION: {
      ret = js_value_from(ctx, params.NFAValidation);
      break;
    }

    case PARAM_PFMODE: {
      ret = js_value_from(ctx, params.PFmode);
      break;
    }

    case PARAM_SCANINTERVAL: {
      ret = js_value_from(ctx, params.ScanInterval);
      break;
    }

    case PARAM_SIGMA: {
      ret = js_value_from(ctx, params.Sigma);
      break;
    }

    case PARAM_SUMFLAG: {
      ret = js_value_from(ctx, params.SumFlag);
      break;
    }
  }

  return ret;
}

static JSValue
js_edge_drawing_params_set(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic) {
  JSEdgeDrawingData* ed;
  JSValue ret = JS_UNDEFINED;

  if(!(ed = js_edge_drawing_data2(ctx, this_val)))
    return JS_EXCEPTION;

  auto& params = ed->get()->params;

  switch(magic) {
    case PARAM_ANCHORTHRESHOLDVALUE: {
      js_value_to(ctx, value, params.AnchorThresholdValue);
      break;
    }

    case PARAM_EDGEDETECTIONOPERATOR: {
      js_value_to(ctx, value, params.EdgeDetectionOperator);
      break;
    }

    case PARAM_GRADIENTTHRESHOLDVALUE: {
      js_value_to(ctx, value, params.GradientThresholdValue);
      break;
    }

    case PARAM_LINEFITERRORTHRESHOLD: {
      js_value_to(ctx, value, params.LineFitErrorThreshold);
      break;
    }

    case PARAM_MAXDISTANCEBETWEENTWOLINES: {
      js_value_to(ctx, value, params.MaxDistanceBetweenTwoLines);
      break;
    }

    case PARAM_MAXERRORTHRESHOLD: {
      js_value_to(ctx, value, params.MaxErrorThreshold);
      break;
    }

    case PARAM_MINLINELENGTH: {
      js_value_to(ctx, value, params.MinLineLength);
      break;
    }

    case PARAM_MINPATHLENGTH: {
      js_value_to(ctx, value, params.MinPathLength);
      break;
    }

    case PARAM_NFAVALIDATION: {
      js_value_to(ctx, value, params.NFAValidation);
      break;
    }

    case PARAM_PFMODE: {
      js_value_to(ctx, value, params.PFmode);
      break;
    }

    case PARAM_SCANINTERVAL: {
      js_value_to(ctx, value, params.ScanInterval);
      break;
    }

    case PARAM_SIGMA: {
      js_value_to(ctx, value, params.Sigma);
      break;
    }

    case PARAM_SUMFLAG: {
      js_value_to(ctx, value, params.SumFlag);
      break;
    }
  }

  return ret;
}

enum {
  EDGEDRAWING_DETECTEDGES,
  EDGEDRAWING_DETECTELLIPSES,
  EDGEDRAWING_DETECTLINES,
  EDGEDRAWING_GETEDGEIMAGE,
  EDGEDRAWING_GETGRADIENTIMAGE,
  EDGEDRAWING_GETSEGMENTINDICESOFLINES,
  EDGEDRAWING_GETSEGMENTS,
  EDGEDRAWING_PARAMS,
  EDGEDRAWING_SETPARAMS,
};

static JSValue
js_edge_drawing_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSEdgeDrawingData* ed;
  JSValue ret = JS_UNDEFINED;

  if(!(ed = js_edge_drawing_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case EDGEDRAWING_PARAMS: {
      ret = js_edge_drawing_constructor(ctx, edge_drawing_params_class, 0, 0);
      JSEdgeDrawingData* ed2 = js_edge_drawing_data(ret);

      *ed2 = *ed;
      break;
    }
  }

  return ret;
}

static JSValue
js_edge_drawing_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSEdgeDrawingData* ed;
  JSValue ret = JS_UNDEFINED;

  if(!(ed = js_edge_drawing_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case EDGEDRAWING_DETECTEDGES: {
      JSInputArray input = js_input_array(ctx, argv[0]);

      ed->get()->detectEdges(input);
      break;
    }

    case EDGEDRAWING_DETECTELLIPSES: {
      JSOutputArray output = js_cv_outputarray(ctx, argv[0]);

      ed->get()->detectEllipses(output);
      break;
    }

    case EDGEDRAWING_DETECTLINES: {
      JSOutputArray output = js_cv_outputarray(ctx, argv[0]);

      ed->get()->detectLines(output);
      break;
    }

    case EDGEDRAWING_GETEDGEIMAGE: {
      JSOutputArray output = js_cv_outputarray(ctx, argv[0]);

      ed->get()->getEdgeImage(output);
      break;
    }

    case EDGEDRAWING_GETGRADIENTIMAGE: {
      JSOutputArray output = js_cv_outputarray(ctx, argv[0]);

      ed->get()->getGradientImage(output);
      break;
    }

    case EDGEDRAWING_GETSEGMENTINDICESOFLINES: {
      std::vector<int> indices = ed->get()->getSegmentIndicesOfLines();

      ret = js_array_from(ctx, indices);
      break;
    }

    case EDGEDRAWING_GETSEGMENTS: {
      auto segments = ed->get()->getSegments();

      ret = js_contours_new(ctx, segments);
      break;
    }

    case EDGEDRAWING_SETPARAMS: {
      JSEdgeDrawingData* ed2 = js_edge_drawing_data(argv[0]);

      if(!ed2)
        return JS_ThrowTypeError(ctx, "argument 1 must be EdgeDrawingParams");

      ed->get()->setParams(ed2->get()->params);
      break;
    }
  }

  return ret;
}

JSClassDef js_edge_drawing_class = {
    .class_name = "EdgeDrawing",
    .finalizer = js_edge_drawing_finalizer,
};

JSClassDef js_edge_drawing_params_class = {
    .class_name = "EdgeDrawingParams",
    .finalizer = js_edge_drawing_finalizer,
};

const JSCFunctionListEntry js_edge_drawing_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("detectEdges", 1, js_edge_drawing_method, EDGEDRAWING_DETECTEDGES),
    JS_CFUNC_MAGIC_DEF("detectEllipses", 1, js_edge_drawing_method, EDGEDRAWING_DETECTELLIPSES),
    JS_CFUNC_MAGIC_DEF("detectLines", 1, js_edge_drawing_method, EDGEDRAWING_DETECTLINES),
    JS_CFUNC_MAGIC_DEF("getEdgeImage", 1, js_edge_drawing_method, EDGEDRAWING_GETEDGEIMAGE),
    JS_CFUNC_MAGIC_DEF("getGradientImage", 1, js_edge_drawing_method, EDGEDRAWING_GETGRADIENTIMAGE),
    JS_CFUNC_MAGIC_DEF("getSegmentIndicesOfLines", 0, js_edge_drawing_method, EDGEDRAWING_GETSEGMENTINDICESOFLINES),
    JS_CFUNC_MAGIC_DEF("getSegments", 0, js_edge_drawing_method, EDGEDRAWING_GETSEGMENTS),
    JS_CFUNC_MAGIC_DEF("setParams", 1, js_edge_drawing_method, EDGEDRAWING_SETPARAMS),

    JS_CGETSET_MAGIC_DEF("params", js_edge_drawing_get, 0, EDGEDRAWING_PARAMS),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "EdgeDrawing", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_edge_drawing_params_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("AnchorThresholdValue", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_ANCHORTHRESHOLDVALUE),
    JS_CGETSET_MAGIC_DEF("EdgeDetectionOperator", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_EDGEDETECTIONOPERATOR),
    JS_CGETSET_MAGIC_DEF("GradientThresholdValue", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_GRADIENTTHRESHOLDVALUE),
    JS_CGETSET_MAGIC_DEF("LineFitErrorThreshold", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_LINEFITERRORTHRESHOLD),
    JS_CGETSET_MAGIC_DEF("MaxDistanceBetweenTwoLines", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_MAXDISTANCEBETWEENTWOLINES),
    JS_CGETSET_MAGIC_DEF("MaxErrorThreshold", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_MAXERRORTHRESHOLD),
    JS_CGETSET_MAGIC_DEF("MinLineLength", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_MINLINELENGTH),
    JS_CGETSET_MAGIC_DEF("MinPathLength", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_MINPATHLENGTH),
    JS_CGETSET_MAGIC_DEF("NFAValidation", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_NFAVALIDATION),
    JS_CGETSET_MAGIC_DEF("PFmode", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_PFMODE),
    JS_CGETSET_MAGIC_DEF("ScanInterval", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_SCANINTERVAL),
    JS_CGETSET_MAGIC_DEF("Sigma", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_SIGMA),
    JS_CGETSET_MAGIC_DEF("SumFlag", js_edge_drawing_params_get, js_edge_drawing_params_set, PARAM_SUMFLAG),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "EdgeDrawingParams", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_edge_drawing_static_funcs[] = {};

typedef cv::Ptr<cv::ximgproc::StructuredEdgeDetection> JSStructuredEdgeDetectionData;

extern "C" {
thread_local JSValue structured_edge_detection_proto = JS_UNDEFINED, structured_edge_detection_class = JS_UNDEFINED,
                     structured_edge_detection_params_proto = JS_UNDEFINED;
thread_local JSClassID js_structured_edge_detection_class_id = 0;
}

JSStructuredEdgeDetectionData*
js_structured_edge_detection_data(JSValueConst val) {
  return static_cast<JSStructuredEdgeDetectionData*>(JS_GetOpaque(val, js_structured_edge_detection_class_id));
}

JSStructuredEdgeDetectionData*
js_structured_edge_detection_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSStructuredEdgeDetectionData*>(JS_GetOpaque2(ctx, val, js_structured_edge_detection_class_id));
}

static JSValue
js_structured_edge_detection_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSStructuredEdgeDetectionData* sed;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(sed = js_allocate<JSStructuredEdgeDetectionData>(ctx)))
    return JS_EXCEPTION;

  std::string model;

  js_value_to(ctx, argv[0], model);

  *sed = cv::ximgproc::createStructuredEdgeDetection(model);

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_structured_edge_detection_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, sed);

  return obj;

fail:
  js_deallocate(ctx, sed);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

void
js_structured_edge_detection_finalizer(JSRuntime* rt, JSValue val) {
  JSStructuredEdgeDetectionData* sed;

  if((sed = js_structured_edge_detection_data(val))) {
    cv::ximgproc::StructuredEdgeDetection* ptr = sed->get();

    ptr->~StructuredEdgeDetection();

    js_deallocate(rt, sed);
  }
}

enum {
  STRUCTUREDEDGEDETECTION_COMPUTEORIENTATION,
  STRUCTUREDEDGEDETECTION_DETECTEDGES,
  STRUCTUREDEDGEDETECTION_EDGESNMS,
};

static JSValue
js_structured_edge_detection_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSStructuredEdgeDetectionData* sed;
  JSValue ret = JS_UNDEFINED;

  if(!(sed = js_structured_edge_detection_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case STRUCTUREDEDGEDETECTION_COMPUTEORIENTATION: {
      JSInputArray input = js_input_array(ctx, argv[0]);
      JSOutputArray output = js_cv_outputarray(ctx, argv[1]);

      sed->get()->computeOrientation(input, output);
      break;
    }

    case STRUCTUREDEDGEDETECTION_DETECTEDGES: {
      JSInputArray input = js_input_array(ctx, argv[0]);
      JSOutputArray output = js_cv_outputarray(ctx, argv[1]);

      sed->get()->detectEdges(input, output);
      break;
    }

    case STRUCTUREDEDGEDETECTION_EDGESNMS: {
      JSInputArray edge_image = js_input_array(ctx, argv[0]);
      JSInputArray orientation_image = js_input_array(ctx, argv[1]);
      JSOutputArray dst = js_cv_outputarray(ctx, argv[2]);
      int32_t r = 2, s = 0;
      double m = 1;
      BOOL isParallel = TRUE;

      if(argc > 3)
        js_value_to(ctx, argv[3], r);
      if(argc > 4)
        js_value_to(ctx, argv[4], s);
      if(argc > 5)
        js_value_to(ctx, argv[5], m);
      if(argc > 6)
        js_value_to(ctx, argv[6], isParallel);

      sed->get()->edgesNms(edge_image, orientation_image, dst, r, s, m, isParallel);
      break;
    }
  }

  return ret;
}

JSClassDef js_structured_edge_detection_class = {
    .class_name = "StructuredEdgeDetection",
    .finalizer = js_structured_edge_detection_finalizer,
};

const JSCFunctionListEntry js_structured_edge_detection_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("computeOrientation", 2, js_structured_edge_detection_method, STRUCTUREDEDGEDETECTION_COMPUTEORIENTATION),
    JS_CFUNC_MAGIC_DEF("detectEdges", 2, js_structured_edge_detection_method, STRUCTUREDEDGEDETECTION_DETECTEDGES),
    JS_CFUNC_MAGIC_DEF("edgesNms", 3, js_structured_edge_detection_method, STRUCTUREDEDGEDETECTION_EDGESNMS),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "StructuredEdgeDetection", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_structured_edge_detection_static_funcs[] = {};

typedef cv::Ptr<cv::Algorithm> JSSuperpixelData;

extern "C" {
thread_local JSValue superpixel_proto = JS_UNDEFINED, superpixel_class = JS_UNDEFINED, superpixel_params_proto = JS_UNDEFINED;
thread_local JSClassID js_superpixel_class_id = 0;
}

JSSuperpixelData*
js_superpixel_data(JSValueConst val) {
  return static_cast<JSSuperpixelData*>(JS_GetOpaque(val, js_superpixel_class_id));
}

JSSuperpixelData*
js_superpixel_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSSuperpixelData*>(JS_GetOpaque2(ctx, val, js_superpixel_class_id));
}

static JSValue
js_superpixel_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSSuperpixelData* sp;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(sp = js_allocate<JSSuperpixelData>(ctx)))
    return JS_EXCEPTION;

  /*std::string model;

  if(argc > 0)
    js_value_to(ctx, argv[0], model);*/

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_superpixel_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, sp);

  return obj;

fail:
  js_deallocate(ctx, sp);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

void
js_superpixel_finalizer(JSRuntime* rt, JSValue val) {
  JSSuperpixelData* sp;

  if((sp = js_superpixel_data(val))) {
    cv::Algorithm* ptr = sp->get();

    cv::ximgproc::SuperpixelSLIC* slic;
    cv::ximgproc::SuperpixelLSC* lsc;
    cv::ximgproc::SuperpixelSEEDS* seeds;

    if((slic = dynamic_cast<cv::ximgproc::SuperpixelSLIC*>(ptr)))
      slic->~SuperpixelSLIC();
    else if((lsc = dynamic_cast<cv::ximgproc::SuperpixelLSC*>(ptr)))
      lsc->~SuperpixelLSC();
    else if((seeds = dynamic_cast<cv::ximgproc::SuperpixelSEEDS*>(ptr)))
      seeds->~SuperpixelSEEDS();

    js_deallocate(rt, sp);
  }
}

enum {
  SUPERPIXEL_GETNUMBEROFSUPERPIXELS,
  SUPERPIXEL_ITERATE,
  SUPERPIXEL_GETLABELS,
  SUPERPIXEL_GETLABELCONTOURMASK,

};

static JSValue
js_superpixel_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSSuperpixelData* sp;
  JSValue ret = JS_UNDEFINED;

  if(!(sp = js_superpixel_data2(ctx, this_val)))
    return JS_EXCEPTION;

  cv::ximgproc::SuperpixelSLIC* slic;
  cv::ximgproc::SuperpixelLSC* lsc;
  cv::ximgproc::SuperpixelSEEDS* seeds;

  switch(magic) {
    case SUPERPIXEL_GETNUMBEROFSUPERPIXELS: {
      int32_t num;
      if((slic = dynamic_cast<cv::ximgproc::SuperpixelSLIC*>(sp->get())))
        num = slic->getNumberOfSuperpixels();
      else if((lsc = dynamic_cast<cv::ximgproc::SuperpixelLSC*>(sp->get())))
        num = lsc->getNumberOfSuperpixels();
      else if((seeds = dynamic_cast<cv::ximgproc::SuperpixelSEEDS*>(sp->get())))
        num = seeds->getNumberOfSuperpixels();
      else {
        ret = JS_ThrowInternalError(ctx, "Superpixel is none of SLIC, LSC, SEEDS");
        break;
      }

      ret = js_value_from(ctx, num);
      break;
    }

    case SUPERPIXEL_ITERATE: {
      if((slic = dynamic_cast<cv::ximgproc::SuperpixelSLIC*>(sp->get()))) {
        int32_t num_iterations = 10;

        if(argc > 0)
          js_value_to(ctx, argv[0], num_iterations);

        slic->iterate(num_iterations);
      } else if((lsc = dynamic_cast<cv::ximgproc::SuperpixelLSC*>(sp->get()))) {
        int32_t num_iterations = 10;

        if(argc > 0)
          js_value_to(ctx, argv[0], num_iterations);

        lsc->iterate(num_iterations);
      } else if((seeds = dynamic_cast<cv::ximgproc::SuperpixelSEEDS*>(sp->get()))) {
        JSInputArray input = js_input_array(ctx, argv[0]);
        int32_t num_iterations = 4;

        if(argc > 1)
          js_value_to(ctx, argv[1], num_iterations);

        seeds->iterate(input, num_iterations);
      } else {
        ret = JS_ThrowInternalError(ctx, "Superpixel is none of SLIC, LSC, SEEDS");
      }

      break;
    }

    case SUPERPIXEL_GETLABELS: {
      JSOutputArray output = js_cv_outputarray(ctx, argv[0]);

      if((slic = dynamic_cast<cv::ximgproc::SuperpixelSLIC*>(sp->get())))
        slic->getLabels(output);
      else if((lsc = dynamic_cast<cv::ximgproc::SuperpixelLSC*>(sp->get())))
        lsc->getLabels(output);
      else if((seeds = dynamic_cast<cv::ximgproc::SuperpixelSEEDS*>(sp->get())))
        seeds->getLabels(output);
      else
        ret = JS_ThrowInternalError(ctx, "Superpixel is none of SLIC, LSC, SEEDS");

      break;
    }

    case SUPERPIXEL_GETLABELCONTOURMASK: {
      JSOutputArray output = js_cv_outputarray(ctx, argv[0]);
      BOOL thick_line = FALSE;

      if(argc > 1)
        js_value_to(ctx, argv[1], thick_line);

      if((slic = dynamic_cast<cv::ximgproc::SuperpixelSLIC*>(sp->get())))
        slic->getLabelContourMask(output, thick_line);
      else if((lsc = dynamic_cast<cv::ximgproc::SuperpixelLSC*>(sp->get())))
        lsc->getLabelContourMask(output, thick_line);
      else if((seeds = dynamic_cast<cv::ximgproc::SuperpixelSEEDS*>(sp->get())))
        seeds->getLabelContourMask(output, thick_line);
      else
        ret = JS_ThrowInternalError(ctx, "Superpixel is none of SLIC, LSC, SEEDS");

      break;
    }
  }

  return ret;
}

JSClassDef js_superpixel_class = {
    .class_name = "Superpixel",
    .finalizer = js_superpixel_finalizer,
};

const JSCFunctionListEntry js_superpixel_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("getNumberOfSuperpixels", 0, js_superpixel_method, SUPERPIXEL_GETNUMBEROFSUPERPIXELS),
    JS_CFUNC_MAGIC_DEF("iterate", 0, js_superpixel_method, SUPERPIXEL_ITERATE),
    JS_CFUNC_MAGIC_DEF("getLabels", 1, js_superpixel_method, SUPERPIXEL_GETLABELS),
    JS_CFUNC_MAGIC_DEF("getLabelContourMask", 1, js_superpixel_method, SUPERPIXEL_GETLABELCONTOURMASK),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Superpixel", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_superpixel_static_funcs[] = {};

typedef cv::Ptr<cv::ximgproc::EdgeBoxes> JSEdgeBoxesData;

extern "C" {
thread_local JSValue edgeboxes_proto = JS_UNDEFINED, edgeboxes_class = JS_UNDEFINED, edgeboxes_params_proto = JS_UNDEFINED;
thread_local JSClassID js_edgeboxes_class_id = 0;
}

JSEdgeBoxesData*
js_edgeboxes_data(JSValueConst val) {
  return static_cast<JSEdgeBoxesData*>(JS_GetOpaque(val, js_edgeboxes_class_id));
}

JSEdgeBoxesData*
js_edgeboxes_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSEdgeBoxesData*>(JS_GetOpaque2(ctx, val, js_edgeboxes_class_id));
}

static JSValue
js_edgeboxes_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSEdgeBoxesData* eb;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(eb = js_allocate<JSEdgeBoxesData>(ctx)))
    return JS_EXCEPTION;

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_edgeboxes_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, eb);

  return obj;

fail:
  js_deallocate(ctx, eb);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

void
js_edgeboxes_finalizer(JSRuntime* rt, JSValue val) {
  JSEdgeBoxesData* eb;

  if((eb = js_edgeboxes_data(val))) {
    cv::Algorithm* ptr = eb->get();

    ptr->~Algorithm();

    js_deallocate(rt, eb);
  }
}

enum {
  EDGEBOXES_GETALPHA,
  EDGEBOXES_GETBETA,
  EDGEBOXES_GETBOUNDINGBOXES,
  EDGEBOXES_GETCLUSTERMINMAG,
  EDGEBOXES_GETEDGEMERGETHR,
  EDGEBOXES_GETEDGEMINMAG,
  EDGEBOXES_GETETA,
  EDGEBOXES_GETGAMMA,
  EDGEBOXES_GETKAPPA,
  EDGEBOXES_GETMAXASPECTRATIO,
  EDGEBOXES_GETMAXBOXES,
  EDGEBOXES_GETMINBOXAREA,
  EDGEBOXES_GETMINSCORE,
  EDGEBOXES_SETALPHA,
  EDGEBOXES_SETBETA,
  EDGEBOXES_SETCLUSTERMINMAG,
  EDGEBOXES_SETEDGEMERGETHR,
  EDGEBOXES_SETEDGEMINMAG,
  EDGEBOXES_SETETA,
  EDGEBOXES_SETGAMMA,
  EDGEBOXES_SETKAPPA,
  EDGEBOXES_SETMAXASPECTRATIO,
  EDGEBOXES_SETMAXBOXES,
  EDGEBOXES_SETMINBOXAREA,
  EDGEBOXES_SETMINSCORE,
};

static JSValue
js_edgeboxes_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSEdgeBoxesData* eb;
  JSValue ret = JS_UNDEFINED;

  if(!(eb = js_edgeboxes_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case EDGEBOXES_GETALPHA: {
      ret = js_value_from(ctx, eb->get()->getAlpha());
      break;
    }

    case EDGEBOXES_GETBETA: {
      ret = js_value_from(ctx, eb->get()->getBeta());
      break;
    }

    case EDGEBOXES_GETBOUNDINGBOXES: {
      JSInputArray edge_map = js_input_array(ctx, argv[0]);
      JSInputArray orientation_map = js_input_array(ctx, argv[1]);
      JSOutputArray scores = cv::noArray();

      std::vector<JSRectData<int>> boxes;
      js_array_to(ctx, argv[2], boxes);

      if(argc > 3)
        scores = js_cv_outputarray(ctx, argv[3]);

      eb->get()->getBoundingBoxes(edge_map, orientation_map, boxes, scores);
      break;
    }

    case EDGEBOXES_GETCLUSTERMINMAG: {
      ret = js_value_from(ctx, eb->get()->getClusterMinMag());
      break;
    }

    case EDGEBOXES_GETEDGEMERGETHR: {
      ret = js_value_from(ctx, eb->get()->getEdgeMergeThr());
      break;
    }

    case EDGEBOXES_GETEDGEMINMAG: {
      ret = js_value_from(ctx, eb->get()->getEdgeMinMag());
      break;
    }

    case EDGEBOXES_GETETA: {
      ret = js_value_from(ctx, eb->get()->getEta());
      break;
    }

    case EDGEBOXES_GETGAMMA: {
      ret = js_value_from(ctx, eb->get()->getGamma());
      break;
    }

    case EDGEBOXES_GETKAPPA: {
      ret = js_value_from(ctx, eb->get()->getKappa());
      break;
    }

    case EDGEBOXES_GETMAXASPECTRATIO: {
      ret = js_value_from(ctx, eb->get()->getMaxAspectRatio());
      break;
    }

    case EDGEBOXES_GETMAXBOXES: {
      ret = js_value_from(ctx, eb->get()->getMaxBoxes());
      break;
    }

    case EDGEBOXES_GETMINBOXAREA: {
      ret = js_value_from(ctx, eb->get()->getMinBoxArea());
      break;
    }

    case EDGEBOXES_GETMINSCORE: {
      ret = js_value_from(ctx, eb->get()->getMinScore());
      break;
    }

    case EDGEBOXES_SETALPHA: {
      eb->get()->setAlpha(js_value_to<double>(ctx, argv[0]));
      break;
    }

    case EDGEBOXES_SETBETA: {
      eb->get()->setBeta(js_value_to<double>(ctx, argv[0]));
      break;
    }

    case EDGEBOXES_SETCLUSTERMINMAG: {
      eb->get()->setClusterMinMag(js_value_to<double>(ctx, argv[0]));
      break;
    }

    case EDGEBOXES_SETEDGEMERGETHR: {
      eb->get()->setEdgeMergeThr(js_value_to<double>(ctx, argv[0]));
      break;
    }

    case EDGEBOXES_SETEDGEMINMAG: {
      eb->get()->setEdgeMinMag(js_value_to<double>(ctx, argv[0]));
      break;
    }

    case EDGEBOXES_SETETA: {
      eb->get()->setEta(js_value_to<double>(ctx, argv[0]));
      break;
    }

    case EDGEBOXES_SETGAMMA: {
      eb->get()->setGamma(js_value_to<double>(ctx, argv[0]));
      break;
    }

    case EDGEBOXES_SETKAPPA: {
      eb->get()->setKappa(js_value_to<double>(ctx, argv[0]));
      break;
    }

    case EDGEBOXES_SETMAXASPECTRATIO: {
      eb->get()->setMaxAspectRatio(js_value_to<double>(ctx, argv[0]));
      break;
    }

    case EDGEBOXES_SETMAXBOXES: {
      eb->get()->setMaxBoxes(js_value_to<int32_t>(ctx, argv[0]));
      break;
    }

    case EDGEBOXES_SETMINBOXAREA: {
      eb->get()->setMinBoxArea(js_value_to<double>(ctx, argv[0]));
      break;
    }

    case EDGEBOXES_SETMINSCORE: {
      eb->get()->setMinScore(js_value_to<double>(ctx, argv[0]));
      break;
    }
  }

  return ret;
}

JSClassDef js_edgeboxes_class = {
    .class_name = "EdgeBoxes",
    .finalizer = js_edgeboxes_finalizer,
};

const JSCFunctionListEntry js_edgeboxes_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("getAlpha", 0, js_edgeboxes_method, EDGEBOXES_GETALPHA),
    JS_CFUNC_MAGIC_DEF("getBeta", 0, js_edgeboxes_method, EDGEBOXES_GETBETA),
    JS_CFUNC_MAGIC_DEF("getBoundingBoxes", 0, js_edgeboxes_method, EDGEBOXES_GETBOUNDINGBOXES),
    JS_CFUNC_MAGIC_DEF("getClusterMinMag", 0, js_edgeboxes_method, EDGEBOXES_GETCLUSTERMINMAG),
    JS_CFUNC_MAGIC_DEF("getEdgeMergeThr", 0, js_edgeboxes_method, EDGEBOXES_GETEDGEMERGETHR),
    JS_CFUNC_MAGIC_DEF("getEdgeMinMag", 0, js_edgeboxes_method, EDGEBOXES_GETEDGEMINMAG),
    JS_CFUNC_MAGIC_DEF("getEta", 0, js_edgeboxes_method, EDGEBOXES_GETETA),
    JS_CFUNC_MAGIC_DEF("getGamma", 0, js_edgeboxes_method, EDGEBOXES_GETGAMMA),
    JS_CFUNC_MAGIC_DEF("getKappa", 0, js_edgeboxes_method, EDGEBOXES_GETKAPPA),
    JS_CFUNC_MAGIC_DEF("getMaxAspectRatio", 0, js_edgeboxes_method, EDGEBOXES_GETMAXASPECTRATIO),
    JS_CFUNC_MAGIC_DEF("getMaxBoxes", 0, js_edgeboxes_method, EDGEBOXES_GETMAXBOXES),
    JS_CFUNC_MAGIC_DEF("getMinBoxArea", 0, js_edgeboxes_method, EDGEBOXES_GETMINBOXAREA),
    JS_CFUNC_MAGIC_DEF("getMinScore", 0, js_edgeboxes_method, EDGEBOXES_GETMINSCORE),
    JS_CFUNC_MAGIC_DEF("setAlpha", 0, js_edgeboxes_method, EDGEBOXES_SETALPHA),
    JS_CFUNC_MAGIC_DEF("setBeta", 0, js_edgeboxes_method, EDGEBOXES_SETBETA),
    JS_CFUNC_MAGIC_DEF("setClusterMinMag", 0, js_edgeboxes_method, EDGEBOXES_SETCLUSTERMINMAG),
    JS_CFUNC_MAGIC_DEF("setEdgeMergeThr", 0, js_edgeboxes_method, EDGEBOXES_SETEDGEMERGETHR),
    JS_CFUNC_MAGIC_DEF("setEdgeMinMag", 0, js_edgeboxes_method, EDGEBOXES_SETEDGEMINMAG),
    JS_CFUNC_MAGIC_DEF("setEta", 0, js_edgeboxes_method, EDGEBOXES_SETETA),
    JS_CFUNC_MAGIC_DEF("setGamma", 0, js_edgeboxes_method, EDGEBOXES_SETGAMMA),
    JS_CFUNC_MAGIC_DEF("setKappa", 0, js_edgeboxes_method, EDGEBOXES_SETKAPPA),
    JS_CFUNC_MAGIC_DEF("setMaxAspectRatio", 0, js_edgeboxes_method, EDGEBOXES_SETMAXASPECTRATIO),
    JS_CFUNC_MAGIC_DEF("setMaxBoxes", 0, js_edgeboxes_method, EDGEBOXES_SETMAXBOXES),
    JS_CFUNC_MAGIC_DEF("setMinBoxArea", 0, js_edgeboxes_method, EDGEBOXES_SETMINBOXAREA),
    JS_CFUNC_MAGIC_DEF("setMinScore", 0, js_edgeboxes_method, EDGEBOXES_SETMINSCORE),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "EdgeBoxes", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_edgeboxes_static_funcs[] = {};

typedef cv::Ptr<cv::ximgproc::segmentation::GraphSegmentation> JSGraphSegmentationData;

extern "C" {
thread_local JSValue graph_segmentation_proto = JS_UNDEFINED, graph_segmentation_class = JS_UNDEFINED, graph_segmentation_params_proto = JS_UNDEFINED;
thread_local JSClassID js_graph_segmentation_class_id = 0;
}

JSGraphSegmentationData*
js_graph_segmentation_data(JSValueConst val) {
  return static_cast<JSGraphSegmentationData*>(JS_GetOpaque(val, js_graph_segmentation_class_id));
}

JSGraphSegmentationData*
js_graph_segmentation_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSGraphSegmentationData*>(JS_GetOpaque2(ctx, val, js_graph_segmentation_class_id));
}

JSValue
js_graph_segmentation_wrap(JSContext* ctx, JSValueConst proto, const JSGraphSegmentationData& arg) {
  JSGraphSegmentationData* gs;

  if(!(gs = js_allocate<JSGraphSegmentationData>(ctx)))
    return JS_EXCEPTION;

  JSValue obj = JS_NewObjectProtoClass(ctx, proto, js_graph_segmentation_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  *gs = arg;

  JS_SetOpaque(obj, gs);

  return obj;

fail:
  js_deallocate(ctx, gs);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

static JSValue
js_graph_segmentation_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSGraphSegmentationData* gs;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(gs = js_allocate<JSGraphSegmentationData>(ctx)))
    return JS_EXCEPTION;

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_graph_segmentation_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, gs);

  return obj;

fail:
  js_deallocate(ctx, gs);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

void
js_graph_segmentation_finalizer(JSRuntime* rt, JSValue val) {
  JSGraphSegmentationData* gs;

  if((gs = js_graph_segmentation_data(val))) {
    cv::Algorithm* ptr = gs->get();

    ptr->~Algorithm();

    js_deallocate(rt, gs);
  }
}

enum {

};

static JSValue
js_graph_segmentation_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSGraphSegmentationData* gs;
  JSValue ret = JS_UNDEFINED;

  if(!(gs = js_graph_segmentation_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {}

  return ret;
}

JSClassDef js_graph_segmentation_class = {
    .class_name = "GraphSegmentation",
    .finalizer = js_graph_segmentation_finalizer,
};

const JSCFunctionListEntry js_graph_segmentation_proto_funcs[] = {

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "GraphSegmentation", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_graph_segmentation_static_funcs[] = {};

typedef cv::Ptr<cv::ximgproc::segmentation::SelectiveSearchSegmentation> JSSelectiveSearchSegmentationData;
typedef cv::Ptr<cv::ximgproc::segmentation::SelectiveSearchSegmentationStrategy> JSSegmentationStrategyData;

extern "C" {
thread_local JSValue selective_search_segmentation_proto = JS_UNDEFINED, selective_search_segmentation_class = JS_UNDEFINED,
                     selective_search_segmentation_params_proto = JS_UNDEFINED;
thread_local JSClassID js_search_segmentation_class_id = 0;

thread_local JSValue segmentation_strategy_proto = JS_UNDEFINED, segmentation_strategy_class = JS_UNDEFINED, segmentation_strategy_params_proto = JS_UNDEFINED;
thread_local JSClassID js_segmentation_strategy_class_id = 0;
}

JSSelectiveSearchSegmentationData*
js_search_segmentation_data(JSValueConst val) {
  return static_cast<JSSelectiveSearchSegmentationData*>(JS_GetOpaque(val, js_search_segmentation_class_id));
}

JSSelectiveSearchSegmentationData*
js_search_segmentation_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSSelectiveSearchSegmentationData*>(JS_GetOpaque2(ctx, val, js_search_segmentation_class_id));
}

JSSegmentationStrategyData*
js_segmentation_strategy_data(JSValueConst val) {
  return static_cast<JSSegmentationStrategyData*>(JS_GetOpaque(val, js_segmentation_strategy_class_id));
}

JSSegmentationStrategyData*
js_segmentation_strategy_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSSegmentationStrategyData*>(JS_GetOpaque2(ctx, val, js_segmentation_strategy_class_id));
}

template<class T>
JSValue
js_search_segmentation_wrap(JSContext* ctx, JSValueConst proto, const cv::Ptr<T>& arg) {
  cv::Ptr<T>* sss;

  if(!(sss = js_allocate<cv::Ptr<T>>(ctx)))
    return JS_EXCEPTION;

  JSValue obj = JS_NewObjectProtoClass(ctx, proto, js_search_segmentation_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  new(sss) cv::Ptr<T>(arg);

  JS_SetOpaque(obj, sss);

  return obj;

fail:
  js_deallocate(ctx, sss);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

static JSValue
js_search_segmentation_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSSelectiveSearchSegmentationData* sss;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(sss = js_allocate<JSSelectiveSearchSegmentationData>(ctx)))
    return JS_EXCEPTION;

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_search_segmentation_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, sss);

  return obj;

fail:
  js_deallocate(ctx, sss);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

void
js_search_segmentation_finalizer(JSRuntime* rt, JSValue val) {
  JSSelectiveSearchSegmentationData* sss;

  if((sss = js_search_segmentation_data(val))) {
    cv::Algorithm* ptr = sss->get();

    ptr->~Algorithm();

    js_deallocate(rt, sss);
  }
}

enum {
  SEARCH_SEGMENTATION_ADD_GRAPH_SEGMENTATION = 0,
  SEARCH_SEGMENTATION_ADD_IMAGE,
  SEARCH_SEGMENTATION_ADD_STRATEGY,
  SEARCH_SEGMENTATION_CLEAR_GRAPH_SEGMENTATIONS,
  SEARCH_SEGMENTATION_CLEAR_IMAGES,
  SEARCH_SEGMENTATION_CLEAR_STRATEGIES,
  SEARCH_SEGMENTATION_PROCESS,
  SEARCH_SEGMENTATION_SET_BASE_IMAGE,
  SEARCH_SEGMENTATION_SWITCH_TO_SELECTIVE_SEARCH_FAST,
  SEARCH_SEGMENTATION_SWITCH_TO_SELECTIVE_SEARCH_QUALITY,
  SEARCH_SEGMENTATION_SWITCH_TO_SINGLE_STRATEGY,

};

static JSValue
js_search_segmentation_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSSelectiveSearchSegmentationData* sss;
  JSValue ret = JS_UNDEFINED;

  if(!(sss = js_search_segmentation_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case SEARCH_SEGMENTATION_ADD_GRAPH_SEGMENTATION: {

      JSGraphSegmentationData* graph_segmentation;

      if(!(graph_segmentation = js_graph_segmentation_data2(ctx, argv[0])))
        return JS_ThrowTypeError(ctx, "Argument 1 must be a GraphSegmentation");

      sss->get()->addGraphSegmentation(*graph_segmentation);
      break;
    }

    case SEARCH_SEGMENTATION_ADD_IMAGE: {
      JSInputArray image = js_input_array(ctx, argv[0]);

      sss->get()->addImage(image);
      break;
    }

    case SEARCH_SEGMENTATION_ADD_STRATEGY: {
      JSSegmentationStrategyData* search_strategy;

      if(!(search_strategy = js_segmentation_strategy_data2(ctx, argv[0])))
        return JS_ThrowTypeError(ctx, "Argument 1 must be a SelectiveSearchSegmentationStrategy");

      sss->get()->addStrategy(*search_strategy);
      break;
    }

    case SEARCH_SEGMENTATION_CLEAR_GRAPH_SEGMENTATIONS: {
      sss->get()->clearGraphSegmentations();
      break;
    }

    case SEARCH_SEGMENTATION_CLEAR_IMAGES: {
      sss->get()->clearImages();
      break;
    }

    case SEARCH_SEGMENTATION_CLEAR_STRATEGIES: {
      sss->get()->clearStrategies();
      break;
    }

    case SEARCH_SEGMENTATION_PROCESS: {
      std::vector<cv::Rect> rects;

      js_array_to(ctx, argv[0], rects);

      sss->get()->process(rects);

      break;
    }

    case SEARCH_SEGMENTATION_SET_BASE_IMAGE: {
      JSInputArray image = js_input_array(ctx, argv[0]);

      sss->get()->setBaseImage(image);
      break;
    }

    case SEARCH_SEGMENTATION_SWITCH_TO_SELECTIVE_SEARCH_FAST: {
      int32_t base_k = -150, inc_k = 150;
      double sigma = 0.8;

      if(argc > 0)
        JS_ToInt32(ctx, &base_k, argv[0]);
      if(argc > 1)
        JS_ToInt32(ctx, &inc_k, argv[1]);
      if(argc > 2)
        JS_ToFloat64(ctx, &sigma, argv[2]);

      sss->get()->switchToSelectiveSearchFast(base_k, inc_k, sigma);
      break;
    }

    case SEARCH_SEGMENTATION_SWITCH_TO_SELECTIVE_SEARCH_QUALITY: {
      int32_t base_k = -150, inc_k = 150;
      double sigma = 0.8;

      if(argc > 0)
        JS_ToInt32(ctx, &base_k, argv[0]);
      if(argc > 1)
        JS_ToInt32(ctx, &inc_k, argv[1]);
      if(argc > 2)
        JS_ToFloat64(ctx, &sigma, argv[2]);

      sss->get()->switchToSelectiveSearchQuality(base_k, inc_k, sigma);
      break;
    }

    case SEARCH_SEGMENTATION_SWITCH_TO_SINGLE_STRATEGY: {
      int32_t k = 200;
      double sigma = 0.8;

      if(argc > 0)
        JS_ToInt32(ctx, &k, argv[0]);
      if(argc > 1)
        JS_ToFloat64(ctx, &sigma, argv[1]);

      sss->get()->switchToSingleStrategy(k, sigma);
      break;
    }
  }

  return ret;
}

JSClassDef js_search_segmentation_class = {
    .class_name = "SelectiveSearchSegmentation",
    .finalizer = js_search_segmentation_finalizer,
};

const JSCFunctionListEntry js_search_segmentation_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("addGraphSegmentation)", 0, js_search_segmentation_method, SEARCH_SEGMENTATION_ADD_GRAPH_SEGMENTATION),
    JS_CFUNC_MAGIC_DEF("addImage)", 0, js_search_segmentation_method, SEARCH_SEGMENTATION_ADD_IMAGE),
    JS_CFUNC_MAGIC_DEF("addStrategy)", 0, js_search_segmentation_method, SEARCH_SEGMENTATION_ADD_STRATEGY),
    JS_CFUNC_MAGIC_DEF("clearGraphSegmentations)", 0, js_search_segmentation_method, SEARCH_SEGMENTATION_CLEAR_GRAPH_SEGMENTATIONS),
    JS_CFUNC_MAGIC_DEF("clearImages)", 0, js_search_segmentation_method, SEARCH_SEGMENTATION_CLEAR_IMAGES),
    JS_CFUNC_MAGIC_DEF("clearStrategies)", 0, js_search_segmentation_method, SEARCH_SEGMENTATION_CLEAR_STRATEGIES),
    JS_CFUNC_MAGIC_DEF("process)", 0, js_search_segmentation_method, SEARCH_SEGMENTATION_PROCESS),
    JS_CFUNC_MAGIC_DEF("setBaseImage)", 0, js_search_segmentation_method, SEARCH_SEGMENTATION_SET_BASE_IMAGE),
    JS_CFUNC_MAGIC_DEF("switchToSelectiveSearchFast)", 0, js_search_segmentation_method, SEARCH_SEGMENTATION_SWITCH_TO_SELECTIVE_SEARCH_FAST),
    JS_CFUNC_MAGIC_DEF("switchToSelectiveSearchQuality)", 0, js_search_segmentation_method, SEARCH_SEGMENTATION_SWITCH_TO_SELECTIVE_SEARCH_QUALITY),
    JS_CFUNC_MAGIC_DEF("switchToSingleStrategy)", 0, js_search_segmentation_method, SEARCH_SEGMENTATION_SWITCH_TO_SINGLE_STRATEGY),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "SelectiveSearchSegmentation", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_search_segmentation_static_funcs[] = {};

template<class T>
JSValue
js_segmentation_strategy_wrap(JSContext* ctx, JSValueConst proto, const cv::Ptr<T>& arg) {
  cv::Ptr<T>* ssss;

  if(!(ssss = js_allocate<cv::Ptr<T>>(ctx)))
    return JS_EXCEPTION;

  JSValue obj = JS_NewObjectProtoClass(ctx, proto, js_segmentation_strategy_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  new(ssss) cv::Ptr<T>(arg);

  JS_SetOpaque(obj, ssss);

  return obj;

fail:
  js_deallocate(ctx, ssss);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

static JSValue
js_segmentation_strategy_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSSegmentationStrategyData* ssss;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(ssss = js_allocate<JSSegmentationStrategyData>(ctx)))
    return JS_EXCEPTION;

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_segmentation_strategy_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, ssss);

  return obj;

fail:
  js_deallocate(ctx, ssss);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

void
js_segmentation_strategy_finalizer(JSRuntime* rt, JSValue val) {
  JSSegmentationStrategyData* ssss;

  if((ssss = js_segmentation_strategy_data(val))) {
    cv::Algorithm* ptr = ssss->get();

    ptr->~Algorithm();

    js_deallocate(rt, ssss);
  }
}

enum {
  SELECTIVE_SEARCH_SEGMENTATION_GET = 0,
  SELECTIVE_SEARCH_SEGMENTATION_MERGE,
  SELECTIVE_SEARCH_SEGMENTATION_SET_IMAGE,
};

static JSValue
js_segmentation_strategy_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSSegmentationStrategyData* ssss;
  JSValue ret = JS_UNDEFINED;

  if(!(ssss = js_segmentation_strategy_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case SELECTIVE_SEARCH_SEGMENTATION_GET: {
      int32_t r1, r2;
      JS_ToInt32(ctx, &r1, argv[0]);
      JS_ToInt32(ctx, &r2, argv[1]);

      ret = JS_NewFloat64(ctx, ssss->get()->get(r1, r2));
      break;
    }

    case SELECTIVE_SEARCH_SEGMENTATION_MERGE: {
      int32_t r1, r2;
      JS_ToInt32(ctx, &r1, argv[0]);
      JS_ToInt32(ctx, &r2, argv[1]);

      ssss->get()->merge(r1, r2);
      break;
    }

    case SELECTIVE_SEARCH_SEGMENTATION_SET_IMAGE: {
      JSInputArray img = js_input_array(ctx, argv[0]);
      JSInputArray regions = js_input_array(ctx, argv[1]);
      JSInputArray sizes = js_input_array(ctx, argv[2]);
      int32_t image_id = -1;

      if(argc > 3)
        JS_ToInt32(ctx, &image_id, argv[3]);

      ssss->get()->setImage(img, regions, sizes);
      break;
    }
  }

  return ret;
}

JSClassDef js_segmentation_strategy_class = {
    .class_name = "SelectiveSearchSegmentationStrategy",
    .finalizer = js_segmentation_strategy_finalizer,
};

const JSCFunctionListEntry js_segmentation_strategy_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("get", 2, js_segmentation_strategy_method, SELECTIVE_SEARCH_SEGMENTATION_GET),
    JS_CFUNC_MAGIC_DEF("merge", 2, js_segmentation_strategy_method, SELECTIVE_SEARCH_SEGMENTATION_MERGE),
    JS_CFUNC_MAGIC_DEF("setImage", 3, js_segmentation_strategy_method, SELECTIVE_SEARCH_SEGMENTATION_SET_IMAGE),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "SelectiveSearchSegmentationStrategy", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_segmentation_strategy_static_funcs[] = {};

enum {
  CREATE_GRAPH_SEGMENTATION,
  CREATE_SELECTIVE_SEARCH_SEGMENTATION,
  CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_COLOR,
  CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_FILL,
  CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_MULTIPLE,
  CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_SIZE,
  CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_TEXTURE,
};

static JSValue
js_segmentation_function(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSGraphSegmentationData* gs;
  JSValue ret = JS_UNDEFINED;

  if(!(gs = js_graph_segmentation_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case CREATE_GRAPH_SEGMENTATION: {
      double sigma = 0.5, k = 300;
      int32_t min_size = 100;

      if(argc > 0)
        JS_ToFloat64(ctx, &sigma, argv[0]);

      if(argc > 1)
        JS_ToFloat64(ctx, &k, argv[1]);

      if(argc > 2)
        JS_ToInt32(ctx, &min_size, argv[2]);

      ret = js_graph_segmentation_wrap(ctx, graph_segmentation_proto, cv::ximgproc::segmentation::createGraphSegmentation(sigma, k, min_size));
      break;
    }

    case CREATE_SELECTIVE_SEARCH_SEGMENTATION: {
      ret = js_search_segmentation_wrap(ctx, selective_search_segmentation_proto, cv::ximgproc::segmentation::createSelectiveSearchSegmentation());
      break;
    }

    case CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_COLOR: {
      ret = js_search_segmentation_wrap(ctx, selective_search_segmentation_proto, cv::ximgproc::segmentation::createSelectiveSearchSegmentationStrategyColor());
      break;
    }

    case CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_FILL: {
      ret = js_search_segmentation_wrap(ctx, selective_search_segmentation_proto, cv::ximgproc::segmentation::createSelectiveSearchSegmentationStrategyFill());
      break;
    }

    case CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_MULTIPLE: {
      ret = js_search_segmentation_wrap(ctx,
                                        selective_search_segmentation_proto,
                                        cv::ximgproc::segmentation::createSelectiveSearchSegmentationStrategyMultiple());
      break;
    }

    case CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_SIZE: {
      ret = js_search_segmentation_wrap(ctx, selective_search_segmentation_proto, cv::ximgproc::segmentation::createSelectiveSearchSegmentationStrategySize());
      break;
    }

    case CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_TEXTURE: {
      ret =
          js_search_segmentation_wrap(ctx, selective_search_segmentation_proto, cv::ximgproc::segmentation::createSelectiveSearchSegmentationStrategyTexture());
      break;
    }
  }

  return ret;
}

enum {
  XIMGPROC_ANISOTROPIC_DIFFUSION,
  XIMGPROC_EDGE_PRESERVING_FILTER,
  XIMGPROC_FIND_ELLIPSES,
  XIMGPROC_THINNING,
  XIMGPROC_NI_BLACK_THRESHOLD,
  XIMGPROC_PEI_LIN_NORMALIZATION,
  XIMGPROC_CREATEEDGEDRAWING,
  XIMGPROC_CONTOURSAMPLING,
  XIMGPROC_COVARIANCEESTIMATION,
  XIMGPROC_COLORMATCHTEMPLATE,
  XIMGPROC_TRANSFORMFD,
  XIMGPROC_FOURIERDESCRIPTOR,
  XIMGPROC_WEIGHTEDMEDIANFILTER,
  XIMGPROC_ROLLINGGUIDANCEFILTER,
  XIMGPROC_CREATESTRUCTUREDEDGEDETECTION,
  XIMGPROC_CREATESUPERPIXELSLIC,
  XIMGPROC_CREATESUPERPIXELLSC,
  XIMGPROC_CREATESUPERPIXELSEEDS,
  XIMGPROC_CREATEEDGEBOXES,
  XIMGPROC_FASTHOUGHTRANSFORM,
  XIMGPROC_HOUGHPOINT2LINE,
};

static JSValue
js_ximgproc_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  JSImageArgument src(ctx, argv[0]);
  JSImageArgument dst(ctx, argv[1]);

  try {
    switch(magic) {
      case XIMGPROC_THINNING: {
        int32_t flags = cv::ximgproc::THINNING_ZHANGSUEN;

        if(argc > 2)
          JS_ToInt32(ctx, &flags, argv[2]);

        cv::ximgproc::thinning(src, dst, flags);
        break;
      }

      case XIMGPROC_EDGE_PRESERVING_FILTER: {
        int32_t d;
        double threshold;
        JS_ToInt32(ctx, &d, argv[2]);
        JS_ToFloat64(ctx, &threshold, argv[3]);

        cv::ximgproc::edgePreservingFilter(src, dst, d, threshold);
        break;
      }

#ifdef HAVE_OPENCV2_XIMGPROC_FIND_ELLIPSES_HPP
      case XIMGPROC_FIND_ELLIPSES: {
        double scoreThreshold = 0.7, reliabilityThreshold = 0.5, centerDistanceThreshold = 0.05;

        JS_ToFloat64(ctx, &scoreThreshold, argv[2]);
        JS_ToFloat64(ctx, &reliabilityThreshold, argv[3]);
        JS_ToFloat64(ctx, &centerDistanceThreshold, argv[4]);

        cv::ximgproc::findEllipses(src, dst, scoreThreshold, reliabilityThreshold, centerDistanceThreshold);
        break;
      }
#endif

      case XIMGPROC_PEI_LIN_NORMALIZATION: {
        cv::ximgproc::PeiLinNormalization(src, dst);
        break;
      }

      case XIMGPROC_NI_BLACK_THRESHOLD: {
        int32_t type = -1, blockSize = -1, binarizationMethod = cv::ximgproc::BINARIZATION_NIBLACK;
        double maxValue, k, r = 128;

        JS_ToFloat64(ctx, &maxValue, argv[2]);
        JS_ToInt32(ctx, &type, argv[3]);
        JS_ToInt32(ctx, &blockSize, argv[4]);
        JS_ToFloat64(ctx, &k, argv[5]);

        if(argc > 6)
          JS_ToInt32(ctx, &binarizationMethod, argv[6]);

        if(argc > 7)
          JS_ToFloat64(ctx, &r, argv[7]);

        cv::ximgproc::niBlackThreshold(src, dst, maxValue, type, blockSize, k, binarizationMethod, r);
        break;
      }

      case XIMGPROC_ANISOTROPIC_DIFFUSION: {
        double alpha = 0, K = 0;
        int32_t niters = -1;
        JS_ToFloat64(ctx, &alpha, argv[2]);
        JS_ToFloat64(ctx, &K, argv[3]);
        JS_ToInt32(ctx, &niters, argv[3]);

        cv::ximgproc::anisotropicDiffusion(src, dst, alpha, K, niters);
        break;
      }

      case XIMGPROC_CREATEEDGEDRAWING: {
        ret = js_edge_drawing_constructor(ctx, edge_drawing_class, argc, argv);

        /*JSEdgeDrawingData* ed = js_edge_drawing_data(ret);
         *ed = cv::ximgproc::createEdgeDrawing();*/
        break;
      }

      case XIMGPROC_CONTOURSAMPLING: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
        int32_t nbElt;

        js_value_to(ctx, argv[2], nbElt);

        cv::ximgproc::contourSampling(src, dst, nbElt);
        break;
      }

      case XIMGPROC_COVARIANCEESTIMATION: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
        int32_t rows, cols;

        js_value_to(ctx, argv[2], rows);
        js_value_to(ctx, argv[3], cols);

        cv::ximgproc::covarianceEstimation(src, dst, rows, cols);
        break;
      }

      case XIMGPROC_COLORMATCHTEMPLATE: {
        JSInputArray img = js_input_array(ctx, argv[0]);
        JSInputArray templ = js_input_array(ctx, argv[1]);
        JSOutputArray result = js_cv_outputarray(ctx, argv[2]);

        cv::ximgproc::colorMatchTemplate(img, templ, result);
        break;
      }

      case XIMGPROC_TRANSFORMFD: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSInputArray t = js_input_array(ctx, argv[1]);
        JSOutputArray dst = js_cv_outputarray(ctx, argv[2]);
        BOOL fd_contour = TRUE;

        if(argc > 3)
          js_value_to(ctx, argv[3], fd_contour);

        cv::ximgproc::transformFD(src, t, dst, fd_contour);
        break;
      }

      case XIMGPROC_FOURIERDESCRIPTOR: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
        int32_t nbElt = -1, nbFD = -1;

        if(argc > 2)
          js_value_to(ctx, argv[2], nbElt);
        if(argc > 3)
          js_value_to(ctx, argv[3], nbFD);

        cv::ximgproc::fourierDescriptor(src, dst, nbElt, nbFD);
        break;
      }

      case XIMGPROC_WEIGHTEDMEDIANFILTER: {
        JSInputArray joint = js_input_array(ctx, argv[0]);
        JSInputArray src = js_input_array(ctx, argv[1]);
        JSOutputArray dst = js_cv_outputarray(ctx, argv[2]);
        int32_t r, weightType = cv::ximgproc::WMF_EXP;
        double sigma = 25.5;
        JSInputArray mask = cv::noArray();

        js_value_to(ctx, argv[3], r);
        if(argc > 4)
          js_value_to(ctx, argv[4], sigma);
        if(argc > 5)
          js_value_to(ctx, argv[5], weightType);
        if(argc > 6)
          mask = js_input_array(ctx, argv[5]);

        cv::ximgproc::weightedMedianFilter(joint, src, dst, r, sigma, weightType, mask);
        break;
      }

      case XIMGPROC_ROLLINGGUIDANCEFILTER: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
        int32_t d, numOfIter = 4, borderType = cv::BORDER_DEFAULT;
        double sigmaColor = 25, sigmaSpace = 3;

        js_value_to(ctx, argv[2], d);
        if(argc > 3)
          js_value_to(ctx, argv[3], sigmaColor);
        if(argc > 4)
          js_value_to(ctx, argv[4], sigmaSpace);
        if(argc > 5)
          js_value_to(ctx, argv[5], numOfIter);
        if(argc > 6)
          js_value_to(ctx, argv[6], borderType);

        cv::ximgproc::rollingGuidanceFilter(src, dst, d, sigmaColor, sigmaSpace, numOfIter, borderType);
        break;
      }

      case XIMGPROC_CREATESTRUCTUREDEDGEDETECTION: {
        ret = js_structured_edge_detection_constructor(ctx, structured_edge_detection_class, argc, argv);
        break;
      }

      case XIMGPROC_FASTHOUGHTRANSFORM: {
        JSInputArray input = js_input_array(ctx, argv[0]);
        JSOutputArray output = js_cv_outputarray(ctx, argv[1]);
        int32_t dstMatDepth, angleRange = cv::ximgproc::ARO_315_135, op = cv::ximgproc::FHT_ADD, makeSkew = cv::ximgproc::HDO_DESKEW;

        js_value_to(ctx, argv[2], dstMatDepth);
        if(argc > 3)
          js_value_to(ctx, argv[3], angleRange);
        if(argc > 4)
          js_value_to(ctx, argv[4], op);
        if(argc > 5)
          js_value_to(ctx, argv[5], makeSkew);

        cv::ximgproc::FastHoughTransform(input, output, dstMatDepth, angleRange, op, makeSkew);
        break;
      }

      case XIMGPROC_HOUGHPOINT2LINE: {
        JSPointData<double> point;
        JSInputArray srcImgInfo = js_input_array(ctx, argv[1]);

        js_point_read(ctx, argv[0], &point);

        int32_t angleRange = cv::ximgproc::ARO_315_135, makeSkew = cv::ximgproc::HDO_DESKEW, rules = cv::ximgproc::RO_IGNORE_BORDERS;

        if(argc > 2)
          js_value_to(ctx, argv[2], angleRange);
        if(argc > 3)
          js_value_to(ctx, argv[3], makeSkew);
        if(argc > 4)
          js_value_to(ctx, argv[4], rules);

        cv::Vec4i v = cv::ximgproc::HoughPoint2Line(point, srcImgInfo, angleRange, makeSkew, rules);

        ret = js_value_from(ctx, v);
        break;
      }

      case XIMGPROC_CREATESUPERPIXELSLIC: {
        JSInputArray image = js_input_array(ctx, argv[0]);
        int32_t algorithm = cv::ximgproc::SLICO, region_size = 10;
        double ruler = 10.0;

        if(argc > 1)
          js_value_to(ctx, argv[1], algorithm);
        if(argc > 2)
          js_value_to(ctx, argv[2], region_size);
        if(argc > 3)
          js_value_to(ctx, argv[3], ruler);

        ret = js_superpixel_constructor(ctx, superpixel_class, 0, 0);
        JSSuperpixelData* sp = js_superpixel_data(ret);

        *sp = cv::ximgproc::createSuperpixelSLIC(image, algorithm, region_size, ruler);
        break;
      }

      case XIMGPROC_CREATESUPERPIXELLSC: {
        JSInputArray image = js_input_array(ctx, argv[0]);
        int32_t region_size = 10;
        double ratio = 0.075;

        if(argc > 1)
          js_value_to(ctx, argv[1], region_size);
        if(argc > 2)
          js_value_to(ctx, argv[2], ratio);

        ret = js_superpixel_constructor(ctx, superpixel_class, 0, 0);
        JSSuperpixelData* sp = js_superpixel_data(ret);

        *sp = cv::ximgproc::createSuperpixelLSC(image, region_size, ratio);
        break;
      }

      case XIMGPROC_CREATESUPERPIXELSEEDS: {
        int32_t image_width, image_height, image_channels, num_superpixels, num_levels, prior = 2, histogram_bins = 5;
        BOOL double_step = FALSE;

        js_value_to(ctx, argv[0], image_width);
        js_value_to(ctx, argv[1], image_height);
        js_value_to(ctx, argv[2], image_channels);
        js_value_to(ctx, argv[3], num_superpixels);
        js_value_to(ctx, argv[4], num_levels);
        if(argc > 5)
          js_value_to(ctx, argv[5], prior);
        if(argc > 6)
          js_value_to(ctx, argv[6], histogram_bins);
        if(argc > 7)
          js_value_to(ctx, argv[7], double_step);

        ret = js_superpixel_constructor(ctx, superpixel_class, 0, 0);
        JSSuperpixelData* sp = js_superpixel_data(ret);

        *sp = cv::ximgproc::createSuperpixelSEEDS(image_width, image_height, image_channels, num_superpixels, num_levels, prior, histogram_bins, double_step);
        break;
      }

      case XIMGPROC_CREATEEDGEBOXES: {
        double alpha = 0.65, beta = 0.75, eta = 1, minScore = 0.01f, edgeMinMag = 0.1, edgeMergeThr = 0.5, clusterMinMag = 0.5, maxAspectRatio = 3,
               minBoxArea = 1000, gamma = 2, kappa = 1.5;
        int32_t maxBoxes = 10000;

        if(argc > 0)
          js_value_to(ctx, argv[0], alpha);
        if(argc > 1)
          js_value_to(ctx, argv[1], beta);
        if(argc > 2)
          js_value_to(ctx, argv[2], eta);
        if(argc > 3)
          js_value_to(ctx, argv[3], minScore);
        if(argc > 4)
          js_value_to(ctx, argv[4], maxBoxes);
        if(argc > 5)
          js_value_to(ctx, argv[5], edgeMinMag);
        if(argc > 6)
          js_value_to(ctx, argv[6], edgeMergeThr);
        if(argc > 7)
          js_value_to(ctx, argv[7], clusterMinMag);
        if(argc > 8)
          js_value_to(ctx, argv[8], maxAspectRatio);
        if(argc > 9)
          js_value_to(ctx, argv[9], minBoxArea);
        if(argc > 10)
          js_value_to(ctx, argv[10], gamma);
        if(argc > 11)
          js_value_to(ctx, argv[11], kappa);

        ret = js_edgeboxes_constructor(ctx, edgeboxes_class, 0, 0);
        JSEdgeBoxesData* eb = js_edgeboxes_data(ret);

        *eb = cv::ximgproc::createEdgeBoxes(
            alpha, beta, eta, minScore, maxBoxes, edgeMinMag, edgeMergeThr, clusterMinMag, maxAspectRatio, minBoxArea, gamma, kappa);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

js_function_list_t js_ximgproc_segmentation_funcs{
    JS_CFUNC_MAGIC_DEF("createGraphSegmentation)", 0, js_segmentation_function, CREATE_GRAPH_SEGMENTATION),
    JS_CFUNC_MAGIC_DEF("createSelectiveSearchSegmentation)", 0, js_segmentation_function, CREATE_SELECTIVE_SEARCH_SEGMENTATION),
    JS_CFUNC_MAGIC_DEF("createSelectiveSearchSegmentationStrategyColor)", 0, js_segmentation_function, CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_COLOR),
    JS_CFUNC_MAGIC_DEF("createSelectiveSearchSegmentationStrategyFill)", 0, js_segmentation_function, CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_FILL),
    JS_CFUNC_MAGIC_DEF(
        "createSelectiveSearchSegmentationStrategyMultiple)", 0, js_segmentation_function, CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_MULTIPLE),
    JS_CFUNC_MAGIC_DEF("createSelectiveSearchSegmentationStrategySize)", 0, js_segmentation_function, CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_SIZE),
    JS_CFUNC_MAGIC_DEF("createSelectiveSearchSegmentationStrategyTexture)", 0, js_segmentation_function, CREATE_SELECTIVE_SEARCH_SEGMENTATION_STRATEGY_TEXTURE),
};

js_function_list_t js_ximgproc_ximgproc_funcs{
    /* Extended Image Processing */
    JS_CFUNC_MAGIC_DEF("anisotropicDiffusion", 5, js_ximgproc_func, XIMGPROC_ANISOTROPIC_DIFFUSION),
    JS_CFUNC_MAGIC_DEF("edgePreservingFilter", 4, js_ximgproc_func, XIMGPROC_EDGE_PRESERVING_FILTER),
#ifdef HAVE_OPENCV2_XIMGPROC_FIND_ELLIPSES_HPP
    JS_CFUNC_MAGIC_DEF("findEllipses", 2, js_ximgproc_func, XIMGPROC_FIND_ELLIPSES),
#endif
    JS_CFUNC_MAGIC_DEF("niBlackThreshold", 6, js_ximgproc_func, XIMGPROC_NI_BLACK_THRESHOLD),
    JS_CFUNC_MAGIC_DEF("PeiLinNormalization", 2, js_ximgproc_func, XIMGPROC_PEI_LIN_NORMALIZATION),
    JS_CFUNC_MAGIC_DEF("thinning", 2, js_ximgproc_func, XIMGPROC_THINNING),
    JS_CFUNC_MAGIC_DEF("createEdgeDrawing", 0, js_ximgproc_func, XIMGPROC_CREATEEDGEDRAWING),
    JS_CFUNC_MAGIC_DEF("contourSampling", 3, js_ximgproc_func, XIMGPROC_CONTOURSAMPLING),
    JS_CFUNC_MAGIC_DEF("covarianceEstimation", 4, js_ximgproc_func, XIMGPROC_COVARIANCEESTIMATION),
    JS_CFUNC_MAGIC_DEF("colorMatchTemplate", 3, js_ximgproc_func, XIMGPROC_COLORMATCHTEMPLATE),
    JS_CFUNC_MAGIC_DEF("transformFD", 3, js_ximgproc_func, XIMGPROC_TRANSFORMFD),
    JS_CFUNC_MAGIC_DEF("fourierDescriptor", 2, js_ximgproc_func, XIMGPROC_FOURIERDESCRIPTOR),
    JS_CFUNC_MAGIC_DEF("weightedMedianFilter", 4, js_ximgproc_func, XIMGPROC_WEIGHTEDMEDIANFILTER),
    JS_CFUNC_MAGIC_DEF("rollingGuidanceFilter", 2, js_ximgproc_func, XIMGPROC_ROLLINGGUIDANCEFILTER),
    JS_CFUNC_MAGIC_DEF("createStructuredEdgeDetection", 1, js_ximgproc_func, XIMGPROC_CREATESTRUCTUREDEDGEDETECTION),
    JS_CFUNC_MAGIC_DEF("createSuperpixelSLIC", 1, js_ximgproc_func, XIMGPROC_CREATESUPERPIXELSLIC),
    JS_CFUNC_MAGIC_DEF("createSuperpixelLSC", 1, js_ximgproc_func, XIMGPROC_CREATESUPERPIXELLSC),
    JS_CFUNC_MAGIC_DEF("createSuperpixelSEEDS", 5, js_ximgproc_func, XIMGPROC_CREATESUPERPIXELSEEDS),
    JS_CFUNC_MAGIC_DEF("createEdgeBoxes", 0, js_ximgproc_func, XIMGPROC_CREATEEDGEBOXES),

    JS_PROP_INT32_DEF("ARO_0_45", cv::ximgproc::ARO_0_45, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("ARO_45_90", cv::ximgproc::ARO_45_90, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("ARO_90_135", cv::ximgproc::ARO_90_135, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("ARO_315_0", cv::ximgproc::ARO_315_0, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("ARO_315_45", cv::ximgproc::ARO_315_45, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("ARO_45_135", cv::ximgproc::ARO_45_135, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("ARO_315_135", cv::ximgproc::ARO_315_135, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("ARO_CTR_HOR", cv::ximgproc::ARO_CTR_HOR, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("ARO_CTR_VER", cv::ximgproc::ARO_CTR_VER, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("FHT_MIN", cv::ximgproc::FHT_MIN, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("FHT_MAX", cv::ximgproc::FHT_MAX, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("FHT_ADD", cv::ximgproc::FHT_ADD, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("FHT_AVE", cv::ximgproc::FHT_AVE, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("HDO_RAW", cv::ximgproc::HDO_RAW, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("HDO_DESKEW", cv::ximgproc::HDO_DESKEW, JS_PROP_ENUMERABLE),

    JS_PROP_INT32_DEF("RO_STRICT", cv::ximgproc::RO_STRICT, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("RO_IGNORE_BORDERS", cv::ximgproc::RO_IGNORE_BORDERS, JS_PROP_ENUMERABLE),

    JS_CFUNC_MAGIC_DEF("FastHoughTransform", 3, js_ximgproc_func, XIMGPROC_FASTHOUGHTRANSFORM),
    JS_CFUNC_MAGIC_DEF("HoughPoint2Line", 2, js_ximgproc_func, XIMGPROC_HOUGHPOINT2LINE),

    JS_PROP_INT32_DEF("THINNING_ZHANGSUEN", cv::ximgproc::THINNING_ZHANGSUEN, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("THINNING_GUOHALL", cv::ximgproc::THINNING_GUOHALL, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("BINARIZATION_NIBLACK", cv::ximgproc::BINARIZATION_NIBLACK, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("BINARIZATION_SAUVOLA", cv::ximgproc::BINARIZATION_SAUVOLA, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("BINARIZATION_WOLF", cv::ximgproc::BINARIZATION_WOLF, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("BINARIZATION_NICK", cv::ximgproc::BINARIZATION_NICK, JS_PROP_ENUMERABLE),

    JS_OBJECT_DEF("segmentation", js_ximgproc_segmentation_funcs.data(), int(js_ximgproc_segmentation_funcs.size()), JS_PROP_ENUMERABLE),

};

js_function_list_t js_ximgproc_static_funcs{
    JS_OBJECT_DEF("ximgproc", js_ximgproc_ximgproc_funcs.data(), int(js_ximgproc_ximgproc_funcs.size()), JS_PROP_ENUMERABLE),
};

extern "C" int
js_ximgproc_init(JSContext* ctx, JSModuleDef* m) {

  /* create the EdgeDrawing class */
  JS_NewClassID(&js_edge_drawing_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_edge_drawing_class_id, &js_edge_drawing_class);

  edge_drawing_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, edge_drawing_proto, js_edge_drawing_proto_funcs, countof(js_edge_drawing_proto_funcs));
  JS_SetClassProto(ctx, js_edge_drawing_class_id, edge_drawing_proto);

  edge_drawing_class = JS_NewCFunction2(ctx, js_edge_drawing_constructor, "EdgeDrawing", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, edge_drawing_class, edge_drawing_proto);
  JS_SetPropertyFunctionList(ctx, edge_drawing_class, js_edge_drawing_static_funcs, countof(js_edge_drawing_static_funcs));

  /* create the EdgeDrawingParams class */
  JS_NewClass(JS_GetRuntime(ctx), js_edge_drawing_class_id, &js_edge_drawing_params_class);

  edge_drawing_params_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, edge_drawing_params_proto, js_edge_drawing_params_proto_funcs, countof(js_edge_drawing_params_proto_funcs));
  JS_SetClassProto(ctx, js_edge_drawing_class_id, edge_drawing_params_proto);

  edge_drawing_params_class = JS_NewCFunction2(ctx, js_edge_drawing_constructor, "EdgeDrawingParams", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, edge_drawing_params_class, edge_drawing_params_proto);

  JS_NewClassID(&js_structured_edge_detection_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_structured_edge_detection_class_id, &js_structured_edge_detection_class);

  structured_edge_detection_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, structured_edge_detection_proto, js_structured_edge_detection_proto_funcs, countof(js_structured_edge_detection_proto_funcs));
  JS_SetClassProto(ctx, js_structured_edge_detection_class_id, structured_edge_detection_proto);

  structured_edge_detection_class = JS_NewCFunction2(ctx, js_structured_edge_detection_constructor, "StructuredEdgeDetection", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, structured_edge_detection_class, structured_edge_detection_proto);
  JS_SetPropertyFunctionList(ctx,
                             structured_edge_detection_class,
                             js_structured_edge_detection_static_funcs,
                             countof(js_structured_edge_detection_static_funcs));

  JS_NewClassID(&js_superpixel_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_superpixel_class_id, &js_superpixel_class);

  superpixel_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, superpixel_proto, js_superpixel_proto_funcs, countof(js_superpixel_proto_funcs));
  JS_SetClassProto(ctx, js_superpixel_class_id, superpixel_proto);

  superpixel_class = JS_NewCFunction2(ctx, js_superpixel_constructor, "Superpixel", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, superpixel_class, superpixel_proto);
  JS_SetPropertyFunctionList(ctx, superpixel_class, js_superpixel_static_funcs, countof(js_superpixel_static_funcs));

  JS_NewClassID(&js_edgeboxes_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_edgeboxes_class_id, &js_edgeboxes_class);

  edgeboxes_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, edgeboxes_proto, js_edgeboxes_proto_funcs, countof(js_edgeboxes_proto_funcs));
  JS_SetClassProto(ctx, js_edgeboxes_class_id, edgeboxes_proto);

  edgeboxes_class = JS_NewCFunction2(ctx, js_edgeboxes_constructor, "EdgeBoxes", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, edgeboxes_class, edgeboxes_proto);
  JS_SetPropertyFunctionList(ctx, edgeboxes_class, js_edgeboxes_static_funcs, countof(js_edgeboxes_static_funcs));

  JS_NewClassID(&js_graph_segmentation_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_graph_segmentation_class_id, &js_graph_segmentation_class);

  graph_segmentation_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, graph_segmentation_proto, js_graph_segmentation_proto_funcs, countof(js_graph_segmentation_proto_funcs));
  JS_SetClassProto(ctx, js_graph_segmentation_class_id, graph_segmentation_proto);

  graph_segmentation_class = JS_NewCFunction2(ctx, js_graph_segmentation_constructor, "GraphSegmentation", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, graph_segmentation_class, graph_segmentation_proto);
  JS_SetPropertyFunctionList(ctx, graph_segmentation_class, js_graph_segmentation_static_funcs, countof(js_graph_segmentation_static_funcs));

  JS_NewClassID(&js_search_segmentation_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_search_segmentation_class_id, &js_search_segmentation_class);

  selective_search_segmentation_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, selective_search_segmentation_proto, js_search_segmentation_proto_funcs, countof(js_search_segmentation_proto_funcs));
  JS_SetClassProto(ctx, js_search_segmentation_class_id, selective_search_segmentation_proto);

  selective_search_segmentation_class = JS_NewCFunction2(ctx, js_search_segmentation_constructor, "SelectiveSearchSegmentation", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, selective_search_segmentation_class, selective_search_segmentation_proto);
  JS_SetPropertyFunctionList(ctx, selective_search_segmentation_class, js_search_segmentation_static_funcs, countof(js_search_segmentation_static_funcs));

  segmentation_strategy_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, segmentation_strategy_proto, js_segmentation_strategy_proto_funcs, countof(js_segmentation_strategy_proto_funcs));
  JS_SetClassProto(ctx, js_segmentation_strategy_class_id, segmentation_strategy_proto);

  segmentation_strategy_class = JS_NewCFunction2(ctx, js_segmentation_strategy_constructor, "SelectiveSearchSegmentationStrategy", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, segmentation_strategy_class, segmentation_strategy_proto);
  JS_SetPropertyFunctionList(ctx, segmentation_strategy_class, js_segmentation_strategy_static_funcs, countof(js_segmentation_strategy_static_funcs));

  if(m) {
    JS_SetModuleExport(ctx, m, "EdgeDrawing", edge_drawing_class);
    JS_SetModuleExport(ctx, m, "EdgeDrawingParams", edge_drawing_params_class);
    JS_SetModuleExport(ctx, m, "StructuredEdgeDetection", structured_edge_detection_class);
    JS_SetModuleExport(ctx, m, "Superpixel", superpixel_class);
    JS_SetModuleExport(ctx, m, "EdgeBoxes", edgeboxes_class);
    JS_SetModuleExportList(ctx, m, js_ximgproc_static_funcs.data(), js_ximgproc_static_funcs.size());
  }

  return 0;
}

extern "C" void
js_ximgproc_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "EdgeDrawing");
  JS_AddModuleExport(ctx, m, "EdgeDrawingParams");
  JS_AddModuleExport(ctx, m, "StructuredEdgeDetection");
  JS_AddModuleExport(ctx, m, "Superpixel");
  JS_AddModuleExport(ctx, m, "EdgeBoxes");

  JS_AddModuleExportList(ctx, m, js_ximgproc_static_funcs.data(), js_ximgproc_static_funcs.size());
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_ximgproc
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_ximgproc_init)))
    return NULL;

  js_ximgproc_export(ctx, m);
  return m;
}
#endif
