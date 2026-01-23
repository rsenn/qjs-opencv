#include "js_filenode.hpp"
#include "include/js_alloc.hpp"
#include "include/util.hpp"

extern "C" {
thread_local JSValue filenode_proto = JS_UNDEFINED, filenode_class = JS_UNDEFINED;
thread_local JSClassID js_filenode_class_id = 0;
}

JSValue
js_filenode_wrap(JSContext* ctx, JSValueConst proto, JSFileNodeData* fn) {
  JSValue ret = JS_NewObjectProtoClass(ctx, proto, js_filenode_class_id);
  JS_SetOpaque(ret, fn);
  return ret;
}

JSValue
js_filenode_new(JSContext* ctx, JSValueConst proto) {
  JSFileNodeData* fn = js_allocate<JSFileNodeData>(ctx);

  new(fn) JSFileNodeData();

  return js_filenode_wrap(ctx, proto, fn);
}

static JSValue
js_filenode_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSFileNodeData* fn;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(fn = js_allocate<JSFileNodeData>(ctx)))
    return JS_EXCEPTION;

  if(argc == 0) {
    new(fn) JSFileNodeData();
  } else {

    JS_ThrowTypeError(ctx, "arguments expected");
    goto fail;
  }

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_filenode_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, fn);

  return obj;

fail:
  js_deallocate(ctx, fn);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSFileNodeData*
js_filenode_data(JSValueConst val) {
  return static_cast<JSFileNodeData*>(JS_GetOpaque(val, js_filenode_class_id));
}

JSFileNodeData*
js_filenode_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSFileNodeData*>(JS_GetOpaque2(ctx, val, js_filenode_class_id));
}

