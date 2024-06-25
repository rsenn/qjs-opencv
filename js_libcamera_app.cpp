#include "js_libcamera_app.hpp"
#include "js_alloc.hpp"
#include "js_mat.hpp"
#include "jsbindings.hpp"
#include <opencv2/core.hpp>
#include <quickjs.h>

//#include "LCCV/include/libcamera_app.hpp"
#include <lccv.hpp>

extern "C" int js_libcamera_app_init(JSContext*, JSModuleDef*);

extern "C" {
JSValue libcamera_app_proto = JS_UNDEFINED, libcamera_app_class = JS_UNDEFINED, libcamera_app_options_proto = JS_UNDEFINED;
thread_local JSClassID js_libcamera_app_class_id = 0, js_libcamera_app_options_class_id = 0;
}
enum {
  OPTION_HELP = 0,
  OPTION_VERSION,
  OPTION_LIST_CAMERAS,
  OPTION_VERBOSE,
  OPTION_TIMEOUT,
  OPTION_PHOTO_WIDTH,
  OPTION_PHOTO_HEIGHT,
  OPTION_VIDEO_WIDTH,
  OPTION_VIDEO_HEIGHT,
  OPTION_RAWFULL,
  OPTION_TRANSFORM,
  OPTION_ROI_X,
  OPTION_ROI_Y,
  OPTION_ROI_WIDTH,
  OPTION_ROI_HEIGHT,
  OPTION_SHUTTER,
  OPTION_GAIN,
  OPTION_EV,
  OPTION_AWB_GAIN_R,
  OPTION_AWB_GAIN_B,
  OPTION_BRIGHTNESS,
  OPTION_CONTRAST,
  OPTION_SATURATION,
  OPTION_SHARPNESS,
  OPTION_FRAMERATE,
  OPTION_DENOISE,
  OPTION_INFO_TEXT,
  OPTION_CAMERA
};

