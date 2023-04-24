#include "js_affine3.hpp"
#include "js_size.hpp"
#include "js_alloc.hpp"
#include "js_array.hpp"
#include "js_point.hpp"
#include "js_typed_array.hpp"
#include "jsbindings.hpp"
#include "affine3.hpp"
#include <quickjs.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <opencv2/core/types.hpp>
#include <utility>

enum { PROP_A = 0, PROP_B, PROP_SLOPE, PROP_PIVOT, PROP_TO, PROP_ANGLE, PROP_ASPECT, PROP_LENGTH };

extern "C" {
JSValue affine3_proto = JS_UNDEFINED, affine3_class = JS_UNDEFINED;
thread_local VISIBLE JSClassID js_affine3_class_id = 0;

VISIBLE cv::Affine3<double>*
js_affine3_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<cv::Affine3<double>*>(JS_GetOpaque2(ctx, val, js_affine3_class_id));
}
VISIBLE cv::Affine3<double>*
js_affine3_data(JSValueConst val) {
  return static_cast<cv::Affine3<double>*>(JS_GetOpaque(val, js_affine3_class_id));
}
}

VISIBLE JSValue
js_affine3_wrap(JSContext* ctx, const cv::Affine3<double>& affine3) {
  return js_affine3_new(ctx, affine3.x1, affine3.y1, affine3.x2, affine3.y2);
}

VISIBLE JSValue
js_affine3_wrap(JSContext* ctx, const cv::Affine3<int>& affine3) {
  return js_affine3_new(ctx, affine3.x1, affine3.y1, affine3.x2, affine3.y2);
}

extern "C" {
VISIBLE JSValue
js_affine3_new(JSContext* ctx, double x1, double y1, double x2, double y2) {
  JSValue ret;
  cv::Affine3<double>* ln;

  if(JS_IsUndefined(affine3_proto))
    js_affine3_init(ctx, NULL);

  ret = JS_NewObjectProtoClass(ctx, affine3_proto, js_affine3_class_id);

  ln = js_allocate<cv::Affine3<double>>(ctx);
/*
  ln->array[0] = x1;
  ln->array[1] = y1;
  ln->array[2] = x2;
  ln->array[3] = y2;
*/
  JS_SetOpaque(ret, ln);
  return ret;
}

static JSValue
js_affine3_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Affine3<double>* ln;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;

  ln = js_allocate<cv::Affine3<double>>(ctx);
  if(!ln)
    return JS_EXCEPTION;
 
  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_affine3_class_id);
  JS_FreeValue(ctx, proto);
  if(JS_IsException(obj))
    goto fail;
  JS_SetOpaque(obj, ln);
  return obj;
fail:
  js_deallocate(ctx, ln);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}
 
static JSValue
js_affine3_get(JSContext* ctx, JSValueConst this_val, int magic) {
  cv::Affine3<double>* ln;

  if(!(ln = static_cast<cv::Affine3<double>*>(JS_GetOpaque /*2*/ (/*ctx,*/ this_val, js_affine3_class_id))))
    return JS_UNDEFINED;

  switch(magic) {
    
  }
  return JS_UNDEFINED;
}

static JSValue
js_affine3_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  cv::Affine3<double>* ln;
  double v;
  if(!(ln = static_cast<cv::Affine3<double>*>(JS_GetOpaque2(ctx, this_val, js_affine3_class_id))))
    return JS_EXCEPTION;
  switch(magic) {
     
  }

  return JS_UNDEFINED;
}
 
 
enum {
  METHOD_SWAP = 0,
  METHOD_AT,
  METHOD_INTERSECT,
  METHOD_ENDPOINT_DISTANCES,
  METHOD_DISTANCE,
  METHOD_XINTERCEPT,
  METHOD_YINTERCEPT,
  METHOD_ADD,
  METHOD_SUB,
  METHOD_MUL,
  METHOD_DIV
};

static JSValue
js_affine3_methods(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  cv::Affine3<double>* ln;
  JSValue ret = JS_UNDEFINED;
  if(!(ln = static_cast<cv::Affine3<double>*>(JS_GetOpaque2(ctx, this_val, js_affine3_class_id))))
    return JS_EXCEPTION;

  switch(magic) {
  
  }
  return ret;
}

static JSValue
js_affine3_toarray(JSContext* ctx, JSValueConst affine3, int argc, JSValueConst* arg) {
  cv::Affine3<double>* ln;

  if(!(ln = static_cast<cv::Affine3<double>*>(JS_GetOpaque2(ctx, affine3, js_affine3_class_id))))
    return JS_EXCEPTION;

  return js_typedarray_from(ctx, ln->array);
}

