#include "js_alloc.hpp"
#include "js_cv.hpp"
#include "js_point.hpp"
#include "js_rect.hpp"
#include "js_size.hpp"
#include "js_umat.hpp"
#include "jsbindings.hpp"
#include <opencv2/core/cvstd.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <quickjs.h>
#include <cctype>
#include <stddef.h>
#include <cstdint>
#include <algorithm>
#include <opencv2/highgui.hpp>
#include <vector>
#include <map>

#ifdef _WIN32
#include <windows.h>
#elif defined(HAVE_X11)
#include <X11/Xlib.h>
#endif

enum { DISPLAY_OVERLAY };

struct Trackbar {
  int32_t value;
  JSValue name, window, count;
  JSValueConst handler;
  JSContext* ctx;
};

static std::vector<cv::String> window_list;
static std::map<cv::String, std::map<cv::String, Trackbar*>> trackbar_list;

static JSValue
js_cv_display_overlay(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;
  const char *winname, *text;
  int32_t delayms = 0;

  winname = JS_ToCString(ctx, argv[0]);
  text = JS_ToCString(ctx, argv[1]);

  if(argc > 2)
    JS_ToInt32(ctx, &delayms, argv[2]);

  cv::displayOverlay(winname, text, delayms);

  return ret;
}

static JSValue
js_cv_named_window(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  const char* name;
  int32_t flags = cv::WINDOW_NORMAL;
  name = JS_ToCString(ctx, argv[0]);

  if(argc > 1)
    JS_ToInt32(ctx, &flags, argv[1]);

  cv::namedWindow(name, flags);

  if(std::find(window_list.cbegin(), window_list.cend(), name) == window_list.cend())
    window_list.push_back(name);

  return JS_UNDEFINED;
}

static JSValue
js_cv_move_window(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  const char* name;
  int32_t x, y;
  JSPointData<double> point;
  name = JS_ToCString(ctx, argv[0]);

  if(js_point_read(ctx, argv[1], &point)) {
    x = point.x;
    y = point.y;
  } else {
    JS_ToInt32(ctx, &x, argv[1]);
    JS_ToInt32(ctx, &y, argv[2]);
  }
  cv::moveWindow(name, x, y);
  return JS_UNDEFINED;
}

static JSValue
js_cv_resize_window(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  const char* name;
  uint32_t w, h;
  JSSizeData<int> size;
  name = JS_ToCString(ctx, argv[0]);

  if(js_size_read(ctx, argv[1], &size)) {
    w = size.width;
    h = size.height;
  } else {
    JS_ToUint32(ctx, &w, argv[1]);
    JS_ToUint32(ctx, &h, argv[2]);
  }

  cv::resizeWindow(name, w, h);
  return JS_UNDEFINED;
}

static JSValue
js_cv_get_screen_resolution(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  int width, height;

#if _WIN32
  width = (int)GetSystemMetrics(SM_CXSCREEN);
  height = (int)GetSystemMetrics(SM_CYSCREEN);
#elif defined(HAVE_X11)
  Display* disp = XOpenDisplay(NULL);
  Screen* scrn = DefaultScreenOfDisplay(disp);
  width = scrn->width;
  height = scrn->height;
#endif
  return js_size_new(ctx, width, height);
}

static JSValue
js_cv_get_window_image_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  const char* name;
  JSRectData<int> rect;
  name = JS_ToCString(ctx, argv[0]);

  rect = cv::getWindowImageRect(name);
  return js_rect_wrap(ctx, rect);
}

static JSValue
js_cv_get_window_property(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  const char* name;
  int32_t propId;
  name = JS_ToCString(ctx, argv[0]);
  JS_ToInt32(ctx, &propId, argv[1]);

  return JS_NewFloat64(ctx, cv::getWindowProperty(name, propId));
}

static JSValue
js_cv_set_window_property(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  const char* name;
  int32_t propId;
  double value;
  name = JS_ToCString(ctx, argv[0]);
  JS_ToInt32(ctx, &propId, argv[1]);
  JS_ToFloat64(ctx, &value, argv[2]);
  cv::setWindowProperty(name, propId, value);
  return JS_UNDEFINED;
}