void
js_libcamera_app_options_finalizer(JSRuntime* rt, JSValue val) {
  JSLibcameraAppOptionsData* s = static_cast<JSLibcameraAppOptionsData*>(JS_GetOpaque(val, js_libcamera_app_options_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  /*s->~JSLibcameraAppOptionsData();
  js_deallocate(rt, s);*/

  JS_FreeValueRT(rt, val);
}

JSValue
js_libcamera_app_options_wrap(JSContext* ctx, JSLibcameraAppOptionsData* opt) {
  JSValue ret;

  ret = JS_NewObjectProtoClass(ctx, libcamera_app_options_proto, js_libcamera_app_options_class_id);

  // cap->addref();

  JS_SetOpaque(ret, opt);

  return ret;
}

static JSValue
js_libcamera_app_options_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSLibcameraAppOptionsData* o = static_cast<JSLibcameraAppOptionsData*>(JS_GetOpaque2(ctx, this_val, js_libcamera_app_options_class_id));
  JSValue ret = JS_UNDEFINED;

  switch(magic) {
    case OPTION_HELP: {
      ret = JS_NewBool(ctx, o->help);
      break;
    }

    case OPTION_VERSION: {
      ret = JS_NewBool(ctx, o->version);
      break;
    }

    case OPTION_LIST_CAMERAS: {
      ret = JS_NewBool(ctx, o->list_cameras);
      break;
    }

    case OPTION_VERBOSE: {
      ret = JS_NewBool(ctx, o->verbose);
      break;
    }

    case OPTION_TIMEOUT: {
      ret = JS_NewInt64(ctx, o->timeout);
      break;
    }

    case OPTION_PHOTO_WIDTH: {
      ret = JS_NewUint32(ctx, o->photo_width);
      break;
    }

    case OPTION_PHOTO_HEIGHT: {
      ret = JS_NewUint32(ctx, o->photo_height);
      break;
    }

    case OPTION_VIDEO_WIDTH: {
      ret = JS_NewUint32(ctx, o->video_width);
      break;
    }

    case OPTION_VIDEO_HEIGHT: {
      ret = JS_NewUint32(ctx, o->video_height);
      break;
    }

    case OPTION_RAWFULL: {
      ret = JS_NewBool(ctx, o->rawfull);
      break;
    }

    case OPTION_TRANSFORM: {
      ret = JS_NewInt32(ctx, int32_t(o->transform));
      break;
    }

    case OPTION_ROI_X: {
      ret = JS_NewFloat64(ctx, o->roi_x);
      break;
    }

    case OPTION_ROI_Y: {
      ret = JS_NewFloat64(ctx, o->roi_y);
      break;
    }

    case OPTION_ROI_WIDTH: {
      ret = JS_NewFloat64(ctx, o->roi_width);
      break;
    }

    case OPTION_ROI_HEIGHT: {
      ret = JS_NewFloat64(ctx, o->roi_height);
      break;
    }

    case OPTION_SHUTTER: {
      ret = JS_NewFloat64(ctx, o->shutter);
      break;
    }

    case OPTION_GAIN: {
      ret = JS_NewFloat64(ctx, o->gain);
      break;
    }

    case OPTION_EV: {
      ret = JS_NewFloat64(ctx, o->ev);
      break;
    }

    case OPTION_AWB_GAIN_R: {
      ret = JS_NewFloat64(ctx, o->awb_gain_r);
      break;
    }

    case OPTION_AWB_GAIN_B: {
      ret = JS_NewFloat64(ctx, o->awb_gain_b);
      break;
    }

    case OPTION_BRIGHTNESS: {
      ret = JS_NewFloat64(ctx, o->brightness);
      break;
    }

    case OPTION_CONTRAST: {
      ret = JS_NewFloat64(ctx, o->contrast);
      break;
    }

    case OPTION_SATURATION: {
      ret = JS_NewFloat64(ctx, o->saturation);
      break;
    }

    case OPTION_SHARPNESS: {
      ret = JS_NewFloat64(ctx, o->sharpness);
      break;
    }

    case OPTION_FRAMERATE: {
      ret = JS_NewFloat64(ctx, o->framerate);
      break;
    }

    case OPTION_DENOISE: {
      ret = JS_NewString(ctx, o->denoise.c_str());
      break;
    }

    case OPTION_INFO_TEXT: {
      ret = JS_NewString(ctx, o->info_text.c_str());
      break;
    }

    case OPTION_CAMERA: {
      ret = JS_NewUint32(ctx, o->camera);
      break;
    }
  }

  return ret;
}

static JSValue
js_libcamera_app_options_set(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic) {
  JSLibcameraAppOptionsData* o = static_cast<JSLibcameraAppOptionsData*>(JS_GetOpaque2(ctx, this_val, js_libcamera_app_options_class_id));

  switch(magic) {
    case OPTION_HELP: {
      o->help = JS_ToBool(ctx, value);
      break;
    }

    case OPTION_VERSION: {
      o->version = JS_ToBool(ctx, value);
      break;
    }

    case OPTION_LIST_CAMERAS: {
      o->list_cameras = JS_ToBool(ctx, value);
      break;
    }

    case OPTION_VERBOSE: {
      o->verbose = JS_ToBool(ctx, value);
      break;
    }

    case OPTION_TIMEOUT: {
      uint64_t ms;
      JS_ToIndex(ctx, &ms, value);
      o->timeout = ms;
      break;
    }

    case OPTION_PHOTO_WIDTH: {
      uint32_t w;
      JS_ToUint32(ctx, &w, value);
      o->photo_width = w;
      break;
    }

    case OPTION_PHOTO_HEIGHT: {
      uint32_t h;
      JS_ToUint32(ctx, &h, value);
      o->photo_height = h;
      break;
      break;
    }

    case OPTION_VIDEO_WIDTH: {
      uint32_t w;
      JS_ToUint32(ctx, &w, value);
      o->video_width = w;
      break;
    }

    case OPTION_VIDEO_HEIGHT: {
      uint32_t h;
      JS_ToUint32(ctx, &h, value);
      o->video_height = h;
      break;
    }

    case OPTION_RAWFULL: {
      o->rawfull = JS_ToBool(ctx, value);
      break;
    }

    case OPTION_TRANSFORM: {
      int32_t t;
      JS_ToInt32(ctx, &t, value);
      o->transform = libcamera::Transform(t);
      break;
    }

    case OPTION_ROI_X: {
      double x;
      JS_ToFloat64(ctx, &x, value);
      o->roi_x = x;
      break;
    }

    case OPTION_ROI_Y: {
      double y;
      JS_ToFloat64(ctx, &y, value);
      o->roi_x = y;
      break;
    }

    case OPTION_ROI_WIDTH: {
      double w;
      JS_ToFloat64(ctx, &w, value);
      o->roi_width = w;
      break;
    }

    case OPTION_ROI_HEIGHT: {
      double h;
      JS_ToFloat64(ctx, &h, value);
      o->roi_height = h;
      break;
    }

    case OPTION_SHUTTER: {
      double s;
      JS_ToFloat64(ctx, &s, value);
      o->shutter = s;
      break;
    }

    case OPTION_GAIN: {
      double g;
      JS_ToFloat64(ctx, &g, value);
      o->gain = g;
      break;
    }

    case OPTION_EV: {
      double ev;
      JS_ToFloat64(ctx, &ev, value);
      o->ev = ev;
      break;
    }

    case OPTION_AWB_GAIN_R: {
      double gr;
      JS_ToFloat64(ctx, &gr, value);
      o->awb_gain_r = gr;
      break;
    }

    case OPTION_AWB_GAIN_B: {
      double gb;
      JS_ToFloat64(ctx, &gb, value);
      o->awb_gain_b = gb;
      break;
    }

    case OPTION_BRIGHTNESS: {
      double br;
      JS_ToFloat64(ctx, &br, value);
      o->brightness = br;
      break;
    }

    case OPTION_CONTRAST: {
      double ct;
      JS_ToFloat64(ctx, &ct, value);
      o->contrast = ct;
      break;
    }

    case OPTION_SATURATION: {
      double st;
      JS_ToFloat64(ctx, &st, value);
      o->saturation = st;
      break;
    }

    case OPTION_SHARPNESS: {
      double sh;
      JS_ToFloat64(ctx, &sh, value);
      o->sharpness = sh;
      break;
    }

    case OPTION_FRAMERATE: {
      double fr;
      JS_ToFloat64(ctx, &fr, value);
      o->framerate = fr;
      break;
    }

    case OPTION_DENOISE: {
      const char* dn;

      if((dn = JS_ToCString(ctx, value))) {
        o->denoise = dn;
        JS_FreeCString(ctx, dn);
      }
      break;
    }

    case OPTION_INFO_TEXT: {
      const char* in;

      if((in = JS_ToCString(ctx, value))) {
        o->info_text = in;
        JS_FreeCString(ctx, in);
      }
      break;
    }

    case OPTION_CAMERA: {
      uint32_t c;
      JS_ToUint32(ctx, &c, value);
      o->camera = c;
      break;
    }
  }

  return JS_UNDEFINED;
}

JSClassDef js_libcamera_app_options_class = {
    .class_name = "LibcameraAppOptions",
    .finalizer = js_libcamera_app_options_finalizer,
};

const JSCFunctionListEntry js_libcamera_app_options_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("help", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_HELP),
    JS_CGETSET_MAGIC_DEF("version", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_VERSION),
    JS_CGETSET_MAGIC_DEF("listCameras", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_LIST_CAMERAS),
    JS_CGETSET_MAGIC_DEF("verbose", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_VERBOSE),
    JS_CGETSET_MAGIC_DEF("ms", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_TIMEOUT),
    JS_CGETSET_MAGIC_DEF("photoWidth", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_PHOTO_WIDTH),
    JS_CGETSET_MAGIC_DEF("photoHeight", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_PHOTO_HEIGHT),
    JS_CGETSET_MAGIC_DEF("videoWidth", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_VIDEO_WIDTH),
    JS_CGETSET_MAGIC_DEF("videoHeight", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_VIDEO_HEIGHT),
    JS_CGETSET_MAGIC_DEF("rawfull", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_RAWFULL),
    JS_CGETSET_MAGIC_DEF("transform", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_TRANSFORM),
    JS_CGETSET_MAGIC_DEF("roiX", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_ROI_X),
    JS_CGETSET_MAGIC_DEF("roiY", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_ROI_Y),
    JS_CGETSET_MAGIC_DEF("roiWidth", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_ROI_WIDTH),
    JS_CGETSET_MAGIC_DEF("roiHeight", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_ROI_HEIGHT),
    JS_CGETSET_MAGIC_DEF("shutter", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_SHUTTER),
    JS_CGETSET_MAGIC_DEF("gain", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_GAIN),
    JS_CGETSET_MAGIC_DEF("ev", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_EV),
    JS_CGETSET_MAGIC_DEF("awbGainR", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_AWB_GAIN_R),
    JS_CGETSET_MAGIC_DEF("awbGainB", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_AWB_GAIN_B),
    JS_CGETSET_MAGIC_DEF("brightness", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_BRIGHTNESS),
    JS_CGETSET_MAGIC_DEF("contrast", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_CONTRAST),
    JS_CGETSET_MAGIC_DEF("saturation", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_SATURATION),
    JS_CGETSET_MAGIC_DEF("sharpness", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_SHARPNESS),
    JS_CGETSET_MAGIC_DEF("framerate", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_FRAMERATE),
    JS_CGETSET_MAGIC_DEF("denoise", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_DENOISE),
    JS_CGETSET_MAGIC_DEF("infoText", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_INFO_TEXT),
    JS_CGETSET_MAGIC_DEF("camera", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_CAMERA),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "LibcameraAppOptions", JS_PROP_CONFIGURABLE),

};

