#include "js_alloc.hpp"
#include "js_size.hpp"
#include "js_point.hpp"
#include "js_mat.hpp"
#include "js_umat.hpp"
#include <opencv2/barcode.hpp>

typedef cv::barcode::BarcodeDetector JSBarcodeDetectorClass;
typedef cv::Ptr<JSBarcodeDetectorClass> JSBarcodeDetectorData;

JSValue barcode_detector_proto = JS_UNDEFINED, barcode_detector_class = JS_UNDEFINED;
JSClassID js_barcode_detector_class_id;

extern "C" int js_barcode_detector_init(JSContext*, JSModuleDef*);

static JSValue
js_barcode_detector_wrap(JSContext* ctx, JSBarcodeDetectorData& wb) {
  JSBarcodeDetectorData* ptr;

  if(!(ptr = js_allocate<JSBarcodeDetectorData>(ctx)))
    return JS_ThrowOutOfMemory(ctx);

  JSValue ret = JS_NewObjectProtoClass(ctx, barcode_detector_proto, js_barcode_detector_class_id);

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
js_barcode_detector_function(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSBarcodeDetectorData wb;

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

  return wb ? js_barcode_detector_wrap(ctx, wb) : JS_NULL;
}

JSBarcodeDetectorData*
js_barcode_detector_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSBarcodeDetectorData*>(JS_GetOpaque2(ctx, val, js_barcode_detector_class_id));
}

static void
js_barcode_detector_finalizer(JSRuntime* rt, JSValue val) {
  JSBarcodeDetectorData* wb = static_cast<JSBarcodeDetectorData*>(JS_GetOpaque(val, js_barcode_detector_class_id));
  /* Note: 'wb' can be NULL in case JS_SetOpaque() was not called */

  (*wb)->~JSBarcodeDetectorClass();

  js_deallocate<JSBarcodeDetectorData>(rt, wb);
}

enum { BALANCE_WHITE = 0 };

static JSValue
js_barcode_detector_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSBarcodeDetectorData* wb = static_cast<JSBarcodeDetectorData*>(JS_GetOpaque2(ctx, this_val, js_barcode_detector_class_id));

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

JSClassDef js_barcode_detector_class = {
    .class_name = "BarcodeDetector",
    .finalizer = js_barcode_detector_finalizer,
};

const JSCFunctionListEntry js_barcode_detector_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("balanceWhite", 2, js_barcode_detector_method, BALANCE_WHITE),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "BarcodeDetector", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_barcode_detector_create_funcs[] = {
    JS_CFUNC_MAGIC_DEF("createSimpleWB", 0, js_barcode_detector_function, WB_SIMPLE),
    JS_CFUNC_MAGIC_DEF("createGrayworldWB", 0, js_barcode_detector_function, WB_GRAYWORLD),
    JS_CFUNC_MAGIC_DEF("createLearningBasedWB", 0, js_barcode_detector_function, WB_LEARNING),
    JS_CFUNC_MAGIC_DEF("applyChannelGains", 0, js_barcode_detector_function, APPLY_CHANNEL_GAINS),
};

extern "C" int
js_barcode_detector_init(JSContext* ctx, JSModuleDef* m) {

  /* create the BarcodeDetector class */
  JS_NewClassID(&js_barcode_detector_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_barcode_detector_class_id, &js_barcode_detector_class);

  barcode_detector_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, barcode_detector_proto, js_barcode_detector_proto_funcs, countof(js_barcode_detector_proto_funcs));
  JS_SetClassProto(ctx, js_barcode_detector_class_id, barcode_detector_proto);

  barcode_detector_class = JS_NewObject(ctx);

  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, barcode_detector_class, barcode_detector_proto);

  if(m) {
    JS_SetModuleExport(ctx, m, "BarcodeDetector", barcode_detector_class);
    JS_SetModuleExportList(ctx, m, js_barcode_detector_create_funcs, countof(js_barcode_detector_create_funcs));
  }

  return 0;
}

void
js_barcode_detector_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(barcode_detector_class))
    js_barcode_detector_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "BarcodeDetector", barcode_detector_class);
}

#ifdef JS_BarcodeDetector_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_barcode_detector
#endif

extern "C" void
js_barcode_detector_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "BarcodeDetector");
  JS_AddModuleExportList(ctx, m, js_barcode_detector_create_funcs, countof(js_barcode_detector_create_funcs));
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_barcode_detector_init);

  if(!m)
    return NULL;

  js_barcode_detector_export(ctx, m);
  return m;
}