static JSValue
js_filenode_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSFileNodeData* fn;
  JSValue ret = JS_UNDEFINED;

  if(!(fn = js_filenode_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {}

  return ret;
}

static JSValue
js_filenode_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSFileNodeData* fn;

  if(!(fn = js_filenode_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {}

  return JS_UNDEFINED;
}

enum {
  FUNCTION_ISCOLLECTION,
  FUNCTION_ISEMPTYCOLLECTION,
  FUNCTION_ISFLOW,
  FUNCTION_ISMAP,
  FUNCTION_ISSEQ,
};

static JSValue
js_filenode_funcs(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_EXCEPTION;

  switch(magic) {
    case FUNCTION_ISCOLLECTION: {
      break;
    }
    case FUNCTION_ISEMPTYCOLLECTION: {
      break;
    }
    case FUNCTION_ISFLOW: {
      break;
    }
    case FUNCTION_ISMAP: {
      break;
    }
    case FUNCTION_ISSEQ: {
      break;
    }
  }

  return ret;
}

enum {
  METHOD_BEGIN,
  METHOD_EMPTY,
  METHOD_END,
  METHOD_ISINT,
  METHOD_ISMAP,
  METHOD_ISNAMED,
  METHOD_ISNONE,
  METHOD_ISREAL,
  METHOD_ISSEQ,
  METHOD_ISSTRING,
  METHOD_KEYS,
  METHOD_MAT,
  METHOD_NAME,
  METHOD_ASSIGN,
  METHOD_AT,
  METHOD_GETNODE,
  METHOD_PTR,
  METHOD_RAWSIZE,
  METHOD_READRAW,
  METHOD_REAL,
  METHOD_SETVALUE,
  METHOD_SIZE,
  METHOD_STRING,
  METHOD_TYPE,

};

static JSValue
js_filenode_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSFileNodeData* fn;
  JSValue ret = JS_UNDEFINED;

  if(!(fn = js_filenode_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case METHOD_BEGIN: {
      break;
    }
    case METHOD_EMPTY: {
      break;
    }
    case METHOD_END: {
      break;
    }
    case METHOD_ISINT: {
      break;
    }
    case METHOD_ISMAP: {
      break;
    }
    case METHOD_ISNAMED: {
      break;
    }
    case METHOD_ISNONE: {
      break;
    }
    case METHOD_ISREAL: {
      break;
    }
    case METHOD_ISSEQ: {
      break;
    }
    case METHOD_ISSTRING: {
      break;
    }
    case METHOD_KEYS: {
      break;
    }
    case METHOD_MAT: {
      break;
    }
    case METHOD_NAME: {
      break;
    }
    case METHOD_ASSIGN: {
      break;
    }
    case METHOD_AT: {
      break;
    }
    case METHOD_GETNODE: {
      break;
    }
    case METHOD_PTR: {
      break;
    }
    case METHOD_RAWSIZE: {
      break;
    }
    case METHOD_READRAW: {
      break;
    }
    case METHOD_REAL: {
      break;
    }
    case METHOD_SETVALUE: {
      break;
    }
    case METHOD_SIZE: {
      break;
    }
    case METHOD_STRING: {
      break;
    }
    case METHOD_TYPE: {
      break;
    }
  }

  return ret;
}

void
js_filenode_finalizer(JSRuntime* rt, JSValue val) {
  JSFileNodeData* fn;
  /* Note: 'fn' can be NULL in case JS_SetOpaque() was not called */

  // fn->~JSFileNodeData();
  if((fn = js_filenode_data(val)))
    js_deallocate(rt, fn);
}

JSClassDef js_filenode_class = {
    .class_name = "FileNode",
    .finalizer = js_filenode_finalizer,
};

const JSCFunctionListEntry js_filenode_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("begin", 0, js_filenode_method, METHOD_BEGIN),
    JS_CFUNC_MAGIC_DEF("empty", 0, js_filenode_method, METHOD_EMPTY),
    JS_CFUNC_MAGIC_DEF("end", 0, js_filenode_method, METHOD_END),
    JS_CFUNC_MAGIC_DEF("isInt", 0, js_filenode_method, METHOD_ISINT),
    JS_CFUNC_MAGIC_DEF("isMap", 0, js_filenode_method, METHOD_ISMAP),
    JS_CFUNC_MAGIC_DEF("isNamed", 0, js_filenode_method, METHOD_ISNAMED),
    JS_CFUNC_MAGIC_DEF("isNone", 0, js_filenode_method, METHOD_ISNONE),
    JS_CFUNC_MAGIC_DEF("isReal", 0, js_filenode_method, METHOD_ISREAL),
    JS_CFUNC_MAGIC_DEF("isSeq", 0, js_filenode_method, METHOD_ISSEQ),
    JS_CFUNC_MAGIC_DEF("isString", 0, js_filenode_method, METHOD_ISSTRING),
    JS_CFUNC_MAGIC_DEF("keys", 0, js_filenode_method, METHOD_KEYS),
    JS_CFUNC_MAGIC_DEF("mat", 0, js_filenode_method, METHOD_MAT),
    JS_CFUNC_MAGIC_DEF("name", 0, js_filenode_method, METHOD_NAME),
    JS_CFUNC_MAGIC_DEF("assign", 1, js_filenode_method, METHOD_ASSIGN),
    JS_CFUNC_MAGIC_DEF("at", 1, js_filenode_method, METHOD_AT),
    JS_CFUNC_MAGIC_DEF("getNode", 1, js_filenode_method, METHOD_GETNODE),
    JS_CFUNC_MAGIC_DEF("ptr", 0, js_filenode_method, METHOD_PTR),
    JS_CFUNC_MAGIC_DEF("rawSize", 0, js_filenode_method, METHOD_RAWSIZE),
    JS_CFUNC_MAGIC_DEF("readRaw", 0, js_filenode_method, METHOD_READRAW),
    JS_CFUNC_MAGIC_DEF("real", 0, js_filenode_method, METHOD_REAL),
    JS_CFUNC_MAGIC_DEF("setValue", 0, js_filenode_method, METHOD_SETVALUE),
    JS_CFUNC_MAGIC_DEF("size", 0, js_filenode_method, METHOD_SIZE),
    JS_CFUNC_MAGIC_DEF("string", 0, js_filenode_method, METHOD_STRING),
    JS_CFUNC_MAGIC_DEF("type", 0, js_filenode_method, METHOD_TYPE),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "FileNode", JS_PROP_CONFIGURABLE),
};
const JSCFunctionListEntry js_filenode_static_funcs[] = {
    JS_CFUNC_MAGIC_DEF("isCollection", 1, js_filenode_funcs, FUNCTION_ISCOLLECTION),
    JS_CFUNC_MAGIC_DEF("isEmptyCollection", 1, js_filenode_funcs, FUNCTION_ISEMPTYCOLLECTION),
    JS_CFUNC_MAGIC_DEF("isFlow", 1, js_filenode_funcs, FUNCTION_ISFLOW),
    JS_CFUNC_MAGIC_DEF("isMap", 1, js_filenode_funcs, FUNCTION_ISMAP),
    JS_CFUNC_MAGIC_DEF("isSeq", 1, js_filenode_funcs, FUNCTION_ISSEQ),

    JS_PROP_INT32_DEF("NONE", cv::FileNode::NONE, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("INT", cv::FileNode::INT, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("REAL", cv::FileNode::REAL, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("FLOAT", cv::FileNode::FLOAT, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("STR", cv::FileNode::STR, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("STRING", cv::FileNode::STRING, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("SEQ", cv::FileNode::SEQ, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("MAP", cv::FileNode::MAP, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("TYPE_MASK", cv::FileNode::TYPE_MASK, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("FLOW", cv::FileNode::FLOW, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("UNIFORM", cv::FileNode::UNIFORM, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("EMPTY", cv::FileNode::EMPTY, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("NAMED", cv::FileNode::NAMED, JS_PROP_ENUMERABLE),

    // JS_CFUNC_DEF("from", 1, js_filenode_from),
};

extern "C" int
js_filenode_init(JSContext* ctx, JSModuleDef* m) {

  if(js_filenode_class_id == 0) {
    /* create the FileNode class */
    JS_NewClassID(&js_filenode_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_filenode_class_id, &js_filenode_class);

    filenode_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, filenode_proto, js_filenode_proto_funcs, countof(js_filenode_proto_funcs));
    JS_SetClassProto(ctx, js_filenode_class_id, filenode_proto);

    filenode_class = JS_NewCFunction2(ctx, js_filenode_constructor, "FileNode", 0, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, filenode_class, filenode_proto);
    JS_SetPropertyFunctionList(ctx, filenode_class, js_filenode_static_funcs, countof(js_filenode_static_funcs));

    // js_object_inspect(ctx, filenode_proto, js_filenode_inspect);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "FileNode", filenode_class);

  return 0;
}

extern "C" void
js_filenode_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "FileNode");
}

#ifdef JS_FILENODE_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_filenode
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  if(!(m = JS_NewCModule(ctx, module_name, &js_filenode_init)))
    return NULL;
  js_filenode_export(ctx, m);
  return m;
}
