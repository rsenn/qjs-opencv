#include "jsbindings.hpp"
#include "js_alloc.hpp"

extern "C" {

JSValue tick_meter_proto = JS_UNDEFINED, tick_meter_class = JS_UNDEFINED;
JSClassID js_tick_meter_class_id = 0;

VISIBLE JSValue
js_tick_meter_new(JSContext* ctx) {
  JSValue ret;
  JSTickMeterData* s;

  ret = JS_NewObjectProtoClass(ctx, tick_meter_proto, js_tick_meter_class_id);

  s = js_allocate<JSTickMeterData>(ctx);

  new(s) cv::TickMeter();

  JS_SetOpaque(ret, s);
  return ret;
}

static JSValue
js_tick_meter_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  JSTickMeterData* s;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;

  s = js_allocate<JSTickMeterData>(ctx);
  if(!s)
    return JS_EXCEPTION;
  new(s) cv::TickMeter();

  /* using new_target to get the prototype is necessary when the
     class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_tick_meter_class_id);
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

JSTickMeterData*
js_tick_meter_data(JSContext* ctx, JSValueConst val) {
  return static_cast<JSTickMeterData*>(JS_GetOpaque2(ctx, val, js_tick_meter_class_id));
}

void
js_tick_meter_finalizer(JSRuntime* rt, JSValue val) {
  JSTickMeterData* s = static_cast<JSTickMeterData*>(JS_GetOpaque(val, js_tick_meter_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */
  js_deallocate(rt, s);
}

static JSValue
js_tick_meter_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSValue ret = JS_UNDEFINED;
  JSTickMeterData* s = static_cast<JSTickMeterData*>(JS_GetOpaque2(ctx, this_val, js_tick_meter_class_id));
  if(!s)
    ret = JS_EXCEPTION;
  else if(magic == 0)
    ret = JS_NewInt64(ctx, s->getCounter());
  else if(magic == 1)
    ret = JS_NewFloat64(ctx, s->getTimeMicro());
  else if(magic == 2)
    ret = JS_NewFloat64(ctx, s->getTimeMilli());
  else if(magic == 3)
    ret = JS_NewFloat64(ctx, s->getTimeSec());
  else if(magic == 4)
    ret = JS_NewInt64(ctx, s->getTimeTicks());
#if CV_VERSION_MAJOR >= 4 && CV_VERSION_MINOR >= 4
  else if(magic == 5)
    ret = JS_NewFloat64(ctx, s->getAvgTimeMilli());
  else if(magic == 6)
    ret = JS_NewFloat64(ctx, s->getAvgTimeSec());
  else if(magic == 7)
    ret = JS_NewFloat64(ctx, s->getFPS());
#endif
  return ret;
}

static JSValue
js_tick_meter_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValue ret = JS_UNDEFINED;
  JSTickMeterData* s = static_cast<JSTickMeterData*>(JS_GetOpaque2(ctx, this_val, js_tick_meter_class_id));
  if(!s)
    ret = JS_EXCEPTION;
  else if(magic == 0)
    s->reset();
  else if(magic == 1)
    s->start();
  else if(magic == 2)
    s->stop();

  return ret;
}

static JSValue
js_tick_meter_tostring(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSTickMeterData* s;

  if((s = static_cast<JSTickMeterData*>(JS_GetOpaque2(ctx, this_val, js_tick_meter_class_id)))) {
    std::ostringstream os;
    os << *s;
    std::string str = os.str();
    return JS_NewStringLen(ctx, str.data(), str.size());
  }

  return JS_EXCEPTION;
}

JSClassDef js_tick_meter_class = {
    .class_name = "TickMeter",
    .finalizer = js_tick_meter_finalizer,
};

const JSCFunctionListEntry js_tick_meter_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("reset", 0, js_tick_meter_method, 0),
    JS_CFUNC_MAGIC_DEF("start", 0, js_tick_meter_method, 1),
    JS_CFUNC_MAGIC_DEF("stop", 0, js_tick_meter_method, 2),
    JS_CGETSET_MAGIC_DEF("counter", js_tick_meter_get, NULL, 0),
    JS_CGETSET_MAGIC_DEF("timeMicro", js_tick_meter_get, NULL, 1),
    JS_CGETSET_MAGIC_DEF("timeMilli", js_tick_meter_get, NULL, 2),
    JS_CGETSET_MAGIC_DEF("timeSec", js_tick_meter_get, NULL, 3),
    JS_CGETSET_MAGIC_DEF("timeTicks", js_tick_meter_get, NULL, 4),
#if CV_VERSION_MAJOR >= 4 && CV_VERSION_MINOR >= 4
    JS_CGETSET_MAGIC_DEF("avgTimeMilli", js_tick_meter_get, NULL, 5),
    JS_CGETSET_MAGIC_DEF("avgTimeSec", js_tick_meter_get, NULL, 6),
    JS_CGETSET_MAGIC_DEF("fps", js_tick_meter_get, NULL, 7),
#endif
    JS_CFUNC_DEF("toString", 0, js_tick_meter_tostring),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "TickMeter", JS_PROP_CONFIGURABLE),
};
int
js_tick_meter_init(JSContext* ctx, JSModuleDef* m) {

  if(js_tick_meter_class_id == 0) {
    /* create the TickMeter class */
    JS_NewClassID(&js_tick_meter_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_tick_meter_class_id, &js_tick_meter_class);

    tick_meter_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, tick_meter_proto, js_tick_meter_proto_funcs, countof(js_tick_meter_proto_funcs));
    JS_SetClassProto(ctx, js_tick_meter_class_id, tick_meter_proto);

    tick_meter_class = JS_NewCFunction2(ctx, js_tick_meter_ctor, "TickMeter", 0, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, tick_meter_class, tick_meter_proto);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "TickMeter", tick_meter_class);

  return 0;
}

void
js_tick_meter_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(tick_meter_class))
    js_tick_meter_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "TickMeter", tick_meter_class);
}

#ifdef JS_UTILITY_MODULE
#define JS_INIT_MODULE /*VISIBLE*/ js_init_module
#else
#define JS_INIT_MODULE /*VISIBLE*/ js_init_module_utility
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_tick_meter_init);
  if(!m)
    return NULL;
  JS_AddModuleExport(ctx, m, "TickMeter");
  return m;
}
}
