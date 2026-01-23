#include "js_filestorage.hpp"
#include "include/js_alloc.hpp"
#include "include/util.hpp"

extern "C" {
thread_local JSValue filestorage_proto = JS_UNDEFINED, filestorage_class = JS_UNDEFINED;
thread_local JSClassID js_filestorage_class_id = 0;
}

static JSValue
js_filestorage_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSFileStorageData* fs;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(fs = js_allocate<JSFileStorageData>(ctx)))
    return JS_EXCEPTION;

  if(argc == 0) {
    new(fs) JSFileStorageData();
  } else {

    const char *filename, *encoding;
    int32_t flags = 0;

    if(!(filename = JS_ToCString(ctx, argv[0])))
      goto fail;

    if(argc > 1)
      JS_ToInt32(ctx, &flags, argv[1]);

    if(argc > 2)
      encoding = JS_ToCString(ctx, argv[2]);

    new(fs) JSFileStorageData(filename, flags, encoding);

    JS_FreeCString(ctx, filename);
    if(encoding)
      JS_FreeCString(ctx, encoding);
  }

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_filestorage_class_id);
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

JSFileStorageData*
js_filestorage_data(JSValueConst val) {
  return static_cast<JSFileStorageData*>(JS_GetOpaque(val, js_filestorage_class_id));
}

JSFileStorageData*
js_filestorage_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSFileStorageData*>(JS_GetOpaque2(ctx, val, js_filestorage_class_id));
}

