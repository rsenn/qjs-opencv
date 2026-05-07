#include "js_cv.hpp"
#include "js_umat.hpp"
#include "include/jsbindings.hpp"
#include <quickjs.h>

#include <opencv2/dnn.hpp>

extern "C" {
thread_local JSValue net_proto, net_class;
thread_local JSClassID js_net_class_id;
}

static JSValue
js_net_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSNetData* fs;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(fs = js_allocate<JSNetData>(ctx)))
    return JS_EXCEPTION;

  if(argc == 0) {
    new(fs) JSNetData();
  } else {
    const char *filename = 0, *encoding = 0;
    int32_t flags = 0;

    if(!(filename = JS_ToCString(ctx, argv[0])))
      goto fail;

    if(argc > 1)
      JS_ToInt32(ctx, &flags, argv[1]);

    if(argc > 2)
      encoding = JS_ToCString(ctx, argv[2]);

    if(encoding)
      new(fs) JSNetData(filename, flags, encoding);
    else
      new(fs) JSNetData(filename, flags);

    JS_FreeCString(ctx, filename);
    if(encoding)
      JS_FreeCString(ctx, encoding);
  }

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_net_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, fs);

  return obj;

fail:
  js_deallocate(ctx, fs);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSNetData*
js_net_data(JSValueConst val) {
  return static_cast<JSNetData*>(JS_GetOpaque(val, js_net_class_id));
}

JSNetData*
js_net_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSNetData*>(JS_GetOpaque2(ctx, val, js_net_class_id));
}

