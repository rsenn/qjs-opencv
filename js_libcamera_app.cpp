#include "js_libcamera_app.hpp"
#include "js_alloc.hpp"
#include "js_mat.hpp"
#include "jsbindings.hpp"
#include <opencv2/core.hpp>
#include <quickjs.h>
/*#include <cctype>
#include <stddef.h>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <new>
#include <string>*/

extern "C" VISIBLE int js_libcamera_app_init(JSContext*, JSModuleDef*);

extern "C" {
JSValue libcamera_app_proto = JS_UNDEFINED, libcamera_app_class = JS_UNDEFINED, libcamera_app_options_proto = JS_UNDEFINED,
        libcamera_app_options_class = JS_UNDEFINED;
thread_local VISIBLE JSClassID js_libcamera_app_class_id = 0, js_libcamera_app_options_class_id = 0;
}
enum {
  OPTION_HELP = 0,
  OPTION_VERSION,
  OPTION_LIST_CAMERAS,
  OPTION_VERBOSE,
  OPTION_MS,
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

  s->~JSLibcameraAppOptionsData();
  js_deallocate(rt, s);

  JS_FreeValueRT(rt, val);
}

VISIBLE JSValue
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

  switch(magic) {
    case OPTION_HELP: {
      break;
    }
    case OPTION_VERSION: {
      break;
    }
    case OPTION_LIST_CAMERAS: {
      break;
    }
    case OPTION_VERBOSE: {
      break;
    }
    case OPTION_MS: {
      break;
    }
    case OPTION_PHOTO_WIDTH: {
      break;
    }
    case OPTION_PHOTO_HEIGHT: {
      break;
    }
    case OPTION_VIDEO_WIDTH: {
      break;
    }
    case OPTION_VIDEO_HEIGHT: {
      break;
    }
    case OPTION_RAWFULL: {
      break;
    }
    case OPTION_TRANSFORM: {
      break;
    }
    case OPTION_ROI_X: {
      break;
    }
    case OPTION_ROI_Y: {
      break;
    }
    case OPTION_ROI_WIDTH: {
      break;
    }
    case OPTION_ROI_HEIGHT: {
      break;
    }
    case OPTION_SHUTTER: {
      break;
    }
    case OPTION_GAIN: {
      break;
    }
    case OPTION_EV: {
      break;
    }
    case OPTION_AWB_GAIN_R: {
      break;
    }
    case OPTION_AWB_GAIN_B: {
      break;
    }
    case OPTION_BRIGHTNESS: {
      break;
    }
    case OPTION_CONTRAST: {
      break;
    }
    case OPTION_SATURATION: {
      break;
    }
    case OPTION_SHARPNESS: {
      break;
    }
    case OPTION_FRAMERATE: {
      break;
    }
    case OPTION_DENOISE: {
      break;
    }
    case OPTION_INFO_TEXT: {
      break;
    }
    case OPTION_CAMERA: {
      break;
    }
  }
  return JS_UNDEFINED;
}

