#include "js_alloc.hpp"
#include "js_size.hpp"
#include "js_umat.hpp"
#include "jsbindings.hpp"
#include <opencv2/core.hpp>
#include <opencv2/core/cvstd.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/videoio.hpp>
#include <quickjs.h>
#include <stddef.h>
#include <cstdint>
#include <new>
#include <string>

typedef cv::VideoWriter JSVideoWriterData;

extern "C" int js_video_writer_init(JSContext*, JSModuleDef*);

extern "C" {
JSValue video_writer_proto = JS_UNDEFINED, video_writer_class = JS_UNDEFINED;
thread_local JSClassID js_video_writer_class_id = 0;
}

static bool
js_video_writer_open(JSContext* ctx, JSVideoWriterData* vw, int argc, JSValueConst argv[]) {
  int32_t apiPreference = cv::CAP_ANY;
  cv::String filename;
  JSSizeData<int> frameSize;
  int sizeIndex, argIndex;
  uint32_t fourcc;
  double fps;

  filename = JS_ToCString(ctx, argv[0]);

  for(sizeIndex = 2; sizeIndex < argc; sizeIndex++) {
    if(js_size_read(ctx, argv[sizeIndex], &frameSize))
      break;
  }
  argIndex = 1;
  if(sizeIndex - argIndex == 3) {
    JS_ToInt32(ctx, &apiPreference, argv[argIndex++]);
  }
  JS_ToUint32(ctx, &fourcc, argv[argIndex++]);
  if(argIndex < sizeIndex)
    JS_ToFloat64(ctx, &fps, argv[argIndex++]);
  if(apiPreference != cv::CAP_ANY)
    return vw->open(filename, apiPreference, fourcc, fps, frameSize);

  return vw->open(filename, fourcc, fps, frameSize);
}