static JSValue
js_net_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSNetData* fs;
  JSValue ret = JS_UNDEFINED;

  if(!(fs = js_net_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {}

  return ret;
}

static JSValue
js_net_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSNetData* fs;

  if(!(fs = js_net_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {}

  return JS_UNDEFINED;
}

enum {
  FUNCTION_GETDEFAULTOBJECTNAME,
};

static JSValue
js_net_funcs(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_EXCEPTION;

  switch(magic) {
    case FUNCTION_GETDEFAULTOBJECTNAME: {
      const char* filename;

      if(!(filename = JS_ToCString(ctx, argv[0])))
        return JS_ThrowTypeError(ctx, "argument 1 must be a string");

      std::string str = cv::Net::getDefaultObjectName(filename);

      JS_FreeCString(ctx, filename);

      ret = JS_NewString(ctx, str.c_str());
      break;
    }
  }

  return ret;
}

enum { GLOBAL_WRITE, GLOBAL_WRITESCALAR };

static JSValue
js_net_global(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_EXCEPTION;
  JSNetData* fs;

  if(!(fs = js_net_data2(ctx, argv[0])))
    return JS_EXCEPTION;

  switch(magic) {
    case GLOBAL_WRITE: {
      if(argc > 2) {
        const char* name;

        if(!(name = JS_ToCString(ctx, argv[1])))
          return JS_ThrowTypeError(ctx, "argument 2 must be a string");

        JSMatData* m;

        if((m = js_mat_data(argv[2]))) {
          cv::write(*fs, name, *m);
        } else if(JS_IsNumber(argv[2])) {
          double val;
          JS_ToFloat64(ctx, &val, argv[2]);

          cv::write(*fs, name, val);
        } else if(JS_IsString(argv[2])) {
          const char* str;

          if(!(str = JS_ToCString(ctx, argv[2])))
            return JS_ThrowTypeError(ctx, "argument 3 must be a string");

          cv::write(*fs, name, str);
          JS_FreeCString(ctx, str);
        }

        JS_FreeCString(ctx, name);
      } else {
        JSPointData<double> pt;
        JSSizeData<double> sz;
        JSRectData<double> rt;

        if(JS_IsNumber(argv[1])) {
          double val;
          JS_ToFloat64(ctx, &val, argv[1]);

          cv::write(*fs, val);
        } else if(JS_IsString(argv[1])) {
          const char* str;

          if(!(str = JS_ToCString(ctx, argv[1])))
            return JS_ThrowTypeError(ctx, "argument 2 must be a string");

          cv::write(*fs, str);
          JS_FreeCString(ctx, str);
        } else if(js_point_read(ctx, argv[1], &pt)) {
          cv::write(*fs, pt);
        } else if(js_size_read(ctx, argv[1], &sz)) {
          cv::write(*fs, sz);
        } else if(js_rect_read(ctx, argv[1], &rt)) {
          cv::write(*fs, rt);
        }
      }

      break;
    }

    case GLOBAL_WRITESCALAR: {
      if(JS_IsNumber(argv[1])) {
        double val;
        JS_ToFloat64(ctx, &val, argv[1]);

        cv::writeScalar(*fs, val);
      } else {
        const char* str;

        if(!(str = JS_ToCString(ctx, argv[1])))
          return JS_ThrowTypeError(ctx, "argument 2 must be a string");

        cv::writeScalar(*fs, str);
        JS_FreeCString(ctx, str);
      }

      break;
    }
  }

  return ret;
}

enum {
  METHOD_OPEN,
  METHOD_RELEASE,
  METHOD_RELEASEANDGETSTRING,
  METHOD_ENDWRITESTRUCT,
  METHOD_GETFIRSTTOPLEVELNODE,
  METHOD_GETFORMAT,
  METHOD_ISOPENED,
  METHOD_ROOT,
  METHOD_STARTWRITESTRUCT,
  METHOD_WRITE,
  METHOD_WRITECOMMENT,
  METHOD_WRITERAW,
  METHOD_GETNODE,
};

static JSValue
js_net_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSNetData* fs;
  JSValue ret = JS_UNDEFINED;

  if(!(fs = js_net_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case METHOD_OPEN: {
      const char *filename, *encoding = 0;
      int32_t flags = 0;

      if(!(filename = JS_ToCString(ctx, argv[0])))
        return JS_ThrowTypeError(ctx, "argument 1 must be a string");

      if(argc > 1)
        JS_ToInt32(ctx, &flags, argv[1]);

      if(argc > 2)
        encoding = JS_ToCString(ctx, argv[2]);

      ret = JS_NewBool(ctx, fs->open(filename, flags, encoding));

      JS_FreeCString(ctx, filename);
      if(encoding)
        JS_FreeCString(ctx, encoding);

      break;
    }

    case METHOD_RELEASE: {
      fs->release();
      break;
    }

    case METHOD_RELEASEANDGETSTRING: {
      std::string str = fs->releaseAndGetString();

      ret = JS_NewString(ctx, str.c_str());
      break;
    }

    case METHOD_ENDWRITESTRUCT: {
      fs->endWriteStruct();
      break;
    }

    case METHOD_GETFIRSTTOPLEVELNODE: {
      ret = js_filenode_new(ctx, fs->getFirstTopLevelNode());
      break;
    }

    case METHOD_GETFORMAT: {
      ret = JS_NewInt32(ctx, fs->getFormat());
      break;
    }

    case METHOD_GETNODE: {
      const char* name = JS_ToCString(ctx, argv[0]);

      ret = js_filenode_new(ctx, fs->operator[](name));

      if(name)
        JS_FreeCString(ctx, name);

      break;
    }

    case METHOD_ISOPENED: {
      ret = JS_NewBool(ctx, fs->isOpened());
      break;
    }

    case METHOD_ROOT: {
      int32_t idx = 0;

      if(argc > 0)
        JS_ToInt32(ctx, &idx, argv[0]);

      ret = js_filenode_new(ctx, fs->root());
      break;
    }

    case METHOD_STARTWRITESTRUCT: {
      const char *name, *typeName;
      int32_t flags = 0;

      if(!(name = JS_ToCString(ctx, argv[0])))
        return JS_ThrowTypeError(ctx, "argument 1 must be a string");

      if(argc > 1)
        JS_ToInt32(ctx, &flags, argv[1]);

      if(argc > 2)
        typeName = JS_ToCString(ctx, argv[2]);

      fs->startWriteStruct(name, flags, typeName);

      JS_FreeCString(ctx, name);
      if(typeName)
        JS_FreeCString(ctx, typeName);
      break;
    }

    case METHOD_WRITE: {
      const char* name;

      if(!(name = JS_ToCString(ctx, argv[0])))
        return JS_ThrowTypeError(ctx, "argument 1 must be a string");

      JSMatData* m;

      if(js_is_array(ctx, argv[1])) {
        std::vector<std::string> strv;

        js_array_to(ctx, argv[1], strv);

        fs->write(name, strv);
      } else if((m = js_mat_data(argv[1]))) {
        fs->write(name, *m);

      } else if(JS_IsNumber(argv[1])) {
        double val;
        JS_ToFloat64(ctx, &val, argv[1]);

        fs->write(name, val);
      } else if(JS_IsString(argv[1])) {
        const char* str;

        if(!(str = JS_ToCString(ctx, argv[1])))
          return JS_ThrowTypeError(ctx, "argument 2 must be a string");

        fs->write(name, str);
        JS_FreeCString(ctx, str);
      }

      break;
    }

    case METHOD_WRITECOMMENT: {
      const char* comment;

      if(!(comment = JS_ToCString(ctx, argv[0])))
        return JS_ThrowTypeError(ctx, "argument 1 must be a string");

      BOOL append = FALSE;

      if(argc > 1)
        append = JS_ToBool(ctx, argv[1]);

      fs->writeComment(comment, append);
      break;
    }

    case METHOD_WRITERAW: {
      const char* fmt;

      if(!(fmt = JS_ToCString(ctx, argv[0])))
        return JS_ThrowTypeError(ctx, "argument 1 must be a string");

      uint8_t* buf;
      size_t len;

      if(!(buf = JS_GetArrayBuffer(ctx, &len, argv[1]))) {
        JS_FreeCString(ctx, fmt);
        return JS_ThrowTypeError(ctx, "argument 2 must be an ArrayBuffer");
      }

      fs->writeRaw(fmt, buf, len);
      break;
    }
  }

  return ret;
}

void
js_net_finalizer(JSRuntime* rt, JSValue val) {
  JSNetData* fs;
  /* Note: 'fs' can be NULL in case JS_SetOpaque() was not called */

  if((fs = js_net_data(val))) {
    fs->~JSNetData();

    js_deallocate(rt, fs);
  }
}

JSClassDef js_net_class = {
    .class_name = "Net",
    .finalizer = js_net_finalizer,
};

const JSCFunctionListEntry js_net_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("open", 2, js_net_method, METHOD_OPEN),
    JS_CFUNC_MAGIC_DEF("release", 0, js_net_method, METHOD_RELEASE),

    JS_CFUNC_MAGIC_DEF("endWriteStruct", 0, js_net_method, METHOD_ENDWRITESTRUCT),
    JS_CFUNC_MAGIC_DEF("getFirstTopLevelNode", 0, js_net_method, METHOD_GETFIRSTTOPLEVELNODE),
    JS_CFUNC_MAGIC_DEF("getFormat", 0, js_net_method, METHOD_GETFORMAT),
    JS_CFUNC_MAGIC_DEF("isOpened", 0, js_net_method, METHOD_ISOPENED),
    JS_CFUNC_MAGIC_DEF("releaseAndGetString", 0, js_net_method, METHOD_RELEASEANDGETSTRING),
    JS_CFUNC_MAGIC_DEF("root", 0, js_net_method, METHOD_ROOT),
    JS_CFUNC_MAGIC_DEF("startWriteStruct", 2, js_net_method, METHOD_STARTWRITESTRUCT),
    JS_CFUNC_MAGIC_DEF("write", 2, js_net_method, METHOD_WRITE),
    JS_CFUNC_MAGIC_DEF("writeComment", 0, js_net_method, METHOD_WRITECOMMENT),
    JS_CFUNC_MAGIC_DEF("writeRaw", 2, js_net_method, METHOD_WRITERAW),

    JS_CFUNC_MAGIC_DEF("getNode", 1, js_net_method, METHOD_GETNODE),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Net", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_net_static_funcs[] = {
    JS_CFUNC_MAGIC_DEF("getDefaultObjectName", 1, js_net_funcs, FUNCTION_GETDEFAULTOBJECTNAME),
};

enum {
  DNN_LAYOUT_NCHW,
  DNN_IMAGE2BLOBPARAMS,
  DNN_IMAGEPADDINGMODE,
  DNN_NET,
  DNN_NMSBOXES,
  DNN_READNET,
};

static JSValue
js_dnn_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
    switch(magic) {}
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

const JSCFunctionListEntry js_dnn_dnn_funcs[] = {
    JS_PROP_INT32_DEF("DNN_LAYOUT_NCHW", cv::dnn::DNN_LAYOUT_NCHW, JS_PROP_ENUMERABLE),
    JS_CFUNC_MAGIC_DEF("Image2BlobParams", 0, js_dnn_func, DNN_IMAGE2BLOBPARAMS),
    JS_CFUNC_MAGIC_DEF("ImagePaddingMode", 0, js_dnn_func, DNN_IMAGEPADDINGMODE),
    //
    //      JS_CFUNC_MAGIC_DEF("Net", 0, js_dnn_func, DNN_NET),
    JS_CFUNC_MAGIC_DEF("NMSBoxes", 0, js_dnn_func, DNN_NMSBOXES),
    JS_CFUNC_MAGIC_DEF("readNet", 0, js_dnn_func, DNN_READNET),
};

const JSCFunctionListEntry js_dnn_static_funcs[] = {
    JS_OBJECT_DEF("dnn", js_dnn_dnn_funcs, countof(js_dnn_dnn_funcs), JS_PROP_C_W_E),
};

extern "C" int
js_dnn_init(JSContext* ctx, JSModuleDef* m) {

  /* create the Net class */
  JS_NewClassID(&js_net_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_net_class_id, &js_net_class);

  net_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, net_proto, js_net_proto_funcs, countof(js_net_proto_funcs));
  JS_SetClassProto(ctx, js_net_class_id, net_proto);

  net_class = JS_NewCFunction2(ctx, js_net_constructor, "Net", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, net_class, net_proto);
  JS_SetPropertyFunctionList(ctx, net_class, js_net_static_funcs, countof(js_net_static_funcs));

  // js_object_inspect(ctx, net_proto, js_net_inspect);
}

if(m) {
  // JS_SetModuleExport(ctx, m, "Net", net_class);
  //  JS_SetModuleExportList(ctx, m, js_net_global_funcs, countof(js_net_global_funcs));

  JS_SetModuleExportList(ctx, m, js_dnn_static_funcs, countof(js_dnn_static_funcs));
}

return 0;
}

extern "C" void
js_dnn_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_dnn_static_funcs, countof(js_dnn_static_funcs));
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_dnn
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_dnn_init)))
    return NULL;

  js_dnn_export(ctx, m);
  return m;
}