static JSValue
js_libcamera_app_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSLibcameraAppData* s;
  JSValue obj = JS_UNDEFINED;
  JSValue proto, ret;

  s = js_allocate<JSLibcameraAppData>(ctx);
  if(!s)
    return JS_ThrowOutOfMemory(ctx);

  new(s) JSLibcameraAppData();

  /* using new_target to get the prototype is necessary when the
     class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_libcamera_app_class_id);
  JS_FreeValue(ctx, proto);
  if(JS_IsException(obj))
    goto fail;
  JS_SetOpaque(obj, s);
  return obj;
fail:
  js_deallocate(ctx, s);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSLibcameraAppData*
js_libcamera_app_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSLibcameraAppData*>(JS_GetOpaque2(ctx, val, js_libcamera_app_class_id));
}

void
js_libcamera_app_finalizer(JSRuntime* rt, JSValue val) {
  JSLibcameraAppData* s = static_cast<JSLibcameraAppData*>(JS_GetOpaque(val, js_libcamera_app_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  s->~JSLibcameraAppData();
  js_deallocate(rt, s);

  JS_FreeValueRT(rt, val);
}

enum {
  METHOD_CAMERA_ID = 0,
  METHOD_OPEN_CAMERA,
  METHOD_CLOSE_CAMERA,
  METHOD_CONFIGURE_STILL,
  METHOD_CONFIGURE_VIEWFINDER,
  METHOD_TEARDOWN,
  METHOD_START_CAMERA,
  METHOD_STOP_CAMERA,
  METHOD_WAIT,
  METHOD_POST_MESSAGE,
  METHOD_GET_STREAM,
  METHOD_VIEWFINDER_STREAM,
  METHOD_STILL_STREAM,
  METHOD_RAW_STREAM,
  METHOD_VIDEO_STREAM,
  METHOD_LORES_STREAM,
  METHOD_GET_MAIN_STREAM,
  METHOD_MMAP,
  METHOD_SET_CONTROLS,
  METHOD_STREAM_DIMENSIONS,

};

static JSValue
js_libcamera_app_options(JSContext* ctx, JSValueConst this_val) {
  JSLibcameraAppData* cam = static_cast<JSLibcameraAppData*>(JS_GetOpaque2(ctx, this_val, js_libcamera_app_class_id));

  return js_libcamera_app_options_wrap(ctx, cam->GetOptions());
}

static JSValue
js_libcamera_app_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSLibcameraAppData* cam = static_cast<JSLibcameraAppData*>(JS_GetOpaque2(ctx, this_val, js_libcamera_app_class_id));
  JSValue ret = JS_UNDEFINED;
  int32_t propID;
  double value = 0;

  try {
    switch(magic) {
      case METHOD_CAMERA_ID: {
        break;
      }

      case METHOD_OPEN_CAMERA: {
        cam->OpenCamera();
        break;
      }

      case METHOD_CLOSE_CAMERA: {
        cam->CloseCamera();
        break;
      }

      case METHOD_CONFIGURE_STILL: {
        break;
      }

      case METHOD_CONFIGURE_VIEWFINDER: {
        break;
      }

      case METHOD_TEARDOWN: {
        break;
      }

      case METHOD_START_CAMERA: {
        break;
      }

      case METHOD_STOP_CAMERA: {
        break;
      }

      case METHOD_WAIT: {
        break;
      }

      case METHOD_POST_MESSAGE: {
        break;
      }

      case METHOD_GET_STREAM: {
        break;
      }

      case METHOD_VIEWFINDER_STREAM: {
        break;
      }

      case METHOD_STILL_STREAM: {
        break;
      }

      case METHOD_RAW_STREAM: {
        break;
      }

      case METHOD_VIDEO_STREAM: {
        break;
      }

      case METHOD_LORES_STREAM: {
        break;
      }

      case METHOD_GET_MAIN_STREAM: {
        break;
      }

      case METHOD_MMAP: {
        break;
      }

      case METHOD_SET_CONTROLS: {
        break;
      }

      case METHOD_STREAM_DIMENSIONS: {
        break;
      }
    }
  } catch(const std::exception& e) {
    std::string msg(e.what());

    ret = JS_ThrowInternalError(ctx, "Exception %s", msg.c_str());
  }

  return ret;
}

JSValue
js_libcamera_app_wrap(JSContext* ctx, JSLibcameraAppData* cam) {
  JSValue ret;

  ret = JS_NewObjectProtoClass(ctx, libcamera_app_proto, js_libcamera_app_class_id);

  // cap->addref();

  JS_SetOpaque(ret, cam);

  return ret;
}

JSClassDef js_libcamera_app_class = {
    .class_name = "LibcameraApp",
    .finalizer = js_libcamera_app_finalizer,
};

const JSCFunctionListEntry js_libcamera_app_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("cameraId", 0, js_libcamera_app_method, METHOD_CAMERA_ID),
    JS_CFUNC_MAGIC_DEF("openCamera", 0, js_libcamera_app_method, METHOD_OPEN_CAMERA),
    JS_CFUNC_MAGIC_DEF("closeCamera", 0, js_libcamera_app_method, METHOD_CLOSE_CAMERA),
    JS_CFUNC_MAGIC_DEF("configureStill", 0, js_libcamera_app_method, METHOD_CONFIGURE_STILL),
    JS_CFUNC_MAGIC_DEF("configureViewfinder", 0, js_libcamera_app_method, METHOD_CONFIGURE_VIEWFINDER),
    JS_CFUNC_MAGIC_DEF("teardown", 0, js_libcamera_app_method, METHOD_TEARDOWN),
    JS_CFUNC_MAGIC_DEF("startCamera", 0, js_libcamera_app_method, METHOD_START_CAMERA),
    JS_CFUNC_MAGIC_DEF("stopCamera", 0, js_libcamera_app_method, METHOD_STOP_CAMERA),
    JS_CFUNC_MAGIC_DEF("wait", 0, js_libcamera_app_method, METHOD_WAIT),
    JS_CFUNC_MAGIC_DEF("postMessage", 0, js_libcamera_app_method, METHOD_POST_MESSAGE),
    JS_CFUNC_MAGIC_DEF("getStream", 0, js_libcamera_app_method, METHOD_GET_STREAM),
    JS_CFUNC_MAGIC_DEF("viewfinderStream", 0, js_libcamera_app_method, METHOD_VIEWFINDER_STREAM),
    JS_CFUNC_MAGIC_DEF("stillStream", 0, js_libcamera_app_method, METHOD_STILL_STREAM),
    JS_CFUNC_MAGIC_DEF("rawStream", 0, js_libcamera_app_method, METHOD_RAW_STREAM),
    JS_CFUNC_MAGIC_DEF("videoStream", 0, js_libcamera_app_method, METHOD_VIDEO_STREAM),
    JS_CFUNC_MAGIC_DEF("loresStream", 0, js_libcamera_app_method, METHOD_LORES_STREAM),
    JS_CFUNC_MAGIC_DEF("getMainStream", 0, js_libcamera_app_method, METHOD_GET_MAIN_STREAM),
    JS_CFUNC_MAGIC_DEF("mmap", 0, js_libcamera_app_method, METHOD_MMAP),
    JS_CFUNC_MAGIC_DEF("setControls", 0, js_libcamera_app_method, METHOD_SET_CONTROLS),
    JS_CFUNC_MAGIC_DEF("streamDimensions", 0, js_libcamera_app_method, METHOD_STREAM_DIMENSIONS),
    JS_CGETSET_DEF("options", js_libcamera_app_options, 0),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "LibcameraApp", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_libcamera_app_static_funcs[] = {
    JS_PROP_INT32_DEF("FLAG_STILL_NONE", LibcameraApp::FLAG_STILL_NONE, 0),
    JS_PROP_INT32_DEF("FLAG_STILL_BGR", LibcameraApp::FLAG_STILL_BGR, 0),
    JS_PROP_INT32_DEF("FLAG_STILL_RGB", LibcameraApp::FLAG_STILL_RGB, 0),
    JS_PROP_INT32_DEF("FLAG_STILL_RAW", LibcameraApp::FLAG_STILL_RAW, 0),
    JS_PROP_INT32_DEF("FLAG_STILL_DOUBLE_BUFFER", LibcameraApp::FLAG_STILL_DOUBLE_BUFFER, 0),
    JS_PROP_INT32_DEF("FLAG_STILL_TRIPLE_BUFFER", LibcameraApp::FLAG_STILL_TRIPLE_BUFFER, 0),
    JS_PROP_INT32_DEF("FLAG_STILL_BUFFER_MASK", LibcameraApp::FLAG_STILL_BUFFER_MASK, 0),
    JS_PROP_INT32_DEF("FLAG_VIDEO_NONE", LibcameraApp::FLAG_VIDEO_NONE, 0),
    JS_PROP_INT32_DEF("FLAG_VIDEO_RAW", LibcameraApp::FLAG_VIDEO_RAW, 0),
    JS_PROP_INT32_DEF("FLAG_VIDEO_JPEG_COLOURSPACE", LibcameraApp::FLAG_VIDEO_JPEG_COLOURSPACE, 0),
    JS_PROP_INT32_DEF("TRANSFORM_IDENTITY", int32_t(libcamera::Transform::Identity), 0),
    JS_PROP_INT32_DEF("TRANSFORM_ROT0", int32_t(libcamera::Transform::Rot0), 0),
    JS_PROP_INT32_DEF("TRANSFORM_HFLIP", int32_t(libcamera::Transform::HFlip), 0),
    JS_PROP_INT32_DEF("TRANSFORM_VFLIP", int32_t(libcamera::Transform::VFlip), 0),
    JS_PROP_INT32_DEF("TRANSFORM_HVFLIP", int32_t(libcamera::Transform::HVFlip), 0),
    JS_PROP_INT32_DEF("TRANSFORM_ROT180", int32_t(libcamera::Transform::Rot180), 0),
    JS_PROP_INT32_DEF("TRANSFORM_TRANSPOSE", int32_t(libcamera::Transform::Transpose), 0),
    JS_PROP_INT32_DEF("TRANSFORM_ROT270", int32_t(libcamera::Transform::Rot270), 0),
    JS_PROP_INT32_DEF("TRANSFORM_ROT90", int32_t(libcamera::Transform::Rot90), 0),
    JS_PROP_INT32_DEF("TRANSFORM_ROT180TRANSPOSE", int32_t(libcamera::Transform::Rot180Transpose), 0),
};

int
js_libcamera_app_init(JSContext* ctx, JSModuleDef* m) {

  if(js_libcamera_app_class_id == 0) {
    /* create the LibcameraApp class */
    JS_NewClassID(&js_libcamera_app_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_libcamera_app_class_id, &js_libcamera_app_class);

    libcamera_app_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, libcamera_app_proto, js_libcamera_app_proto_funcs, countof(js_libcamera_app_proto_funcs));
    JS_SetClassProto(ctx, js_libcamera_app_class_id, libcamera_app_proto);

    libcamera_app_class = JS_NewCFunction2(ctx, js_libcamera_app_constructor, "LibcameraApp", 2, JS_CFUNC_constructor, 0);
    JS_SetPropertyFunctionList(ctx, libcamera_app_class, js_libcamera_app_static_funcs, countof(js_libcamera_app_static_funcs));

    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, libcamera_app_class, libcamera_app_proto);
    JS_NewClassID(&js_libcamera_app_options_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_libcamera_app_options_class_id, &js_libcamera_app_options_class);

    libcamera_app_options_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, libcamera_app_options_proto, js_libcamera_app_options_proto_funcs, countof(js_libcamera_app_options_proto_funcs));
    JS_SetClassProto(ctx, js_libcamera_app_options_class_id, libcamera_app_options_proto);
  }

  if(m) {
    JS_SetModuleExport(ctx, m, "LibcameraApp", libcamera_app_class);
  }

  return 0;
}

extern "C" void
js_libcamera_app_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "LibcameraApp");
}

#ifdef JS_METHOD_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_libcamera_app
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_libcamera_app_init)))
    return NULL;

  js_libcamera_app_export(ctx, m);
  return m;
}
