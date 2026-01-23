#include "js_filenode.hpp"
#include "js_mat.hpp"
#include "include/js_alloc.hpp"
#include "include/js_array.hpp"
#include "include/jsbindings.hpp"
#include "include/util.hpp"

extern "C" {
thread_local JSValue filenode_proto = JS_UNDEFINED, filenode_class = JS_UNDEFINED, filenode_iterator_proto = JS_UNDEFINED,
                     filenode_iterator_class = JS_UNDEFINED;
thread_local JSClassID js_filenode_class_id = 0, js_filenode_iterator_class_id = 0;
}

static JSValue
js_filenode_iterator_wrap(JSContext* ctx, JSValueConst proto, JSFileNodeIteratorData* fn) {
  JSValue ret = JS_NewObjectProtoClass(ctx, proto, js_filenode_iterator_class_id);
  JS_SetOpaque(ret, fn);
  return ret;
}

JSValue
js_filenode_iterator_new(JSContext* ctx, JSValueConst proto, const JSFileNodeIteratorData& other) {
  JSFileNodeIteratorData* fn = js_allocate<JSFileNodeIteratorData>(ctx);

  new(fn) JSFileNodeIteratorData(other);

  return js_filenode_iterator_wrap(ctx, proto, fn);
}

void
js_filenode_iterator_finalizer(JSRuntime* rt, JSValue val) {
  JSFileNodeIteratorData* fni;
  /* Note: 'fni' can be NULL in case JS_SetOpaque() was not called */

  if((fni = static_cast<JSFileNodeIteratorData*>(JS_GetOpaque(val, js_filenode_iterator_class_id)))) {
    fni->~JSFileNodeIteratorData();

    js_deallocate(rt, fni);
  }
}

JSClassDef js_filenode_iterator_class = {
    .class_name = "FileNodeIterator",
    .finalizer = js_filenode_iterator_finalizer,
};