static JSValue
js_filestorage_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSFileStorageData* fs;
  JSValue ret = JS_UNDEFINED;

  if(!(fs = js_filestorage_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {}

  return ret;
}

static JSValue
js_filestorage_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSFileStorageData* fs;

  if(!(fs = js_filestorage_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {}

  return JS_UNDEFINED;
}

enum {};

static JSValue
js_filestorage_funcs(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_EXCEPTION;

  switch(magic) {}

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
js_filestorage_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSFileStorageData* fs;
  JSValue ret = JS_UNDEFINED;

  if(!(fs = js_filestorage_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case METHOD_OPEN: {
      break;
    }

    case METHOD_RELEASE: {
      break;
    }

    case METHOD_RELEASEANDGETSTRING: {
      break;
    }

    case METHOD_ENDWRITESTRUCT: {
      break;
    }

    case METHOD_GETFIRSTTOPLEVELNODE: {
      break;
    }

    case METHOD_GETFORMAT: {
      break;
    }

    case METHOD_GETNODE: {
      const char* name = JS_ToCString(ctx, argv[0]);

      fs->operator[](name);

      if(name)
        JS_FreeCString(ctx, name);

      break;
    }

    case METHOD_ISOPENED: {
      break;
    }

    case METHOD_ROOT: {
      break;
    }

    case METHOD_STARTWRITESTRUCT: {
      break;
    }

    case METHOD_WRITE: {
      break;
    }

    case METHOD_WRITECOMMENT: {
      break;
    }

    case METHOD_WRITERAW: {
      break;
    }
  }

  return ret;
}

void
js_filestorage_finalizer(JSRuntime* rt, JSValue val) {
  JSFileStorageData* fs;
  /* Note: 'fs' can be NULL in case JS_SetOpaque() was not called */

  // fs->~JSFileStorageData();
  if((fs = js_filestorage_data(val)))
    js_deallocate(rt, fs);
}

JSClassDef js_filestorage_class = {
    .class_name = "FileStorage",
    .finalizer = js_filestorage_finalizer,
};

const JSCFunctionListEntry js_filestorage_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("open", 2, js_filestorage_method, METHOD_OPEN),
    JS_CFUNC_MAGIC_DEF("release", 0, js_filestorage_method, METHOD_RELEASE),

    JS_CFUNC_MAGIC_DEF("endWriteStruct", 0, js_filestorage_method, METHOD_ENDWRITESTRUCT),
    JS_CFUNC_MAGIC_DEF("getFirstTopLevelNode", 0, js_filestorage_method, METHOD_GETFIRSTTOPLEVELNODE),
    JS_CFUNC_MAGIC_DEF("getFormat", 0, js_filestorage_method, METHOD_GETFORMAT),
    JS_CFUNC_MAGIC_DEF("isOpened", 0, js_filestorage_method, METHOD_ISOPENED),
    JS_CFUNC_MAGIC_DEF("releaseAndGetString", 0, js_filestorage_method, METHOD_RELEASEANDGETSTRING),
    JS_CFUNC_MAGIC_DEF("root", 0, js_filestorage_method, METHOD_ROOT),
    JS_CFUNC_MAGIC_DEF("startWriteStruct", 2, js_filestorage_method, METHOD_STARTWRITESTRUCT),
    JS_CFUNC_MAGIC_DEF("write", 2, js_filestorage_method, METHOD_WRITE),
    JS_CFUNC_MAGIC_DEF("writeComment", 0, js_filestorage_method, METHOD_WRITECOMMENT),
    JS_CFUNC_MAGIC_DEF("writeRaw", 2, js_filestorage_method, METHOD_WRITERAW),

    JS_CFUNC_MAGIC_DEF("getNode", 1, js_filestorage_method, METHOD_GETNODE),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "FileStorage", JS_PROP_CONFIGURABLE),
};
const JSCFunctionListEntry js_filestorage_static_funcs[] = {
    JS_CFUNC_MAGIC_DEF("getDefaultObjectName", 1, js_filestorage_funcs, FUNCTION_GETDEFAULTOBJECTNAME),
    /* enum Mode */
    JS_PROP_INT32_DEF("READ", cv::FileStorage::READ, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("WRITE", cv::FileStorage::WRITE, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("APPEND", cv::FileStorage::APPEND, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("MEMORY", cv::FileStorage::MEMORY, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("FORMAT_MASK", cv::FileStorage::FORMAT_MASK, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("FORMAT_AUTO", cv::FileStorage::FORMAT_AUTO, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("FORMAT_XML", cv::FileStorage::FORMAT_XML, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("FORMAT_YAML", cv::FileStorage::FORMAT_YAML, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("FORMAT_JSON", cv::FileStorage::FORMAT_JSON, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("BASE64", cv::FileStorage::BASE64, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("WRITE_BASE64", cv::FileStorage::WRITE_BASE64, JS_PROP_ENUMERABLE),

    /* enum State */
    JS_PROP_INT32_DEF("UNDEFINED", cv::FileStorage::UNDEFINED, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("VALUE_EXPECTED", cv::FileStorage::VALUE_EXPECTED, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("NAME_EXPECTED", cv::FileStorage::NAME_EXPECTED, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("INSIDE_MAP", cv::FileStorage::INSIDE_MAP, JS_PROP_ENUMERABLE),

    // JS_CFUNC_DEF("from", 1, js_filestorage_from),
};

extern "C" int
js_filestorage_init(JSContext* ctx, JSModuleDef* m) {

  if(js_filestorage_class_id == 0) {
    /* create the FileStorage class */
    JS_NewClassID(&js_filestorage_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_filestorage_class_id, &js_filestorage_class);

    filestorage_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, filestorage_proto, js_filestorage_proto_funcs, countof(js_filestorage_proto_funcs));
    JS_SetClassProto(ctx, js_filestorage_class_id, filestorage_proto);

    filestorage_class = JS_NewCFunction2(ctx, js_filestorage_constructor, "FileStorage", 0, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, filestorage_class, filestorage_proto);
    JS_SetPropertyFunctionList(ctx, filestorage_class, js_filestorage_static_funcs, countof(js_filestorage_static_funcs));

    // js_object_inspect(ctx, filestorage_proto, js_filestorage_inspect);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "FileStorage", filestorage_class);

  return 0;
}

extern "C" void
js_filestorage_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "FileStorage");
}

#ifdef JS_FILESTORAGE_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_filestorage
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  if(!(m = JS_NewCModule(ctx, module_name, &js_filestorage_init)))
    return NULL;
  js_filestorage_export(ctx, m);
  return m;
}
