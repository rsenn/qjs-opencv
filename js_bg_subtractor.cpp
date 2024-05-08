#include "js_alloc.hpp"
#include "js_size.hpp"
#include "js_point.hpp"
#include "js_mat.hpp"
#include "js_umat.hpp"
#include <opencv2/video/background_segm.hpp>
#include <opencv2/bgsegm.hpp>

typedef cv::BackgroundSubtractor JSBackgroundSubtractorClass;
typedef cv::Ptr<JSBackgroundSubtractorClass> JSBackgroundSubtractorData;

JSValue bg_subtractor_proto = JS_UNDEFINED, bg_subtractor_class = JS_UNDEFINED;
JSClassID js_bg_subtractor_class_id;

extern "C" int js_bg_subtractor_init(JSContext*, JSModuleDef*);

static JSValue
js_bg_subtractor_wrap(JSContext* ctx, JSBackgroundSubtractorData& s) {
  JSBackgroundSubtractorData* ptr;

  if(!(ptr = js_allocate<JSBackgroundSubtractorData>(ctx)))
    return JS_ThrowOutOfMemory(ctx);

  JSValue ret = JS_NewObjectProtoClass(ctx, bg_subtractor_proto, js_bg_subtractor_class_id);

  *ptr = s;
  JS_SetOpaque(ret, ptr);

  return ret;
}

enum {
  BGSEGM_MOG,
  BGSEGM_GMG,
  BGSEGM_CNT,
  BGSEGM_GSOC,
  BGSEGM_LSBP,
  BGSEGM_MOG2,
  BGSEGM_KNN,
};