static JSValue
js_libcamera_app_options_set(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic) {
  JSLibcameraAppOptionsData* o = static_cast<JSLibcameraAppOptionsData*>(JS_GetOpaque2(ctx, this_val, js_libcamera_app_options_class_id));

  switch(magic) {
    case OPTION_HELP: {
      break;
    }
    case OPTION_VERSION: {
      break;
    }
    case OPTION_LIST_CAMERAS: {
      break;
    }
    case OPTION_VERBOSE: {
      break;
    }
    case OPTION_MS: {
      break;
    }
    case OPTION_PHOTO_WIDTH: {
      break;
    }
    case OPTION_PHOTO_HEIGHT: {
      break;
    }
    case OPTION_VIDEO_WIDTH: {
      break;
    }
    case OPTION_VIDEO_HEIGHT: {
      break;
    }
    case OPTION_RAWFULL: {
      break;
    }
    case OPTION_TRANSFORM: {
      break;
    }
    case OPTION_ROI_X: {
      break;
    }
    case OPTION_ROI_Y: {
      break;
    }
    case OPTION_ROI_WIDTH: {
      break;
    }
    case OPTION_ROI_HEIGHT: {
      break;
    }
    case OPTION_SHUTTER: {
      break;
    }
    case OPTION_GAIN: {
      break;
    }
    case OPTION_EV: {
      break;
    }
    case OPTION_AWB_GAIN_R: {
      break;
    }
    case OPTION_AWB_GAIN_B: {
      break;
    }
    case OPTION_BRIGHTNESS: {
      break;
    }
    case OPTION_CONTRAST: {
      break;
    }
    case OPTION_SATURATION: {
      break;
    }
    case OPTION_SHARPNESS: {
      break;
    }
    case OPTION_FRAMERATE: {
      break;
    }
    case OPTION_DENOISE: {
      break;
    }
    case OPTION_INFO_TEXT: {
      break;
    }
    case OPTION_CAMERA: {
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
    JS_CGETSET_MAGIC_DEF("ms", js_libcamera_app_options_get, js_libcamera_app_options_set, OPTION_MS),
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
js_libcamera_app_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
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

extern "C" VISIBLE JSLibcameraAppData*
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
js_libcamera_app_options(JSContext* ctx, JSValueConst this_val, int magic) {
  JSLibcameraAppData* cam = static_cast<JSLibcameraAppData*>(JS_GetOpaque2(ctx, this_val, js_libcamera_app_class_id));

  return js_libcamera_app_options_wrap(ctx, cam->GetOptions());
}

static JSValue
js_libcamera_app_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSLibcameraAppData* cam = static_cast<JSLibcameraAppData*>(JS_GetOpaque2(ctx, this_val, js_libcamera_app_class_id));
  JSValue ret = JS_UNDEFINED;
  int32_t propID;
  double value = 0;

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

  return ret;
}

VISIBLE JSValue
js_libcamera_app_wrap(JSContext* ctx, LibcameraAppData* cam) {
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
    JS_CV_CONSTANT(LibcameraApp::FLAG_STILL_NONE),
    JS_CV_CONSTANT(LibcameraApp::FLAG_STILL_BGR),
    JS_CV_CONSTANT(LibcameraApp::FLAG_STILL_RGB),
    JS_CV_CONSTANT(LibcameraApp::FLAG_STILL_RAW),
    JS_CV_CONSTANT(LibcameraApp::FLAG_STILL_DOUBLE_BUFFER),
    JS_CV_CONSTANT(LibcameraApp::FLAG_STILL_TRIPLE_BUFFER),
    JS_CV_CONSTANT(LibcameraApp::FLAG_STILL_BUFFER_MASK),
    JS_CV_CONSTANT(LibcameraApp::FLAG_VIDEO_NONE),
    JS_CV_CONSTANT(LibcameraApp::FLAG_VIDEO_RAW),
    JS_CV_CONSTANT(LibcameraApp::FLAG_VIDEO_JPEG_COLOURSPACE),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "LibcameraApp", JS_PROP_CONFIGURABLE),

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

    libcamera_app_class = JS_NewCFunction2(ctx, js_libcamera_app_ctor, "LibcameraApp", 2, JS_CFUNC_constructor, 0);
    /* set proto.c    JS_SetConstructor(ctx, libcamera_app_class, libcamera_app_proto);
onstructor and ctor.prototype */

    JS_NewClassID(&js_libcamera_app_options_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_libcamera_app_options_class_id, &js_libcamera_app_options_class);

    libcamera_app_options_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, libcamera_app_options_proto, js_libcamera_app_options_proto_funcs, countof(js_libcamera_app_options_proto_funcs));
    JS_SetClassProto(ctx, js_libcamera_app_options_class_id, libcamera_app_options_proto);

    libcamera_app_options_class = JS_NewCFunction2(ctx, js_libcamera_app_options_ctor, "LibcameraAppOptions", 2, JS_CFUNC_constructor, 0);
  }

  if(m) {
    JS_SetModuleExport(ctx, m, "LibcameraApp", libcamera_app_class);
  }

  return 0;
}

extern "C" VISIBLE void
js_libcamera_app_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "LibcameraApp");
}

void
js_libcamera_app_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(libcamera_app_class))
    js_libcamera_app_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "LibcameraApp", libcamera_app_class);
}

#ifdef JS_METHOD_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_libcamera_app
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_libcamera_app_init);
  if(!m)
    return NULL;
  js_libcamera_app_export(ctx, m);
  return m;
}
