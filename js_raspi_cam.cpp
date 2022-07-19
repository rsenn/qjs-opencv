#include "js_alloc.hpp"
#include "js_mat.hpp"
#include "jsbindings.hpp"
#include <opencv2/core.hpp>
#include <lccv.hpp>
#include <quickjs.h>
/*#include <cctype>
#include <stddef.h>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <new>
#include <string>*/

extern "C" VISIBLE int js_raspi_cam_init(JSContext*, JSModuleDef*);

extern "C" {
JSValue raspi_cam_proto = JS_UNDEFINED, raspi_cam_class = JS_UNDEFINED;
thread_local VISIBLE JSClassID js_raspi_cam_class_id = 0;
}

static bool
js_raspi_cam_open(JSContext* ctx, JSRaspiCamData* s, int argc, JSValueConst argv[]) {
  int32_t camID = -1, apiPreference = cv::CAP_ANY;
  cv::String filename;

  filename = JS_ToCString(ctx, argv[0]);

  if(argc > 1)
    JS_ToInt32(ctx, &apiPreference, argv[1]);

  if(is_numeric(filename)) {
    JS_ToInt32(ctx, &camID, argv[0]);
    filename = "";
  }

  std::cerr << "RaspiCam.open filename='" << filename << "', camID=" << camID << ", apiPreference=" << apiPreference << std::endl;

  if(filename.empty())
    return s->open(camID, apiPreference);

  return s->open(filename, apiPreference);
}