static JSValue
js_call_method(JSContext* ctx, JSValue obj, const char* name, int argc, JSValueConst argv[]) {
  JSValue fn, ret = JS_UNDEFINED;

  fn = JS_GetPropertyStr(ctx, obj, name);
  if(!JS_IsUndefined(fn))
    ret = JS_Call(ctx, fn, obj, argc, argv);
  return ret;
} 

static JSValue
js_affine3_from(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  cv::Affine3<double> affine3 ;
  JSValue ret = JS_EXCEPTION;
 
  return ret;
}

static JSValue
js_affine3_sum(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  cv::Affine3<double> affine3;

  for(int i = 0; i < argc; i++) {
    cv::Affine3<double> tmp;
    if(js_affine3_read(ctx, argv[i], &tmp)) {
      affine3.x1 += tmp.x1;
      affine3.y1 += tmp.y1;
      affine3.x2 += tmp.x2;
      affine3.y2 += tmp.y2;
    }
  }

  return js_affine3_new(ctx, affine3.array[0], affine3.array[1], affine3.array[2], affine3.array[3]);
}

void
js_affine3_finalizer(JSRuntime* rt, JSValue val) {
  cv::Affine3<double>* ln;

  if((ln = static_cast<cv::Affine3<double>*>(JS_GetOpaque(val, js_affine3_class_id))))
    /* Note: 'ln' can be NULL in case JS_SetOpaque() was not called */
    js_deallocate(rt, ln);
}

JSClassDef js_affine3_class = {
    .class_name = "Affine3",
    .finalizer = js_affine3_finalizer,
};

const JSCFunctionListEntry js_affine3_proto_funcs[] = {
      JS_CGETSET_MAGIC_DEF("a", js_affine3_get, js_affine3_set_ab, PROP_A),
    JS_CGETSET_MAGIC_DEF("b", js_affine3_get, js_affine3_set_ab, PROP_B),
    JS_CGETSET_MAGIC_DEF("0", js_affine3_get, js_affine3_set_ab, PROP_A),
    JS_CGETSET_MAGIC_DEF("1", js_affine3_get, js_affine3_set_ab, PROP_B),
  
    JS_CFUNC_MAGIC_DEF("xIntercept", 0, js_affine3_methods, METHOD_XINTERCEPT),
    JS_CFUNC_MAGIC_DEF("yIntercept", 0, js_affine3_methods, METHOD_YINTERCEPT),
    JS_CFUNC_MAGIC_DEF("swap", 0, js_affine3_methods, METHOD_SWAP),
    JS_CFUNC_MAGIC_DEF("at", 1, js_affine3_methods, METHOD_AT),
    JS_CFUNC_MAGIC_DEF("intersect", 1, js_affine3_methods, METHOD_INTERSECT),
    JS_CFUNC_MAGIC_DEF("endpointDistances", 1, js_affine3_methods, METHOD_ENDPOINT_DISTANCES),
    JS_CFUNC_MAGIC_DEF("distance", 1, js_affine3_methods, METHOD_DISTANCE),
    JS_CFUNC_MAGIC_DEF("add", 1, js_affine3_methods, METHOD_ADD),
    JS_CFUNC_MAGIC_DEF("sub", 1, js_affine3_methods, METHOD_SUB),
    JS_CFUNC_MAGIC_DEF("mul", 1, js_affine3_methods, METHOD_MUL),
    JS_CFUNC_MAGIC_DEF("div", 1, js_affine3_methods, METHOD_DIV),
    JS_CFUNC_DEF("toArray", 0, js_affine3_toarray),
      JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Affine3", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_affine3_static_funcs[] = {
    JS_CFUNC_DEF("from", 1, js_affine3_from),
    JS_CFUNC_DEF("sum", 1, js_affine3_sum),
};

int
js_affine3_init(JSContext* ctx, JSModuleDef* m) {

  if(js_affine3_class_id == 0) {
    /* create the Affine3 class */
    JS_NewClassID(&js_affine3_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_affine3_class_id, &js_affine3_class);

    affine3_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, affine3_proto, js_affine3_proto_funcs, countof(js_affine3_proto_funcs));
    JS_SetClassProto(ctx, js_affine3_class_id, affine3_proto);

    affine3_class = JS_NewCFunction2(ctx, js_affine3_ctor, "Affine3", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, affine3_class, affine3_proto);
    JS_SetPropertyFunctionList(ctx, affine3_class, js_affine3_static_funcs, countof(js_affine3_static_funcs));

    js_set_inspect_method(ctx, affine3_proto, js_affine3_inspect);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Affine3", affine3_class);

  return 0;
}

extern "C" VISIBLE void
js_affine3_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Affine3");
}

void
js_affine3_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(affine3_class))
    js_affine3_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "Affine3", affine3_class);
}

#ifdef JS_AFFINE3_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_affine3
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_affine3_init);
  if(!m)
    return NULL;
  js_affine3_export(ctx, m);
  return m;
}
}
