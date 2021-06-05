#include <opencv2/core.hpp>

#include "jsbindings.hpp"
#include "js_point.hpp"
#include "js_alloc.hpp"

typedef cv::KeyPoint JSKeyPointData;

JSValue keypoint_proto = JS_UNDEFINED, keypoint_class = JS_UNDEFINED;
JSClassID js_keypoint_class_id;

extern "C" VISIBLE int js_keypoint_init(JSContext*, JSModuleDef*);

static JSValue
js_keypoint_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  JSKeyPointData keypoint, *s;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;

  if(!(s = js_allocate<JSKeyPointData>(ctx)))
    return JS_EXCEPTION;
  new(s) JSKeyPointData();

  /* using new_target to get the prototype is necessary when the
     class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_keypoint_class_id);
  JS_FreeValue(ctx, proto);
  if(JS_IsException(obj))
    goto fail;
  JS_SetOpaque(obj, s);
  return obj;
fail:
  js_deallocate(ctx, s);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

VISIBLE JSKeyPointData*
js_keypoint_data(JSContext* ctx, JSValueConst val) {
  return static_cast<JSKeyPointData*>(JS_GetOpaque(val, js_keypoint_class_id));
}

VISIBLE JSValue
js_keypoint_new(JSContext* ctx, const JSKeyPointData& kp) {
  JSValue ret;
  JSKeyPointData* s;

  if(JS_IsUndefined(keypoint_proto))
    js_keypoint_init(ctx, NULL);

  ret = JS_NewObjectProtoClass(ctx, keypoint_proto, js_keypoint_class_id);

  s = js_allocate<JSKeyPointData>(ctx);

  *s = kp;

  JS_SetOpaque(ret, s);
  return ret;
}

VISIBLE JSValue
js_keypoint_wrap(JSContext* ctx, const JSKeyPointData& kp) {
  return js_keypoint_new(ctx, kp);
}

static JSValue
js_keypoint_fast(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  cv::Ptr<cv::FastFeatureDetector> fast;

  fast = cv::FastFeatureDetector::create();

  return js_keypoint_wrap(ctx, fast);
}

static JSValue
js_keypoint_gftt(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  cv::Ptr<cv::GFTTDetector> gftt;

  gftt = cv::GFTTDetector::create();

  return js_keypoint_wrap(ctx, gftt);
}

static JSValue
js_keypoint_surf(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  cv::Ptr<cv::xfeatures2d::SURF> surf;

  surf = cv::xfeatures2d::SURF::create();

  return js_keypoint_wrap(ctx, surf);
}

static JSValue
js_keypoint_kaze(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  cv::Ptr<cv::KAZE> kaze;

  kaze = cv::KAZE::create();

  return js_keypoint_wrap(ctx, kaze);
}

static JSValue
js_keypoint_freak(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  cv::Ptr<cv::xfeatures2d::FREAK> freak;

  freak = cv::xfeatures2d::FREAK::create();

  return js_keypoint_wrap(ctx, freak);
}

void
js_keypoint_finalizer(JSRuntime* rt, JSValue val) {
  JSKeyPointData* s = static_cast<JSKeyPointData*>(JS_GetOpaque(val, js_keypoint_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  s->~JSKeyPointData();
  js_deallocate(rt, s);
}

static JSValue
js_keypoint_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSKeyPointData* s = js_keypoint_data(ctx, this_val);
  JSValue obj = JS_NewObjectClass(ctx, js_keypoint_class_id);

  JS_DefinePropertyValueStr(ctx, obj, "empty", JS_NewBool(ctx, (*s)->empty()), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "defaultName", JS_NewString(ctx, (*s)->getDefaultName().c_str()), JS_PROP_ENUMERABLE);
  return obj;
}

enum { METHOD_DETECT = 0 };

static JSValue
js_keypoint_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSKeyPointData* s = static_cast<JSKeyPointData*>(JS_GetOpaque2(ctx, this_val, js_keypoint_class_id));
  JSValue ret = JS_UNDEFINED;

  switch(magic) {}
  return ret;
}

enum { PROP_EMPTY = 0, PROP_DEFAULT_NAME };

static JSValue
js_keypoint_getter(JSContext* ctx, JSValueConst this_val, int magic) {
  JSKeyPointData* s = static_cast<JSKeyPointData*>(JS_GetOpaque2(ctx, this_val, js_keypoint_class_id));
  JSValue ret = JS_UNDEFINED;

  switch(magic) {
    case PROP_DEFAULT_NAME: {
      std::string name = (*s)->getDefaultName();

      ret = JS_NewStringLen(ctx, name.data(), name.size());
      break;
    }
  }
  return ret;
}

JSClassDef js_keypoint_class = {
    .class_name = "KeyPoint",
    .finalizer = js_keypoint_finalizer,
};

const JSCFunctionListEntry js_keypoint_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("detect", 2, js_keypoint_method, METHOD_DETECT),
    JS_CGETSET_MAGIC_DEF("empty", js_keypoint_getter, 0, PROP_EMPTY),
    JS_CGETSET_MAGIC_DEF("defaultName", js_keypoint_getter, 0, PROP_DEFAULT_NAME),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "KeyPoint", JS_PROP_CONFIGURABLE),
};

extern "C" int
js_keypoint_init(JSContext* ctx, JSModuleDef* m) {

  /* create the KeyPoint class */
  JS_NewClassID(&js_keypoint_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_keypoint_class_id, &js_keypoint_class);

  keypoint_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, keypoint_proto, js_keypoint_proto_funcs, countof(js_keypoint_proto_funcs));
  JS_SetClassProto(ctx, js_keypoint_class_id, keypoint_proto);
  js_set_inspect_method(ctx, keypoint_proto, js_keypoint_inspect);

  // keypoint_class = JS_NewCFunction2(ctx, js_keypoint_ctor, "KeyPoint", 2, JS_CFUNC_constructor, 0);
  // JS_SetConstructor(ctx, keypoint_class, keypoint_proto);
  JSValue keypoint_fast = JS_NewCFunction2(ctx, js_keypoint_fast, "FastFeatureDetector", 0, JS_CFUNC_constructor, 0);
  JSValue keypoint_gftt = JS_NewCFunction2(ctx, js_keypoint_gftt, "GFTTDetector", 0, JS_CFUNC_constructor, 0);
  JSValue keypoint_surf = JS_NewCFunction2(ctx, js_keypoint_surf, "SURF", 0, JS_CFUNC_constructor, 0);
  JSValue keypoint_kaze = JS_NewCFunction2(ctx, js_keypoint_kaze, "KAZE", 0, JS_CFUNC_constructor, 0);
  JSValue keypoint_freak = JS_NewCFunction2(ctx, js_keypoint_freak, "FREAK", 0, JS_CFUNC_constructor, 0);

  if(m) {
    JS_SetModuleExport(ctx, m, "FastFeatureDetector", keypoint_fast);
    JS_SetModuleExport(ctx, m, "GFTTDetector", keypoint_gftt);
    JS_SetModuleExport(ctx, m, "SURF", keypoint_surf);
    JS_SetModuleExport(ctx, m, "KAZE", keypoint_kaze);
    JS_SetModuleExport(ctx, m, "FREAK", keypoint_freak);
  }

  return 0;
}

#ifdef JS_KEYPOINT_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_keypoint
#endif

extern "C" VISIBLE void
js_keypoint_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "FastFeatureDetector");
  JS_AddModuleExport(ctx, m, "GFTTDetector");
  JS_AddModuleExport(ctx, m, "SURF");
  JS_AddModuleExport(ctx, m, "KAZE");
  JS_AddModuleExport(ctx, m, "FREAK");
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_keypoint_init);
  if(!m)
    return NULL;
  js_keypoint_export(ctx, m);
  return m;
}