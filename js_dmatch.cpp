#include "js_dmatch.hpp"
#include "js_alloc.hpp"
#include <quickjs.h>
#include <new>

extern "C" {
thread_local JSValue dmatch_proto = JS_UNDEFINED, dmatch_class = JS_UNDEFINED;
thread_local JSClassID js_dmatch_class_id;
}

extern "C" int js_dmatch_init(JSContext*, JSModuleDef*);

static JSValue
js_dmatch_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSDMatchData* dm;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;
  int queryIdx = 0, trainIdx = 0, imgIdx = 0;
  float distance = 0.0f;

  if(!(dm = js_allocate<JSDMatchData>(ctx)))
    return JS_EXCEPTION;

  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_dmatch_class_id);
  JS_FreeValue(ctx, proto);
  if(JS_IsException(obj))
    goto fail;

  if(argc >= 1)
    JS_ToInt32(ctx, &queryIdx, argv[0]);
  if(argc >= 2)
    JS_ToInt32(ctx, &trainIdx, argv[1]);

  // Handle both 3-arg and 4-arg constructors
  if(argc == 3) {
    // DMatch(queryIdx, trainIdx, distance) - 3 arg constructor
    double d;
    JS_ToFloat64(ctx, &d, argv[2]);
    distance = (float)d;
    new(dm) JSDMatchData(queryIdx, trainIdx, distance);
  } else if(argc >= 4) {
    // DMatch(queryIdx, trainIdx, imgIdx, distance) - 4 arg constructor
    JS_ToInt32(ctx, &imgIdx, argv[2]);
    double d;
    JS_ToFloat64(ctx, &d, argv[3]);
    distance = (float)d;
    new(dm) JSDMatchData(queryIdx, trainIdx, imgIdx, distance);
  } else {
    // Default constructor
    new(dm) JSDMatchData();
  }

  JS_SetOpaque(obj, dm);
  return obj;
fail:
  js_deallocate(ctx, dm);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSDMatchData*
js_dmatch_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSDMatchData*>(JS_GetOpaque2(ctx, val, js_dmatch_class_id));
}

JSDMatchData*
js_dmatch_data(JSValueConst val) {
  return static_cast<JSDMatchData*>(JS_GetOpaque(val, js_dmatch_class_id));
}

JSValue
js_dmatch_new(JSContext* ctx, const JSDMatchData& dm) {
  JSValue ret;
  JSDMatchData* ptr;

  if(JS_IsUndefined(dmatch_proto))
    js_dmatch_init(ctx, NULL);

  ret = JS_NewObjectProtoClass(ctx, dmatch_proto, js_dmatch_class_id);

  ptr = js_allocate<JSDMatchData>(ctx);

  *ptr = dm;

  JS_SetOpaque(ret, ptr);
  return ret;
}

JSValue
js_dmatch_wrap(JSContext* ctx, const JSDMatchData& dm) {
  return js_dmatch_new(ctx, dm);
}

void
js_dmatch_finalizer(JSRuntime* rt, JSValue val) {
  JSDMatchData* dm = static_cast<JSDMatchData*>(JS_GetOpaque(val, js_dmatch_class_id));
  if(dm) {
    dm->~JSDMatchData();
    js_deallocate(rt, dm);
  }
}

static JSValue
js_dmatch_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSDMatchData* dm = js_dmatch_data2(ctx, this_val);
  JSValue obj = JS_NewObjectProto(ctx, dmatch_proto);

  JS_DefinePropertyValueStr(ctx, obj, "queryIdx", JS_NewInt32(ctx, dm->queryIdx), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "trainIdx", JS_NewInt32(ctx, dm->trainIdx), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "imgIdx", JS_NewInt32(ctx, dm->imgIdx), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "distance", JS_NewFloat64(ctx, dm->distance), JS_PROP_ENUMERABLE);
  return obj;
}

enum {
  PROP_QUERY_IDX = 0,
  PROP_TRAIN_IDX,
  PROP_IMG_IDX,
  PROP_DISTANCE,
};

static JSValue
js_dmatch_getter(JSContext* ctx, JSValueConst this_val, int magic) {
  JSDMatchData* dm = static_cast<JSDMatchData*>(JS_GetOpaque2(ctx, this_val, js_dmatch_class_id));
  JSValue ret = JS_UNDEFINED;

  switch(magic) {
    case PROP_QUERY_IDX: ret = JS_NewInt32(ctx, dm->queryIdx); break;
    case PROP_TRAIN_IDX: ret = JS_NewInt32(ctx, dm->trainIdx); break;
    case PROP_IMG_IDX: ret = JS_NewInt32(ctx, dm->imgIdx); break;
    case PROP_DISTANCE: ret = JS_NewFloat64(ctx, dm->distance); break;
  }

  return ret;
}

JSClassDef js_dmatch_class = {
    .class_name = "DMatch",
    .finalizer = js_dmatch_finalizer,
};

const JSCFunctionListEntry js_dmatch_proto_funcs[] = {
    JS_CGETSET_ENUMERABLE_DEF("queryIdx", js_dmatch_getter, 0, PROP_QUERY_IDX),
    JS_CGETSET_ENUMERABLE_DEF("trainIdx", js_dmatch_getter, 0, PROP_TRAIN_IDX),
    JS_CGETSET_ENUMERABLE_DEF("imgIdx", js_dmatch_getter, 0, PROP_IMG_IDX),
    JS_CGETSET_ENUMERABLE_DEF("distance", js_dmatch_getter, 0, PROP_DISTANCE),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "DMatch", JS_PROP_CONFIGURABLE),
};

extern "C" int
js_dmatch_init(JSContext* ctx, JSModuleDef* m) {

  JS_NewClassID(&js_dmatch_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_dmatch_class_id, &js_dmatch_class);

  dmatch_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, dmatch_proto, js_dmatch_proto_funcs, countof(js_dmatch_proto_funcs));
  JS_SetClassProto(ctx, js_dmatch_class_id, dmatch_proto);

  dmatch_class = JS_NewCFunction2(ctx, js_dmatch_constructor, "DMatch", 4, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, dmatch_class, dmatch_proto);

  if(m) {
    JS_SetModuleExport(ctx, m, "DMatch", dmatch_class);
  }

  return 0;
}

extern "C" void
js_dmatch_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "DMatch");
}
