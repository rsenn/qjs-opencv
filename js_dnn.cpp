#include "js_cv.hpp"
#include "js_umat.hpp"
#include "include/jsbindings.hpp"
#include <quickjs.h>

#include <opencv2/dnn.hpp>

using JSNetData = cv::dnn::Net;
using JSImage2BlobParamsData = cv::dnn::Image2BlobParams;

extern "C" {
thread_local JSValue dnn_object, net_proto, net_class, imageblob2params_proto, imageblob2params_class;
thread_local JSClassID js_net_class_id, js_imageblob2params_class_id;
}

static JSValue
js_net_wrap(JSContext* ctx, JSValueConst proto, JSNetData* ib2p) {
  JSValue ret = JS_NewObjectProtoClass(ctx, proto, js_net_class_id);
  JS_SetOpaque(ret, ib2p);
  return ret;
}

JSValue
js_net_new(JSContext* ctx, JSValueConst proto) {
  JSNetData* ib2p = js_allocate<JSNetData>(ctx);

  new(ib2p) JSNetData();

  return js_net_wrap(ctx, proto, ib2p);
}

JSValue
js_net_new(JSContext* ctx, JSValueConst proto, const JSNetData& other) {
  JSNetData* ib2p = js_allocate<JSNetData>(ctx);

  new(ib2p) JSNetData(other);

  return js_net_wrap(ctx, proto, ib2p);
}

JSValue
js_net_new(JSContext* ctx, const JSNetData& ib2p) {
  return js_net_new(ctx, net_proto, ib2p);
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
  DNN_NET_FORWARDALL,
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

  switch(magic) {
    case DNN_NET_FORWARD: {
      if(argc == 0 || JS_IsString(argv[0])) {
        cv::String name;

        if(argc > 0)
          js_value_to(ctx, argv[0], name);

        ret = js_mat_wrap(ctx, dn->forward(name));
      } else {
        std::vector<cv::Mat> vecOfBlobs;
        cv::String outputName;

        if(argc == 1) {
          dn->forward(vecOfBlobs);

          js_array_copy(ctx, argv[0], vecOfBlobs);
        } else if(JS_IsString(argv[1])) {
          js_value_to(ctx, argv[1], outputName);

          dn->forward(vecOfBlobs, outputName);

          js_array_copy(ctx, argv[0], vecOfBlobs);
        } else if(JS_IsObject(argv[1])) {
          std::vector<cv::String> outputNames;

          js_value_to(ctx, argv[1], outputNames);

          dn->forward(vecOfBlobs, outputNames);

          js_array_copy(ctx, argv[0], vecOfBlobs);
        }
      }

      break;
    }

    case DNN_NET_FORWARDALL: {
      std::vector<cv::String> outputNames;
      std::vector<std::vector<cv::Mat>> vecOfVecOfBlobs;

      js_value_to(ctx, argv[1], outputNames);

      dn->forward(vecOfVecOfBlobs, outputNames);

      js_array_copy(ctx, argv[0], vecOfVecOfBlobs);
      break;
    }

    case DNN_NET_GETUNCONNECTEDOUTLAYERSNAMES: {
      std::vector<std::string> names = dn->getUnconnectedOutLayersNames();

      ret = js_value_from(ctx, names);
      break;
    }

    case DNN_NET_SETINPUT: {
      JSInputArray blob;
      cv::String name;
      double scalefactor = 1.0;
      cv::Scalar mean;

      blob = js_input_array(ctx, argv[0]);

      if(argc > 1)
        js_value_to(ctx, argv[1], name);
      if(argc > 2)
        js_value_to(ctx, argv[2], scalefactor);
      if(argc > 3)
        js_scalar_read(ctx, argv[3], mean);

      dn->setInput(blob, name, scalefactor, mean);
      break;
    }

    case DNN_NET_SETPREFERABLEBACKEND: {
      int32_t backendId = -1;
      js_value_to(ctx, argv[0], backendId);

      dn->setPreferableBackend(backendId);
      break;
    }

    case DNN_NET_SETPREFERABLETARGET: {
      int32_t targetId = -1;
      js_value_to(ctx, argv[0], targetId);

      dn->setPreferableTarget(targetId);

      break;
    }
  }

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
    JS_CFUNC_MAGIC_DEF("forwardAll", 1, js_net_method, DNN_NET_FORWARDALL),
    JS_CFUNC_MAGIC_DEF("getUnconnectedOutLayersNames", 0, js_net_method, DNN_NET_GETUNCONNECTEDOUTLAYERSNAMES),
    JS_CFUNC_MAGIC_DEF("setInput", 0, js_net_method, DNN_NET_SETINPUT),
    JS_CFUNC_MAGIC_DEF("setPreferableBackend", 0, js_net_method, DNN_NET_SETPREFERABLEBACKEND),
    JS_CFUNC_MAGIC_DEF("setPreferableTarget", 0, js_net_method, DNN_NET_SETPREFERABLETARGET),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Net", JS_PROP_CONFIGURABLE),
};

