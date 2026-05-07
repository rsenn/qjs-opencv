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

using JSImage2BlobParamsData = cv::dnn::Image2BlobParams;

extern "C" {
thread_local JSValue imageblob2params_proto, imageblob2params_class;
thread_local JSClassID js_imageblob2params_class_id;
}

static JSValue
js_imageblob2params_wrap(JSContext* ctx, JSValueConst proto, JSImage2BlobParamsData* fn) {
  JSValue ret = JS_NewObjectProtoClass(ctx, proto, js_imageblob2params_class_id);
  JS_SetOpaque(ret, fn);
  return ret;
}

JSValue
js_imageblob2params_new(JSContext* ctx, JSValueConst proto) {
  JSImage2BlobParamsData* fn = js_allocate<JSImage2BlobParamsData>(ctx);

  new(fn) JSImage2BlobParamsData();

  return js_imageblob2params_wrap(ctx, proto, fn);
}

JSValue
js_imageblob2params_new(JSContext* ctx, JSValueConst proto, const JSImage2BlobParamsData& other) {
  JSImage2BlobParamsData* fn = js_allocate<JSImage2BlobParamsData>(ctx);

  new(fn) JSImage2BlobParamsData(other);

  return js_imageblob2params_wrap(ctx, proto, fn);
}

JSValue
js_imageblob2params_new(JSContext* ctx, const JSImage2BlobParamsData& fn) {
  return js_imageblob2params_new(ctx, imageblob2params_proto, fn);
}

static JSValue
js_imageblob2params_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSImage2BlobParamsData* dn;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(dn = js_allocate<JSImage2BlobParamsData>(ctx)))
    return JS_EXCEPTION;

  if(argc > 0) {
    cv::Scalar scalefactor, mean;
    cv::Size size;
    bool swapRB = false;
    int32_t ddepth = CV_32F, datalayout = cv::dnn::DNN_LAYOUT_NCHW, mode = cv::dnn::DNN_PMODE_NULL;
    cv::Scalar borderValue{0.0};

    js_value_to(ctx, argv[0], scalefactor);
    if(argc > 1)
      js_value_to(ctx, argv[1], size);
    if(argc > 2)
      js_value_to(ctx, argv[2], mean);
    if(argc > 3)
      js_value_to(ctx, argv[3], swapRB);
    if(argc > 4)
      js_value_to(ctx, argv[4], ddepth);
    if(argc > 5)
      js_value_to(ctx, argv[5], datalayout);
    if(argc > 6)
      js_value_to(ctx, argv[6], mode);
    if(argc > 7)
      js_value_to(ctx, argv[7], borderValue);

    new(dn) JSImage2BlobParamsData(scalefactor, size, mean, swapRB, ddepth, cv::dnn::DataLayout(datalayout), cv::dnn::ImagePaddingMode(mode), borderValue);
  } else {
    new(dn) JSImage2BlobParamsData();
  }

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_imageblob2params_class_id);
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

JSImage2BlobParamsData*
js_imageblob2params_data(JSValueConst val) {
  return static_cast<JSImage2BlobParamsData*>(JS_GetOpaque(val, js_imageblob2params_class_id));
}

JSImage2BlobParamsData*
js_imageblob2params_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSImage2BlobParamsData*>(JS_GetOpaque2(ctx, val, js_imageblob2params_class_id));
}

enum {
  IMAGEBLOB2PARAMS_BORDERVALUE,
  IMAGEBLOB2PARAMS_DATALAYOUT,
  IMAGEBLOB2PARAMS_DDEPTH,
  IMAGEBLOB2PARAMS_MEAN,
  IMAGEBLOB2PARAMS_PADDINGMODE,
  IMAGEBLOB2PARAMS_SCALEFACTOR,
  IMAGEBLOB2PARAMS_SIZE,
  IMAGEBLOB2PARAMS_SWAPRB,
};