static JSValue
js_raspi_cam_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSRaspiCamData* s;
  JSValue obj = JS_UNDEFINED;
  JSValue proto, ret;

  s = js_allocate<JSRaspiCamData>(ctx);
  if(!s)
    return JS_ThrowOutOfMemory(ctx);

  new(s) JSRaspiCamData();

  if(argc > 0) {
    if(!js_raspi_cam_open(ctx, s, argc, argv)) {
      delete s;
      return JS_ThrowInternalError(ctx, "RaspiCam.open error");
    }
  }

  /* using new_target to get the prototype is necessary when the
     class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_raspi_cam_class_id);
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

extern "C" VISIBLE JSRaspiCamData*
js_raspi_cam_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSRaspiCamData*>(JS_GetOpaque2(ctx, val, js_raspi_cam_class_id));
}

void
js_raspi_cam_finalizer(JSRuntime* rt, JSValue val) {
  JSRaspiCamData* s = static_cast<JSRaspiCamData*>(JS_GetOpaque(val, js_raspi_cam_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  s->~JSRaspiCamData();
  js_deallocate(rt, s);

  JS_FreeValueRT(rt, val);
}
enum {
  RASPI_CAM_CAMERA_ID = 0,
  RASPI_CAM_OPEN_CAMERA,
  RASPI_CAM_CLOSE_CAMERA,
  RASPI_CAM_CONFIGURE_STILL,
  RASPI_CAM_CONFIGURE_VIEWFINDER,
  RASPI_CAM_TEARDOWN,
  RASPI_CAM_START_CAMERA,
  RASPI_CAM_STOP_CAMERA,
  RASPI_CAM_WAIT,
  RASPI_CAM_POST_MESSAGE,
  RASPI_CAM_GET_STREAM,
  RASPI_CAM_VIEWFINDER_STREAM,
  RASPI_CAM_STILL_STREAM,
  RASPI_CAM_RAW_STREAM,
  RASPI_CAM_VIDEO_STREAM,
  RASPI_CAM_LORES_STREAM,
  RASPI_CAM_GET_MAIN_STREAM,
  RASPI_CAM_MMAP,
  RASPI_CAM_SET_CONTROLS,
  RASPI_CAM_STREAM_DIMENSIONS,

};

static JSValue
js_raspi_cam_method(JSContext* ctx, JSValueConst raspi_cam, int argc, JSValueConst argv[], int magic) {
  JSRaspiCamData* s = static_cast<JSRaspiCamData*>(JS_GetOpaque2(ctx, raspi_cam, js_raspi_cam_class_id));
  JSValue ret = JS_UNDEFINED;
  int32_t propID;
  double value = 0;
  
  switch(magic) {
    case RASPI_CAM_CAMERA_ID: {
      break;
    }
    case RASPI_CAM_OPEN_CAMERA: {
      break;
    }
    case RASPI_CAM_CLOSE_CAMERA: {
      break;
    }
    case RASPI_CAM_CONFIGURE_STILL: {
      break;
    }
    case RASPI_CAM_CONFIGURE_VIEWFINDER: {
      break;
    }
    case RASPI_CAM_TEARDOWN: {
      break;
    }
    case RASPI_CAM_START_CAMERA: {
      break;
    }
    case RASPI_CAM_STOP_CAMERA: {
      break;
    }
    case RASPI_CAM_WAIT: {
      break;
    }
    case RASPI_CAM_POST_MESSAGE: {
      break;
    }
    case RASPI_CAM_GET_STREAM: {
      break;
    }
    case RASPI_CAM_VIEWFINDER_STREAM: {
      break;
    }
    case RASPI_CAM_STILL_STREAM: {
      break;
    }
    case RASPI_CAM_RAW_STREAM: {
      break;
    }
    case RASPI_CAM_VIDEO_STREAM: {
      break;
    }
    case RASPI_CAM_LORES_STREAM: {
      break;
    }
    case RASPI_CAM_GET_MAIN_STREAM: {
      break;
    }
    case RASPI_CAM_MMAP: {
      break;
    }
    case RASPI_CAM_SET_CONTROLS: {
      break;
    }
    case RASPI_CAM_STREAM_DIMENSIONS: {
      break;
    }
  }

  return ret;
}

VISIBLE JSValue
js_raspi_cam_wrap(JSContext* ctx, cv::RaspiCam* cap) {
  JSValue ret;

  ret = JS_NewObjectProtoClass(ctx, raspi_cam_proto, js_raspi_cam_class_id);

  // cap->addref();

  JS_SetOpaque(ret, cap);

  return ret;
}

JSClassDef js_raspi_cam_class = {
    .class_name = "RaspiCam",
    .finalizer = js_raspi_cam_finalizer,
};

const JSCFunctionListEntry js_raspi_cam_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("cameraId", 0, js_raspi_cam_method, RASPI_CAM_CAMERA_ID),
    JS_CFUNC_MAGIC_DEF("openCamera", 0, js_raspi_cam_method, RASPI_CAM_OPEN_CAMERA),
    JS_CFUNC_MAGIC_DEF("closeCamera", 0, js_raspi_cam_method, RASPI_CAM_CLOSE_CAMERA),
    JS_CFUNC_MAGIC_DEF("configureStill", 0, js_raspi_cam_method, RASPI_CAM_CONFIGURE_STILL),
    JS_CFUNC_MAGIC_DEF("configureViewfinder", 0, js_raspi_cam_method, RASPI_CAM_CONFIGURE_VIEWFINDER),
    JS_CFUNC_MAGIC_DEF("teardown", 0, js_raspi_cam_method, RASPI_CAM_TEARDOWN),
    JS_CFUNC_MAGIC_DEF("startCamera", 0, js_raspi_cam_method, RASPI_CAM_START_CAMERA),
    JS_CFUNC_MAGIC_DEF("stopCamera", 0, js_raspi_cam_method, RASPI_CAM_STOP_CAMERA),
    JS_CFUNC_MAGIC_DEF("wait", 0, js_raspi_cam_method, RASPI_CAM_WAIT),
    JS_CFUNC_MAGIC_DEF("postMessage", 0, js_raspi_cam_method, RASPI_CAM_POST_MESSAGE),
    JS_CFUNC_MAGIC_DEF("getStream", 0, js_raspi_cam_method, RASPI_CAM_GET_STREAM),
    JS_CFUNC_MAGIC_DEF("viewfinderStream", 0, js_raspi_cam_method, RASPI_CAM_VIEWFINDER_STREAM),
    JS_CFUNC_MAGIC_DEF("stillStream", 0, js_raspi_cam_method, RASPI_CAM_STILL_STREAM),
    JS_CFUNC_MAGIC_DEF("rawStream", 0, js_raspi_cam_method, RASPI_CAM_RAW_STREAM),
    JS_CFUNC_MAGIC_DEF("videoStream", 0, js_raspi_cam_method, RASPI_CAM_VIDEO_STREAM),
    JS_CFUNC_MAGIC_DEF("loresStream", 0, js_raspi_cam_method, RASPI_CAM_LORES_STREAM),
    JS_CFUNC_MAGIC_DEF("getMainStream", 0, js_raspi_cam_method, RASPI_CAM_GET_MAIN_STREAM),
    JS_CFUNC_MAGIC_DEF("mmap", 0, js_raspi_cam_method, RASPI_CAM_MMAP),
    JS_CFUNC_MAGIC_DEF("setControls", 0, js_raspi_cam_method, RASPI_CAM_SET_CONTROLS),
    JS_CFUNC_MAGIC_DEF("streamDimensions", 0, js_raspi_cam_method, RASPI_CAM_STREAM_DIMENSIONS),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "RaspiCam", JS_PROP_CONFIGURABLE),

};

int
js_raspi_cam_init(JSContext* ctx, JSModuleDef* m) {

  if(js_raspi_cam_class_id == 0) {
    /* create the RaspiCam class */
    JS_NewClassID(&js_raspi_cam_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_raspi_cam_class_id, &js_raspi_cam_class);

    raspi_cam_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, raspi_cam_proto, js_raspi_cam_proto_funcs, countof(js_raspi_cam_proto_funcs));
    JS_SetClassProto(ctx, js_raspi_cam_class_id, raspi_cam_proto);

    raspi_cam_class = JS_NewCFunction2(ctx, js_raspi_cam_ctor, "RaspiCam", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, raspi_cam_class, raspi_cam_proto);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "RaspiCam", raspi_cam_class);

  return 0;
}

extern "C" VISIBLE void
js_raspi_cam_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "RaspiCam");
}

void
js_raspi_cam_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(raspi_cam_class))
    js_raspi_cam_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "RaspiCam", raspi_cam_class);
}

#ifdef JS_RASPI_CAM_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_raspi_cam
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_raspi_cam_init);
  if(!m)
    return NULL;
  js_raspi_cam_export(ctx, m);
  return m;
}
