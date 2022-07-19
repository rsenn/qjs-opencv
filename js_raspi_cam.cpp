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

typedef lccv::PiCamera JSRaspiCamData;

extern "C" VISIBLE int js_raspi_cam_init(JSContext*, JSModuleDef*);

extern "C" {
JSValue raspi_cam_proto = JS_UNDEFINED, raspi_cam_class = JS_UNDEFINED;
thread_local VISIBLE JSClassID js_raspi_cam_class_id = 0;
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
  RASPI_CAM_CAPTURE_PHOTO = 0,
  RASPI_CAM_STOP_PHOTO,
  RASPI_CAM_START_VIDEO,
  RASPI_CAM_GET_VIDEO_FRAME,
  RASPI_CAM_STOP_VIDEO,

};

static JSValue
js_raspi_cam_options(JSContext* ctx, JSValueConst this_val, int magic) {
  JSRaspiCamData* cam = static_cast<JSRaspiCamData*>(JS_GetOpaque2(ctx, this_val, js_raspi_cam_class_id));

  return js_raspi_cam_options_wrap(ctx, cam->GetOptions());
}

static JSValue
js_raspi_cam_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSRaspiCamData* cam = static_cast<JSRaspiCamData*>(JS_GetOpaque2(ctx, this_val, js_raspi_cam_class_id));
  JSValue ret = JS_UNDEFINED;
  int32_t propID;
  double value = 0;

  switch(magic) {
    case RASPI_CAM_CAPTURE_PHOTO: {
      break;
    }
    case RASPI_CAM_STOP_PHOTO: {
      break;
    }
    case RASPI_CAM_START_VIDEO: {
      break;
    }
    case RASPI_CAM_GET_VIDEO_FRAME: {
      break;
    }
    case RASPI_CAM_STOP_VIDEO: {
      break;
    }
  }

  return ret;
}

VISIBLE JSValue
js_raspi_cam_wrap(JSContext* ctx, RaspiCamData* cam) {
  JSValue ret;

  ret = JS_NewObjectProtoClass(ctx, raspi_cam_proto, js_raspi_cam_class_id);

  // cap->addref();

  JS_SetOpaque(ret, cam);

  return ret;
}

JSClassDef js_raspi_cam_class = {
    .class_name = "RaspiCam",
    .finalizer = js_raspi_cam_finalizer,
};

const JSCFunctionListEntry js_raspi_cam_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("capturePhoto", 0, js_raspi_cam_method, RASPI_CAM_CAPTURE_PHOTO),
    JS_CFUNC_MAGIC_DEF("stopPhoto", 0, js_raspi_cam_method, RASPI_CAM_STOP_PHOTO),
    JS_CFUNC_MAGIC_DEF("startVideo", 0, js_raspi_cam_method, RASPI_CAM_START_VIDEO),
    JS_CFUNC_MAGIC_DEF("getVideoFrame", 0, js_raspi_cam_method, RASPI_CAM_GET_VIDEO_FRAME),
    JS_CFUNC_MAGIC_DEF("stopVideo", 0, js_raspi_cam_method, RASPI_CAM_STOP_VIDEO),
    JS_CGETSET_DEF("options", js_raspi_cam_options, 0),

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
    /* set proto.c    JS_SetConstructor(ctx, raspi_cam_class, raspi_cam_proto);
onstructor and ctor.prototype */

    JS_NewClassID(&js_raspi_cam_options_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_raspi_cam_options_class_id, &js_raspi_cam_options_class);

    raspi_cam_options_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, raspi_cam_options_proto, js_raspi_cam_options_proto_funcs, countof(js_raspi_cam_options_proto_funcs));
    JS_SetClassProto(ctx, js_raspi_cam_options_class_id, raspi_cam_options_proto);

    raspi_cam_options_class = JS_NewCFunction2(ctx, js_raspi_cam_options_ctor, "RaspiCamOptions", 2, JS_CFUNC_constructor, 0);
  }

  if(m) {
    JS_SetModuleExport(ctx, m, "RaspiCam", raspi_cam_class);
  }

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

#ifdef JS_METHOD_MODULE
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