static JSValue
js_imageblob2params_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSImage2BlobParamsData* fn;
  JSValue ret = JS_UNDEFINED;

  if(!(fn = js_imageblob2params_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case IMAGEBLOB2PARAMS_BORDERVALUE: {
      ret = js_value_from(ctx, fn->borderValue);
      break;
    }

    case IMAGEBLOB2PARAMS_DATALAYOUT: {
      ret = js_value_from(ctx, fn->datalayout);
      break;
    }

    case IMAGEBLOB2PARAMS_DDEPTH: {
      ret = js_value_from(ctx, fn->ddepth);
      break;
    }

    case IMAGEBLOB2PARAMS_MEAN: {
      ret = js_value_from(ctx, fn->mean);
      break;
    }

    case IMAGEBLOB2PARAMS_PADDINGMODE: {
      ret = js_value_from(ctx, fn->paddingmode);
      break;
    }

    case IMAGEBLOB2PARAMS_SCALEFACTOR: {

      ret = js_value_from(ctx, fn->scalefactor);

      break;
    }

    case IMAGEBLOB2PARAMS_SIZE: {
      ret = js_value_from(ctx, fn->size);

      break;
    }

    case IMAGEBLOB2PARAMS_SWAPRB: {
      ret = js_value_from(ctx, fn->swapRB);

      break;
    }
  }

  return ret;
}

static JSValue
js_imageblob2params_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSImage2BlobParamsData* fn;

  if(!(fn = js_imageblob2params_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case IMAGEBLOB2PARAMS_BORDERVALUE: {
      js_value_to(ctx, val, fn->borderValue);
      break;
    }

    case IMAGEBLOB2PARAMS_DATALAYOUT: {
      int32_t datalayout;
      js_value_to(ctx, val, datalayout);

      fn->datalayout = cv::dnn::DataLayout(datalayout);
      break;
    }

    case IMAGEBLOB2PARAMS_DDEPTH: {
      js_value_to(ctx, val, fn->ddepth);
      break;
    }

    case IMAGEBLOB2PARAMS_MEAN: {
      js_value_to(ctx, val, fn->mean);
      break;
    }

    case IMAGEBLOB2PARAMS_PADDINGMODE: {
      int32_t paddingmode;
      js_value_to(ctx, val, paddingmode);
      fn->paddingmode = cv::dnn::ImagePaddingMode(paddingmode);
      break;
    }

    case IMAGEBLOB2PARAMS_SCALEFACTOR: {
      js_value_to(ctx, val, fn->scalefactor);
      break;
    }

    case IMAGEBLOB2PARAMS_SIZE: {
      js_value_to(ctx, val, fn->size);
      break;
    }

    case IMAGEBLOB2PARAMS_SWAPRB: {
      js_value_to(ctx, val, fn->swapRB);
      break;
    }
  }

  return JS_UNDEFINED;
}

enum {
  IMAGEBLOB2PARAMS_BLOBRECTSTOIMAGERECTS,
  IMAGEBLOB2PARAMS_BLOBRECTTOIMAGERECT,
};

static JSValue
js_imageblob2params_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSImage2BlobParamsData* dn;
  JSValue ret = JS_UNDEFINED;

  if(!(dn = js_imageblob2params_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case IMAGEBLOB2PARAMS_BLOBRECTSTOIMAGERECTS: {
      std::vector<cv::Rect> rBlob, rImg;
      cv::Size size;

      js_value_to(ctx, argv[0], rBlob);
      js_value_to(ctx, argv[2], size);

      dn->blobRectsToImageRects(rBlob, rImg, size);

      js_array_copy(ctx, argv[1], rImg);

      break;
    }

    case IMAGEBLOB2PARAMS_BLOBRECTTOIMAGERECT: {
      cv::Rect rBlob;
      cv::Size size;

      js_value_to(ctx, argv[0], rBlob);
      js_value_to(ctx, argv[1], size);

      ret = js_value_from(ctx, dn->blobRectToImageRect(rBlob, size));
      break;
    }
  }

  return ret;
}

void
js_imageblob2params_finalizer(JSRuntime* rt, JSValue val) {
  JSImage2BlobParamsData* dn;
  /* Note: 'dn' can be NULL in case JS_SetOpaque() was not called */

  if((dn = js_imageblob2params_data(val))) {
    dn->~JSImage2BlobParamsData();

    js_deallocate(rt, dn);
  }
}

