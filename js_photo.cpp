#include "js_cv.hpp"
#include "js_umat.hpp"
#include <opencv2/photo.hpp>

extern "C" int js_photo_init(JSContext*, JSModuleDef*);

enum {
  PENCIL_SKETCH,
  STYLIZATION,
  DETAIL_ENHANCE,
  EDGE_PRESERVING_FILTER,
  FAST_NL_MEANS_DENOISING,
  FAST_NL_MEANS_DENOISING_COLORED,
  INPAINT,
};

static JSValue
js_photo_functions(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
    switch(magic) {
      case PENCIL_SKETCH: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSOutputArray dst1 = js_cv_outputarray(ctx, argv[1]), dst2 = js_cv_outputarray(ctx, argv[2]);
        double sigma_s = 60, sigma_r = 0.07, shade_factor = 0.02;

        if(argc > 3)
          JS_ToFloat64(ctx, &sigma_s, argv[3]);
        if(argc > 4)
          JS_ToFloat64(ctx, &sigma_r, argv[4]);
        if(argc > 5)
          JS_ToFloat64(ctx, &shade_factor, argv[5]);

        cv::pencilSketch(src, dst1, dst2, sigma_s, sigma_r, shade_factor);
        break;
      }

      case STYLIZATION: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
        double sigma_s = 60, sigma_r = 0.45;

        if(argc > 2)
          JS_ToFloat64(ctx, &sigma_s, argv[2]);
        if(argc > 3)
          JS_ToFloat64(ctx, &sigma_r, argv[3]);

        cv::stylization(src, dst, sigma_s, sigma_r);
        break;
      }

      case DETAIL_ENHANCE: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
        double sigma_s = 10, sigma_r = 0.15;

        if(argc > 2)
          JS_ToFloat64(ctx, &sigma_s, argv[2]);
        if(argc > 3)
          JS_ToFloat64(ctx, &sigma_r, argv[3]);

        cv::detailEnhance(src, dst, sigma_s, sigma_r);
        break;
      }

      case EDGE_PRESERVING_FILTER: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
        int32_t flags = cv::RECURS_FILTER;
        double sigma_s = 60, sigma_r = 0.4;

        if(argc > 2)
          JS_ToInt32(ctx, &flags, argv[2]);
        if(argc > 3)
          JS_ToFloat64(ctx, &sigma_s, argv[3]);
        if(argc > 4)
          JS_ToFloat64(ctx, &sigma_r, argv[4]);

        cv::edgePreservingFilter(src, dst, flags, sigma_s, sigma_r);
        break;
      }

      case FAST_NL_MEANS_DENOISING: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
        double h = 3;
        int32_t templateWindowSize = 7, searchWindowSize = 21;

        if(argc > 2)
          JS_ToFloat64(ctx, &h, argv[2]);
        if(argc > 3)
          JS_ToInt32(ctx, &templateWindowSize, argv[3]);
        if(argc > 4)
          JS_ToInt32(ctx, &searchWindowSize, argv[4]);

        cv::fastNlMeansDenoising(src, dst, h, templateWindowSize, searchWindowSize);
        break;
      }

      case FAST_NL_MEANS_DENOISING_COLORED: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
        double h = 3, hColor = 3;
        int32_t templateWindowSize = 7, searchWindowSize = 21;

        if(argc > 2)
          JS_ToFloat64(ctx, &h, argv[2]);
        if(argc > 3)
          JS_ToFloat64(ctx, &hColor, argv[3]);
        if(argc > 4)
          JS_ToInt32(ctx, &templateWindowSize, argv[4]);
        if(argc > 5)
          JS_ToInt32(ctx, &searchWindowSize, argv[5]);

        cv::fastNlMeansDenoisingColored(src, dst, h, hColor, templateWindowSize, searchWindowSize);
        break;
      }

      case INPAINT: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSInputArray inpaintMask = js_input_array(ctx, argv[1]);
        JSOutputArray dst = js_cv_outputarray(ctx, argv[2]);
        double inpaintRadius;
        int32_t flags = cv::INPAINT_TELEA;

        JS_ToFloat64(ctx, &inpaintRadius, argv[3]);
        if(argc > 4)
          JS_ToInt32(ctx, &flags, argv[4]);

        cv::inpaint(src, inpaintMask, dst, inpaintRadius, flags);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

const JSCFunctionListEntry js_photo_static_funcs[] = {
    JS_CFUNC_MAGIC_DEF("pencilSketch", 3, js_photo_functions, PENCIL_SKETCH),
    JS_CFUNC_MAGIC_DEF("stylization", 2, js_photo_functions, STYLIZATION),
    JS_CFUNC_MAGIC_DEF("detailEnhance", 2, js_photo_functions, DETAIL_ENHANCE),
    JS_CFUNC_MAGIC_DEF("edgePreservingFilter", 2, js_photo_functions, EDGE_PRESERVING_FILTER),
    JS_CFUNC_MAGIC_DEF("fastNlMeansDenoising", 2, js_photo_functions, FAST_NL_MEANS_DENOISING),
    JS_CFUNC_MAGIC_DEF("fastNlMeansDenoisingColored", 2, js_photo_functions, FAST_NL_MEANS_DENOISING_COLORED),
    JS_CFUNC_MAGIC_DEF("inpaint", 4, js_photo_functions, INPAINT),
    JS_PROP_INT32_DEF("RECURS_FILTER", cv::RECURS_FILTER, 0),
    JS_PROP_INT32_DEF("NORMCONV_FILTER", cv::NORMCONV_FILTER, 0),
    JS_PROP_INT32_DEF("INPAINT_NS", cv::INPAINT_NS, 0),
    JS_PROP_INT32_DEF("INPAINT_TELEA", cv::INPAINT_TELEA, 0),
};

extern "C" int
js_photo_init(JSContext* ctx, JSModuleDef* m) {
  if(m)
    JS_SetModuleExportList(ctx, m, js_photo_static_funcs, countof(js_photo_static_funcs));

  return 0;
}

#ifdef JS_Photo_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_photo
#endif

extern "C" void
js_photo_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_photo_static_funcs, countof(js_photo_static_funcs));
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_photo_init);

  if(!m)
    return NULL;

  js_photo_export(ctx, m);
  return m;
}
