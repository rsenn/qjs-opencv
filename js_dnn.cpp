#include "js_cv.hpp"
#include "js_umat.hpp"
#include "include/jsbindings.hpp"
#include <quickjs.h>

#include <opencv2/dnn.hpp>

using JSNetData = cv::dnn::Net;

extern "C" {
thread_local JSValue dnn_object, net_proto, net_class;
thread_local JSClassID js_net_class_id;
}

static JSValue
js_net_wrap(JSContext* ctx, JSValueConst proto, JSNetData* fn) {
  JSValue ret = JS_NewObjectProtoClass(ctx, proto, js_net_class_id);
  JS_SetOpaque(ret, fn);
  return ret;
}

JSValue
js_net_new(JSContext* ctx, JSValueConst proto) {
  JSNetData* fn = js_allocate<JSNetData>(ctx);

  new(fn) JSNetData();

  return js_net_wrap(ctx, proto, fn);
}

JSValue
js_net_new(JSContext* ctx, JSValueConst proto, const JSNetData& other) {
  JSNetData* fn = js_allocate<JSNetData>(ctx);

  new(fn) JSNetData(other);

  return js_net_wrap(ctx, proto, fn);
}

JSValue
js_net_new(JSContext* ctx, const JSNetData& fn) {
  return js_net_new(ctx, net_proto, fn);
}

static JSValue
js_net_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSNetData* dn;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(dn = js_allocate<JSNetData>(ctx)))
    return JS_EXCEPTION;

  new(dn) JSNetData();

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_net_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, dn);

  return obj;

fail:
  js_deallocate(ctx, dn);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSNetData*
js_net_data(JSValueConst val) {
  return static_cast<JSNetData*>(JS_GetOpaque(val, js_net_class_id));
}

JSNetData*
js_net_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSNetData*>(JS_GetOpaque2(ctx, val, js_net_class_id));
}

enum {
  DNN_NET_FORWARD,
  DNN_NET_GETUNCONNECTEDOUTLAYERSNAMES,
  DNN_NET_SETINPUT,
  DNN_NET_SETPREFERABLEBACKEND,
  DNN_NET_SETPREFERABLETARGET,
};

static JSValue
js_net_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSNetData* dn;
  JSValue ret = JS_UNDEFINED;

  if(!(dn = js_net_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {}

  return ret;
}

void
js_net_finalizer(JSRuntime* rt, JSValue val) {
  JSNetData* dn;
  /* Note: 'dn' can be NULL in case JS_SetOpaque() was not called */

  if((dn = js_net_data(val))) {
    dn->~JSNetData();

    js_deallocate(rt, dn);
  }
}

JSClassDef js_net_class = {
    .class_name = "Net",
    .finalizer = js_net_finalizer,
};

const JSCFunctionListEntry js_net_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("forward", 0, js_net_method, DNN_NET_FORWARD),
    JS_CFUNC_MAGIC_DEF("getUnconnectedOutLayersNames", 0, js_net_method, DNN_NET_GETUNCONNECTEDOUTLAYERSNAMES),
    JS_CFUNC_MAGIC_DEF("setInput", 0, js_net_method, DNN_NET_SETINPUT),
    JS_CFUNC_MAGIC_DEF("setPreferableBackend", 0, js_net_method, DNN_NET_SETPREFERABLEBACKEND),
    JS_CFUNC_MAGIC_DEF("setPreferableTarget", 0, js_net_method, DNN_NET_SETPREFERABLETARGET),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Net", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_net_static_funcs[] = {};

enum {
  DNN_LAYOUT_NCHW,
  DNN_IMAGE2BLOBPARAMS,
  DNN_IMAGEPADDINGMODE,
  DNN_NET,
  DNN_NMSBOXES,
  DNN_READNET,
};

static JSValue
js_dnn_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
    switch(magic) {

      case DNN_LAYOUT_NCHW: {
        break;
      }
      case DNN_IMAGE2BLOBPARAMS: {
        break;
      }
      case DNN_IMAGEPADDINGMODE: {
        break;
      }
      case DNN_NET: {
        break;
      }
      case DNN_NMSBOXES: {
        break;
      }
      case DNN_READNET: {
        const char *model, *config, *framework;

        if(argc > 0)
          model = JS_ToCString(ctx, argv[0]);

        if(argc > 1)
          config = JS_ToCString(ctx, argv[1]);

        if(argc > 2)
          framework = JS_ToCString(ctx, argv[2]);

        JSNetData n = cv::dnn::readNet(model, config ? config : "", framework ? framework : "");

        ret = js_net_new(ctx, net_proto, n);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

const JSCFunctionListEntry js_dnn_dnn_funcs[] = {
    JS_PROP_INT32_DEF("DNN_LAYOUT_NCHW", cv::dnn::DNN_LAYOUT_NCHW, JS_PROP_ENUMERABLE),
    JS_CFUNC_MAGIC_DEF("Image2BlobParams", 0, js_dnn_func, DNN_IMAGE2BLOBPARAMS),
    JS_CFUNC_MAGIC_DEF("ImagePaddingMode", 0, js_dnn_func, DNN_IMAGEPADDINGMODE),
    //
    //      JS_CFUNC_MAGIC_DEF("Net", 0, js_dnn_func, DNN_NET),
    JS_CFUNC_MAGIC_DEF("NMSBoxes", 0, js_dnn_func, DNN_NMSBOXES),
    JS_CFUNC_MAGIC_DEF("readNet", 0, js_dnn_func, DNN_READNET),
};

const JSCFunctionListEntry js_dnn_static_funcs[] = {
    JS_OBJECT_DEF("dnn", js_dnn_dnn_funcs, countof(js_dnn_dnn_funcs), JS_PROP_C_W_E),
};

extern "C" int
js_dnn_init(JSContext* ctx, JSModuleDef* m) {
  dnn_object = JS_NewObjectProto(ctx, JS_NULL);

  /* create the Net class */
  JS_NewClassID(&js_net_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_net_class_id, &js_net_class);

  net_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, net_proto, js_net_proto_funcs, countof(js_net_proto_funcs));
  JS_SetClassProto(ctx, js_net_class_id, net_proto);

  net_class = JS_NewCFunction2(ctx, js_net_constructor, "Net", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, net_class, net_proto);
  JS_SetPropertyFunctionList(ctx, net_class, js_net_static_funcs, countof(js_net_static_funcs));

  // js_object_inspect(ctx, net_proto, js_net_inspect);

  if(m) {
    JS_SetModuleExport(ctx, m, "dnn", dnn_object);

    JS_SetPropertyFunctionList(ctx, dnn_object, js_dnn_static_funcs, countof(js_dnn_static_funcs));

    JS_SetPropertyStr(ctx, dnn_object, "Net", net_class);

    //    JS_SetModuleExportList(ctx, m, js_dnn_static_funcs, countof(js_dnn_static_funcs));
  }

  return 0;
}

extern "C" void
js_dnn_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "dnn");
  // JS_AddModuleExportList(ctx, m, js_dnn_static_funcs, countof(js_dnn_static_funcs));
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_dnn
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_dnn_init)))
    return NULL;

  js_dnn_export(ctx, m);
  return m;
}
