#include "js_alloc.hpp"
#include "include/jsbindings.hpp"
#include <opencv2/core/utility.hpp>
#include <opencv2/core/version.hpp>
#include <quickjs.h>
#include <stddef.h>
#include <stdint.h>
#include <iosfwd>
#include <new>
#include <string>

extern "C" {
#include "third_party/Hash-Algorithms/sha1.h"
#include "third_party/Hash-Algorithms/sha256.h"
}

extern "C" int js_utility_init(JSContext*, JSModuleDef*);

extern "C" {

thread_local JSValue tick_meter_proto = JS_UNDEFINED, tick_meter_class = JS_UNDEFINED;
thread_local JSClassID js_tick_meter_class_id = 0;

JSValue
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
js_tick_meter_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
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
js_tick_meter_data2(JSContext* ctx, JSValueConst val) {
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
js_tick_meter_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
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
js_tick_meter_tostring(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSTickMeterData* s;

  if((s = static_cast<JSTickMeterData*>(JS_GetOpaque2(ctx, this_val, js_tick_meter_class_id)))) {
    std::ostringstream os;
    os << *s;
    std::string str = os.str();
    return JS_NewStringLen(ctx, str.data(), str.size());
  }

  return JS_EXCEPTION;
}

/* digest of a string, ArrayBuffer or typed array as lowercase hex */
static JSValue
js_hash_digest(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  static const char hex[] = "0123456789abcdef";
  uint32_t words[8];
  char out[65];
  size_t len = 0, nwords = magic ? 8 : 5;
  const char* str = NULL;
  uint8_t* ptr = NULL;
  JSValue buf = JS_UNDEFINED;

  if(argc < 1)
    return JS_ThrowTypeError(ctx, "expected string, ArrayBuffer or typed array");

  if(JS_IsString(argv[0])) {
    if(!(str = JS_ToCStringLen(ctx, &len, argv[0])))
      return JS_EXCEPTION;
    ptr = (uint8_t*)str;
  } else {
    size_t offset = 0, bytes = 0, size = 0;

    buf = JS_GetTypedArrayBuffer(ctx, argv[0], &offset, &bytes, &size);

    /* not a view: clear the probe exception, take it as an ArrayBuffer */
    if(JS_IsException(buf)) {
      JS_FreeValue(ctx, JS_GetException(ctx));
      buf = JS_DupValue(ctx, argv[0]);
      offset = 0;
      bytes = (size_t)-1;
    }

    if(!(ptr = JS_GetArrayBuffer(ctx, &len, buf))) {
      JS_FreeValue(ctx, buf);
      return JS_EXCEPTION;
    }

    ptr += offset;
    len = bytes == (size_t)-1 ? len : bytes;
  }

  if(magic)
    sha256_digest(ptr, len, words, false);
  else
    sha1_digest(ptr, len, words, false);

  if(str)
    JS_FreeCString(ctx, str);
  JS_FreeValue(ctx, buf);

  for(size_t i = 0; i < nwords * 4; ++i) {
    uint8_t byte = words[i / 4] >> (24 - 8 * (i % 4));

    out[2 * i] = hex[byte >> 4];
    out[2 * i + 1] = hex[byte & 15];
  }

  return JS_NewStringLen(ctx, out, nwords * 8);
}

/* process-wide cv:: settings that scripts need for reproducibility */
static JSValue
js_utility_system(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  int32_t n = 0;

  switch(magic) {
    case 0:
      if(argc < 1 || JS_ToInt32(ctx, &n, argv[0]))
        return JS_EXCEPTION;
      cv::setRNGSeed(n);
      return JS_UNDEFINED;
    case 1:
      if(argc < 1 || JS_ToInt32(ctx, &n, argv[0]))
        return JS_EXCEPTION;
      cv::setNumThreads(n);
      return JS_UNDEFINED;
    case 2:
      return JS_NewInt32(ctx, cv::getNumThreads());
    default: {
      std::string info = cv::getBuildInformation();

      return JS_NewStringLen(ctx, info.data(), info.size());
    }
  }
}

static const JSCFunctionListEntry js_hash_funcs[] = {
    JS_CFUNC_MAGIC_DEF("sha1", 1, js_hash_digest, 0),
    JS_CFUNC_MAGIC_DEF("sha256", 1, js_hash_digest, 1),
};

static const JSCFunctionListEntry js_utility_funcs[] = {
    JS_CFUNC_MAGIC_DEF("setRNGSeed", 1, js_utility_system, 0),
    JS_CFUNC_MAGIC_DEF("setNumThreads", 1, js_utility_system, 1),
    JS_CFUNC_MAGIC_DEF("getNumThreads", 0, js_utility_system, 2),
    JS_CFUNC_MAGIC_DEF("getBuildInformation", 0, js_utility_system, 3),
    JS_OBJECT_DEF("hash", js_hash_funcs, countof(js_hash_funcs), JS_PROP_C_W_E),
};

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

extern "C" int
js_utility_init(JSContext* ctx, JSModuleDef* m) {

  if(js_tick_meter_class_id == 0) {
    /* create the TickMeter class */
    JS_NewClassID(&js_tick_meter_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_tick_meter_class_id, &js_tick_meter_class);

    tick_meter_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, tick_meter_proto, js_tick_meter_proto_funcs, countof(js_tick_meter_proto_funcs));
    JS_SetClassProto(ctx, js_tick_meter_class_id, tick_meter_proto);

    tick_meter_class = JS_NewCFunction2(ctx, js_tick_meter_constructor, "TickMeter", 0, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, tick_meter_class, tick_meter_proto);
  }

  if(m) {
    JS_SetModuleExport(ctx, m, "TickMeter", tick_meter_class);
    JS_SetModuleExportList(ctx, m, js_utility_funcs, countof(js_utility_funcs));
  }

  return 0;
}

extern "C" void
js_utility_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "TickMeter");
  JS_AddModuleExportList(ctx, m, js_utility_funcs, countof(js_utility_funcs));
}

#ifdef JS_UTILITY_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_utility
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  if(!(m = JS_NewCModule(ctx, module_name, &js_utility_init)))
    return NULL;
  js_utility_export(ctx, m);
  return m;
}
}
