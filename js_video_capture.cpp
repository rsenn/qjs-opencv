#include "js_alloc.hpp"
#include "js_mat.hpp"
#include "jsbindings.hpp"
#include <opencv2/core.hpp>
#include <opencv2/core/cvstd.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/videoio.hpp>
#include <quickjs.h>
#include <cctype>
#include <stddef.h>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <new>
#include <string>

extern "C" int js_video_capture_init(JSContext*, JSModuleDef*);

extern "C" {
JSValue video_capture_proto = JS_UNDEFINED, video_capture_class = JS_UNDEFINED;
thread_local JSClassID js_video_capture_class_id = 0;
}

static inline int
is_numeric(const std::string& s) {
  return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}

static bool
js_video_capture_open(JSContext* ctx, JSVideoCaptureData* s, int argc, JSValueConst argv[]) {
  int32_t camID = -1, apiPreference = cv::CAP_ANY;
  cv::String filename;

  filename = JS_ToCString(ctx, argv[0]);

  if(argc > 1)
    JS_ToInt32(ctx, &apiPreference, argv[1]);

  if(is_numeric(filename)) {
    JS_ToInt32(ctx, &camID, argv[0]);
    filename = "";
  }

  std::cerr << "VideoCapture.open filename='" << filename << "', camID=" << camID << ", apiPreference=" << apiPreference << std::endl;

  if(filename.empty())
    return s->open(camID, apiPreference);

  return s->open(filename, apiPreference);
}

