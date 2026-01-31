#include "js_cv.hpp"
#include "js_umat.hpp"
#include "js_affine3.hpp"
#include "jsbindings.hpp"
#include <quickjs.h>

#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/objdetect/charuco_detector.hpp"

enum {
  ARUCO_DRAW_DETECTED_CORNERS_CHARUCO=0,
};

static JSValue
js_aruco_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
    switch(magic) {
      case ARUCO_DRAW_DETECTED_CORNERS_CHARUCO: {
        break;
      }
 
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

js_function_list_t js_aruco_aruco_funcs{
     JS_CFUNC_MAGIC_DEF("drawDetectedCornersCharuco", 2, js_aruco_func, ARUCO_DRAW_DETECTED_CORNERS_CHARUCO), 
};

js_function_list_t js_aruco_static_funcs{
    JS_OBJECT_DEF("aruco", js_aruco_aruco_funcs.data(), int(js_aruco_aruco_funcs.size()), JS_PROP_C_W_E),
};

extern "C" int
js_aruco_init(JSContext* ctx, JSModuleDef* m) {

  if(m) {
    JS_SetModuleExportList(ctx, m, js_aruco_static_funcs.data(), js_aruco_static_funcs.size());
  }

  return 0;
}

extern "C" void
js_aruco_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_aruco_static_funcs.data(), js_aruco_static_funcs.size());
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_aruco
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_aruco_init)))
    return NULL;

  js_aruco_export(ctx, m);
  return m;
}
