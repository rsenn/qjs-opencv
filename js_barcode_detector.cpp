#include "js_alloc.hpp"
#include "js_size.hpp"
#include "js_point.hpp"
#include "js_mat.hpp"
#include "js_umat.hpp"
#include <opencv2/barcode.hpp>

typedef cv::barcode::BarcodeDetector JSBarcodeDetectorClass;
typedef JSBarcodeDetectorClass JSBarcodeDetectorData;

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
static JSValue
js_barcode_detector_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSValue proto, obj = JS_UNDEFINED;
  JSBarcodeDetectorData* bd;
  const char *prototxt_path = nullptr, *model_path = nullptr;

  if(!(bd = js_allocate<cv::barcode::BarcodeDetector>(ctx)))
    return JS_EXCEPTION;

  if(argc > 0)
    prototxt_path = JS_ToCString(ctx, argv[0]);
  if(argc > 1)
    model_path = JS_ToCString(ctx, argv[1]);

  new(bd) cv::barcode::BarcodeDetector(prototxt_path ? prototxt_path : "", model_path ? model_path : "");

  if(prototxt_path)
    JS_FreeCString(ctx, prototxt_path);
  if(model_path)
    JS_FreeCString(ctx, model_path);

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_barcode_detector_class_id);

  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, bd);

  return obj;

fail:
  js_deallocate(ctx, bd);
fail2:
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSBarcodeDetectorData*
js_barcode_detector_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSBarcodeDetectorData*>(JS_GetOpaque2(ctx, val, js_barcode_detector_class_id));
}

static void
js_barcode_detector_finalizer(JSRuntime* rt, JSValue val) {
  JSBarcodeDetectorData* wb = static_cast<JSBarcodeDetectorData*>(JS_GetOpaque(val, js_barcode_detector_class_id));
  /* Note: 'wb' can be NULL in case JS_SetOpaque() was not called */

  wb->~JSBarcodeDetectorClass();

  js_deallocate<JSBarcodeDetectorData>(rt, wb);
}

enum {
  METHOD_DETECT,
  METHOD_DECODE,
  METHOD_DETECT_AND_DECODE,
};

static JSValue
js_barcode_detector_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSBarcodeDetectorData* wb = static_cast<JSBarcodeDetectorData*>(JS_GetOpaque2(ctx, this_val, js_barcode_detector_class_id));
  JSValue ret = JS_UNDEFINED;

  switch(magic) {

    case METHOD_DETECT: {
      JSInputArray img = js_input_array(ctx, argv[0]);
      JSOutputArray points = js_cv_outputarray(ctx, argv[1]);

      BOOL result = wb->detect(img, points);
      ret = JS_NewBool(ctx, result);
      break;
    }

    case METHOD_DECODE: {
      JSInputArray img = js_input_array(ctx, argv[0]);
      JSInputArray points = js_input_array(ctx, argv[1]);
      std::vector<std::string> decoded_info;
      std::vector<cv::barcode::BarcodeType> decoded_type;

      BOOL result = wb->decode(img, points, decoded_info, decoded_type);
      ret = JS_NewBool(ctx, result);

      js_array_copy(ctx, argv[2], decoded_info);
      js_array_copy(ctx, argv[3], decoded_type);

      break;
    }

    case METHOD_DETECT_AND_DECODE: {
      JSInputArray img = js_input_array(ctx, argv[0]);
      std::vector<std::string> decoded_info;
      std::vector<cv::barcode::BarcodeType> decoded_type;
      JSOutputArray points = js_cv_outputarray(ctx, argv[3]);

      BOOL result = wb->detectAndDecode(img, decoded_info, decoded_type, points);
      ret = JS_NewBool(ctx, result);

      js_array_copy(ctx, argv[1], decoded_info);
      js_array_copy(ctx, argv[2], decoded_type);

      break;
    }
  }

  return ret;
}

JSClassDef js_barcode_detector_class = {
    .class_name = "BarcodeDetector",
    .finalizer = js_barcode_detector_finalizer,
};

const JSCFunctionListEntry js_barcode_detector_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("detect", 2, js_barcode_detector_method, METHOD_DETECT),
    JS_CFUNC_MAGIC_DEF("decode", 4, js_barcode_detector_method, METHOD_DECODE),
    JS_CFUNC_MAGIC_DEF("detectAndDecode", 4, js_barcode_detector_method, METHOD_DETECT_AND_DECODE),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "BarcodeDetector", JS_PROP_CONFIGURABLE),
};

extern "C" int
js_barcode_detector_init(JSContext* ctx, JSModuleDef* bd) {

  /* create the BarcodeDetector class */
  JS_NewClassID(&js_barcode_detector_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_barcode_detector_class_id, &js_barcode_detector_class);

  barcode_detector_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, barcode_detector_proto, js_barcode_detector_proto_funcs, countof(js_barcode_detector_proto_funcs));
  JS_SetClassProto(ctx, js_barcode_detector_class_id, barcode_detector_proto);

  barcode_detector_class = JS_NewCFunction2(ctx, js_barcode_detector_constructor, "BarcodeDetector", 0, JS_CFUNC_constructor, 0);

  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, barcode_detector_class, barcode_detector_proto);

  if(bd) {
    JS_SetModuleExport(ctx, bd, "BarcodeDetector", barcode_detector_class);
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
js_barcode_detector_export(JSContext* ctx, JSModuleDef* bd) {
  JS_AddModuleExport(ctx, bd, "BarcodeDetector");
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* bd;
  bd = JS_NewCModule(ctx, module_name, &js_barcode_detector_init);

  if(!bd)
    return NULL;

  js_barcode_detector_export(ctx, bd);
  return bd;
}
