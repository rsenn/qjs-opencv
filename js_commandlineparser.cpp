#include "js_commandlineparser.hpp"
#include "js_cv.hpp"
#include "include/js_alloc.hpp"
#include "include/js_array.hpp"
#include "include/jsbindings.hpp"
#include "include/util.hpp"

extern "C" {
thread_local JSValue commandlineparser_proto, commandlineparser_class;
thread_local JSClassID js_commandlineparser_class_id;
}

static JSValue
js_commandlineparser_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSCommandLineParserData *clp, *other;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(clp = js_allocate<JSCommandLineParserData>(ctx)))
    return JS_EXCEPTION;

  if(argc == 0) {
    JS_ThrowInternalError(ctx, "argument expected");
    goto fail;
  }

  if((other = js_commandlineparser_data(argv[0]))) {
    new(clp) JSCommandLineParserData(*other);
  } else {
    std::vector<cv::String> args;
    cv::String keys;
    int i = 0;

    if(js_is_array(ctx, argv[i])) {
      js_array_to(ctx, argv[i++], args);
    } else {
      JSValue sa = js_global_get(ctx, "scriptArgs");
      js_array_to(ctx, sa, args);
      JS_FreeValue(ctx, sa);
    }

    size_t argc = args.size();
    std::vector<const char*> stra;
    stra.resize(argc + 1);
    std::transform(args.begin(), args.end(), stra.begin(), [](const std::string& str) -> const char* { return str.c_str(); });

    stra[argc] = nullptr;

    const char** strb = static_cast<const char**>(js_mallocz(ctx, sizeof(const char*) * (argc + 16)));

    for(size_t j = 0; j < stra.size(); ++j)
      strb[j] = stra[j] ? js_strdup(ctx, stra[j]) : nullptr;

    if(i < argc)
      js_value_to(ctx, argv[i], keys);

    try {
      new(clp) JSCommandLineParserData(argc, strb, keys);
    } catch(const std::exception& e) {
      js_cv_throw(ctx, e);
      goto fail;
    }
  }

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_commandlineparser_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, clp);

  return obj;

fail:
  js_deallocate(ctx, clp);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSCommandLineParserData*
js_commandlineparser_data(JSValueConst val) {
  return static_cast<JSCommandLineParserData*>(JS_GetOpaque(val, js_commandlineparser_class_id));
}

JSCommandLineParserData*
js_commandlineparser_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSCommandLineParserData*>(JS_GetOpaque2(ctx, val, js_commandlineparser_class_id));
}

static JSValue
js_commandlineparser_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSCommandLineParserData* clp;
  JSValue ret = JS_UNDEFINED;

  if(!(clp = js_commandlineparser_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {}

  return ret;
}

static JSValue
js_commandlineparser_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSCommandLineParserData* clp;

  if(!(clp = js_commandlineparser_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {}

  return JS_UNDEFINED;
}

enum {
  METHOD_ABOUT,
  METHOD_CHECK,
  METHOD_GET,
  METHOD_GET_PATH_TO_APPLICATION,
  METHOD_HAS,
  METHOD_PRINT_ERRORS,
  METHOD_PRINT_MESSAGE,
};

static JSValue
js_commandlineparser_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSCommandLineParserData* clp;
  JSValue ret = JS_UNDEFINED;

  if(!(clp = js_commandlineparser_data2(ctx, this_val)))
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case METHOD_ABOUT: {
        cv::String message;
        js_value_to(ctx, argv[0], message);

        clp->about(message);
        break;
      }

      case METHOD_CHECK: {
        ret = JS_NewBool(ctx, clp->check());
        break;
      }

      case METHOD_GET: {
        BOOL space_delete = TRUE;
        cv::String s;

        if(argc > 1)
          space_delete = JS_ToBool(ctx, argv[1]);

        if(JS_IsNumber(argv[0])) {
          int32_t index = -1;
          JS_ToInt32(ctx, &index, argv[0]);

          s = clp->get<cv::String>(index, space_delete);
        } else {
          const char* name;

          name = JS_ToCString(ctx, argv[0]);

          s = clp->get<cv::String>(name, space_delete);

          JS_FreeCString(ctx, name);
        }

        ret = JS_NewString(ctx, s.c_str());
        break;
      }

      case METHOD_GET_PATH_TO_APPLICATION: {
        cv::String s = clp->getPathToApplication();
        ret = JS_NewString(ctx, s.c_str());
        break;
      }

      case METHOD_HAS: {
        const char* name;

        name = JS_ToCString(ctx, argv[0]);

        ret = JS_NewBool(ctx, clp->has(name));

        JS_FreeCString(ctx, name);
        break;
      }

      case METHOD_PRINT_ERRORS: {
        clp->printErrors();
        break;
      }

      case METHOD_PRINT_MESSAGE: {
        clp->printMessage();
        break;
      }
    }
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return ret;
}

void
js_commandlineparser_finalizer(JSRuntime* rt, JSValue val) {
  JSCommandLineParserData* clp;
  /* Note: 'clp' can be NULL in case JS_SetOpaque() was not called */

  if((clp = js_commandlineparser_data(val))) {
    clp->~JSCommandLineParserData();

    js_deallocate(rt, clp);
  }
}

JSClassDef js_commandlineparser_class = {
    .class_name = "CommandLineParser",
    .finalizer = js_commandlineparser_finalizer,
};

JSCFunctionListEntry js_commandlineparser_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("about", 1, js_commandlineparser_method, METHOD_ABOUT),
    JS_CFUNC_MAGIC_DEF("check", 0, js_commandlineparser_method, METHOD_CHECK),
    JS_CFUNC_MAGIC_DEF("get", 1, js_commandlineparser_method, METHOD_GET),
    JS_CFUNC_MAGIC_DEF("getPathToApplication", 0, js_commandlineparser_method, METHOD_GET_PATH_TO_APPLICATION),
    JS_CFUNC_MAGIC_DEF("has", 1, js_commandlineparser_method, METHOD_HAS),
    JS_CFUNC_MAGIC_DEF("printErrors", 0, js_commandlineparser_method, METHOD_PRINT_ERRORS),
    JS_CFUNC_MAGIC_DEF("printMessage", 0, js_commandlineparser_method, METHOD_PRINT_MESSAGE),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "CommandLineParser", JS_PROP_CONFIGURABLE),
};

extern "C" int
js_commandlineparser_init(JSContext* ctx, JSModuleDef* m) {

  /* create the CommandLineParser class */
  JS_NewClassID(&js_commandlineparser_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_commandlineparser_class_id, &js_commandlineparser_class);

  commandlineparser_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, commandlineparser_proto, js_commandlineparser_proto_funcs, countof(js_commandlineparser_proto_funcs));
  JS_SetClassProto(ctx, js_commandlineparser_class_id, commandlineparser_proto);

  commandlineparser_class = JS_NewCFunction2(ctx, js_commandlineparser_constructor, "CommandLineParser", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, commandlineparser_class, commandlineparser_proto);

  if(m) {
    JS_SetModuleExport(ctx, m, "CommandLineParser", commandlineparser_class);
  }

  return 0;
}

extern "C" void
js_commandlineparser_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "CommandLineParser");
}

#ifdef JS_COMMANDLINEPARSER_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_commandlineparser
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  if(!(m = JS_NewCModule(ctx, module_name, &js_commandlineparser_init)))
    return NULL;
  js_commandlineparser_export(ctx, m);
  return m;
}