static JSValue
js_video_writer_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSVideoWriterData* vw;
  JSValue obj = JS_UNDEFINED;
  JSValue proto, ret;

  if(!(vw = js_allocate<JSVideoWriterData>(ctx)))
    return JS_EXCEPTION;
  new(vw) JSVideoWriterData();

  /* using new_target to get the prototype is necessary when the
     class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_video_writer_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  if(argc > 0) {
    if(!js_video_writer_open(ctx, vw, argc, argv))
      return JS_EXCEPTION;
  }

  JS_SetOpaque(obj, vw);
  return obj;
fail:
  js_deallocate(ctx, vw);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSVideoWriterData*
js_video_writer_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSVideoWriterData*>(JS_GetOpaque2(ctx, val, js_video_writer_class_id));
}

JSVideoWriterData*
js_video_writer_data(JSValueConst val) {
  return static_cast<JSVideoWriterData*>(JS_GetOpaque(val, js_video_writer_class_id));
}

void
js_video_writer_finalizer(JSRuntime* rt, JSValue val) {
  JSVideoWriterData* s = static_cast<JSVideoWriterData*>(JS_GetOpaque(val, js_video_writer_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  s->~JSVideoWriterData();
  js_deallocate(rt, s);

  JS_FreeValueRT(rt, val);
}
enum {
  VIDEO_WRITER_METHOD_GET = 0,
  VIDEO_WRITER_METHOD_SET,
  VIDEO_WRITER_METHOD_GET_BACKEND_NAME,
  VIDEO_WRITER_METHOD_IS_OPENED,
  VIDEO_WRITER_METHOD_OPEN,
  VIDEO_WRITER_METHOD_WRITE
};

static JSValue
js_video_writer_method(JSContext* ctx, JSValueConst video_writer, int argc, JSValueConst argv[], int magic) {
  JSVideoWriterData* vw = static_cast<JSVideoWriterData*>(JS_GetOpaque2(ctx, video_writer, js_video_writer_class_id));
  JSValue ret = JS_UNDEFINED;
  int32_t propID;
  double value = 0;

  switch(magic) {
    case VIDEO_WRITER_METHOD_GET: {
      if(!JS_ToInt32(ctx, &propID, argv[0])) {
        value = vw->get(propID);
        ret = JS_NewFloat64(ctx, value);
      } else {
        ret = JS_EXCEPTION;
      }
      break;
    }

    case VIDEO_WRITER_METHOD_SET: {
      if(!JS_ToInt32(ctx, &propID, argv[0])) {
        JS_ToFloat64(ctx, &value, argv[1]);

        vw->set(propID, value);
      } else
        ret = JS_EXCEPTION;
      break;
    }

    case VIDEO_WRITER_METHOD_GET_BACKEND_NAME: {
      std::string backend;
      try {
        backend = vw->getBackendName();
      } catch(const cv::Exception& e) { backend = e.msg; }
      ret = JS_NewString(ctx, backend.c_str());
      break;
    }

    case VIDEO_WRITER_METHOD_IS_OPENED: {
      ret = JS_NewBool(ctx, vw->isOpened());
      break;
    }

    case VIDEO_WRITER_METHOD_OPEN: {
      ret = JS_NewBool(ctx, js_video_writer_open(ctx, vw, argc, argv));
      break;
    }

    case VIDEO_WRITER_METHOD_WRITE: {
      JSInputArray mat = js_umat_or_mat(ctx, argv[0]);

      vw->write(mat);
      break;
    }
  }

  return ret;
}

static JSValue
js_video_writer_fourcc(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  char chars[4];
  size_t i;
  if(argc >= 4) {
    for(i = 0; i < 4; i++) {
      if(JS_IsNumber(argv[i])) {
        int32_t n;
        JS_ToInt32(ctx, &n, argv[i]);
        chars[i] = n;
      } else {
        const char* s = JS_ToCString(ctx, argv[i]);
        chars[i] = s[0];
        JS_FreeCString(ctx, s);
      }
    }
  } else {
    size_t len;
    const char* s = JS_ToCStringLen(ctx, &len, argv[0]);

    if(len >= 4) {
      for(i = 0; i < 4; i++)
        chars[i] = s[i];
    }
    JS_FreeCString(ctx, s);
  }

  return JS_NewInt32(ctx, cv::VideoWriter::fourcc(chars[0], chars[1], chars[2], chars[3]));
}

JSValue
js_video_writer_wrap(JSContext* ctx, cv::VideoWriter* cap) {
  JSValue ret;

  ret = JS_NewObjectProtoClass(ctx, video_writer_proto, js_video_writer_class_id);

  // cap->addref();

  JS_SetOpaque(ret, cap);

  return ret;
}

JSClassDef js_video_writer_class = {
    .class_name = "VideoWriter",
    .finalizer = js_video_writer_finalizer,
};

const JSCFunctionListEntry js_video_writer_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("get", 1, js_video_writer_method, VIDEO_WRITER_METHOD_GET),
    JS_CFUNC_MAGIC_DEF("set", 2, js_video_writer_method, VIDEO_WRITER_METHOD_SET),
    JS_CFUNC_MAGIC_DEF("getBackendName", 0, js_video_writer_method, VIDEO_WRITER_METHOD_GET_BACKEND_NAME),
    JS_CFUNC_MAGIC_DEF("isOpened", 0, js_video_writer_method, VIDEO_WRITER_METHOD_IS_OPENED),
    JS_CFUNC_MAGIC_DEF("open", 1, js_video_writer_method, VIDEO_WRITER_METHOD_OPEN),
    JS_CFUNC_MAGIC_DEF("write", 1, js_video_writer_method, VIDEO_WRITER_METHOD_WRITE),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "VideoWriter", JS_PROP_CONFIGURABLE),

};

const JSCFunctionListEntry js_video_writer_static_funcs[] = {
    JS_CFUNC_DEF("fourcc", 4, js_video_writer_fourcc),
};

int
js_video_writer_init(JSContext* ctx, JSModuleDef* m) {

  if(js_video_writer_class_id == 0) {
    /* create the VideoWriter class */
    JS_NewClassID(&js_video_writer_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_video_writer_class_id, &js_video_writer_class);

    video_writer_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, video_writer_proto, js_video_writer_proto_funcs, countof(js_video_writer_proto_funcs));
    JS_SetClassProto(ctx, js_video_writer_class_id, video_writer_proto);

    video_writer_class = JS_NewCFunction2(ctx, js_video_writer_ctor, "VideoWriter", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, video_writer_class, video_writer_proto);
    JS_SetPropertyFunctionList(ctx, video_writer_class, js_video_writer_static_funcs, countof(js_video_writer_static_funcs));
  }

  if(m)
    JS_SetModuleExport(ctx, m, "VideoWriter", video_writer_class);

  return 0;
}

extern "C" void
js_video_writer_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "VideoWriter");
}

#ifdef JS_VIDEO_WRITER_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_video_writer
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  if(!(m = JS_NewCModule(ctx, module_name, &js_video_writer_init)))
    return NULL;
  js_video_writer_export(ctx, m);
  return m;
}
