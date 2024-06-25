#include "js_umat.hpp"
#include "jsbindings.hpp"
#include <quickjs.h>

#ifdef HAVE_OPENCV2_XIMGPROC_HPP
#include <opencv2/ximgproc.hpp>

enum {
  XIMGPROC_ANISOTROPIC_DIFFUSION,
  XIMGPROC_EDGE_PRESERVING_FILTER,
  XIMGPROC_FIND_ELLIPSES,
  XIMGPROC_THINNING,
  XIMGPROC_NI_BLACK_THRESHOLD,
  XIMGPROC_PEI_LIN_NORMALIZATION
};

static JSValue
js_ximgproc_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSInputArray src;
  JSInputOutputArray dst;
  JSValue ret = JS_UNDEFINED;

  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

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

      case XIMGPROC_FIND_ELLIPSES: {
        double scoreThreshold = 0.7, reliabilityThreshold = 0.5, centerDistanceThreshold = 0.05;

        JS_ToFloat64(ctx, &scoreThreshold, argv[2]);
        JS_ToFloat64(ctx, &reliabilityThreshold, argv[3]);
        JS_ToFloat64(ctx, &centerDistanceThreshold, argv[4]);

        cv::ximgproc::findEllipses(src, dst, scoreThreshold, reliabilityThreshold, centerDistanceThreshold);
        break;
      }

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
    }
  } catch(const cv::Exception& e) { ret = js_handle_exception(ctx, e); }

  return ret;
}

js_function_list_t js_ximgproc_ximgproc_funcs{
    /* Extended Image Processing */
    JS_CFUNC_MAGIC_DEF("anisotropicDiffusion", 5, js_ximgproc_func, XIMGPROC_ANISOTROPIC_DIFFUSION),
    JS_CFUNC_MAGIC_DEF("edgePreservingFilter", 4, js_ximgproc_func, XIMGPROC_EDGE_PRESERVING_FILTER),
    JS_CFUNC_MAGIC_DEF("findEllipses", 2, js_ximgproc_func, XIMGPROC_FIND_ELLIPSES),
    JS_CFUNC_MAGIC_DEF("niBlackThreshold", 6, js_ximgproc_func, XIMGPROC_NI_BLACK_THRESHOLD),
    JS_CFUNC_MAGIC_DEF("PeiLinNormalization", 2, js_ximgproc_func, XIMGPROC_PEI_LIN_NORMALIZATION),
    JS_CFUNC_MAGIC_DEF("thinning", 2, js_ximgproc_func, XIMGPROC_THINNING),
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

  if(m) {
    JS_SetModuleExportList(ctx, m, js_ximgproc_static_funcs.data(), js_ximgproc_static_funcs.size());
  }

  return 0;
}

extern "C" void
js_ximgproc_export(JSContext* ctx, JSModuleDef* m) {
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
