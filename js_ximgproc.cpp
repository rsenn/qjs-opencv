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
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

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
};

js_function_list_t js_ximgproc_static_funcs{
    JS_OBJECT_DEF("ximgproc", js_ximgproc_ximgproc_funcs.data(), int(js_ximgproc_ximgproc_funcs.size()), JS_PROP_C_W_E),
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

  if(m) {
    JS_SetModuleExport(ctx, m, "EdgeDrawing", edge_drawing_class);
    JS_SetModuleExport(ctx, m, "EdgeDrawingParams", edge_drawing_params_class);
    JS_SetModuleExport(ctx, m, "StructuredEdgeDetection", structured_edge_detection_class);
    JS_SetModuleExport(ctx, m, "Superpixel", superpixel_class);
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
