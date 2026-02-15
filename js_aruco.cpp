#include "js_cv.hpp"
#include "js_umat.hpp"
#include "include/jsbindings.hpp"
#include <quickjs.h>

#include <opencv2/objdetect/charuco_detector.hpp>

enum {
  ARUCO_DRAW_DETECTED_CORNERS_CHARUCO = 0,
  ARUCO_DRAW_DETECTED_DIAMONDS,
  ARUCO_DRAW_DETECTED_MARKERS,
};

static JSValue
js_aruco_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
    switch(magic) {
      case ARUCO_DRAW_DETECTED_CORNERS_CHARUCO: {
        JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[0]);
        JSInputArray charucoCorners = js_input_array(ctx, argv[1]);
        JSInputArray charucoIds = cv::noArray();

        if(argc > 2)
          charucoIds = js_input_array(ctx, argv[2]);

        JSColorData<double> cornerColor{255, 0, 0};

        if(argc > 3)
          js_color_read(ctx, argv[3], &cornerColor);

        cv::aruco::drawDetectedCornersCharuco(image, charucoCorners, charucoIds, cornerColor);
        break;
      }

      case ARUCO_DRAW_DETECTED_DIAMONDS: {
        JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[0]);
        JSInputArray diamondCorners = js_input_array(ctx, argv[1]);
        JSInputArray diamondIds = cv::noArray();

        if(argc > 2)
          diamondIds = js_input_array(ctx, argv[2]);

        JSColorData<double> borderColor{0, 0, 255};

        if(argc > 3)
          js_color_read(ctx, argv[3], &borderColor);

        cv::aruco::drawDetectedDiamonds(image, diamondCorners, diamondIds, borderColor);
        break;
      }

      case ARUCO_DRAW_DETECTED_MARKERS: {
        JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[0]);
        JSInputArray corners = js_input_array(ctx, argv[1]);
        JSInputArray ids = cv::noArray();

        if(argc > 2)
          ids = js_input_array(ctx, argv[2]);

        JSColorData<double> borderColor{0, 0, 255};

        if(argc > 3)
          js_color_read(ctx, argv[3], &borderColor);

        cv::aruco::drawDetectedMarkers(image, corners, ids, borderColor);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

js_function_list_t js_aruco_aruco_funcs{
    JS_CFUNC_MAGIC_DEF("drawDetectedCornersCharuco", 2, js_aruco_func, ARUCO_DRAW_DETECTED_CORNERS_CHARUCO),
    JS_CFUNC_MAGIC_DEF("drawDetectedDiamonds", 2, js_aruco_func, ARUCO_DRAW_DETECTED_DIAMONDS),
    JS_CFUNC_MAGIC_DEF("drawDetectedMarkers", 2, js_aruco_func, ARUCO_DRAW_DETECTED_MARKERS),
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