const JSCFunctionListEntry js_filenode_iterator_proto_funcs[] = {
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "FileNodeIterator", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_filenode_iterator_static_funcs[] = {};

static JSValue
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

JSValue
js_filenode_new(JSContext* ctx, JSValueConst proto, const JSFileNodeData& other) {
  JSFileNodeData* fn = js_allocate<JSFileNodeData>(ctx);

  new(fn) JSFileNodeData(other);

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
  METHOD_TOSTRING,
  METHOD_VALUEOF,
};

static void
js_filenode_freebuf(JSRuntime* rt, void* opaque, void* ptr) {
  JSValue obj = JS_MKPTR(JS_TAG_OBJECT, opaque);

  JS_FreeValueRT(rt, obj);
}

enum { GLOBAL_READ };

static JSValue
js_filenode_global(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_EXCEPTION;
  JSFileNodeData* fn;

  if(!(fn = js_filenode_data2(ctx, argv[0])))
    return JS_EXCEPTION;

  switch(magic) {
    case GLOBAL_READ: {
      break;
    }
  }

  return ret;
}

static JSValue
js_filenode_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSFileNodeData* fn;
  JSValue ret = JS_UNDEFINED;

  if(!(fn = js_filenode_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case METHOD_BEGIN: {
      ret = js_filenode_iterator_new(ctx, filenode_iterator_proto, fn->begin());
      break;
    }

    case METHOD_END: {
      ret = js_filenode_iterator_new(ctx, filenode_iterator_proto, fn->end());
      break;
    }

    case METHOD_EMPTY: {
      ret = JS_NewBool(ctx, fn->empty());
      break;
    }

    case METHOD_ISINT: {
      ret = JS_NewBool(ctx, fn->isInt());
      break;
    }

    case METHOD_ISMAP: {
      ret = JS_NewBool(ctx, fn->isMap());
      break;
    }

    case METHOD_ISNAMED: {
      ret = JS_NewBool(ctx, fn->isNamed());
      break;
    }

    case METHOD_ISNONE: {
      ret = JS_NewBool(ctx, fn->isNone());
      break;
    }

    case METHOD_ISREAL: {
      ret = JS_NewBool(ctx, fn->isReal());
      break;
    }

    case METHOD_ISSEQ: {
      ret = JS_NewBool(ctx, fn->isSeq());
      break;
    }

    case METHOD_ISSTRING: {
      ret = JS_NewBool(ctx, fn->isString());
      break;
    }

    case METHOD_KEYS: {
      std::vector<std::string> keys = fn->keys();

      ret = js_array_from(ctx, keys);
      break;
    }

    case METHOD_MAT: {
      cv::Mat mat = fn->mat();

      ret = js_mat_wrap(ctx, mat);
      break;
    }

    case METHOD_NAME: {
      std::string name = fn->name();

      ret = js_value_from(ctx, name);
      break;
    }

    case METHOD_ASSIGN: {
      JSFileNodeData* other;

      if(!(other = js_filenode_data(argv[0])))
        return JS_ThrowTypeError(ctx, "argument 1 must be of type cv::FileNode");

      *fn = *other;
      break;
    }

    case METHOD_AT: {
      int32_t idx;

      if(!fn->isSeq())
        return JS_ThrowTypeError(ctx, "FileNode must be of type SEQ");

      if(JS_ToInt32(ctx, &idx, argv[0]))
        return JS_ThrowTypeError(ctx, "argument 1 must be a number");

      ret = js_filenode_new(ctx, fn->operator[](idx));
      break;
    }

    case METHOD_GETNODE: {
      const char* name;

      if(!(name = JS_ToCString(ctx, argv[0])))
        return JS_ThrowTypeError(ctx, "argument 1 must be a string");

      ret = js_filenode_new(ctx, fn->operator[](name));

      JS_FreeCString(ctx, name);
      break;
    }

    case METHOD_PTR: {
      ret = JS_NewArrayBuffer(
          ctx, reinterpret_cast<uint8_t*>(fn->ptr()), fn->rawSize(), js_filenode_freebuf, JS_VALUE_GET_PTR(JS_DupValue(ctx, this_val)), FALSE);
      break;
    }

    case METHOD_RAWSIZE: {
      ret = js_value_from(ctx, fn->rawSize());
      break;
    }

    case METHOD_READRAW: {
      const char* fmt;

      if(!(fmt = JS_ToCString(ctx, argv[0])))
        return JS_ThrowTypeError(ctx, "argument 1 must be a string");

      size_t len;
      uint8_t* buf;

      if(!(buf = JS_GetArrayBuffer(ctx, &len, argv[1])))
        ret = JS_ThrowTypeError(ctx, "argument 2 must be an ArrayBuffer");
      else
        fn->readRaw(fmt, buf, len);

      JS_FreeCString(ctx, fmt);

      break;
    }

    case METHOD_REAL: {
      ret = js_value_from(ctx, fn->real());
      break;
    }

    case METHOD_SIZE: {
      ret = js_value_from(ctx, fn->size());
      break;
    }

    case METHOD_STRING: {
      ret = js_value_from(ctx, fn->string());
      break;
    }

    case METHOD_TYPE: {
      ret = js_value_from(ctx, fn->type());
      break;
    }

    case METHOD_TOSTRING: {
      ret = js_value_from(ctx, std::string(*fn));
      break;
    }

    case METHOD_VALUEOF: {
      switch(fn->type()) {
        case cv::FileNode::NONE: {
          ret = JS_NULL;
          break;
        }
        case cv::FileNode::INT:
        case cv::FileNode::FLOAT: {
          ret = js_value_from(ctx, double(*fn));
          break;
        }

        case cv::FileNode::STRING: {
          ret = js_value_from(ctx, std::string(*fn));
          break;
        }

        case cv::FileNode::SEQ: {
          size_t n = fn->size();
          std::vector<JSFileNodeData> vec;

          for(size_t i = 0; i < n; ++i)
            vec.push_back(fn->operator[](i));

          ret = js_value_from(ctx, vec);
          break;
        }

        case cv::FileNode::MAP: {
          ret = JS_NewObjectProto(ctx, JS_NULL);

          for(const auto& key : fn->keys()) {
            JSValue val = js_value_from(ctx, fn->operator[](key));

            JS_SetPropertyStr(ctx, ret, key.c_str(), val);
          }

          break;
        }
      }

      break;
    }
  }

  return ret;
}

void
js_filenode_finalizer(JSRuntime* rt, JSValue val) {
  JSFileNodeData* fn;
  /* Note: 'fn' can be NULL in case JS_SetOpaque() was not called */

  if((fn = js_filenode_data(val))) {
    fn->~JSFileNodeData();

    js_deallocate(rt, fn);
  }
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
    JS_CFUNC_MAGIC_DEF("size", 0, js_filenode_method, METHOD_SIZE),
    JS_CFUNC_MAGIC_DEF("string", 0, js_filenode_method, METHOD_STRING),
    JS_CFUNC_MAGIC_DEF("type", 0, js_filenode_method, METHOD_TYPE),

    JS_CFUNC_MAGIC_DEF("toString", 0, js_filenode_method, METHOD_TOSTRING),
    JS_CFUNC_MAGIC_DEF("valueOf", 0, js_filenode_method, METHOD_VALUEOF),

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

const JSCFunctionListEntry js_filenode_global_funcs[] = {
    JS_CFUNC_MAGIC_DEF("read", 2, js_filenode_global, GLOBAL_READ),
};

extern "C" int
js_filenode_init(JSContext* ctx, JSModuleDef* m) {

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

  /* create the FileNodeIterator class */
  JS_NewClassID(&js_filenode_iterator_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_filenode_iterator_class_id, &js_filenode_iterator_class);

  filenode_iterator_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, filenode_iterator_proto, js_filenode_iterator_proto_funcs, countof(js_filenode_iterator_proto_funcs));
  JS_SetClassProto(ctx, js_filenode_iterator_class_id, filenode_iterator_proto);

  filenode_iterator_class = JS_NewObjectProto(ctx, JS_NULL);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, filenode_iterator_class, filenode_iterator_proto);
  JS_SetPropertyFunctionList(ctx, filenode_iterator_class, js_filenode_iterator_static_funcs, countof(js_filenode_iterator_static_funcs));

  if(m) {
    JS_SetModuleExport(ctx, m, "FileNode", filenode_class);
    JS_SetModuleExport(ctx, m, "FileNodeIterator", filenode_iterator_class);
    JS_SetModuleExportList(ctx, m, js_filenode_global_funcs, countof(js_filenode_global_funcs));
  }

  return 0;
}

extern "C" void
js_filenode_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "FileNode");
  JS_AddModuleExport(ctx, m, "FileNodeIterator");
  JS_AddModuleExportList(ctx, m, js_filenode_global_funcs, countof(js_filenode_global_funcs));
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