static JSValue
js_bg_subtractor_function(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSBackgroundSubtractorData s;

  switch(magic) {
    case BGSEGM_MOG: {
      int32_t history = 200, nmixtures = 5;
      double background_ratio = 0.7, noise_sigma = 0;

      if(argc > 0)
        JS_ToInt32(ctx, &history, argv[0]);
      if(argc > 1)
        JS_ToInt32(ctx, &nmixtures, argv[1]);
      if(argc > 2)
        JS_ToFloat64(ctx, &background_ratio, argv[2]);
      if(argc > 3)
        JS_ToFloat64(ctx, &noise_sigma, argv[3]);

      s = cv::bgsegm::createBackgroundSubtractorMOG(history, nmixtures, background_ratio, noise_sigma);
      break;
    }

    case BGSEGM_GMG: {
      int32_t initialization_frames = 120;
      double decision_threshold = 0.8;

      if(argc > 0)
        JS_ToInt32(ctx, &initialization_frames, argv[0]);
      if(argc > 1)
        JS_ToFloat64(ctx, &decision_threshold, argv[1]);

      s = cv::bgsegm::createBackgroundSubtractorGMG(initialization_frames, decision_threshold);
      break;
    }

    case BGSEGM_CNT: {
      int32_t min_pixel_stability = 15, max_pixel_stability = 15 * 60;
      BOOL use_history = TRUE, is_parallel = TRUE;

      if(argc > 0)
        JS_ToInt32(ctx, &min_pixel_stability, argv[0]);
      if(argc > 1)
        use_history = JS_ToBool(ctx, argv[1]);
      if(argc > 2)
        JS_ToInt32(ctx, &max_pixel_stability, argv[2]);
      if(argc > 3)
        is_parallel = JS_ToBool(ctx, argv[3]);

      s = cv::bgsegm::createBackgroundSubtractorCNT(min_pixel_stability, use_history, max_pixel_stability, is_parallel);
      break;
    }

    case BGSEGM_GSOC: {
      int32_t mc = cv::bgsegm::LSBP_CAMERA_MOTION_COMPENSATION_NONE, n_samples = 20, hits_threshold = 32;
      double replace_rate = 0.003, propagation_rate = 0.01, alpha = 0.01, beta = 0.0022, blinking_supression_decay = 0.1, blinking_supression_multiplier = 0.1,
             nrtf_bg = 0.0004, nrtf_fg = 0.0008;

      if(argc > 0)
        JS_ToInt32(ctx, &mc, argv[0]);
      if(argc > 1)
        JS_ToInt32(ctx, &n_samples, argv[1]);
      if(argc > 2)
        JS_ToFloat64(ctx, &replace_rate, argv[2]);
      if(argc > 3)
        JS_ToFloat64(ctx, &propagation_rate, argv[3]);
      if(argc > 4)
        JS_ToInt32(ctx, &hits_threshold, argv[4]);
      if(argc > 5)
        JS_ToFloat64(ctx, &alpha, argv[5]);
      if(argc > 6)
        JS_ToFloat64(ctx, &beta, argv[6]);
      if(argc > 7)
        JS_ToFloat64(ctx, &blinking_supression_decay, argv[7]);
      if(argc > 8)
        JS_ToFloat64(ctx, &blinking_supression_multiplier, argv[8]);
      if(argc > 9)
        JS_ToFloat64(ctx, &nrtf_bg, argv[9]);
      if(argc > 10)
        JS_ToFloat64(ctx, &nrtf_fg, argv[10]);

      s = cv::bgsegm::createBackgroundSubtractorGSOC(mc,
                                                     n_samples,
                                                     replace_rate,
                                                     propagation_rate,
                                                     hits_threshold,
                                                     alpha,
                                                     beta,
                                                     blinking_supression_decay,
                                                     blinking_supression_multiplier,
                                                     nrtf_bg,
                                                     nrtf_fg);
      break;
    }

    case BGSEGM_LSBP: {
      int32_t mc = cv::bgsegm::LSBP_CAMERA_MOTION_COMPENSATION_NONE, n_samples = 20, radius = 16, LSBPthreshold = 8, min_count = 2;

      double t_lower = 2.0, t_upper = 32.0, t_inc = 1.0, t_dec = 0.05, r_scale = 10.0, r_incdec = 0.005, nrtf_bg = 0.0004, nrtf_fg = 0.0008;

      if(argc > 0)
        JS_ToInt32(ctx, &mc, argv[0]);
      if(argc > 1)
        JS_ToInt32(ctx, &n_samples, argv[1]);
      if(argc > 2)
        JS_ToInt32(ctx, &radius, argv[2]);
      if(argc > 3)
        JS_ToFloat64(ctx, &t_lower, argv[3]);
      if(argc > 4)
        JS_ToFloat64(ctx, &t_upper, argv[4]);
      if(argc > 5)
        JS_ToFloat64(ctx, &t_inc, argv[5]);
      if(argc > 6)
        JS_ToFloat64(ctx, &t_dec, argv[6]);
      if(argc > 7)
        JS_ToFloat64(ctx, &r_scale, argv[7]);
      if(argc > 8)
        JS_ToFloat64(ctx, &r_incdec, argv[8]);
      if(argc > 9)
        JS_ToFloat64(ctx, &nrtf_bg, argv[9]);
      if(argc > 10)
        JS_ToFloat64(ctx, &nrtf_fg, argv[10]);

      s = cv::bgsegm::createBackgroundSubtractorLSBP(mc, n_samples, radius, t_lower, t_upper, t_inc, t_dec, r_scale, r_incdec, nrtf_bg, nrtf_fg);
      break;
    }

    case BGSEGM_MOG2: {
      int32_t history = 500;
      double var_threshold = 16;
      BOOL detect_shadows = TRUE;

      if(argc > 0)
        JS_ToInt32(ctx, &history, argv[0]);
      if(argc > 1)
        JS_ToFloat64(ctx, &var_threshold, argv[1]);
      if(argc > 2)
        detect_shadows = JS_ToBool(ctx, argv[2]);

      s = cv::createBackgroundSubtractorMOG2(history, var_threshold, detect_shadows);
      break;
    }

    case BGSEGM_KNN: {
      int32_t history = 500;
      double dist2Threshold = 16;
      BOOL detect_shadows = TRUE;

      if(argc > 0)
        JS_ToInt32(ctx, &history, argv[0]);

      if(argc > 1)
        JS_ToFloat64(ctx, &dist2Threshold, argv[1]);

      if(argc > 2)
        detect_shadows = JS_ToBool(ctx, argv[2]);

      s = cv::createBackgroundSubtractorKNN(history, dist2Threshold, detect_shadows);
      break;
    }
  }

  return s ? js_bg_subtractor_wrap(ctx, s) : JS_NULL;
}

JSBackgroundSubtractorData*
js_bg_subtractor_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSBackgroundSubtractorData*>(JS_GetOpaque2(ctx, val, js_bg_subtractor_class_id));
}