static JSValue
js_imageblob2params_wrap(JSContext* ctx, JSValueConst proto, JSImage2BlobParamsData* ib2p) {
  JSValue ret = JS_NewObjectProtoClass(ctx, proto, js_imageblob2params_class_id);
  JS_SetOpaque(ret, ib2p);
  return ret;
}

JSValue
js_imageblob2params_new(JSContext* ctx, JSValueConst proto) {
  JSImage2BlobParamsData* ib2p = js_allocate<JSImage2BlobParamsData>(ctx);

  new(ib2p) JSImage2BlobParamsData();

  return js_imageblob2params_wrap(ctx, proto, ib2p);
}

JSValue
js_imageblob2params_new(JSContext* ctx, JSValueConst proto, const JSImage2BlobParamsData& other) {
  JSImage2BlobParamsData* ib2p = js_allocate<JSImage2BlobParamsData>(ctx);

  new(ib2p) JSImage2BlobParamsData(other);

  return js_imageblob2params_wrap(ctx, proto, ib2p);
}

JSValue
js_imageblob2params_new(JSContext* ctx, const JSImage2BlobParamsData& ib2p) {
  return js_imageblob2params_new(ctx, imageblob2params_proto, ib2p);
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

    js_scalar_read(ctx, argv[0], scalefactor);
    if(argc > 1)
      js_value_to(ctx, argv[1], size);
    if(argc > 2)
      js_scalar_read(ctx, argv[2], mean);
    if(argc > 3)
      js_value_to(ctx, argv[3], swapRB);
    if(argc > 4)
      js_value_to(ctx, argv[4], ddepth);
    if(argc > 5)
      js_value_to(ctx, argv[5], datalayout);
    if(argc > 6)
      js_value_to(ctx, argv[6], mode);
    if(argc > 7)
      js_scalar_read(ctx, argv[7], borderValue);

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
  JSImage2BlobParamsData* ib2p;
  JSValue ret = JS_UNDEFINED;

  if(!(ib2p = js_imageblob2params_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case IMAGEBLOB2PARAMS_BORDERVALUE: {
      ret = js_value_from(ctx, ib2p->borderValue);
      break;
    }

    case IMAGEBLOB2PARAMS_DATALAYOUT: {
      ret = js_value_from(ctx, ib2p->datalayout);
      break;
    }

    case IMAGEBLOB2PARAMS_DDEPTH: {
      ret = js_value_from(ctx, ib2p->ddepth);
      break;
    }

    case IMAGEBLOB2PARAMS_MEAN: {
      ret = js_value_from(ctx, ib2p->mean);
      break;
    }

    case IMAGEBLOB2PARAMS_PADDINGMODE: {
      ret = js_value_from(ctx, ib2p->paddingmode);
      break;
    }

    case IMAGEBLOB2PARAMS_SCALEFACTOR: {

      ret = js_value_from(ctx, ib2p->scalefactor);

      break;
    }

    case IMAGEBLOB2PARAMS_SIZE: {
      ret = js_value_from(ctx, ib2p->size);

      break;
    }

    case IMAGEBLOB2PARAMS_SWAPRB: {
      ret = js_value_from(ctx, ib2p->swapRB);

      break;
    }
  }

  return ret;
}

static JSValue
js_imageblob2params_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSImage2BlobParamsData* ib2p;

  if(!(ib2p = js_imageblob2params_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case IMAGEBLOB2PARAMS_BORDERVALUE: {
      js_value_to(ctx, val, ib2p->borderValue);
      break;
    }

    case IMAGEBLOB2PARAMS_DATALAYOUT: {
      int32_t datalayout;
      js_value_to(ctx, val, datalayout);

      ib2p->datalayout = cv::dnn::DataLayout(datalayout);
      break;
    }

    case IMAGEBLOB2PARAMS_DDEPTH: {
      js_value_to(ctx, val, ib2p->ddepth);
      break;
    }

    case IMAGEBLOB2PARAMS_MEAN: {
      js_scalar_read(ctx, val, ib2p->mean);
      break;
    }

    case IMAGEBLOB2PARAMS_PADDINGMODE: {
      int32_t paddingmode;
      js_value_to(ctx, val, paddingmode);
      ib2p->paddingmode = cv::dnn::ImagePaddingMode(paddingmode);
      break;
    }

    case IMAGEBLOB2PARAMS_SCALEFACTOR: {
      js_scalar_read(ctx, val, ib2p->scalefactor);
      break;
    }

    case IMAGEBLOB2PARAMS_SIZE: {
      js_value_to(ctx, val, ib2p->size);
      break;
    }

    case IMAGEBLOB2PARAMS_SWAPRB: {
      js_value_to(ctx, val, ib2p->swapRB);
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
  JSImage2BlobParamsData* ib2p;
  /* Note: 'ib2p' can be NULL in case JS_SetOpaque() was not called */

  if((ib2p = js_imageblob2params_data(val))) {
    ib2p->~JSImage2BlobParamsData();

    js_deallocate(rt, ib2p);
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
  /*DNN_GETLAYERFACTORYIMPL,
    DNN_GETLAYERFACTORYMUTEX,*/
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
          js_scalar_read(ctx, argv[argi++], mean);
        if(argc > argi)
          js_value_to(ctx, argv[argi++], swapRB);
        if(argc > argi)
          js_value_to(ctx, argv[argi++], crop);
        if(argc > argi)
          js_value_to(ctx, argv[argi++], ddepth);

        if(blobArg)
          cv::dnn::blobFromImage(image, blob, scalefactor, size, mean, swapRB, crop, ddepth);
        else
          ret = js_value_from(ctx, cv::dnn::blobFromImage(image, scalefactor, size, mean, swapRB, crop, ddepth));

        break;
      }

      case DNN_BLOBFROMIMAGES: {
        JSInputArray images = js_input_array(ctx, argv[0]);
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
          js_scalar_read(ctx, argv[argi++], mean);
        if(argc > argi)
          js_value_to(ctx, argv[argi++], swapRB);
        if(argc > argi)
          js_value_to(ctx, argv[argi++], crop);
        if(argc > argi)
          js_value_to(ctx, argv[argi++], ddepth);

        if(blobArg)
          cv::dnn::blobFromImages(images, blob, scalefactor, size, mean, swapRB, crop, ddepth);
        else
          ret = js_value_from(ctx, cv::dnn::blobFromImages(images, scalefactor, size, mean, swapRB, crop, ddepth));

        break;
      }

      case DNN_BLOBFROMIMAGESWITHPARAMS: {
        JSInputArray images = js_input_array(ctx, argv[0]);
        JSOutputArray blob;
        cv::dnn::Image2BlobParams* i2bp;
        bool blobArg = false;
        int argi = 1;

        if(argc > argi) {
          if(JS_IsObject(argv[argi]) && !(i2bp = js_imageblob2params_data(argv[argi]))) {
            blob = js_cv_outputarray(ctx, argv[argi++]);
            blobArg = true;
          }
        }

        if(argc > argi)
          if((i2bp = js_imageblob2params_data(argv[argi])))
            argi++;

        if(blobArg)
          cv::dnn::blobFromImagesWithParams(images, blob, i2bp ? *i2bp : cv::dnn::Image2BlobParams());
        else
          ret = js_value_from(ctx, cv::dnn::blobFromImagesWithParams(images, i2bp ? *i2bp : cv::dnn::Image2BlobParams()));

        break;
      }

      case DNN_BLOBFROMIMAGEWITHPARAMS: {
        JSInputArray image = js_input_array(ctx, argv[0]);
        JSOutputArray blob;
        cv::dnn::Image2BlobParams* i2bp;
        bool blobArg = false;
        int argi = 1;

        if(argc > argi) {
          if(JS_IsObject(argv[argi]) && !(i2bp = js_imageblob2params_data(argv[argi]))) {
            blob = js_cv_outputarray(ctx, argv[argi++]);
            blobArg = true;
          }
        }

        if(argc > argi)
          if((i2bp = js_imageblob2params_data(argv[argi])))
            argi++;

        if(blobArg)
          cv::dnn::blobFromImageWithParams(image, blob, i2bp ? *i2bp : cv::dnn::Image2BlobParams());
        else
          ret = js_value_from(ctx, cv::dnn::blobFromImageWithParams(image, i2bp ? *i2bp : cv::dnn::Image2BlobParams()));

        break;
      }

      case DNN_ENABLEMODELDIAGNOSTICS: {
        bool isDiagnosticsMode = false;

        if(argc > 0)
          js_value_to(ctx, argv[0], isDiagnosticsMode);

        cv::dnn::enableModelDiagnostics(isDiagnosticsMode);
        break;
      }

      case DNN_GETAVAILABLEBACKENDS: {
        auto backends = cv::dnn::getAvailableBackends();

        typedef std::vector<int32_t> int_pair_t;
        std::vector<int_pair_t> out;
        out.resize(backends.size());

        std::transform(backends.begin(), backends.end(), out.begin(), [](const std::pair<cv::dnn::Backend, cv::dnn::Target>& arg) -> int_pair_t {
          int_pair_t r{int32_t(arg.first), int32_t(arg.second)};
          return r;
        });

        ret = js_value_from(ctx, out);
        break;
      }

      case DNN_GETAVAILABLETARGETS: {
        int32_t backend = -1;
        js_value_to(ctx, argv[0], backend);

        auto targets = cv::dnn::getAvailableTargets(cv::dnn::Backend(backend));
        std::vector<int32_t> out;
        out.resize(targets.size());

        std::transform(targets.begin(), targets.end(), out.begin(), [](const cv::dnn::Target& arg) -> int32_t { return int32_t(arg); });

        ret = js_value_from(ctx, out);
        break;
      }

        /*case DNN_GETLAYERFACTORYIMPL: {
          break;
        }

        case DNN_GETLAYERFACTORYMUTEX: {
          break;
          }*/

      case DNN_IMAGESFROMBLOB: {
        JSInputArray blob = js_input_array(ctx, argv[0]);
        JSOutputArray images = js_cv_outputarray(ctx, argv[1]);

        cv::dnn::imagesFromBlob(blob.getMat(), images);
        break;
      }

      case DNN_NMSBOXESBATCHED: {
        std::vector<cv::Rect> bboxes;
        std::vector<float> scores;
        double score_threshold, nms_threshold, eta = 1.0;
        std::vector<int> class_ids, indices;
        int32_t top_k = 0;

        js_value_to(ctx, argv[0], bboxes);
        js_value_to(ctx, argv[1], scores);
        js_value_to(ctx, argv[2], class_ids);
        js_value_to(ctx, argv[3], score_threshold);
        js_value_to(ctx, argv[4], nms_threshold);

        // js_value_to(ctx, argv[5], indices);

        if(argc > 6)
          js_value_to(ctx, argv[6], eta);
        if(argc > 7)
          js_value_to(ctx, argv[7], top_k);

        cv::dnn::NMSBoxesBatched(bboxes, scores, class_ids, score_threshold, nms_threshold, indices, eta, top_k);

        js_array_copy(ctx, argv[5], indices);
        break;
      }

      case DNN_READNETFROMCAFFE: {
        int argi = 0;

        if(js_is_arraybuffer(ctx, argv[argi])) {
          size_t size, size2;
          uint8_t* ptr = JS_GetArrayBuffer(ctx, &size2, argv[argi++]);

          if(argc > argi && JS_IsNumber(argv[argi])) {
            js_value_to(ctx, argv[argi++], size);

            if(size > size2)
              size = size2;
          } else
            size = size2;

          size_t msize, msize2;
          uint8_t* mptr;

          if(argc > argi) {
            mptr = JS_GetArrayBuffer(ctx, &msize2, argv[argi++]);

            if(argc > argi && JS_IsNumber(argv[argi])) {
              js_value_to(ctx, argv[argi++], msize);

              if(msize > msize2)
                msize = msize2;
            } else
              msize = msize2;
          }

          ret = js_net_new(ctx, cv::dnn::readNetFromCaffe(reinterpret_cast<const char*>(ptr), size, reinterpret_cast<const char*>(mptr), msize));
        } else {
          std::string prototxt, caffeeModel;

          js_value_to(ctx, argv[0], prototxt);
          js_value_to(ctx, argv[1], caffeeModel);

          ret = js_net_new(ctx, cv::dnn::readNetFromCaffe(prototxt, caffeeModel));
        }

        break;
      }

      case DNN_READNETFROMDARKNET: {
        int argi = 0;

        if(js_is_arraybuffer(ctx, argv[argi])) {
          size_t size, size2;
          uint8_t* ptr = JS_GetArrayBuffer(ctx, &size2, argv[argi++]);

          if(argc > argi && JS_IsNumber(argv[argi])) {
            js_value_to(ctx, argv[argi++], size);

            if(size > size2)
              size = size2;
          } else
            size = size2;

          size_t msize, msize2;
          uint8_t* mptr;

          if(argc > argi) {
            mptr = JS_GetArrayBuffer(ctx, &msize2, argv[argi++]);

            if(argc > argi && JS_IsNumber(argv[argi])) {
              js_value_to(ctx, argv[argi++], msize);

              if(msize > msize2)
                msize = msize2;
            } else
              msize = msize2;
          }

          ret = js_net_new(ctx, cv::dnn::readNetFromDarknet(reinterpret_cast<const char*>(ptr), size, reinterpret_cast<const char*>(mptr), msize));
        } else {
          std::string prototxt, darknetModel;

          js_value_to(ctx, argv[0], prototxt);
          js_value_to(ctx, argv[1], darknetModel);

          ret = js_net_new(ctx, cv::dnn::readNetFromDarknet(prototxt, darknetModel));
        }

        break;
      }

      case DNN_READNETFROMMODELOPTIMIZER: {
        int argi = 0;

        if(js_is_arraybuffer(ctx, argv[argi])) {
          size_t size, size2;
          uint8_t* ptr = JS_GetArrayBuffer(ctx, &size2, argv[argi++]);

          if(argc > argi && JS_IsNumber(argv[argi])) {
            js_value_to(ctx, argv[argi++], size);

            if(size > size2)
              size = size2;
          } else
            size = size2;

          size_t msize, msize2;
          uint8_t* mptr;

          if(argc > argi) {
            mptr = JS_GetArrayBuffer(ctx, &msize2, argv[argi++]);

            if(argc > argi && JS_IsNumber(argv[argi])) {
              js_value_to(ctx, argv[argi++], msize);

              if(msize > msize2)
                msize = msize2;
            } else
              msize = msize2;
          }

          ret = js_net_new(ctx, cv::dnn::readNetFromModelOptimizer(reinterpret_cast<const uchar*>(ptr), size, reinterpret_cast<const uchar*>(mptr), msize));
        } else {
          std::string prototxt, modeloptimizerModel;

          js_value_to(ctx, argv[0], prototxt);
          js_value_to(ctx, argv[1], modeloptimizerModel);

          ret = js_net_new(ctx, cv::dnn::readNetFromModelOptimizer(prototxt, modeloptimizerModel));
        }

        break;
      }

      case DNN_READNETFROMONNX: {
        int argi = 0;

        if(js_is_arraybuffer(ctx, argv[argi])) {
          size_t size, size2;
          uint8_t* ptr = JS_GetArrayBuffer(ctx, &size2, argv[argi++]);

          if(argc > argi && JS_IsNumber(argv[argi])) {
            js_value_to(ctx, argv[argi++], size);

            if(size > size2)
              size = size2;
          } else
            size = size2;

          ret = js_net_new(ctx, cv::dnn::readNetFromONNX(reinterpret_cast<const char*>(ptr), size));
        } else {
          std::string onnxFile;

          js_value_to(ctx, argv[0], onnxFile);

          ret = js_net_new(ctx, cv::dnn::readNetFromONNX(onnxFile));
        }

        break;
      }

      case DNN_READNETFROMTENSORFLOW: {
        int argi = 0;

        if(js_is_arraybuffer(ctx, argv[argi])) {
          size_t size, size2;
          uint8_t* ptr = JS_GetArrayBuffer(ctx, &size2, argv[argi++]);

          if(argc > argi && JS_IsNumber(argv[argi])) {
            js_value_to(ctx, argv[argi++], size);

            if(size > size2)
              size = size2;
          } else
            size = size2;

          size_t msize, msize2;
          uint8_t* mptr;

          if(argc > argi) {
            mptr = JS_GetArrayBuffer(ctx, &msize2, argv[argi++]);

            if(argc > argi && JS_IsNumber(argv[argi])) {
              js_value_to(ctx, argv[argi++], msize);

              if(msize > msize2)
                msize = msize2;
            } else
              msize = msize2;
          }

          ret = js_net_new(ctx, cv::dnn::readNetFromTensorflow(reinterpret_cast<const char*>(ptr), size, reinterpret_cast<const char*>(mptr), msize));
        } else {
          std::string prototxt, tensorflowModel;

          js_value_to(ctx, argv[0], prototxt);
          js_value_to(ctx, argv[1], tensorflowModel);

          ret = js_net_new(ctx, cv::dnn::readNetFromTensorflow(prototxt, tensorflowModel));
        }

        break;
      }

      case DNN_READNETFROMTFLITE: {
        int argi = 0;

        if(js_is_arraybuffer(ctx, argv[argi])) {
          size_t size, size2;
          uint8_t* ptr = JS_GetArrayBuffer(ctx, &size2, argv[argi++]);

          if(argc > argi && JS_IsNumber(argv[argi])) {
            js_value_to(ctx, argv[argi++], size);

            if(size > size2)
              size = size2;
          } else
            size = size2;

          ret = js_net_new(ctx, cv::dnn::readNetFromTFLite(reinterpret_cast<const char*>(ptr), size));
        } else {
          std::string model;

          js_value_to(ctx, argv[0], model);

          ret = js_net_new(ctx, cv::dnn::readNetFromTFLite(model));
        }

        break;
      }

      case DNN_READNETFROMTORCH: {
        std::string model;
        bool isBinary = true, evaluate = true;

        js_value_to(ctx, argv[0], model);

        if(argc > 1)
          js_value_to(ctx, argv[1], isBinary);
        if(argc > 2)
          js_value_to(ctx, argv[2], evaluate);

        ret = js_net_new(ctx, cv::dnn::readNetFromTorch(model, isBinary, evaluate));
        break;
      }

      case DNN_READTENSORFROMONNX: {
        std::string path;

        js_value_to(ctx, argv[0], path);

        ret = js_value_from(ctx, cv::dnn::readTensorFromONNX(path));
        break;
      }

      case DNN_READTORCHBLOB: {
        std::string filename;
        bool isBinary = true;

        js_value_to(ctx, argv[0], filename);

        if(argc > 1)
          js_value_to(ctx, argv[1], isBinary);

        ret = js_value_from(ctx, cv::dnn::readTorchBlob(filename, isBinary));
        break;
      }

      case DNN_SHRINKCAFFEMODEL: {
        std::string src, dst;
        std::vector<cv::String> layersTypes;

        js_value_to(ctx, argv[0], src);
        js_value_to(ctx, argv[1], dst);
        if(argc > 2)
          js_value_to(ctx, argv[2], layersTypes);

        cv::dnn::shrinkCaffeModel(src, dst, layersTypes);

        break;
      }

      case DNN_SOFTNMSBOXES: {
        std::vector<cv::Rect> bboxes;
        std::vector<float> scores, updated_scores;
        double score_threshold, nms_threshold, sigma = 0.5;
        std::vector<int> indices;
        int32_t method = int32_t(cv::dnn::SoftNMSMethod::SOFTNMS_GAUSSIAN);
        size_t top_k = 0;

        js_array_to(ctx, argv[0], bboxes);
        js_array_to(ctx, argv[1], scores);

        js_value_to(ctx, argv[3], score_threshold);
        js_value_to(ctx, argv[4], nms_threshold);

        if(argc > 5)
          js_array_to(ctx, argv[5], indices);
        if(argc > 6)
          js_value_to(ctx, argv[6], top_k);
        if(argc > 7)
          js_value_to(ctx, argv[7], sigma);
        if(argc > 8)
          js_value_to(ctx, argv[8], method);

        cv::dnn::softNMSBoxes(bboxes, scores, updated_scores, score_threshold, nms_threshold, indices, top_k, sigma, cv::dnn::SoftNMSMethod(method));

        js_array_copy(ctx, argv[2], updated_scores);
        break;
      }

      case DNN_WRITETEXTGRAPH: {
        std::string model, output;

        js_value_to(ctx, argv[0], model);
        js_value_to(ctx, argv[1], output);

        cv::dnn::writeTextGraph(model, output);
        break;
      }

      case DNN_NMSBOXES: {
        std::vector<cv::Rect> bboxes;
        std::vector<float> scores;
        double score_threshold, nms_threshold, eta = 1.0;
        std::vector<int> indices;
        int32_t top_k = 0;

        js_value_to(ctx, argv[0], bboxes);
        js_value_to(ctx, argv[1], scores);
        js_value_to(ctx, argv[2], score_threshold);
        js_value_to(ctx, argv[3], nms_threshold);
        // js_value_to(ctx, argv[4], indices);

        if(argc > 5)
          js_value_to(ctx, argv[5], eta);
        if(argc > 6)
          js_value_to(ctx, argv[6], top_k);

        cv::dnn::NMSBoxes(bboxes, scores, score_threshold, nms_threshold, indices, eta, top_k);

        js_array_copy(ctx, argv[4], indices);
        break;
      }

      case DNN_READNET: {
        const char *model = 0, *config = 0, *framework = 0;

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
    JS_PROP_INT32_DEF("DNN_BACKEND_DEFAULT", cv::dnn::DNN_BACKEND_DEFAULT, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_BACKEND_HALIDE", cv::dnn::DNN_BACKEND_HALIDE, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_BACKEND_INFERENCE_ENGINE", cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_BACKEND_OPENCV", cv::dnn::DNN_BACKEND_OPENCV, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_BACKEND_VKCOM", cv::dnn::DNN_BACKEND_VKCOM, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_BACKEND_CUDA", cv::dnn::DNN_BACKEND_CUDA, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_BACKEND_WEBNN", cv::dnn::DNN_BACKEND_WEBNN, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_BACKEND_TIMVX", cv::dnn::DNN_BACKEND_TIMVX, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_BACKEND_CANN", cv::dnn::DNN_BACKEND_CANN, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_TARGET_CPU", cv::dnn::DNN_TARGET_CPU, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_TARGET_OPENCL", cv::dnn::DNN_TARGET_OPENCL, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_TARGET_OPENCL_FP16", cv::dnn::DNN_TARGET_OPENCL_FP16, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_TARGET_MYRIAD", cv::dnn::DNN_TARGET_MYRIAD, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_TARGET_VULKAN", cv::dnn::DNN_TARGET_VULKAN, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_TARGET_FPGA", cv::dnn::DNN_TARGET_FPGA, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_TARGET_CUDA", cv::dnn::DNN_TARGET_CUDA, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_TARGET_CUDA_FP16", cv::dnn::DNN_TARGET_CUDA_FP16, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_TARGET_HDDL", cv::dnn::DNN_TARGET_HDDL, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_TARGET_NPU", cv::dnn::DNN_TARGET_NPU, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_TARGET_CPU_FP16", cv::dnn::DNN_TARGET_CPU_FP16, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_UNKNOWN", cv::dnn::DNN_LAYOUT_UNKNOWN, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_ND", cv::dnn::DNN_LAYOUT_ND, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_NCHW", cv::dnn::DNN_LAYOUT_NCHW, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_NCDHW", cv::dnn::DNN_LAYOUT_NCDHW, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_NHWC", cv::dnn::DNN_LAYOUT_NHWC, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_NDHWC", cv::dnn::DNN_LAYOUT_NDHWC, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_PLANAR", cv::dnn::DNN_LAYOUT_PLANAR, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_PMODE_NULL", cv::dnn::DNN_PMODE_NULL, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_PMODE_CROP_CENTER", cv::dnn::DNN_PMODE_CROP_CENTER, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_PMODE_LETTERBOX", cv::dnn::DNN_PMODE_LETTERBOX, JS_PROP_ENUMERABLE),

    JS_CFUNC_MAGIC_DEF("blobFromImage", 1, js_dnn_func, DNN_BLOBFROMIMAGE),
    JS_CFUNC_MAGIC_DEF("blobFromImages", 1, js_dnn_func, DNN_BLOBFROMIMAGES),
    JS_CFUNC_MAGIC_DEF("blobFromImagesWithParams", 1, js_dnn_func, DNN_BLOBFROMIMAGESWITHPARAMS),
    JS_CFUNC_MAGIC_DEF("blobFromImageWithParams", 1, js_dnn_func, DNN_BLOBFROMIMAGEWITHPARAMS),
    JS_CFUNC_MAGIC_DEF("enableModelDiagnostics", 1, js_dnn_func, DNN_ENABLEMODELDIAGNOSTICS),
    /*JS_CFUNC_MAGIC_DEF("getAvailableBackends", 0, js_dnn_func, DNN_GETAVAILABLEBACKENDS),
      JS_CFUNC_MAGIC_DEF("getAvailableTargets", 0, js_dnn_func, DNN_GETAVAILABLETARGETS),
      JS_CFUNC_MAGIC_DEF("getLayerFactoryImpl", 0, js_dnn_func, DNN_GETLAYERFACTORYIMPL),
      JS_CFUNC_MAGIC_DEF("getLayerFactoryMutex", 0, js_dnn_func, DNN_GETLAYERFACTORYMUTEX),*/
    JS_CFUNC_MAGIC_DEF("imagesFromBlob", 2, js_dnn_func, DNN_IMAGESFROMBLOB),
    JS_CFUNC_MAGIC_DEF("NMSBoxes", 5, js_dnn_func, DNN_NMSBOXES),
    JS_CFUNC_MAGIC_DEF("NMSBoxesBatched", 6, js_dnn_func, DNN_NMSBOXESBATCHED),
    JS_CFUNC_MAGIC_DEF("readNet", 1, js_dnn_func, DNN_READNET),
    JS_CFUNC_MAGIC_DEF("readNetFromCaffe", 1, js_dnn_func, DNN_READNETFROMCAFFE),
    JS_CFUNC_MAGIC_DEF("readNetFromDarknet", 1, js_dnn_func, DNN_READNETFROMDARKNET),
    JS_CFUNC_MAGIC_DEF("readNetFromModelOptimizer", 1, js_dnn_func, DNN_READNETFROMMODELOPTIMIZER),
    JS_CFUNC_MAGIC_DEF("readNetFromONNX", 1, js_dnn_func, DNN_READNETFROMONNX),
    JS_CFUNC_MAGIC_DEF("readNetFromTensorflow", 1, js_dnn_func, DNN_READNETFROMTENSORFLOW),
    JS_CFUNC_MAGIC_DEF("readNetFromTFLite", 1, js_dnn_func, DNN_READNETFROMTFLITE),
    JS_CFUNC_MAGIC_DEF("readNetFromTorch", 1, js_dnn_func, DNN_READNETFROMTORCH),
    JS_CFUNC_MAGIC_DEF("readTensorFromONNX", 1, js_dnn_func, DNN_READTENSORFROMONNX),
    JS_CFUNC_MAGIC_DEF("readTorchBlob", 1, js_dnn_func, DNN_READTORCHBLOB),
    JS_CFUNC_MAGIC_DEF("shrinkCaffeModel", 1, js_dnn_func, DNN_SHRINKCAFFEMODEL),
    JS_CFUNC_MAGIC_DEF("softNMSBoxes", 1, js_dnn_func, DNN_SOFTNMSBOXES),
    JS_CFUNC_MAGIC_DEF("writeTextGraph", 2, js_dnn_func, DNN_WRITETEXTGRAPH),
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

  JS_SetPropertyStr(ctx, dnn_object, "Net", net_class);
  JS_SetPropertyStr(ctx, dnn_object, "Image2BlobParams", imageblob2params_class);

  JS_SetPropertyFunctionList(ctx, dnn_object, js_dnn_dnn_funcs, countof(js_dnn_dnn_funcs));

  if(m) {
    JS_SetModuleExport(ctx, m, "dnn", dnn_object);
  }

  return 0;
}

extern "C" void
js_dnn_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "dnn");
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
