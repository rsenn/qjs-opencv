#include "js_matx.hpp"
#include "js_alloc.hpp"
#include "js_umat.hpp"

extern "C" {
thread_local JSValue matx_proto = JS_UNDEFINED, matx_class = JS_UNDEFINED;
thread_local JSClassID js_matx_class_id = 0;
}

static JSValue
js_matx_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSInputOutputArray* matx;
  JSValue obj, proto;

  if(!(matx = js_allocate<JSInputOutputArray>(ctx)))
    return JS_EXCEPTION;

  if(argc > 0)
    new(matx) JSInputOutputArray(js_cv_inputoutputarray(ctx, argv[0]));
  else
    new(matx) JSInputOutputArray();

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_contour_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, matx);

  return obj;

fail:
  js_deallocate(ctx, matx);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSInputOutputArray*
js_matx_data(JSValueConst val) {
  return static_cast<JSInputOutputArray*>(JS_GetOpaque(val, js_matx_class_id));
}

JSInputOutputArray*
js_matx_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSInputOutputArray*>(JS_GetOpaque2(ctx, val, js_matx_class_id));
}

enum {};

static JSValue
js_matx_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSValue ret = JS_UNDEFINED;
  JSInputOutputArray* s;

  if((s = static_cast<JSInputOutputArray*>(JS_GetOpaque /*2*/ (/*ctx, */ this_val, js_matx_class_id))) == nullptr)
    return JS_UNDEFINED;

  switch(magic) {}

  return ret;
}

static JSValue
js_matx_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSInputOutputArray* s;
  double v;
  if((s = static_cast<JSInputOutputArray*>(JS_GetOpaque2(ctx, this_val, js_matx_class_id))) == nullptr)
    return JS_EXCEPTION;

  if(JS_ToFloat64(ctx, &v, val))
    return JS_EXCEPTION;

  switch(magic) {}

  return JS_UNDEFINED;
}

enum { FUNC_ALL };

static JSValue
js_matx_funcs(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSInputOutputArray matx;
  JSValue ret = JS_EXCEPTION;

  switch(magic) {

    case FUNC_ALL: {
      break;
    }
  }

  return ret;
}

enum {

};

static JSValue
js_matx_method(JSContext* ctx, JSValueConst matx, int argc, JSValueConst argv[], int magic) {
  JSInputOutputArray* s = static_cast<JSInputOutputArray*>(JS_GetOpaque2(ctx, matx, js_matx_class_id));
  JSValue ret = JS_UNDEFINED;

  switch(magic) {}

  return ret;
}

void
js_matx_finalizer(JSRuntime* rt, JSValue val) {
  JSInputOutputArray* matx = static_cast<JSInputOutputArray*>(JS_GetOpaque(val, js_matx_class_id));
  /* Note: 'matx can be NULL in case JS_SetOpaque() was not called */

  if(matx) {
    matx->~JSInputOutputArray();
    js_deallocate(rt, matx);
  }
}

JSClassDef js_matx_class = {
    .class_name = "Matx",
    .finalizer = js_matx_finalizer,
};

const JSCFunctionListEntry js_matx_proto_funcs[] = {
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Matx", JS_PROP_CONFIGURABLE),
};
const JSCFunctionListEntry js_matx_static_funcs[] = {
    JS_CFUNC_MAGIC_DEF("all", 1, js_matx_funcs, FUNC_ALL),
};

extern "C" int
js_matx_init(JSContext* ctx, JSModuleDef* m) {

  if(js_matx_class_id == 0) {
    /* create the Matx class */
    JS_NewClassID(&js_matx_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_matx_class_id, &js_matx_class);

    matx_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, matx_proto, js_matx_proto_funcs, countof(js_matx_proto_funcs));
    JS_SetClassProto(ctx, js_matx_class_id, matx_proto);

    matx_class = JS_NewCFunction2(ctx, js_matx_constructor, "Matx", 0, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, matx_class, matx_proto);
    JS_SetPropertyFunctionList(ctx, matx_class, js_matx_static_funcs, countof(js_matx_static_funcs));

    // js_object_inspect(ctx, matx_proto, js_matx_inspect);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Matx", matx_class);

  return 0;
}

extern "C" void
js_matx_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Matx");
}

#ifdef JS_MATX_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_matx
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_matx_init)))
    return NULL;

  js_matx_export(ctx, m);
  return m;
}
