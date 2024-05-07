#include "js_alloc.hpp"
#include "js_size.hpp"
#include "js_point.hpp"
#include "js_mat.hpp"
#include "js_umat.hpp"
#include "jsbindings.hpp"
#include <opencv2/video/background_segm.hpp>

typedef cv::BackgroundSubtractor* JSBackgroundSubtractorData;

JSValue bgsubtractor_proto = JS_UNDEFINED, bgsubtractor_class = JS_UNDEFINED;
JSClassID js_bgsubtractor_class_id;

extern "C" int js_bgsubtractor_init(JSContext*, JSModuleDef*);

static JSValue
js_bgsubtractor_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {

  return JS_UNDEFINED;
}

JSBackgroundSubtractorData*
js_bgsubtractor_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSBackgroundSubtractorData*>(JS_GetOpaque2(ctx, val, js_bgsubtractor_class_id));
}

void
js_bgsubtractor_finalizer(JSRuntime* rt, JSValue val) {
  JSBackgroundSubtractorData* s = static_cast<JSBackgroundSubtractorData*>(JS_GetOpaque(val, js_bgsubtractor_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  s->~JSBackgroundSubtractorData();
  js_deallocate(rt, s);
}

enum { METHOD_APPLY = 0, METHOD_GET_BACKGROUND_IMAGE };

static JSValue
js_bgsubtractor_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSBackgroundSubtractorData* s = static_cast<JSBackgroundSubtractorData*>(JS_GetOpaque2(ctx, this_val, js_bgsubtractor_class_id));

  switch(magic) {
    case METHOD_APPLY: {
      JSInputOutputArray input = js_cv_inputoutputarray(ctx, argv[0]);
      JSOutputArray fgmask = cv::noArray();
      double learningRate = -1;

      if(argc > 1)
        fgmask = js_cv_outputarray(ctx, argv[1]);

      if(argc > 2)
        JS_ToFloat64(ctx, &learningRate, argv[2]);

      (*s)->apply(input, fgmask, learningRate);
      break;
    }

    case METHOD_GET_BACKGROUND_IMAGE: {
      JSOutputArray out = js_cv_outputarray(ctx, argv[0]);
      (*s)->getBackgroundImage(out);
      break;
    }
  }

  return JS_UNDEFINED;
}

enum { PROP_CLIP_LIMIT = 0, PROP_TILES_GRID_SIZE };

static JSValue
js_bgsubtractor_getter(JSContext* ctx, JSValueConst this_val, int magic) {
  JSBackgroundSubtractorData* s = static_cast<JSBackgroundSubtractorData*>(JS_GetOpaque2(ctx, this_val, js_bgsubtractor_class_id));
  JSValue ret = JS_UNDEFINED;

  switch(magic) {}

  return ret;
}

static JSValue
js_bgsubtractor_setter(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic) {
  JSBackgroundSubtractorData* s = static_cast<JSBackgroundSubtractorData*>(JS_GetOpaque2(ctx, this_val, js_bgsubtractor_class_id));

  switch(magic) {}

  return JS_UNDEFINED;
}

JSClassDef js_bgsubtractor_class = {
    .class_name = "BackgroundSubtractor",
    .finalizer = js_bgsubtractor_finalizer,
};

const JSCFunctionListEntry js_bgsubtractor_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("apply", 0, js_bgsubtractor_method, METHOD_APPLY),
    JS_CFUNC_MAGIC_DEF("getBackgroundImage", 0, js_bgsubtractor_method, METHOD_GET_BACKGROUND_IMAGE),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "BackgroundSubtractor", JS_PROP_CONFIGURABLE),
};

extern "C" int
js_bgsubtractor_init(JSContext* ctx, JSModuleDef* m) {

  /* create the BackgroundSubtractor class */
  JS_NewClassID(&js_bgsubtractor_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_bgsubtractor_class_id, &js_bgsubtractor_class);

  bgsubtractor_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, bgsubtractor_proto, js_bgsubtractor_proto_funcs, countof(js_bgsubtractor_proto_funcs));
  JS_SetClassProto(ctx, js_bgsubtractor_class_id, bgsubtractor_proto);

  bgsubtractor_class = JS_NewCFunction2(ctx, js_bgsubtractor_ctor, "BackgroundSubtractor", 2, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, bgsubtractor_class, bgsubtractor_proto);

  if(m)
    JS_SetModuleExport(ctx, m, "BackgroundSubtractor", bgsubtractor_class);

  return 0;
}

void
js_bgsubtractor_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(bgsubtractor_class))
    js_bgsubtractor_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "BackgroundSubtractor", bgsubtractor_class);
}

#ifdef JS_BackgroundSubtractor_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_bgsubtractor
#endif

extern "C" void
js_bgsubtractor_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "BackgroundSubtractor");
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_bgsubtractor_init);
  if(!m)
    return NULL;
  js_bgsubtractor_export(ctx, m);
  return m;
}