static JSValue
js_cv_set_window_title(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  const char *name, *title;
  name = JS_ToCString(ctx, argv[0]);
  title = JS_ToCString(ctx, argv[1]);

  cv::setWindowTitle(name, title);
  return JS_UNDEFINED;
}

static JSValue
js_cv_destroy_window(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  const char* name;
  int32_t propId;
  name = JS_ToCString(ctx, argv[0]);

  cv::destroyWindow(name);
  auto it = std::find(window_list.cbegin(), window_list.cend(), name);

  if(it != window_list.cend())
    window_list.erase(it);

  return JS_UNDEFINED;
}

static JSValue
js_cv_destroy_all_windows(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  cv::destroyAllWindows();

  window_list.clear();

  return JS_UNDEFINED;
}

static JSValue
js_cv_create_trackbar(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  int32_t ret, count;

  const char* name = JS_ToCString(ctx, argv[0]);
  const char* window = JS_ToCString(ctx, argv[1]);

  if(name == nullptr || window == nullptr)
    return JS_EXCEPTION;

  Trackbar* userdata = js_allocate<Trackbar>(ctx);

  JS_ToInt32(ctx, &userdata->value, argv[2]);
  JS_ToInt32(ctx, &count, argv[3]);

  userdata->name = JS_NewString(ctx, name);
  userdata->window = JS_NewString(ctx, window);
  userdata->count = JS_NewInt32(ctx, count);
  userdata->handler = JS_DupValue(ctx, argv[4]);
  userdata->ctx = JS_DupContext(ctx);

  /*JSValue str = JS_ToString(ctx, userdata->handler);
  std::cout << "handler: " << JS_ToCString(ctx, str) << std::endl;*/

  auto& trackbars = trackbar_list[cv::String(window)];
  auto it = trackbars.find(cv::String(name));

  if(it != trackbars.end()) {
    JS_FreeValue(ctx, (*it).second->name);
    JS_FreeValue(ctx, (*it).second->window);
    JS_FreeValue(ctx, (*it).second->count);
    JS_FreeValue(ctx, (*it).second->handler);
    JS_FreeContext((*it).second->ctx);

    trackbars.erase(it);
  }

  try {
    ret = cv::createTrackbar(
        name,
        window,
        &userdata->value,
        count,
        [](int newValue, void* ptr) {
          Trackbar const& data = *static_cast<Trackbar*>(ptr);

          if(js_is_function(data.ctx, data.handler)) {
            JSValueConst argv[] = {
                JS_NewInt32(data.ctx, newValue),
                data.count,
                data.name,
                data.window,
            };

            JS_Call(data.ctx, data.handler, JS_UNDEFINED, 4, argv);
          }
        },
        userdata);
  } catch(const cv::Exception& e) {
    JS_FreeValue(ctx, userdata->name);
    JS_FreeValue(ctx, userdata->window);
    JS_FreeValue(ctx, userdata->count);
    JS_FreeValue(ctx, userdata->handler);
    JS_FreeContext(userdata->ctx);

    js_deallocate(ctx, userdata);
    return js_cv_throw(ctx, e);
  }

  trackbars[cv::String(name)] = userdata;

  return JS_NewInt32(ctx, ret);
}