JSClassDef js_imageblob2params_class = {
    .class_name = "Image2BlobParams",
    .finalizer = js_imageblob2params_finalizer,
};

const JSCFunctionListEntry js_imageblob2params_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("blobRectsToImageRects", 3, js_imageblob2params_method, IMAGEBLOB2PARAMS_BLOBRECTSTOIMAGERECTS),
    JS_CFUNC_MAGIC_DEF("blobRectToImageRect", 2, js_imageblob2params_method, IMAGEBLOB2PARAMS_BLOBRECTTOIMAGERECT),
    JS_CGETSET_MAGIC_DEF("borderValue", js_imageblob2params_get, js_imageblob2params_set, IMAGEBLOB2PARAMS_BORDERVALUE),
    JS_CGETSET_MAGIC_DEF("datalayout", js_imageblob2params_get, js_imageblob2params_set, IMAGEBLOB2PARAMS_DATALAYOUT),
    JS_CGETSET_MAGIC_DEF("ddepth", js_imageblob2params_get, js_imageblob2params_set, IMAGEBLOB2PARAMS_DDEPTH),
    JS_CGETSET_MAGIC_DEF("mean", js_imageblob2params_get, js_imageblob2params_set, IMAGEBLOB2PARAMS_MEAN),
    JS_CGETSET_MAGIC_DEF("paddingmode", js_imageblob2params_get, js_imageblob2params_set, IMAGEBLOB2PARAMS_PADDINGMODE),
    JS_CGETSET_MAGIC_DEF("scalefactor", js_imageblob2params_get, js_imageblob2params_set, IMAGEBLOB2PARAMS_SCALEFACTOR),
    JS_CGETSET_MAGIC_DEF("size", js_imageblob2params_get, js_imageblob2params_set, IMAGEBLOB2PARAMS_SIZE),
    JS_CGETSET_MAGIC_DEF("swapRB", js_imageblob2params_get, js_imageblob2params_set, IMAGEBLOB2PARAMS_SWAPRB),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Image2BlobParams", JS_PROP_CONFIGURABLE),
};

enum {
  DNN_BLOBFROMIMAGE,
  DNN_BLOBFROMIMAGES,
  DNN_BLOBFROMIMAGESWITHPARAMS,
  DNN_BLOBFROMIMAGEWITHPARAMS,
  DNN_ENABLEMODELDIAGNOSTICS,
  DNN_GETAVAILABLEBACKENDS,
  DNN_GETAVAILABLETARGETS,
  DNN_GETLAYERFACTORYIMPL,
  DNN_GETLAYERFACTORYMUTEX,
  DNN_IMAGESFROMBLOB,
  DNN_NMSBOXES,
  DNN_NMSBOXESBATCHED,
  DNN_READNET,
  DNN_READNETFROMCAFFE,
  DNN_READNETFROMDARKNET,
  DNN_READNETFROMMODELOPTIMIZER,
  DNN_READNETFROMONNX,
  DNN_READNETFROMTENSORFLOW,
  DNN_READNETFROMTFLITE,
  DNN_READNETFROMTORCH,
  DNN_READTENSORFROMONNX,
  DNN_READTORCHBLOB,
  DNN_SHRINKCAFFEMODEL,
  DNN_SOFTNMSBOXES,
  DNN_WRITETEXTGRAPH,
};

