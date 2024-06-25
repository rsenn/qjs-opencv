#include "js_alloc.hpp"
#include "js_size.hpp"
#include "js_point.hpp"
#include "js_mat.hpp"
#include "js_umat.hpp"
#include <opencv2/xphoto/white_balance.hpp>

typedef cv::xphoto::WhiteBalancer JSWhiteBalancerClass;
typedef cv::Ptr<JSWhiteBalancerClass> JSWhiteBalancerData;

extern "C" {
thread_local JSValue white_balancer_proto = JS_UNDEFINED, white_balancer_class = JS_UNDEFINED;
thread_local JSClassID js_white_balancer_class_id;
}

extern "C" int js_white_balancer_init(JSContext*, JSModuleDef*);

static JSValue
js_white_balancer_wrap(JSContext* ctx, JSWhiteBalancerData& wb) {
  JSWhiteBalancerData* ptr;

  if(!(ptr = js_allocate<JSWhiteBalancerData>(ctx)))
    return JS_ThrowOutOfMemory(ctx);

  JSValue ret = JS_NewObjectProtoClass(ctx, white_balancer_proto, js_white_balancer_class_id);

  *ptr = wb;
  JS_SetOpaque(ret, ptr);

  return ret;
}

enum {
  WB_SIMPLE,
  WB_GRAYWORLD,
  WB_LEARNING,
  APPLY_CHANNEL_GAINS,
};

static JSValue
js_white_balancer_function(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSWhiteBalancerData wb;

  switch(magic) {

    case WB_SIMPLE: {
      wb = cv::xphoto::createSimpleWB();
      break;
    }

    case WB_GRAYWORLD: {
      wb = cv::xphoto::createGrayworldWB();
      break;
    }

    case WB_LEARNING: {
      const char* s = nullptr;

      if(argc > 0)
        s = JS_ToCString(ctx, argv[0]);

      wb = cv::xphoto::createLearningBasedWB(s ? s : cv::String());

      if(s)
        JS_FreeCString(ctx, s);
      break;
    }

    case APPLY_CHANNEL_GAINS: {
      JSInputArray src = js_input_array(ctx, argv[0]);
      JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);
      double gain_r = 1, gain_g = 1, gain_b = 1;

      JS_ToFloat64(ctx, &gain_b, argv[2]);
      JS_ToFloat64(ctx, &gain_g, argv[3]);
      JS_ToFloat64(ctx, &gain_r, argv[4]);

      cv::xphoto::applyChannelGains(src, dst, gain_b, gain_g, gain_r);
      return JS_UNDEFINED;
    }
  }

  return wb ? js_white_balancer_wrap(ctx, wb) : JS_NULL;
}

JSWhiteBalancerData*
js_white_balancer_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSWhiteBalancerData*>(JS_GetOpaque2(ctx, val, js_white_balancer_class_id));
}

static void
js_white_balancer_finalizer(JSRuntime* rt, JSValue val) {
  JSWhiteBalancerData* wb = static_cast<JSWhiteBalancerData*>(JS_GetOpaque(val, js_white_balancer_class_id));
  /* Note: 'wb' can be NULL in case JS_SetOpaque() was not called */

  (*wb)->~JSWhiteBalancerClass();

  js_deallocate<JSWhiteBalancerData>(rt, wb);
}

enum { BALANCE_WHITE = 0 };

static JSValue
js_white_balancer_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSWhiteBalancerData* wb = static_cast<JSWhiteBalancerData*>(JS_GetOpaque2(ctx, this_val, js_white_balancer_class_id));

  switch(magic) {
    case BALANCE_WHITE: {
      JSInputArray src = js_input_array(ctx, argv[0]);
      JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);

      (*wb)->balanceWhite(src, dst);
      break;
    }
  }

  return JS_UNDEFINED;
}

JSClassDef js_white_balancer_class = {
    .class_name = "WhiteBalancer",
    .finalizer = js_white_balancer_finalizer,
};

const JSCFunctionListEntry js_white_balancer_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("balanceWhite", 2, js_white_balancer_method, BALANCE_WHITE),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "WhiteBalancer", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_white_balancer_create_funcs[] = {
    JS_CFUNC_MAGIC_DEF("createSimpleWB", 0, js_white_balancer_function, WB_SIMPLE),
    JS_CFUNC_MAGIC_DEF("createGrayworldWB", 0, js_white_balancer_function, WB_GRAYWORLD),
    JS_CFUNC_MAGIC_DEF("createLearningBasedWB", 0, js_white_balancer_function, WB_LEARNING),
    JS_CFUNC_MAGIC_DEF("applyChannelGains", 0, js_white_balancer_function, APPLY_CHANNEL_GAINS),
};

extern "C" int
js_white_balancer_init(JSContext* ctx, JSModuleDef* m) {

  /* create the WhiteBalancer class */
  JS_NewClassID(&js_white_balancer_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_white_balancer_class_id, &js_white_balancer_class);

  white_balancer_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, white_balancer_proto, js_white_balancer_proto_funcs, countof(js_white_balancer_proto_funcs));
  JS_SetClassProto(ctx, js_white_balancer_class_id, white_balancer_proto);

  white_balancer_class = JS_NewObject(ctx);

  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, white_balancer_class, white_balancer_proto);

  if(m) {
    JS_SetModuleExport(ctx, m, "WhiteBalancer", white_balancer_class);
    JS_SetModuleExportList(ctx, m, js_white_balancer_create_funcs, countof(js_white_balancer_create_funcs));
  }

  return 0;
}

void
js_white_balancer_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(white_balancer_class))
    js_white_balancer_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "WhiteBalancer", white_balancer_class);
}

#ifdef JS_WhiteBalancer_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_white_balancer
#endif

extern "C" void
js_white_balancer_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "WhiteBalancer");
  JS_AddModuleExportList(ctx, m, js_white_balancer_create_funcs, countof(js_white_balancer_create_funcs));
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_white_balancer_init);

  if(!m)
    return NULL;

  js_white_balancer_export(ctx, m);
  return m;
}