static void
js_bg_subtractor_finalizer(JSRuntime* rt, JSValue val) {
  JSBackgroundSubtractorData* s = static_cast<JSBackgroundSubtractorData*>(JS_GetOpaque(val, js_bg_subtractor_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  (*s)->~JSBackgroundSubtractorClass();

  js_deallocate<JSBackgroundSubtractorData>(rt, s);
}

enum { METHOD_APPLY = 0, METHOD_GET_BACKGROUND_IMAGE };

static JSValue
js_bg_subtractor_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSBackgroundSubtractorData* s = static_cast<JSBackgroundSubtractorData*>(JS_GetOpaque2(ctx, this_val, js_bg_subtractor_class_id));

  switch(magic) {
    case METHOD_APPLY: {
      JSInputArray input = js_input_array(ctx, argv[0]);
      JSInputOutputArray fgmask = cv::noArray();
      double learningRate = -1;

      if(argc > 1)
        fgmask = js_cv_inputoutputarray(ctx, argv[1]);

      if(argc > 2)
        JS_ToFloat64(ctx, &learningRate, argv[2]);

      (*s)->apply(input, fgmask, learningRate);
      break;
    }

    case METHOD_GET_BACKGROUND_IMAGE: {
      JSOutputArray out = js_cv_outputarray(ctx, argv[0]);

      (*s)->getBackgroundImage(out);
      break;
    }
  }

  return JS_UNDEFINED;
}

JSClassDef js_bg_subtractor_class = {
    .class_name = "BackgroundSubtractor",
    .finalizer = js_bg_subtractor_finalizer,
};

const JSCFunctionListEntry js_bg_subtractor_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("apply", 1, js_bg_subtractor_method, METHOD_APPLY),
    JS_CFUNC_MAGIC_DEF("getBackgroundImage", 1, js_bg_subtractor_method, METHOD_GET_BACKGROUND_IMAGE),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "BackgroundSubtractor", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_bg_subtractor_create_funcs[] = {
    JS_CFUNC_MAGIC_DEF("createBackgroundSubtractorMOG", 0, js_bg_subtractor_function, BGSEGM_MOG),
    JS_CFUNC_MAGIC_DEF("createBackgroundSubtractorGMG", 0, js_bg_subtractor_function, BGSEGM_GMG),
    JS_CFUNC_MAGIC_DEF("createBackgroundSubtractorCNT", 0, js_bg_subtractor_function, BGSEGM_CNT),
    JS_CFUNC_MAGIC_DEF("createBackgroundSubtractorGSOC", 0, js_bg_subtractor_function, BGSEGM_GSOC),
    JS_CFUNC_MAGIC_DEF("createBackgroundSubtractorLSBP", 0, js_bg_subtractor_function, BGSEGM_LSBP),
    JS_CFUNC_MAGIC_DEF("createBackgroundSubtractorMOG2", 0, js_bg_subtractor_function, BGSEGM_MOG2),
    JS_CFUNC_MAGIC_DEF("createBackgroundSubtractorKNN", 0, js_bg_subtractor_function, BGSEGM_KNN),
    JS_PROP_INT32_DEF("LSBP_CAMERA_MOTION_COMPENSATION_NONE", cv::bgsegm::LSBP_CAMERA_MOTION_COMPENSATION_NONE, 0),
    JS_PROP_INT32_DEF("LSBP_CAMERA_MOTION_COMPENSATION_LK", cv::bgsegm::LSBP_CAMERA_MOTION_COMPENSATION_LK, 0),
};

extern "C" int
js_bg_subtractor_init(JSContext* ctx, JSModuleDef* m) {

  /* create the BackgroundSubtractor class */
  JS_NewClassID(&js_bg_subtractor_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_bg_subtractor_class_id, &js_bg_subtractor_class);

  bg_subtractor_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, bg_subtractor_proto, js_bg_subtractor_proto_funcs, countof(js_bg_subtractor_proto_funcs));
  JS_SetClassProto(ctx, js_bg_subtractor_class_id, bg_subtractor_proto);

  bg_subtractor_class = JS_NewObject(ctx);

  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, bg_subtractor_class, bg_subtractor_proto);

  if(m) {
    JS_SetModuleExport(ctx, m, "BackgroundSubtractor", bg_subtractor_class);
    JS_SetModuleExportList(ctx, m, js_bg_subtractor_create_funcs, countof(js_bg_subtractor_create_funcs));
  }

  return 0;
}

void
js_bg_subtractor_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(bg_subtractor_class))
    js_bg_subtractor_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "BackgroundSubtractor", bg_subtractor_class);
}

#ifdef JS_BackgroundSubtractor_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_bg_subtractor
#endif

extern "C" void
js_bg_subtractor_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "BackgroundSubtractor");
  JS_AddModuleExportList(ctx, m, js_bg_subtractor_create_funcs, countof(js_bg_subtractor_create_funcs));
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_bg_subtractor_init);

  if(!m)
    return NULL;

  js_bg_subtractor_export(ctx, m);
  return m;
}
