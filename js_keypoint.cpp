#include "js_alloc.hpp"            // for js_allocate, js_deallocate
#include "js_point.hpp"            // for js_point_new, js_point_argument
#include "jsbindings.hpp"          // for VISIBLE, JSPointData, js_set_inspe...
#include <opencv2/core/types.hpp>  // for KeyPoint
#include <quickjs.h>               // for JSValue, JS_DefinePropertyValueStr
#include <cstdint>                // for int32_t
#include <cstdio>                 // for snprintf, NULL
#include <new>                     // for operator new


typedef cv::KeyPoint JSKeyPointData;

JSValue keypoint_proto = JS_UNDEFINED, keypoint_class = JS_UNDEFINED;
JSClassID js_keypoint_class_id;

extern "C" VISIBLE int js_keypoint_init(JSContext*, JSModuleDef*);

static JSValue
js_keypoint_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  JSKeyPointData keypoint, *kp;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;
  int optind = 0;
  double size, angle = -1, response = 0;
  int32_t octave = -1, class_id = -1;
  JSPointData<double> pt;

  if(!(kp = js_allocate<JSKeyPointData>(ctx)))
    return JS_EXCEPTION;

  /* using new_target to get the prototype is necessary when the
     class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_keypoint_class_id);
  JS_FreeValue(ctx, proto);
  if(JS_IsException(obj))
    goto fail;

  if(argc >= 2) {

    if((optind = js_point_argument(ctx, argc, argv, &pt))) {

      if(optind < argc) {
        JS_ToFloat64(ctx, &size, argv[optind++]);

        if(optind < argc) {
          JS_ToFloat64(ctx, &response, argv[optind++]);
          if(optind < argc) {
            JS_ToInt32(ctx, &octave, argv[optind++]);
            if(optind < argc) {
              JS_ToInt32(ctx, &class_id, argv[optind++]);
            }
          }
        }
      }
    }
  }

  if(optind >= 2)
    new(kp) JSKeyPointData(pt, size, angle, response, octave, class_id);
  else
    new(kp) JSKeyPointData();

  JS_SetOpaque(obj, kp);
  return obj;
fail:
  js_deallocate(ctx, kp);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

VISIBLE JSKeyPointData*
js_keypoint_data(JSContext* ctx, JSValueConst val) {
  return static_cast<JSKeyPointData*>(JS_GetOpaque2(ctx, val, js_keypoint_class_id));
}

VISIBLE JSKeyPointData*
js_keypoint_data(JSValueConst val) {
  return static_cast<JSKeyPointData*>(JS_GetOpaque(val, js_keypoint_class_id));
}

VISIBLE JSValue
js_keypoint_new(JSContext* ctx, const JSKeyPointData& kp) {
  JSValue ret;
  JSKeyPointData* ptr;

  if(JS_IsUndefined(keypoint_proto))
    js_keypoint_init(ctx, NULL);

  ret = JS_NewObjectProtoClass(ctx, keypoint_proto, js_keypoint_class_id);

  ptr = js_allocate<JSKeyPointData>(ctx);

  *ptr = kp;

  JS_SetOpaque(ret, ptr);
  return ret;
}

VISIBLE JSValue
js_keypoint_wrap(JSContext* ctx, const JSKeyPointData& kp) {
  return js_keypoint_new(ctx, kp);
}

void
js_keypoint_finalizer(JSRuntime* rt, JSValue val) {
  JSKeyPointData* kp = static_cast<JSKeyPointData*>(JS_GetOpaque(val, js_keypoint_class_id));
  /* Note: 'kp' can be NULL in case JS_SetOpaque() was not called */

  kp->~JSKeyPointData();
  js_deallocate(rt, kp);
}

static JSValue
js_keypoint_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSKeyPointData* kp = js_keypoint_data(ctx, this_val);
  JSValue obj = JS_NewObjectClass(ctx, js_keypoint_class_id);

  JS_DefinePropertyValueStr(ctx, obj, "angle", JS_NewFloat64(ctx, kp->angle), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "class_id", JS_NewInt32(ctx, kp->class_id), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "octave", JS_NewInt32(ctx, kp->octave), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "pt", js_point_new(ctx, kp->pt), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "response", JS_NewFloat64(ctx, kp->response), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "size", JS_NewFloat64(ctx, kp->size), JS_PROP_ENUMERABLE);
  return obj;
}

enum { METHOD_HASH = 0 };

static JSValue
js_keypoint_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSKeyPointData* kp = static_cast<JSKeyPointData*>(JS_GetOpaque2(ctx, this_val, js_keypoint_class_id));
  JSValue ret = JS_UNDEFINED;

  switch(magic) {

    case METHOD_HASH: {
      char buf[128];
      snprintf(buf, sizeof(buf), "0x%016zx", kp->hash());
      ret = JS_NewString(ctx, buf);
      break;
    }
  }
  return ret;
}

enum { PROP_ANGLE = 0, PROP_CLASS_ID, PROP_OCTAVE, PROP_PT, PROP_RESPONSE, PROP_SIZE };

static JSValue
js_keypoint_getter(JSContext* ctx, JSValueConst this_val, int magic) {
  JSKeyPointData* kp = static_cast<JSKeyPointData*>(JS_GetOpaque2(ctx, this_val, js_keypoint_class_id));
  JSValue ret = JS_UNDEFINED;

  switch(magic) {
    case PROP_ANGLE: {
      ret = JS_NewFloat64(ctx, kp->angle);
      break;
    }
    case PROP_CLASS_ID: {
      ret = JS_NewInt32(ctx, kp->class_id);
      break;
    }
    case PROP_OCTAVE: {
      ret = JS_NewInt32(ctx, kp->octave);
      break;
    }
    case PROP_PT: {
      ret = js_point_new(ctx, kp->pt);
      break;
    }
    case PROP_RESPONSE: {
      ret = JS_NewFloat64(ctx, kp->response);
      break;
    }
    case PROP_SIZE: {
      ret = JS_NewFloat64(ctx, kp->size);
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
    JS_CFUNC_MAGIC_DEF("hash", 0, js_keypoint_method, METHOD_HASH),
    JS_CGETSET_MAGIC_DEF("angle", js_keypoint_getter, 0, PROP_ANGLE),
    JS_CGETSET_MAGIC_DEF("class_id", js_keypoint_getter, 0, PROP_CLASS_ID),
    JS_CGETSET_MAGIC_DEF("octave", js_keypoint_getter, 0, PROP_OCTAVE),
    JS_CGETSET_MAGIC_DEF("pt", js_keypoint_getter, 0, PROP_PT),
    JS_CGETSET_MAGIC_DEF("response", js_keypoint_getter, 0, PROP_RESPONSE),
    JS_CGETSET_MAGIC_DEF("size", js_keypoint_getter, 0, PROP_SIZE),
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

  keypoint_class = JS_NewCFunction2(ctx, js_keypoint_ctor, "KeyPoint", 2, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, keypoint_class, keypoint_proto);

  if(m) {
    JS_SetModuleExport(ctx, m, "KeyPoint", keypoint_class);
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
  JS_AddModuleExport(ctx, m, "KeyPoint");
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