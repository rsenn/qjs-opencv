#include "jsbindings.hpp"
#include "js_size.hpp"
#include "js_point.hpp"
#include "js_mat.hpp"
#include "js_alloc.hpp"

JSValue clahe_proto = JS_UNDEFINED, clahe_class = JS_UNDEFINED;
JSClassID js_clahe_class_id;

extern "C" VISIBLE int js_clahe_init(JSContext*, JSModuleDef*);

extern "C" VISIBLE JSValue
js_clahe_new(JSContext* ctx, double clipLimit = 40.0, cv::Size tileGridSize = cv::Size(8, 8)) {
  JSValue ret;
  JSCLAHEData* s;
  ret = JS_NewObjectProtoClass(ctx, clahe_proto, js_clahe_class_id);
  s = js_allocate<JSCLAHEData>(ctx);
  new(s) JSCLAHEData();
  *s = cv::createCLAHE(clipLimit, tileGridSize);
  JS_SetOpaque(ret, s);
  return ret;
}

static JSValue
js_clahe_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  double clipLimit = 40.0;
  JSSizeData<double> tileGridSize = cv::Size2d(8, 8);
  if(argc >= 1)
    JS_ToFloat64(ctx, &clipLimit, argv[0]);
  if(argc >= 2)
    if(!js_size_read(ctx, argv[1], &tileGridSize))
      return JS_EXCEPTION;
  return js_clahe_new(ctx, clipLimit, tileGridSize);
}

JSCLAHEData*
js_clahe_data(JSContext* ctx, JSValueConst val) {
  return static_cast<JSCLAHEData*>(JS_GetOpaque2(ctx, val, js_clahe_class_id));
}

void
js_clahe_finalizer(JSRuntime* rt, JSValue val) {
  JSCLAHEData* s = static_cast<JSCLAHEData*>(JS_GetOpaque(val, js_clahe_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  s->~JSCLAHEData();
  js_deallocate(rt, s);
}

static JSValue
js_clahe_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSCLAHEData* s = static_cast<JSCLAHEData*>(JS_GetOpaque2(ctx, this_val, js_clahe_class_id));
  JSValue ret = JS_UNDEFINED;
  JSPointData<double> point = js_point_get(ctx, argv[0]);

  switch(magic) {
    case 0: {
      cv::Mat *input, *output;
      if(argc < 2)
        return JS_EXCEPTION;
      input = js_mat_data(ctx, argv[0]);
      output = js_mat_data(ctx, argv[1]);
      if(input == nullptr || output == nullptr)
        return JS_EXCEPTION;
      (*s)->apply(*input, *output);
      break;
    }
    case 1: {
      (*s)->collectGarbage();
      break;
    }
    case 2: {
      ret = JS_NewFloat64(ctx, (*s)->getClipLimit());
      break;
    }
    case 3: {
      ret = js_size_wrap(ctx, (*s)->getTilesGridSize());
      break;
    }
    case 4: {
      double clipLimit;
      if(argc < 1 || JS_ToFloat64(ctx, &clipLimit, argv[0]) == -1)
        return JS_EXCEPTION;
      (*s)->setClipLimit(clipLimit);
      break;
    }
    case 5: {
      JSSizeData<double> size;
      if(argc < 1 || !js_size_read(ctx, argv[0], &size))
        return JS_EXCEPTION;
      (*s)->setTilesGridSize(size);
      break;
    }
  }
  return ret;
}

JSClassDef js_clahe_class = {
    .class_name = "CLAHE",
    .finalizer = js_clahe_finalizer,
};

const JSCFunctionListEntry js_clahe_proto_funcs[] = {JS_CFUNC_MAGIC_DEF("apply", 0, js_clahe_method, 0),
                                                     JS_CFUNC_MAGIC_DEF("collectGarbage", 0, js_clahe_method, 1),
                                                     JS_CFUNC_MAGIC_DEF("getClipLimit", 0, js_clahe_method, 2),
                                                     JS_CFUNC_MAGIC_DEF("getTilesGridSize", 0, js_clahe_method, 3),
                                                     JS_CFUNC_MAGIC_DEF("setClipLimit", 0, js_clahe_method, 4),
                                                     JS_CFUNC_MAGIC_DEF("setTilesGridSize", 0, js_clahe_method, 5),
                                                     JS_PROP_STRING_DEF("[Symbol.toStringTag]", "CLAHE", JS_PROP_CONFIGURABLE)};

extern "C" int
js_clahe_init(JSContext* ctx, JSModuleDef* m) {

  /* create the CLAHE class */
  JS_NewClassID(&js_clahe_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_clahe_class_id, &js_clahe_class);

  clahe_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, clahe_proto, js_clahe_proto_funcs, countof(js_clahe_proto_funcs));
  JS_SetClassProto(ctx, js_clahe_class_id, clahe_proto);

  clahe_class = JS_NewCFunction2(ctx, js_clahe_ctor, "CLAHE", 2, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, clahe_class, clahe_proto);

  if(m)
    JS_SetModuleExport(ctx, m, "CLAHE", clahe_class);

  return 0;
}

void
js_clahe_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(clahe_class))
    js_clahe_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "CLAHE", clahe_class);
}

#ifdef JS_CLAHE_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_clahe
#endif

extern "C" VISIBLE void
js_clahe_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "CLAHE");
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_clahe_init);
  if(!m)
    return NULL;
  js_clahe_export(ctx, m);
  return m;
}