static JSValue
js_cv_create_button(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  int32_t ret, type;
  bool initial_button_state = false;

  struct Button {
    int32_t state;
    JSValue bar_name, type;
    JSValueConst callback;
    JSContext* ctx;
  };

  const char* bar_name = JS_ToCString(ctx, argv[0]);

  if(bar_name == nullptr)
    return JS_EXCEPTION;

  Button* userdata = js_allocate<Button>(ctx);
  userdata->callback = JS_DupValue(ctx, argv[1]);

  JS_ToInt32(ctx, &type, argv[2]);
  JS_ToInt32(ctx, &userdata->state, argv[3]);

  userdata->bar_name = JS_NewString(ctx, bar_name);
  userdata->type = JS_NewInt32(ctx, type);
  userdata->ctx = ctx;

  // initial_button_state = JS_ToBool(ctx, argv[4]);

  /*JSValue str = JS_ToString(ctx, userdata->callback);
  std::cout << "callback: " << JS_ToCString(ctx, str) << std::endl;*/

  try {
    ret = cv::createButton(
        bar_name,
        [](int state, void* ptr) {
          Button const& data = *static_cast<Button*>(ptr);

          if(js_is_function(data.ctx, data.callback)) {
            JSValueConst argv[] = {JS_NewInt32(data.ctx, state), data.bar_name, data.type};
            JS_Call(data.ctx, data.callback, JS_UNDEFINED, 3, argv);
          }
        },
        userdata,
        type,
        initial_button_state);
  } catch(const cv::Exception& exception) { return js_cv_throw(ctx, exception); }

  return JS_NewInt32(ctx, ret);
}

static JSValue
js_cv_get_trackbar_pos(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  const char* name = JS_ToCString(ctx, argv[0]);
  const char* window = JS_ToCString(ctx, argv[1]);

  if(name == nullptr || window == nullptr)
    return JS_EXCEPTION;

  return JS_NewInt32(ctx, cv::getTrackbarPos(name, window));
}

static JSValue
js_cv_set_trackbar(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  int32_t val;

  const char* name = JS_ToCString(ctx, argv[0]);
  const char* window = JS_ToCString(ctx, argv[1]);

  if(name == nullptr || window == nullptr)
    return JS_EXCEPTION;

  JS_ToInt32(ctx, &val, argv[2]);

  switch(magic) {
    case 0: cv::setTrackbarPos(name, window, val); break;
    case 1: cv::setTrackbarMin(name, window, val); break;
    case 2: cv::setTrackbarMax(name, window, val); break;
  }

  return JS_UNDEFINED;
}

static JSValue
js_cv_get_mouse_wheel_delta(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  int32_t flags;

  JS_ToInt32(ctx, &flags, argv[0]);

  return JS_NewInt32(ctx, cv::getMouseWheelDelta(flags));
}

static JSValue
js_cv_set_mouse_callback(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  struct MouseHandler {
    JSValue window;
    JSValueConst handler;
    JSContext* ctx;
  };

  MouseHandler* userdata = js_allocate<MouseHandler>(ctx);

  const char* name = JS_ToCString(ctx, argv[0]);

  userdata->window = JS_DupValue(ctx, argv[0]);
  userdata->handler = JS_DupValue(ctx, argv[1]);
  userdata->ctx = ctx;

  try {
    cv::setMouseCallback(
        name,
        [](int event, int x, int y, int flags, void* ptr) {
          MouseHandler const& data = *static_cast<MouseHandler*>(ptr);

          if(js_is_function(data.ctx, data.handler)) {
            JSValueConst argv[] = {
                JS_NewInt32(data.ctx, event),
                JS_NewInt32(data.ctx, x),
                JS_NewInt32(data.ctx, y),
                JS_NewInt32(data.ctx, flags),
            };

            JS_Call(data.ctx, data.handler, JS_UNDEFINED, 4, argv);
          }
        },
        userdata);
  } catch(const cv::Exception& exception) { return js_cv_throw(ctx, exception); }

  return JS_UNDEFINED;
}

enum {
  POLL_KEY,
  WAIT_KEY,
  WAIT_KEY_EX,
};

static JSValue
js_cv_wait_key(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  int32_t delay = 0;
  union {
    int32_t i;
    char c;
  } key;

  if(argc > 0 && magic != POLL_KEY)
    JS_ToInt32(ctx, &delay, argv[0]);

  switch(magic) {

    case WAIT_KEY: {
      key.i = cv::waitKey(delay);
      break;
    }

    case WAIT_KEY_EX: {
      key.i = cv::waitKeyEx(delay);
      break;
    }

    case POLL_KEY: {
      key.i = cv::pollKey();
      break;
    }
  }

  return JS_NewInt32(ctx, key.i);
}

