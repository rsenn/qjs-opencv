#include "jsbindings.hpp"
#include "js_mat.hpp"
#include "js_alloc.hpp"

extern "C" VISIBLE int js_video_capture_init(JSContext*, JSModuleDef*);

extern "C" {
JSValue video_capture_proto = JS_UNDEFINED, video_capture_class = JS_UNDEFINED;
JSClassID js_video_capture_class_id = 0;
}

static inline int
is_numeric(const std::string& s) {
  return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}

static bool
js_video_capture_open(JSContext* ctx, JSVideoCaptureData* s, int argc, JSValueConst* argv) {
  int32_t camID, apiPreference = cv::CAP_ANY;
  cv::String filename;

  filename = JS_ToCString(ctx, argv[0]);

  if(argc > 1)
    JS_ToInt32(ctx, &apiPreference, argv[1]);

  if(is_numeric(filename))
    JS_ToInt32(ctx, &camID, argv[0]);

  std::cerr << "VideoCapture.open filename='" << filename << "', camID=" << camID << ", apiPreference=" << apiPreference
            << std::endl;

  if(filename.empty())
    return s->open(camID, apiPreference);

  return s->open(filename, apiPreference);
}

static JSValue
js_video_capture_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  JSVideoCaptureData* s;
  JSValue obj = JS_UNDEFINED;
  JSValue proto, ret;

  s = js_allocate<JSVideoCaptureData>(ctx);
  if(!s)
    return JS_EXCEPTION;

  new(s) JSVideoCaptureData();

  if(argc > 0) {
    if(!js_video_capture_open(ctx, s, argc, argv))
      return JS_EXCEPTION;
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

VISIBLE JSVideoCaptureData*
js_video_capture_data(JSContext* ctx, JSValueConst val) {
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

static JSValue
js_video_capture_method(JSContext* ctx, JSValueConst video_capture, int argc, JSValueConst* argv, int magic) {
  JSVideoCaptureData* s = static_cast<JSVideoCaptureData*>(JS_GetOpaque2(ctx, video_capture, js_video_capture_class_id));
  JSValue ret = JS_UNDEFINED;
  int32_t propID;
  double value = 0;

  if(magic == 0) {
    if(!JS_ToInt32(ctx, &propID, argv[0])) {
      value = s->get(propID);
      ret = JS_NewFloat64(ctx, value);
    } else {
      ret = JS_EXCEPTION;
    }
  } else if(magic == 1) {
    if(!JS_ToInt32(ctx, &propID, argv[0])) {
      JS_ToFloat64(ctx, &value, argv[1]);

      s->set(propID, value);
    } else
      ret = JS_EXCEPTION;
  } else if(magic == 2) {
    std::string backend;
    try {
      backend = s->getBackendName();
    } catch(const cv::Exception& e) { backend = e.msg; }
    ret = JS_NewString(ctx, backend.c_str());
  } else if(magic == 3) {
    ret = JS_NewBool(ctx, s->grab());
  } else if(magic == 4) {
    ret = JS_NewBool(ctx, s->isOpened());
  } else if(magic == 5) {
    ret = JS_NewBool(ctx, js_video_capture_open(ctx, s, argc, argv));
  }

  else if(magic == 6 || magic == 7) {
    JSMatData* m = js_mat_data(ctx, argv[0]);

    if(m == nullptr)
      return JS_EXCEPTION;

    ret = JS_NewBool(ctx, magic == 6 ? s->read(*m) : s->retrieve(*m));
  }

  return ret;
}

VISIBLE JSValue
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
    JS_CFUNC_MAGIC_DEF("get", 1, js_video_capture_method, 0),
    JS_CFUNC_MAGIC_DEF("set", 2, js_video_capture_method, 1),
    JS_CFUNC_MAGIC_DEF("getBackendName", 0, js_video_capture_method, 2),
    JS_CFUNC_MAGIC_DEF("grab", 0, js_video_capture_method, 3),
    JS_CFUNC_MAGIC_DEF("isOpened", 0, js_video_capture_method, 4),
    JS_CFUNC_MAGIC_DEF("open", 1, js_video_capture_method, 5),
    JS_CFUNC_MAGIC_DEF("read", 1, js_video_capture_method, 6),
    JS_CFUNC_MAGIC_DEF("retrieve", 1, js_video_capture_method, 7),

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

    video_capture_class = JS_NewCFunction2(ctx, js_video_capture_ctor, "VideoCapture", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, video_capture_class, video_capture_proto);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "VideoCapture", video_capture_class);

  return 0;
}

extern "C" VISIBLE void
js_video_capture_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "VideoCapture");
}

void
js_video_capture_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(video_capture_class))
    js_video_capture_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "VideoCapture", video_capture_class);
}

#ifdef JS_VIDEO_CAPTURE_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_video_capture
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_video_capture_init);
  if(!m)
    return NULL;
  js_video_capture_export(ctx, m);
  return m;
}