static JSValue
js_dnn_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
    switch(magic) {
      case DNN_BLOBFROMIMAGE: {
        JSInputArray image = js_input_array(ctx, argv[0]);
        JSOutputArray blob;
        double scalefactor = 1.0;
        cv::Size size;
        cv::Scalar mean;
        bool swapRB = false, crop = false;
        int32_t ddepth = CV_32F;
        int argi = 1;
        bool blobArg = false;

        if(argc > argi && JS_IsObject(argv[argi])) {
          blob = js_cv_outputarray(ctx, argv[argi++]);
          blobArg = true;
        }

        if(argc > argi)
          js_value_to(ctx, argv[argi++], scalefactor);
        if(argc > argi)
          js_value_to(ctx, argv[argi++], size);
        if(argc > argi)
          js_value_to(ctx, argv[argi++], mean);
        if(argc > argi)
          js_value_to(ctx, argv[argi++], swapRB);
        if(argc > argi)
          js_value_to(ctx, argv[argi++], crop);
        if(argc > argi)
          js_value_to(ctx, argv[argi++], ddepth);

        if(blobArg) {
          cv::dnn::blobFromImage(image, blob, scalefactor, size, mean, swapRB, crop, ddepth);
        } else {
          /*cv::Mat m = cv::dnn::blobFromImage(image, scalefactor, size, mean, swapRB, crop, ddepth);
          ret = js_mat_wrap(ctx, m);*/

          ret = js_value_from(ctx, cv::dnn::blobFromImage(image, scalefactor, size, mean, swapRB, crop, ddepth));
        }

        break;
      }

      case DNN_BLOBFROMIMAGES: {
        break;
      }

      case DNN_BLOBFROMIMAGESWITHPARAMS: {
        break;
      }

      case DNN_BLOBFROMIMAGEWITHPARAMS: {
        break;
      }

      case DNN_ENABLEMODELDIAGNOSTICS: {
        break;
      }

      case DNN_GETAVAILABLEBACKENDS: {
        break;
      }

      case DNN_GETAVAILABLETARGETS: {
        break;
      }

      case DNN_GETLAYERFACTORYIMPL: {
        break;
      }

      case DNN_GETLAYERFACTORYMUTEX: {
        break;
      }

      case DNN_IMAGESFROMBLOB: {
        break;
      }

      case DNN_NMSBOXESBATCHED: {
        break;
      }

      case DNN_READNETFROMCAFFE: {
        break;
      }

      case DNN_READNETFROMDARKNET: {
        break;
      }

      case DNN_READNETFROMMODELOPTIMIZER: {
        break;
      }

      case DNN_READNETFROMONNX: {
        break;
      }

      case DNN_READNETFROMTENSORFLOW: {
        break;
      }

      case DNN_READNETFROMTFLITE: {
        break;
      }

      case DNN_READNETFROMTORCH: {
        break;
      }

      case DNN_READTENSORFROMONNX: {
        break;
      }

      case DNN_READTORCHBLOB: {
        break;
      }

      case DNN_SHRINKCAFFEMODEL: {
        break;
      }

      case DNN_SOFTNMSBOXES: {
        break;
      }

      case DNN_WRITETEXTGRAPH: {
        break;
      }

      case DNN_NMSBOXES: {
        std::vector<cv::Rect> bboxes;
        std::vector<float> scores;
        double score_threshold, nms_threshold, eta = 1.0;
        std::vector<int> indices;
        int32_t top_k = 0;

        js_array_to(ctx, argv[0], bboxes);
        js_array_to(ctx, argv[1], scores);

        js_value_to(ctx, argv[2], score_threshold);
        js_value_to(ctx, argv[3], nms_threshold);

        if(argc > 4)
          js_array_to(ctx, argv[4], indices);
        if(argc > 5)
          js_value_to(ctx, argv[5], eta);
        if(argc > 6)
          js_value_to(ctx, argv[6], top_k);

        cv::dnn::NMSBoxes(bboxes, scores, score_threshold, nms_threshold, indices, eta, top_k);
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

    JS_CFUNC_MAGIC_DEF("blobFromImage", 0, js_dnn_func, DNN_BLOBFROMIMAGE),
    JS_CFUNC_MAGIC_DEF("blobFromImages", 0, js_dnn_func, DNN_BLOBFROMIMAGES),
    JS_CFUNC_MAGIC_DEF("blobFromImagesWithParams", 0, js_dnn_func, DNN_BLOBFROMIMAGESWITHPARAMS),
    JS_CFUNC_MAGIC_DEF("blobFromImageWithParams", 0, js_dnn_func, DNN_BLOBFROMIMAGEWITHPARAMS),
    JS_CFUNC_MAGIC_DEF("enableModelDiagnostics", 0, js_dnn_func, DNN_ENABLEMODELDIAGNOSTICS),
    JS_CFUNC_MAGIC_DEF("getAvailableBackends", 0, js_dnn_func, DNN_GETAVAILABLEBACKENDS),
    JS_CFUNC_MAGIC_DEF("getAvailableTargets", 0, js_dnn_func, DNN_GETAVAILABLETARGETS),
    JS_CFUNC_MAGIC_DEF("getLayerFactoryImpl", 0, js_dnn_func, DNN_GETLAYERFACTORYIMPL),
    JS_CFUNC_MAGIC_DEF("getLayerFactoryMutex", 0, js_dnn_func, DNN_GETLAYERFACTORYMUTEX),
    JS_CFUNC_MAGIC_DEF("imagesFromBlob", 0, js_dnn_func, DNN_IMAGESFROMBLOB),
    JS_CFUNC_MAGIC_DEF("NMSBoxes", 0, js_dnn_func, DNN_NMSBOXES),
    JS_CFUNC_MAGIC_DEF("NMSBoxesBatched", 0, js_dnn_func, DNN_NMSBOXESBATCHED),
    JS_CFUNC_MAGIC_DEF("readNet", 0, js_dnn_func, DNN_READNET),
    JS_CFUNC_MAGIC_DEF("readNetFromCaffe", 0, js_dnn_func, DNN_READNETFROMCAFFE),
    JS_CFUNC_MAGIC_DEF("readNetFromDarknet", 0, js_dnn_func, DNN_READNETFROMDARKNET),
    JS_CFUNC_MAGIC_DEF("readNetFromModelOptimizer", 0, js_dnn_func, DNN_READNETFROMMODELOPTIMIZER),
    JS_CFUNC_MAGIC_DEF("readNetFromONNX", 0, js_dnn_func, DNN_READNETFROMONNX),
    JS_CFUNC_MAGIC_DEF("readNetFromTensorflow", 0, js_dnn_func, DNN_READNETFROMTENSORFLOW),
    JS_CFUNC_MAGIC_DEF("readNetFromTFLite", 0, js_dnn_func, DNN_READNETFROMTFLITE),
    JS_CFUNC_MAGIC_DEF("readNetFromTorch", 0, js_dnn_func, DNN_READNETFROMTORCH),
    JS_CFUNC_MAGIC_DEF("readTensorFromONNX", 0, js_dnn_func, DNN_READTENSORFROMONNX),
    JS_CFUNC_MAGIC_DEF("readTorchBlob", 0, js_dnn_func, DNN_READTORCHBLOB),
    JS_CFUNC_MAGIC_DEF("shrinkCaffeModel", 0, js_dnn_func, DNN_SHRINKCAFFEMODEL),
    JS_CFUNC_MAGIC_DEF("softNMSBoxes", 0, js_dnn_func, DNN_SOFTNMSBOXES),
    JS_CFUNC_MAGIC_DEF("writeTextGraph", 0, js_dnn_func, DNN_WRITETEXTGRAPH),

    JS_CFUNC_MAGIC_DEF("NMSBoxes", 5, js_dnn_func, DNN_NMSBOXES),
    JS_CFUNC_MAGIC_DEF("readNet", 1, js_dnn_func, DNN_READNET),
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

  /* create the Image2BlobParams class */
  JS_NewClassID(&js_imageblob2params_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_imageblob2params_class_id, &js_imageblob2params_class);

  imageblob2params_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, imageblob2params_proto, js_imageblob2params_proto_funcs, countof(js_imageblob2params_proto_funcs));
  JS_SetClassProto(ctx, js_imageblob2params_class_id, imageblob2params_proto);

  imageblob2params_class = JS_NewCFunction2(ctx, js_imageblob2params_constructor, "Image2BlobParams", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, imageblob2params_class, imageblob2params_proto);

  if(m) {
    JS_SetModuleExport(ctx, m, "dnn", dnn_object);

    JS_SetPropertyFunctionList(ctx, dnn_object, js_dnn_static_funcs, countof(js_dnn_static_funcs));

    JS_SetPropertyStr(ctx, dnn_object, "Net", net_class);
    JS_SetPropertyStr(ctx, dnn_object, "Image2BlobParams", imageblob2params_class);
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