static JSValue
js_cv_imshow(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  const char* winname = JS_ToCString(ctx, argv[0]);
  JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[1]);

  if(image.empty())
    return JS_ThrowInternalError(ctx, "Empty image");

  /*cv::_InputArray input_array(image);

  if(input_array.isUMat())
    input_array.getUMat().addref();
  else if(input_array.isMat())
    input_array.getMat().addref();*/

  cv::imshow(winname, image);

  return JS_UNDEFINED;
}

static JSValue
js_cv_select_roi(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;
  const char* winname = JS_ToCString(ctx, argv[0]);
  JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[1]);
  BOOL showCrosshair = TRUE, fromCenter = FALSE;
  cv::Rect2d rect;

  if(argc >= 3)
    showCrosshair = JS_ToBool(ctx, argv[2]);
  if(argc >= 4)
    fromCenter = JS_ToBool(ctx, argv[3]);

  if(image.empty())
    return JS_ThrowInternalError(ctx, "argument 1 must be image");

  rect = cv::selectROI(winname, image, showCrosshair, fromCenter);

  if(rect.width > 0 && rect.height > 0)
    ret = js_rect_wrap(ctx, rect);
  else
    ret = JS_NULL;

  return ret;
}

static JSValue
js_cv_select_rois(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;
  const char* winname = JS_ToCString(ctx, argv[0]);
  JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[1]);
  BOOL showCrosshair = TRUE, fromCenter = FALSE;
  std::vector<cv::Rect> rects;

  if(argc >= 3)
    showCrosshair = JS_ToBool(ctx, argv[2]);
  if(argc >= 4)
    fromCenter = JS_ToBool(ctx, argv[3]);

  if(image.empty())
    return JS_ThrowInternalError(ctx, "argument 1 must be image");

  cv::selectROIs(winname, image, rects, showCrosshair, fromCenter);

  if(rects.size())
    ret = js_array_from(ctx, rects);
  else
    ret = JS_NULL;

  return ret;
}

static JSValue
js_cv_start_window_thread(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  return JS_NewInt32(ctx, cv::startWindowThread());
}

js_function_list_t js_highgui_static_funcs{
    JS_CFUNC_DEF("imshow", 2, js_cv_imshow),
    JS_CFUNC_DEF("namedWindow", 1, js_cv_named_window),
    JS_CFUNC_DEF("moveWindow", 2, js_cv_move_window),
    JS_CFUNC_DEF("resizeWindow", 2, js_cv_resize_window),
    JS_CFUNC_DEF("destroyWindow", 1, js_cv_destroy_window),
    JS_CFUNC_DEF("destroyAllWindows", 0, js_cv_destroy_all_windows),
    JS_CFUNC_DEF("getWindowImageRect", 1, js_cv_get_window_image_rect),
    JS_CFUNC_DEF("getWindowProperty", 2, js_cv_get_window_property),
    JS_CFUNC_DEF("setWindowProperty", 3, js_cv_set_window_property),
    JS_CFUNC_DEF("setWindowTitle", 2, js_cv_set_window_title),
    JS_CFUNC_DEF("createTrackbar", 5, js_cv_create_trackbar),
    JS_CFUNC_DEF("createButton", 2, js_cv_create_button),
    JS_CFUNC_DEF("getTrackbarPos", 2, js_cv_get_trackbar_pos),
    JS_CFUNC_MAGIC_DEF("setTrackbarPos", 3, js_cv_set_trackbar, 0),
    JS_CFUNC_MAGIC_DEF("setTrackbarMin", 3, js_cv_set_trackbar, 1),
    JS_CFUNC_MAGIC_DEF("setTrackbarMax", 3, js_cv_set_trackbar, 2),
    JS_CFUNC_DEF("getMouseWheelDelta", 1, js_cv_get_mouse_wheel_delta),
    JS_CFUNC_DEF("setMouseCallback", 2, js_cv_set_mouse_callback),
    JS_CFUNC_MAGIC_DEF("pollKey", 0, js_cv_wait_key, POLL_KEY),
    JS_CFUNC_MAGIC_DEF("waitKey", 0, js_cv_wait_key, WAIT_KEY),
    JS_CFUNC_MAGIC_DEF("waitKeyEx", 0, js_cv_wait_key, WAIT_KEY_EX),
    JS_CFUNC_DEF("getScreenResolution", 0, js_cv_get_screen_resolution),
    JS_CFUNC_DEF("selectROI", 1, js_cv_select_roi),
    JS_CFUNC_DEF("selectROIs", 2, js_cv_select_rois),
    JS_CFUNC_DEF("displayOverlay", 2, js_cv_display_overlay),
    JS_CFUNC_DEF("startWindowThread", 0, js_cv_start_window_thread),
};

