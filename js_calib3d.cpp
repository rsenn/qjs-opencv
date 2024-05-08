#include "js_alloc.hpp"
#include "js_size.hpp"
#include "js_mat.hpp"
#include "js_umat.hpp"
#include <opencv2/calib3d.hpp>

extern "C" int js_calib3d_init(JSContext*, JSModuleDef*);

enum {
  FIND_CHESSBOARD_CORNERS,
};

static JSValue
js_calib3d_functions(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  switch(magic) {
    case FIND_CHESSBOARD_CORNERS: {
      JSInputOutputArray image = js_umat_or_mat(ctx, argv[0]);
      cv::Size pattern_size = js_size_get(ctx, argv[1]);
      std::vector<cv::Point2f> corners;
      int32_t flags = cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE;

      if(argc > 3)
        JS_ToInt32(ctx, &flags, argv[3]);

      BOOL result = cv::findChessboardCorners(image, pattern_size, corners, flags);

      js_array_copy(ctx, argv[2], corners);

      ret = JS_NewBool(ctx, result);
      break;
    }
  }

  return ret;
}

const JSCFunctionListEntry js_calib3d_static_funcs[] = {
    JS_CFUNC_MAGIC_DEF("findChessboardCorners", 3, js_calib3d_functions, FIND_CHESSBOARD_CORNERS),
    JS_PROP_INT32_DEF("CALIB_CB_ADAPTIVE_THRESH", cv::CALIB_CB_ADAPTIVE_THRESH, 0),
    JS_PROP_INT32_DEF("CALIB_CB_NORMALIZE_IMAGE", cv::CALIB_CB_NORMALIZE_IMAGE, 0),
    JS_PROP_INT32_DEF("CALIB_CB_FILTER_QUADS", cv::CALIB_CB_FILTER_QUADS, 0),
    JS_PROP_INT32_DEF("CALIB_CB_FAST_CHECK", cv::CALIB_CB_FAST_CHECK, 0),
};

extern "C" int
js_calib3d_init(JSContext* ctx, JSModuleDef* bd) {

  if(bd) {
    JS_SetModuleExportList(ctx, bd, js_calib3d_static_funcs, countof(js_calib3d_static_funcs));
  }

  return 0;
}

#ifdef JS_Calib3D_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_calib3d
#endif

extern "C" void
js_calib3d_export(JSContext* ctx, JSModuleDef* bd) {
  JS_AddModuleExportList(ctx, bd, js_calib3d_static_funcs, countof(js_calib3d_static_funcs));
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* bd;
  bd = JS_NewCModule(ctx, module_name, &js_calib3d_init);

  if(!bd)
    return NULL;

  js_calib3d_export(ctx, bd);
  return bd;
}
