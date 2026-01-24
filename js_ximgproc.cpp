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

typedef cv::Ptr<cv::ximgproc::EdgeDrawing> JSEdgeDrawingData;

extern "C" int js_edge_drawing_init(JSContext*, JSModuleDef*);

extern "C" {
thread_local JSValue edge_drawing_proto = JS_UNDEFINED, edge_drawing_class = JS_UNDEFINED;
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
  EDGEDRAWING_DETECTEDGES,
  EDGEDRAWING_DETECTELLIPSES,
  EDGEDRAWING_DETECTLINES,
  EDGEDRAWING_GETEDGEIMAGE,
  EDGEDRAWING_GETGRADIENTIMAGE,
  EDGEDRAWING_GETSEGMENTINDICESOFLINES,
  EDGEDRAWING_GETSEGMENTS,
};

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
  }

  return ret;
}

JSClassDef js_edge_drawing_class = {
    .class_name = "EdgeDrawing",
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
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "EdgeDrawing", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_edge_drawing_static_funcs[] = {};

enum {
  XIMGPROC_ANISOTROPIC_DIFFUSION,
  XIMGPROC_EDGE_PRESERVING_FILTER,
  XIMGPROC_FIND_ELLIPSES,
  XIMGPROC_THINNING,
  XIMGPROC_NI_BLACK_THRESHOLD,
  XIMGPROC_PEI_LIN_NORMALIZATION,
  XIMGPROC_CREATEEDGEDRAWING,
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

        JSEdgeDrawingData* ed = js_edge_drawing_data(ret);

        *ed = cv::ximgproc::createEdgeDrawing();
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

  edge_drawing_class = JS_NewCFunction2(ctx, js_edge_drawing_constructor, "EdgeDrawing", 2, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, edge_drawing_class, edge_drawing_proto);
  JS_SetPropertyFunctionList(ctx, edge_drawing_class, js_edge_drawing_static_funcs, countof(js_edge_drawing_static_funcs));

  if(m) {
    JS_SetModuleExport(ctx, m, "EdgeDrawing", edge_drawing_class);
    JS_SetModuleExportList(ctx, m, js_ximgproc_static_funcs.data(), js_ximgproc_static_funcs.size());
  }

  return 0;
}

extern "C" void
js_ximgproc_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "EdgeDrawing");
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