js_function_list_t js_highgui_constants{
    JS_CV_CONSTANT(WINDOW_NORMAL),       JS_CV_CONSTANT(WINDOW_AUTOSIZE),     JS_CV_CONSTANT(WINDOW_OPENGL),         JS_CV_CONSTANT(WINDOW_FULLSCREEN),
    JS_CV_CONSTANT(WINDOW_FREERATIO),    JS_CV_CONSTANT(WINDOW_KEEPRATIO),    JS_CV_CONSTANT(WINDOW_GUI_EXPANDED),   JS_CV_CONSTANT(WINDOW_GUI_NORMAL),
    JS_CV_CONSTANT(WND_PROP_FULLSCREEN), JS_CV_CONSTANT(WND_PROP_AUTOSIZE),   JS_CV_CONSTANT(WND_PROP_ASPECT_RATIO), JS_CV_CONSTANT(WND_PROP_OPENGL),
    JS_CV_CONSTANT(WND_PROP_VISIBLE),    JS_CV_CONSTANT(WND_PROP_TOPMOST),    JS_CV_CONSTANT(EVENT_MOUSEMOVE),       JS_CV_CONSTANT(EVENT_LBUTTONDOWN),
    JS_CV_CONSTANT(EVENT_RBUTTONDOWN),   JS_CV_CONSTANT(EVENT_MBUTTONDOWN),   JS_CV_CONSTANT(EVENT_LBUTTONUP),       JS_CV_CONSTANT(EVENT_RBUTTONUP),
    JS_CV_CONSTANT(EVENT_MBUTTONUP),     JS_CV_CONSTANT(EVENT_LBUTTONDBLCLK), JS_CV_CONSTANT(EVENT_RBUTTONDBLCLK),   JS_CV_CONSTANT(EVENT_MBUTTONDBLCLK),
    JS_CV_CONSTANT(EVENT_MOUSEWHEEL),    JS_CV_CONSTANT(EVENT_MOUSEHWHEEL),   JS_CV_CONSTANT(EVENT_FLAG_LBUTTON),    JS_CV_CONSTANT(EVENT_FLAG_RBUTTON),
    JS_CV_CONSTANT(EVENT_FLAG_MBUTTON),  JS_CV_CONSTANT(EVENT_FLAG_CTRLKEY),  JS_CV_CONSTANT(EVENT_FLAG_SHIFTKEY),   JS_CV_CONSTANT(EVENT_FLAG_ALTKEY),
};

extern "C" int
js_highgui_init(JSContext* ctx, JSModuleDef* m) {

  if(m) {
    JS_SetModuleExportList(ctx, m, js_highgui_static_funcs.data(), js_highgui_static_funcs.size());
    JS_SetModuleExportList(ctx, m, js_highgui_constants.data(), js_highgui_constants.size());
  }

  /*if(JS_IsObject(cv_class)) {
    JS_SetPropertyFunctionList(ctx, cv_class, js_highgui_static_funcs.data(),
  js_highgui_static_funcs.size()); JS_SetPropertyFunctionList(ctx, cv_class,
  js_highgui_constants.data(), js_highgui_constants.size());
  }*/

  return 0;
}

extern "C" void
js_highgui_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_highgui_static_funcs.data(), js_highgui_static_funcs.size());
  JS_AddModuleExportList(ctx, m, js_highgui_constants.data(), js_highgui_constants.size());
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_highgui
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  if(!(m = JS_NewCModule(ctx, module_name, &js_highgui_init)))
    return NULL;
  js_highgui_export(ctx, m);
  return m;
}
