#include "jsbindings.hpp"
#include "js_alloc.hpp"
#include "js_point.hpp"
#include "js_size.hpp"
#include "js_rect.hpp"
#include "js_umat.hpp"
#include <opencv2/highgui.hpp>

enum { DISPLAY_OVERLAY };

static std::vector<cv::String> window_list;

static JSValue
js_cv_gui_methods(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValue ret = JS_UNDEFINED;
  switch(magic) {
    case DISPLAY_OVERLAY: {
      const char *winname, *text;
      int32_t delayms = 0;
      winname = JS_ToCString(ctx, argv[0]);
      text = JS_ToCString(ctx, argv[1]);
      if(argc > 2)
        JS_ToInt32(ctx, &delayms, argv[2]);

      cv::displayOverlay(winname, text, delayms);
      break;
    }
  }
  return ret;
}

static JSValue
js_cv_named_window(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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
js_cv_move_window(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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
js_cv_resize_window(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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
js_cv_get_window_image_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  const char* name;
  JSRectData<int> rect;
  name = JS_ToCString(ctx, argv[0]);

  rect = cv::getWindowImageRect(name);
  return js_rect_wrap(ctx, rect);
}

static JSValue
js_cv_get_window_property(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  const char* name;
  int32_t propId;
  name = JS_ToCString(ctx, argv[0]);
  JS_ToInt32(ctx, &propId, argv[1]);

  return JS_NewFloat64(ctx, cv::getWindowProperty(name, propId));
}

static JSValue
js_cv_set_window_property(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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
js_cv_set_window_title(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  const char *name, *title;
  name = JS_ToCString(ctx, argv[0]);
  title = JS_ToCString(ctx, argv[1]);

  cv::setWindowTitle(name, title);
  return JS_UNDEFINED;
}

static JSValue
js_cv_destroy_window(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
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
js_cv_create_trackbar(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  const char *name, *window;
  int32_t ret, count;
  struct Trackbar {
    int32_t value;
    JSValue name, window, count;
    JSValueConst handler;
    JSContext* ctx;
  };
  Trackbar* userdata;

  name = JS_ToCString(ctx, argv[0]);
  window = JS_ToCString(ctx, argv[1]);

  if(name == nullptr || window == nullptr)
    return JS_EXCEPTION;

  userdata = js_allocate<Trackbar>(ctx);

  JS_ToInt32(ctx, &userdata->value, argv[2]);
  JS_ToInt32(ctx, &count, argv[3]);

  userdata->name = JS_NewString(ctx, name);
  userdata->window = JS_NewString(ctx, window);
  userdata->count = JS_NewInt32(ctx, count);
  userdata->handler = JS_DupValue(ctx, argv[4]);
  userdata->ctx = ctx;

  /*JSValue str = JS_ToString(ctx, userdata->handler);
  std::cout << "handler: " << JS_ToCString(ctx, str) << std::endl;*/

  ret = cv::createTrackbar(
      name,
      window,
      &userdata->value,
      count,
      [](int newValue, void* ptr) {
        Trackbar const& data = *static_cast<Trackbar*>(ptr);

        if(JS_IsFunction(data.ctx, data.handler)) {
          JSValueConst argv[] = {JS_NewInt32(data.ctx, newValue), data.count, data.name, data.window};

          JS_Call(data.ctx, data.handler, JS_UNDEFINED, 4, argv);
        }
      },
      userdata);

  return JS_NewInt32(ctx, ret);
}

static JSValue
js_cv_create_button(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  const char* bar_name;
  int32_t ret, type;
  bool initial_button_state = false;

  struct Button {
    int32_t state;
    JSValue bar_name, type;
    JSValueConst callback;
    JSContext* ctx;
  };
  Button* userdata;

  bar_name = JS_ToCString(ctx, argv[0]);

  if(bar_name == nullptr)
    return JS_EXCEPTION;

  userdata = js_allocate<Button>(ctx);
  userdata->callback = JS_DupValue(ctx, argv[1]);

  JS_ToInt32(ctx, &type, argv[2]);
  JS_ToInt32(ctx, &userdata->state, argv[3]);

  userdata->bar_name = JS_NewString(ctx, bar_name);
  userdata->type = JS_NewInt32(ctx, type);

  userdata->ctx = ctx;

  // initial_button_state = JS_ToBool(ctx, argv[4]);

  /*JSValue str = JS_ToString(ctx, userdata->callback);
  std::cout << "callback: " << JS_ToCString(ctx, str) << std::endl;*/

  ret = cv::createButton(
      bar_name,
      [](int state, void* ptr) {
        Button const& data = *static_cast<Button*>(ptr);
        if(JS_IsFunction(data.ctx, data.callback)) {
          JSValueConst argv[] = {JS_NewInt32(data.ctx, state), data.bar_name, data.type};
          JS_Call(data.ctx, data.callback, JS_UNDEFINED, 3, argv);
        }
      },
      userdata,
      type,
      initial_button_state);

  return JS_NewInt32(ctx, ret);
}

static JSValue
js_cv_get_trackbar_pos(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  const char *name, *window;

  name = JS_ToCString(ctx, argv[0]);
  window = JS_ToCString(ctx, argv[1]);

  if(name == nullptr || window == nullptr)
    return JS_EXCEPTION;

  return JS_NewInt32(ctx, cv::getTrackbarPos(name, window));
}

static JSValue
js_cv_set_trackbar(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  const char *name, *window;
  int32_t val;

  name = JS_ToCString(ctx, argv[0]);
  window = JS_ToCString(ctx, argv[1]);

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
js_cv_get_mouse_wheel_delta(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  int32_t flags;

  JS_ToInt32(ctx, &flags, argv[0]);

  return JS_NewInt32(ctx, cv::getMouseWheelDelta(flags));
}

static JSValue
js_cv_set_mouse_callback(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  const char* name;
  struct MouseHandler {
    JSValue window;
    JSValueConst handler;
    JSContext* ctx;
  };
  MouseHandler* userdata;

  userdata = js_allocate<MouseHandler>(ctx);

  name = JS_ToCString(ctx, argv[0]);

  userdata->window = JS_DupValue(ctx, argv[0]);
  userdata->handler = JS_DupValue(ctx, argv[1]);
  userdata->ctx = ctx;

  cv::setMouseCallback(
      name,
      [](int event, int x, int y, int flags, void* ptr) {
        MouseHandler const& data = *static_cast<MouseHandler*>(ptr);

        if(JS_IsFunction(data.ctx, data.handler)) {
          JSValueConst argv[] = {JS_NewInt32(data.ctx, event),
                                 JS_NewInt32(data.ctx, x),
                                 JS_NewInt32(data.ctx, y),

                                 JS_NewInt32(data.ctx, flags)};

          JS_Call(data.ctx, data.handler, JS_UNDEFINED, 4, argv);
        }
      },
      userdata);
  return JS_UNDEFINED;
}

static JSValue
js_cv_wait_key(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  int32_t delay = 0;
  union {
    int32_t i;
    char c;
  } key;
  JSValue ret;

  if(argc > 0)
    JS_ToInt32(ctx, &delay, argv[0]);

  key.i = cv::waitKey(delay);

  if(0 && isalnum(key.c)) {
    char ch[2] = {key.c, 0};

    ret = JS_NewString(ctx, ch);
  } else {
    ret = JS_NewInt32(ctx, key.i);
  }
  return ret;
}

static JSValue
js_cv_wait_key_ex(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  int32_t delay = 0;
  int keyCode;
  JSValue ret;

  if(argc > 0)
    JS_ToInt32(ctx, &delay, argv[0]);

  keyCode = cv::waitKeyEx(delay);

  return JS_NewInt32(ctx, keyCode);
}

static JSValue
js_cv_imshow(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  const char* winname = JS_ToCString(ctx, argv[0]);
  JSInputOutputArray image = js_umat_or_mat(ctx, argv[1]);

  if(image.empty())
    return JS_ThrowInternalError(ctx, "Empty image");
  cv::_InputArray input_array(image);

  /*  if(input_array.isUMat())
      input_array.getUMat().addref();
    else if(input_array.isMat())
      input_array.getMat().addref();*/

  cv::imshow(winname, image);

  return JS_UNDEFINED;
}

void
js_cv_finalizer(JSRuntime* rt, JSValue val) {

  for(const auto& name : window_list) {
    std::cerr << "Destroy window '" << name << "'" << std::endl;
    cv::destroyWindow(name);
  }

  // JS_FreeValueRT(rt, val);
  // JS_FreeValueRT(rt, cv_class);
}

JSClassDef js_cv_class = {.class_name = "cv", .finalizer = js_cv_finalizer};

typedef std::vector<JSCFunctionListEntry> js_function_list_t;

js_function_list_t js_cv_static_funcs{
    JS_CFUNC_DEF("imshow", 2, js_cv_imshow),
    JS_CFUNC_DEF("namedWindow", 1, js_cv_named_window),
    JS_CFUNC_DEF("moveWindow", 2, js_cv_move_window),
    JS_CFUNC_DEF("resizeWindow", 2, js_cv_resize_window),
    JS_CFUNC_DEF("destroyWindow", 1, js_cv_destroy_window),
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
    JS_CFUNC_DEF("waitKey", 0, js_cv_wait_key),
    JS_CFUNC_DEF("waitKeyEx", 0, js_cv_wait_key_ex),
    JS_CFUNC_MAGIC_DEF("displayOverlay", 2, js_cv_gui_methods, DISPLAY_OVERLAY),

};
js_function_list_t js_cv_constants{

    JS_CV_CONSTANT(WINDOW_NORMAL),       JS_CV_CONSTANT(WINDOW_AUTOSIZE),       JS_CV_CONSTANT(WINDOW_OPENGL),
    JS_CV_CONSTANT(WINDOW_FULLSCREEN),   JS_CV_CONSTANT(WINDOW_FREERATIO),      JS_CV_CONSTANT(WINDOW_KEEPRATIO),
    JS_CV_CONSTANT(WINDOW_GUI_EXPANDED), JS_CV_CONSTANT(WINDOW_GUI_NORMAL),     JS_CV_CONSTANT(WND_PROP_FULLSCREEN),
    JS_CV_CONSTANT(WND_PROP_AUTOSIZE),   JS_CV_CONSTANT(WND_PROP_ASPECT_RATIO), JS_CV_CONSTANT(WND_PROP_OPENGL),
    JS_CV_CONSTANT(WND_PROP_VISIBLE),    JS_CV_CONSTANT(WND_PROP_TOPMOST),      JS_CV_CONSTANT(EVENT_MOUSEMOVE),
    JS_CV_CONSTANT(EVENT_LBUTTONDOWN),   JS_CV_CONSTANT(EVENT_RBUTTONDOWN),     JS_CV_CONSTANT(EVENT_MBUTTONDOWN),
    JS_CV_CONSTANT(EVENT_LBUTTONUP),     JS_CV_CONSTANT(EVENT_RBUTTONUP),       JS_CV_CONSTANT(EVENT_MBUTTONUP),
    JS_CV_CONSTANT(EVENT_LBUTTONDBLCLK), JS_CV_CONSTANT(EVENT_RBUTTONDBLCLK),   JS_CV_CONSTANT(EVENT_MBUTTONDBLCLK),
    JS_CV_CONSTANT(EVENT_MOUSEWHEEL),    JS_CV_CONSTANT(EVENT_MOUSEHWHEEL),     JS_CV_CONSTANT(EVENT_FLAG_LBUTTON),
    JS_CV_CONSTANT(EVENT_FLAG_RBUTTON),  JS_CV_CONSTANT(EVENT_FLAG_MBUTTON),    JS_CV_CONSTANT(EVENT_FLAG_CTRLKEY),
    JS_CV_CONSTANT(EVENT_FLAG_SHIFTKEY), JS_CV_CONSTANT(EVENT_FLAG_ALTKEY),

};

extern "C" int
js_cv_init(JSContext* ctx, JSModuleDef* m) {
  JSAtom atom;
  JSValue cv_class, g = JS_GetGlobalObject(ctx);
  if(m) {
    JS_SetModuleExportList(ctx, m, js_cv_static_funcs.data(), js_cv_static_funcs.size());
    JS_SetModuleExportList(ctx, m, js_cv_constants.data(), js_cv_constants.size());
  }
  atom = JS_NewAtom(ctx, "cv");

  if(JS_HasProperty(ctx, g, atom)) {
    cv_class = JS_GetProperty(ctx, g, atom);
  } else {
    cv_class = JS_NewObject(ctx);
  }
  JS_SetPropertyFunctionList(ctx, cv_class, js_cv_static_funcs.data(), js_cv_static_funcs.size());
  JS_SetPropertyFunctionList(ctx, cv_class, js_cv_constants.data(), js_cv_constants.size());

  if(!JS_HasProperty(ctx, g, atom)) {
    JS_SetPropertyInternal(ctx, g, atom, cv_class, 0);
  }

  JS_SetModuleExport(ctx, m, "default", cv_class);

  JS_FreeValue(ctx, g);
  JS_FreeAtom(ctx, atom);
  return 0;
}

extern "C" VISIBLE void
js_cv_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_cv_static_funcs.data(), js_cv_static_funcs.size());
  JS_AddModuleExportList(ctx, m, js_cv_constants.data(), js_cv_constants.size());
  JS_AddModuleExport(ctx, m, "default");
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_highgui
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_cv_init);
  if(!m)
    return NULL;
  js_cv_export(ctx, m);
  return m;
}