static JSValue
js_video_capture_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSVideoCaptureData* s;
  JSValue obj = JS_UNDEFINED;
  JSValue proto, ret;

  s = js_allocate<JSVideoCaptureData>(ctx);
  if(!s)
    return JS_ThrowOutOfMemory(ctx);

  new(s) JSVideoCaptureData();

  if(argc > 0) {
    if(!js_video_capture_open(ctx, s, argc, argv)) {
      delete s;
      return JS_ThrowInternalError(ctx, "VideoCapture.open error");
    }
  }

  /* using new_target to get the prototype is necessary when the
     class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_video_capture_class_id);
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

extern "C" JSVideoCaptureData*
js_video_capture_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSVideoCaptureData*>(JS_GetOpaque2(ctx, val, js_video_capture_class_id));
}

void
js_video_capture_finalizer(JSRuntime* rt, JSValue val) {
  JSVideoCaptureData* s = static_cast<JSVideoCaptureData*>(JS_GetOpaque(val, js_video_capture_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  s->~JSVideoCaptureData();
  js_deallocate(rt, s);

  JS_FreeValueRT(rt, val);
}
enum {
  VIDEO_CAPTURE_METHOD_GET = 0,
  VIDEO_CAPTURE_METHOD_SET,
  VIDEO_CAPTURE_METHOD_GET_BACKEND_NAME,
  VIDEO_CAPTURE_METHOD_GRAB,
  VIDEO_CAPTURE_METHOD_IS_OPENED,
  VIDEO_CAPTURE_METHOD_OPEN,
  VIDEO_CAPTURE_METHOD_READ,
  VIDEO_CAPTURE_METHOD_RETRIEVE
};

static JSValue
js_video_capture_method(JSContext* ctx, JSValueConst video_capture, int argc, JSValueConst argv[], int magic) {
  JSVideoCaptureData* s = static_cast<JSVideoCaptureData*>(JS_GetOpaque2(ctx, video_capture, js_video_capture_class_id));
  JSValue ret = JS_UNDEFINED;
  int32_t propID;
  double value = 0;
  switch(magic) {
    case VIDEO_CAPTURE_METHOD_GET: {
      if(!JS_ToInt32(ctx, &propID, argv[0])) {
        value = s->get(propID);
        ret = JS_NewFloat64(ctx, value);
      } else {
        ret = JS_EXCEPTION;
      }
      break;
    }

    case VIDEO_CAPTURE_METHOD_SET: {
      if(!JS_ToInt32(ctx, &propID, argv[0])) {
        JS_ToFloat64(ctx, &value, argv[1]);

        s->set(propID, value);
      } else {
        const char* arg = JS_ToCString(ctx, argv[0]);
        ret = JS_ThrowInternalError(ctx, "VideoCapture.set propertyId = %s", arg);
        JS_FreeCString(ctx, arg);
      }
      break;
    }

    case VIDEO_CAPTURE_METHOD_GET_BACKEND_NAME: {
      std::string backend;
      try {
        backend = s->getBackendName();
      } catch(const cv::Exception& e) { backend = e.msg; }
      ret = JS_NewString(ctx, backend.c_str());
      break;
    }

    case VIDEO_CAPTURE_METHOD_GRAB: {
      ret = JS_NewBool(ctx, s->grab());
      break;
    }

    case VIDEO_CAPTURE_METHOD_IS_OPENED: {
      ret = JS_NewBool(ctx, s->isOpened());
      break;
    }

    case VIDEO_CAPTURE_METHOD_OPEN: {
      ret = JS_NewBool(ctx, js_video_capture_open(ctx, s, argc, argv));
      break;
    }

    case VIDEO_CAPTURE_METHOD_READ: {
      JSMatData* m = js_mat_data2(ctx, argv[0]);

      if(m == nullptr)
        return JS_EXCEPTION;

      ret = JS_NewBool(ctx, s->read(*m));
      break;
    }

    case VIDEO_CAPTURE_METHOD_RETRIEVE: {
      JSMatData* m = js_mat_data2(ctx, argv[0]);

      if(m == nullptr)
        return JS_EXCEPTION;

      ret = JS_NewBool(ctx, s->retrieve(*m));
      break;
    }
  }

  return ret;
}

JSValue
js_video_capture_wrap(JSContext* ctx, cv::VideoCapture* cap) {
  JSValue ret;

  ret = JS_NewObjectProtoClass(ctx, video_capture_proto, js_video_capture_class_id);

  // cap->addref();

  JS_SetOpaque(ret, cap);

  return ret;
}

JSClassDef js_video_capture_class = {
    .class_name = "VideoCapture",
    .finalizer = js_video_capture_finalizer,
};

const JSCFunctionListEntry js_video_capture_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("get", 1, js_video_capture_method, VIDEO_CAPTURE_METHOD_GET),
    JS_CFUNC_MAGIC_DEF("set", 2, js_video_capture_method, VIDEO_CAPTURE_METHOD_SET),
    JS_CFUNC_MAGIC_DEF("getBackendName", 0, js_video_capture_method, VIDEO_CAPTURE_METHOD_GET_BACKEND_NAME),
    JS_CFUNC_MAGIC_DEF("grab", 0, js_video_capture_method, VIDEO_CAPTURE_METHOD_GRAB),
    JS_CFUNC_MAGIC_DEF("isOpened", 0, js_video_capture_method, VIDEO_CAPTURE_METHOD_IS_OPENED),
    JS_CFUNC_MAGIC_DEF("open", 1, js_video_capture_method, VIDEO_CAPTURE_METHOD_OPEN),
    JS_CFUNC_MAGIC_DEF("read", 1, js_video_capture_method, VIDEO_CAPTURE_METHOD_READ),
    JS_CFUNC_MAGIC_DEF("retrieve", 1, js_video_capture_method, VIDEO_CAPTURE_METHOD_RETRIEVE),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "VideoCapture", JS_PROP_CONFIGURABLE),

};

int
js_video_capture_init(JSContext* ctx, JSModuleDef* m) {

  if(js_video_capture_class_id == 0) {
    /* create the VideoCapture class */
    JS_NewClassID(&js_video_capture_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_video_capture_class_id, &js_video_capture_class);

    video_capture_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, video_capture_proto, js_video_capture_proto_funcs, countof(js_video_capture_proto_funcs));
    JS_SetClassProto(ctx, js_video_capture_class_id, video_capture_proto);

    video_capture_class = JS_NewCFunction2(ctx, js_video_capture_constructor, "VideoCapture", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, video_capture_class, video_capture_proto);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "VideoCapture", video_capture_class);

  return 0;
}

extern "C" void
js_video_capture_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "VideoCapture");
}

#ifdef JS_VIDEO_CAPTURE_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_video_capture
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  if(!(m = JS_NewCModule(ctx, module_name, &js_video_capture_init)))
    return NULL;
  js_video_capture_export(ctx, m);
  return m;
}
