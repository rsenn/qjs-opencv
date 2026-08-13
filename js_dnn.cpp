#include "js_cv.hpp"
#include "js_umat.hpp"
#include "js_rect.hpp"
#include "js_rotated_rect.hpp"
#include "include/jsbindings.hpp"
#include <quickjs.h>

#include <opencv2/dnn.hpp>

using JSNetData = cv::dnn::Net;
using JSImage2BlobParamsData = cv::dnn::Image2BlobParams;
using JSLayerData = cv::Ptr<cv::dnn::Layer>;

/* No generic js_value_from(cv::RotatedRect) exists elsewhere (js_rotated_rect.hpp
 * only provides js_rotated_rect_new) - added here so std::vector<cv::RotatedRect>
 * (TextDetectionModel_EAST/_DB::detectTextRectangles results) can go through the
 * generic std::vector<T> marshaling in jsbindings.hpp like every other vector type
 * in this file. */
static inline JSValue
js_value_from(JSContext* ctx, const cv::RotatedRect& r) {
  return js_rotated_rect_new(ctx, r);
}

/* OpenCV 5 moved DataLayout (and its DNN_LAYOUT_* enumerators) from
 * cv::dnn::DataLayout to cv::DataLayout - dnn.hpp now just reuses the core
 * enum. Qualified lookup doesn't fall back to the enclosing namespace, so
 * cv::dnn::DataLayout stops resolving on 5.x even though it's still valid
 * on 4.x. This shim gives both a single portable spelling. */
namespace qjs_dnn_compat {
#ifdef HAVE_OPENCV_DNN_NEW_ENGINE
using DataLayout = cv::DataLayout;
constexpr DataLayout DNN_LAYOUT_UNKNOWN = cv::DNN_LAYOUT_UNKNOWN;
constexpr DataLayout DNN_LAYOUT_ND = cv::DNN_LAYOUT_ND;
constexpr DataLayout DNN_LAYOUT_NCHW = cv::DNN_LAYOUT_NCHW;
constexpr DataLayout DNN_LAYOUT_NCDHW = cv::DNN_LAYOUT_NCDHW;
constexpr DataLayout DNN_LAYOUT_NHWC = cv::DNN_LAYOUT_NHWC;
constexpr DataLayout DNN_LAYOUT_NDHWC = cv::DNN_LAYOUT_NDHWC;
constexpr DataLayout DNN_LAYOUT_PLANAR = cv::DNN_LAYOUT_PLANAR;
#else
using DataLayout = cv::dnn::DataLayout;
constexpr DataLayout DNN_LAYOUT_UNKNOWN = cv::dnn::DNN_LAYOUT_UNKNOWN;
constexpr DataLayout DNN_LAYOUT_ND = cv::dnn::DNN_LAYOUT_ND;
constexpr DataLayout DNN_LAYOUT_NCHW = cv::dnn::DNN_LAYOUT_NCHW;
constexpr DataLayout DNN_LAYOUT_NCDHW = cv::dnn::DNN_LAYOUT_NCDHW;
constexpr DataLayout DNN_LAYOUT_NHWC = cv::dnn::DNN_LAYOUT_NHWC;
constexpr DataLayout DNN_LAYOUT_NDHWC = cv::dnn::DNN_LAYOUT_NDHWC;
constexpr DataLayout DNN_LAYOUT_PLANAR = cv::dnn::DNN_LAYOUT_PLANAR;
#endif
} // namespace qjs_dnn_compat

extern "C" {
thread_local JSValue dnn_object, net_proto, net_class, imageblob2params_proto, imageblob2params_class, layer_proto, layer_class;
thread_local JSClassID js_net_class_id, js_imageblob2params_class_id, js_layer_class_id;
}

static JSValue js_layer_wrap(JSContext* ctx, JSLayerData const& layer);

static JSValue
js_net_wrap(JSContext* ctx, JSValueConst proto, JSNetData* ib2p) {
  JSValue ret = JS_NewObjectProtoClass(ctx, proto, js_net_class_id);
  JS_SetOpaque(ret, ib2p);
  return ret;
}

JSValue
js_net_new(JSContext* ctx, JSValueConst proto) {
  JSNetData* dn = js_allocate<JSNetData>(ctx);

  new(dn) JSNetData();

  return js_net_wrap(ctx, proto, dn);
}

JSValue
js_net_new(JSContext* ctx, JSValueConst proto, const JSNetData& other) {
  JSNetData* dn = js_allocate<JSNetData>(ctx);

  new(dn) JSNetData(other);

  return js_net_wrap(ctx, proto, dn);
}

JSValue
js_net_new(JSContext* ctx, const JSNetData& dn) {
  return js_net_new(ctx, net_proto, dn);
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
  DNN_NET_EMPTY,
};

static JSValue
js_net_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSNetData* dn;
  JSValue ret = JS_UNDEFINED;

  if(!(dn = js_net_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case DNN_NET_EMPTY: {
      ret = js_value_from(ctx, dn->empty());
      break;
    }
  }

  return ret;
}

enum {
  DNN_NET_CONNECT,
  DNN_NET_DUMP,
  DNN_NET_DUMPTOFILE,
  DNN_NET_DUMPTOPBTXT,
  DNN_NET_ENABLEFUSION,
  DNN_NET_ENABLEWINOGRAD,
  DNN_NET_FORWARD,
  DNN_NET_FORWARDALL,
  DNN_NET_GETUNCONNECTEDOUTLAYERS,
  DNN_NET_GETUNCONNECTEDOUTLAYERSNAMES,
  DNN_NET_SETINPUT,
  DNN_NET_SETINPUTSNAMES,
  DNN_NET_SETPARAM,
  DNN_NET_SETPREFERABLEBACKEND,
  DNN_NET_SETPREFERABLETARGET,
  DNN_NET_GETINPUTDETAILS,
  DNN_NET_GETLAYER,
  DNN_NET_GETLAYERNAMES,
  DNN_NET_GETLAYERSCOUNT,
  DNN_NET_GETLAYERID,
  DNN_NET_GETLAYERTYPES,
  DNN_NET_GETOUTPUTDETAILS,
  DNN_NET_GETPARAM,
  DNN_NET_GETPERFPROFILE,
  DNN_NET_QUANTIZE,
  DNN_NET_REGISTEROUTPUT,
  DNN_NET_SETHALIDESCHEDULER,
#ifdef HAVE_OPENCV_DNN_TOKENIZER
  DNN_NET_ENABLEKVCACHE,
  DNN_NET_DISABLEKVCACHE,
  DNN_NET_RESETKVCACHE,
#endif
};

static JSValue
js_net_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSNetData* dn;
  JSValue ret = JS_UNDEFINED;

  if(!(dn = js_net_data2(ctx, this_val)))
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case DNN_NET_CONNECT: {
        if(argc >= 4) {
          int32_t outLayerId, inpLayerId;
          uint32_t outNum, inpNum;

          js_value_to(ctx, argv[0], outLayerId);
          js_value_to(ctx, argv[1], outNum);
          js_value_to(ctx, argv[2], inpLayerId);
          js_value_to(ctx, argv[3], inpNum);

          dn->connect(outLayerId, outNum, inpLayerId, inpNum);
        } else {
          cv::String outPin, inpPin;

          js_value_to(ctx, argv[0], outPin);
          js_value_to(ctx, argv[1], inpPin);

          dn->connect(outPin, inpPin);
        }

        break;
      }

      case DNN_NET_DUMP: {
        ret = js_value_from(ctx, dn->dump());
        break;
      }

      case DNN_NET_DUMPTOFILE: {
        cv::String path;
        js_value_to(ctx, argv[0], path);
        dn->dumpToFile(path);
        break;
      }

      case DNN_NET_DUMPTOPBTXT: {
        cv::String path;
        js_value_to(ctx, argv[0], path);
        dn->dumpToPbtxt(path);
        break;
      }

      case DNN_NET_ENABLEFUSION: {
        BOOL fusion = FALSE;
        js_value_to(ctx, argv[0], fusion);
        dn->enableFusion(fusion);
        break;
      }

      case DNN_NET_ENABLEWINOGRAD: {
        BOOL winograd = FALSE;
        js_value_to(ctx, argv[0], winograd);
        dn->enableWinograd(winograd);
        break;
      }

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

      case DNN_NET_GETUNCONNECTEDOUTLAYERS: {
        ret = js_value_from(ctx, dn->getUnconnectedOutLayers());
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

      case DNN_NET_SETINPUTSNAMES: {
        std::vector<cv::String> names;
        js_value_to(ctx, argv[0], names);

        dn->setInputsNames(names);
        break;
      }

      case DNN_NET_SETPARAM: {
        int32_t numParam;

        js_value_to(ctx, argv[1], numParam);
        cv::Mat* blob = js_mat_data2(ctx, argv[2]);

        if(JS_IsString(argv[0])) {
          cv::String name;
          js_value_to(ctx, argv[0], name);

          dn->setParam(name, numParam, *blob);
        } else {
          int32_t id;
          js_value_to(ctx, argv[0], id);

          dn->setParam(id, numParam, *blob);
        }

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

#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
      case DNN_NET_GETINPUTDETAILS: {
        std::vector<float> scales;
        std::vector<int> zeropoints;

        dn->getInputDetails(scales, zeropoints);

        js_array_clear(ctx, argv[0]);
        js_array_copy(ctx, argv[0], scales);

        js_array_clear(ctx, argv[1]);
        js_array_copy(ctx, argv[1], zeropoints);
        break;
      }
#endif

      case DNN_NET_GETLAYER: {
        JSLayerData dl;

        if(JS_IsString(argv[0])) {
          cv::String name;
          js_value_to(ctx, argv[0], name);

          dl = dn->getLayer(name);
        } else {
          int32_t id;
          js_value_to(ctx, argv[0], id);

          dl = dn->getLayer(id);
        }

        ret = js_layer_wrap(ctx, dl);
        break;
      }

      case DNN_NET_GETLAYERID: {
        cv::String name;
        js_value_to(ctx, argv[0], name);

        ret = js_value_from(ctx, dn->getLayerId(name));
        break;
      }

      case DNN_NET_GETLAYERNAMES: {
        ret = js_value_from(ctx, dn->getLayerNames());
        break;
      }

      case DNN_NET_GETLAYERSCOUNT: {
        cv::String name;
        js_value_to(ctx, argv[0], name);

        ret = js_value_from(ctx, dn->getLayersCount(name));
        break;
      }

      case DNN_NET_GETLAYERTYPES: {
        std::vector<cv::String> types;

        dn->getLayerTypes(types);

        js_array_clear(ctx, argv[0]);
        js_array_copy(ctx, argv[0], types);
        break;
      }

#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
      case DNN_NET_GETOUTPUTDETAILS: {
        std::vector<float> scales;
        std::vector<int> zeropoints;

        dn->getOutputDetails(scales, zeropoints);

        js_array_clear(ctx, argv[0]);
        js_array_copy(ctx, argv[0], scales);

        js_array_clear(ctx, argv[1]);
        js_array_copy(ctx, argv[1], zeropoints);
        break;
      }
#endif

      case DNN_NET_GETPARAM: {
        int32_t numParam = 0;

        if(argc > 1)
          js_value_to(ctx, argv[1], numParam);

        if(JS_IsString(argv[0])) {
          cv::String name;
          js_value_to(ctx, argv[0], name);

          ret = js_value_from(ctx, dn->getParam(name, numParam));
        } else {
          int32_t id;
          js_value_to(ctx, argv[0], id);

          ret = js_value_from(ctx, dn->getParam(id, numParam));
        }

        break;
      }

      case DNN_NET_GETPERFPROFILE: {
        std::vector<double> timings;

        ret = js_value_from(ctx, dn->getPerfProfile(timings));

        js_array_clear(ctx, argv[0]);
        js_array_copy(ctx, argv[0], timings);
        break;
      }

#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
      case DNN_NET_QUANTIZE: {
        JSInputArray calibData = js_input_array(ctx, argv[0]);
        int32_t inputsDtype, outputsDtype;
        BOOL perChannel = TRUE;

        js_value_to(ctx, argv[1], inputsDtype);
        js_value_to(ctx, argv[2], outputsDtype);

        if(argc > 3)
          js_value_to(ctx, argv[3], perChannel);

        ret = js_net_new(ctx, dn->quantize(calibData, inputsDtype, outputsDtype, perChannel));
        break;
      }
#endif

      case DNN_NET_REGISTEROUTPUT: {
        cv::String outputName;
        int32_t layerId, outputPort;

        js_value_to(ctx, argv[0], outputName);
        js_value_to(ctx, argv[1], layerId);
        js_value_to(ctx, argv[2], outputPort);

        ret = js_value_from(ctx, dn->registerOutput(outputName, layerId, outputPort));
        break;
      }

#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
      case DNN_NET_SETHALIDESCHEDULER: {
        cv::String scheduler;

        js_value_to(ctx, argv[0], scheduler);

        dn->setHalideScheduler(scheduler);
        break;
      }
#endif

#ifdef HAVE_OPENCV_DNN_TOKENIZER
      /* enableKVCache()/resetKVCache() segfault inside OpenCV itself (not a
       * catchable cv::Exception) when called on a Net with no layers loaded -
       * see BUGS: opencv-net-enablekvcache-segfaults-on-empty-net. Guard with
       * a JS-level check so this throws cleanly instead of crashing the
       * process; disableKVCache() alone is safe on an empty Net and needs no
       * guard. */
      case DNN_NET_ENABLEKVCACHE: {
        if(dn->empty())
          return JS_ThrowTypeError(ctx, "enableKVCache() requires a Net with layers loaded (segfaults inside OpenCV on an empty Net)");

        dn->enableKVCache();
        break;
      }

      case DNN_NET_DISABLEKVCACHE: {
        dn->disableKVCache();
        break;
      }

      case DNN_NET_RESETKVCACHE: {
        if(dn->empty())
          return JS_ThrowTypeError(ctx, "resetKVCache() requires a Net with layers loaded (segfaults inside OpenCV on an empty Net)");

        dn->resetKVCache();
        break;
      }
#endif
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

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
    JS_CGETSET_MAGIC_DEF("empty", js_net_get, 0, DNN_NET_EMPTY),
    JS_CFUNC_MAGIC_DEF("connect", 2, js_net_method, DNN_NET_CONNECT),
    JS_CFUNC_MAGIC_DEF("dump", 0, js_net_method, DNN_NET_DUMP),
    JS_CFUNC_MAGIC_DEF("dumpToFile", 1, js_net_method, DNN_NET_DUMPTOFILE),
    JS_CFUNC_MAGIC_DEF("dumpToPbtxt", 1, js_net_method, DNN_NET_DUMPTOPBTXT),
    JS_CFUNC_MAGIC_DEF("enableFusion", 1, js_net_method, DNN_NET_ENABLEFUSION),
    JS_CFUNC_MAGIC_DEF("enableWinograd", 1, js_net_method, DNN_NET_ENABLEWINOGRAD),
    JS_CFUNC_MAGIC_DEF("forward", 0, js_net_method, DNN_NET_FORWARD),
    JS_CFUNC_MAGIC_DEF("forwardAll", 1, js_net_method, DNN_NET_FORWARDALL),
    JS_CFUNC_MAGIC_DEF("getUnconnectedOutLayers", 0, js_net_method, DNN_NET_GETUNCONNECTEDOUTLAYERS),
    JS_CFUNC_MAGIC_DEF("getUnconnectedOutLayersNames", 0, js_net_method, DNN_NET_GETUNCONNECTEDOUTLAYERSNAMES),
#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
    JS_CFUNC_MAGIC_DEF("getInputDetails", 2, js_net_method, DNN_NET_GETINPUTDETAILS),
#endif
    JS_CFUNC_MAGIC_DEF("getLayer", 1, js_net_method, DNN_NET_GETLAYER),
    JS_CFUNC_MAGIC_DEF("getLayerId", 1, js_net_method, DNN_NET_GETLAYERID),
    JS_CFUNC_MAGIC_DEF("getLayerNames", 0, js_net_method, DNN_NET_GETLAYERNAMES),
    JS_CFUNC_MAGIC_DEF("getLayersCount", 1, js_net_method, DNN_NET_GETLAYERSCOUNT),
    JS_CFUNC_MAGIC_DEF("getLayerTypes", 1, js_net_method, DNN_NET_GETLAYERTYPES),
#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
    JS_CFUNC_MAGIC_DEF("getOutputDetails", 2, js_net_method, DNN_NET_GETOUTPUTDETAILS),
#endif
    JS_CFUNC_MAGIC_DEF("getParam", 2, js_net_method, DNN_NET_GETPARAM),
    JS_CFUNC_MAGIC_DEF("getPerfProfile", 1, js_net_method, DNN_NET_GETPERFPROFILE),
#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
    JS_CFUNC_MAGIC_DEF("quantize", 3, js_net_method, DNN_NET_QUANTIZE),
#endif
    JS_CFUNC_MAGIC_DEF("registerOuput", 3, js_net_method, DNN_NET_REGISTEROUTPUT),
#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
    JS_CFUNC_MAGIC_DEF("setHalideScheduler", 1, js_net_method, DNN_NET_SETHALIDESCHEDULER),
#endif
#ifdef HAVE_OPENCV_DNN_TOKENIZER
    JS_CFUNC_MAGIC_DEF("enableKVCache", 0, js_net_method, DNN_NET_ENABLEKVCACHE),
    JS_CFUNC_MAGIC_DEF("disableKVCache", 0, js_net_method, DNN_NET_DISABLEKVCACHE),
    JS_CFUNC_MAGIC_DEF("resetKVCache", 0, js_net_method, DNN_NET_RESETKVCACHE),
#endif
    JS_CFUNC_MAGIC_DEF("setInput", 0, js_net_method, DNN_NET_SETINPUT),
    JS_CFUNC_MAGIC_DEF("setInputsNames", 1, js_net_method, DNN_NET_SETINPUTSNAMES),
    JS_CFUNC_MAGIC_DEF("setParam", 3, js_net_method, DNN_NET_SETPARAM),
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
    int32_t ddepth = CV_32F, datalayout = qjs_dnn_compat::DNN_LAYOUT_NCHW, mode = cv::dnn::DNN_PMODE_NULL;
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

    new(dn) JSImage2BlobParamsData(scalefactor, size, mean, swapRB, ddepth, qjs_dnn_compat::DataLayout(datalayout), cv::dnn::ImagePaddingMode(mode), borderValue);
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

      ib2p->datalayout = qjs_dnn_compat::DataLayout(datalayout);
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

static JSValue
js_layer_wrap(JSContext* ctx, JSValueConst proto, JSLayerData const& layer) {
  JSLayerData* dl = js_allocate<JSLayerData>(ctx);
  JSValue ret = JS_NewObjectProtoClass(ctx, proto, js_layer_class_id);

  *dl = layer;

  JS_SetOpaque(ret, dl);
  return ret;
}

static JSValue
js_layer_wrap(JSContext* ctx, JSLayerData const& layer) {
  return js_layer_wrap(ctx, layer_proto, layer);
}

static JSValue
js_layer_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSLayerData* dl;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(dl = js_allocate<JSLayerData>(ctx)))
    return JS_EXCEPTION;

  /* XXX: TODO: LayerParams */
  new(dl) JSLayerData();

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_layer_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, dl);

  return obj;

fail:
  js_deallocate(ctx, dl);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSLayerData*
js_layer_data(JSValueConst val) {
  return static_cast<JSLayerData*>(JS_GetOpaque(val, js_layer_class_id));
}

JSLayerData*
js_layer_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSLayerData*>(JS_GetOpaque2(ctx, val, js_layer_class_id));
}

enum {
  LAYER_BLOBS,
  LAYER_NAME,
  LAYER_PREFERABLETARGET,
  LAYER_TYPE,
};

static JSValue
js_layer_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSLayerData* dl;
  JSValue ret = JS_UNDEFINED;

  if(!(dl = js_layer_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case LAYER_BLOBS: {
      ret = js_value_from(ctx, dl->get()->blobs);
      break;
    }

    case LAYER_NAME: {
      ret = js_value_from(ctx, dl->get()->name);
      break;
    }

    case LAYER_PREFERABLETARGET: {
      ret = js_value_from(ctx, dl->get()->preferableTarget);
      break;
    }

    case LAYER_TYPE: {
      ret = js_value_from(ctx, dl->get()->type);
      break;
    }
  }

  return ret;
}

static JSValue
js_layer_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSLayerData* dl;

  if(!(dl = js_layer_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case LAYER_BLOBS: {
      js_value_to(ctx, val, dl->get()->blobs);
      break;
    }

    case LAYER_NAME: {
      js_value_to(ctx, val, dl->get()->name);
      break;
    }

    case LAYER_PREFERABLETARGET: {
      js_value_to(ctx, val, dl->get()->preferableTarget);
      break;
    }

    case LAYER_TYPE: {
      js_value_to(ctx, val, dl->get()->type);
      break;
    }
  }

  return JS_UNDEFINED;
}

enum {
  LAYER_INPUTNAMETOINDEX,
  LAYER_OUTPUTNAMETOINDEX,
};

static JSValue
js_layer_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSLayerData* dl;
  JSValue ret = JS_UNDEFINED;

  if(!(dl = js_layer_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case LAYER_INPUTNAMETOINDEX: {
      cv::String name;
      js_value_to(ctx, argv[0], name);

      ret = JS_NewInt32(ctx, dl->get()->inputNameToIndex(name));
      break;
    }

    case LAYER_OUTPUTNAMETOINDEX: {
      cv::String name;
      js_value_to(ctx, argv[0], name);

      ret = JS_NewInt32(ctx, dl->get()->outputNameToIndex(name));
      break;
    }
  }

  return ret;
}

void
js_layer_finalizer(JSRuntime* rt, JSValue val) {
  JSLayerData* dl;

  /* Note: 'dl' can be NULL in case JS_SetOpaque() was not called */
  if((dl = js_layer_data(val))) {
    dl->~JSLayerData();

    js_deallocate(rt, dl);
  }
}

JSClassDef js_layer_class = {
    .class_name = "Layer",
    .finalizer = js_layer_finalizer,
};

const JSCFunctionListEntry js_layer_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("blobs", js_layer_get, js_layer_set, LAYER_BLOBS),
    JS_CGETSET_MAGIC_DEF("name", js_layer_get, js_layer_set, LAYER_NAME),
    JS_CGETSET_MAGIC_DEF("preferableTarget", js_layer_get, js_layer_set, LAYER_PREFERABLETARGET),
    JS_CGETSET_MAGIC_DEF("type", js_layer_get, js_layer_set, LAYER_TYPE),
    JS_CFUNC_MAGIC_DEF("inputNameToIndex", 1, js_layer_method, LAYER_INPUTNAMETOINDEX),
    JS_CFUNC_MAGIC_DEF("outputNameToIndex", 1, js_layer_method, LAYER_OUTPUTNAMETOINDEX),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Layer", JS_PROP_CONFIGURABLE),
};

#ifdef HAVE_OPENCV_DNN_TOKENIZER
/* cv::dnn::Tokenizer (OpenCV 5.0+) - BPE tokenizer for LLM inference (e.g. Qwen2.5
 * exported to ONNX). See migration-opencv5.md, "Wrapping LLM inference (Qwen2.5)".
 * Real entry point is the static factory Tokenizer.load(modelDir), not `new
 * Tokenizer()` - mirrors the C++ API, where the default ctor exists but isn't
 * useful standalone. `modelDir` must end with a path separator: OpenCV
 * concatenates "config.json"/"tokenizer.json" directly onto it. */
using JSTokenizerData = cv::dnn::Tokenizer;

extern "C" {
thread_local JSValue tokenizer_proto, tokenizer_class;
thread_local JSClassID js_tokenizer_class_id;
}

static JSValue
js_tokenizer_wrap(JSContext* ctx, JSValueConst proto, JSTokenizerData* tok) {
  JSValue ret = JS_NewObjectProtoClass(ctx, proto, js_tokenizer_class_id);
  JS_SetOpaque(ret, tok);
  return ret;
}

static JSValue
js_tokenizer_new(JSContext* ctx, JSValueConst proto, const JSTokenizerData& other) {
  JSTokenizerData* tok = js_allocate<JSTokenizerData>(ctx);

  new(tok) JSTokenizerData(other);

  return js_tokenizer_wrap(ctx, proto, tok);
}

static JSValue
js_tokenizer_new(JSContext* ctx, const JSTokenizerData& other) {
  return js_tokenizer_new(ctx, tokenizer_proto, other);
}

static JSValue
js_tokenizer_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSTokenizerData* tok;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(tok = js_allocate<JSTokenizerData>(ctx)))
    return JS_EXCEPTION;

  new(tok) JSTokenizerData();

  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_tokenizer_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, tok);

  return obj;

fail:
  js_deallocate(ctx, tok);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

static JSTokenizerData*
js_tokenizer_data(JSValueConst val) {
  return static_cast<JSTokenizerData*>(JS_GetOpaque(val, js_tokenizer_class_id));
}

static JSTokenizerData*
js_tokenizer_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSTokenizerData*>(JS_GetOpaque2(ctx, val, js_tokenizer_class_id));
}

enum {
  TOKENIZER_ENCODE,
  TOKENIZER_DECODE,
};

static JSValue
js_tokenizer_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSTokenizerData* tok;
  JSValue ret = JS_UNDEFINED;

  if(!(tok = js_tokenizer_data2(ctx, this_val)))
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case TOKENIZER_ENCODE: {
        std::string text;
        js_value_to(ctx, argv[0], text);

        ret = js_value_from(ctx, tok->encode(text));
        break;
      }

      case TOKENIZER_DECODE: {
        std::vector<int> tokens;
        js_value_to(ctx, argv[0], tokens);

        ret = js_value_from(ctx, tok->decode(tokens));
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

enum {
  TOKENIZER_STATIC_LOAD,
};

static JSValue
js_tokenizer_static(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
    switch(magic) {
      case TOKENIZER_STATIC_LOAD: {
        std::string modelConfig;
        js_value_to(ctx, argv[0], modelConfig);

        ret = js_tokenizer_new(ctx, cv::dnn::Tokenizer::load(modelConfig));
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

static void
js_tokenizer_finalizer(JSRuntime* rt, JSValue val) {
  JSTokenizerData* tok;

  if((tok = js_tokenizer_data(val))) {
    tok->~JSTokenizerData();

    js_deallocate(rt, tok);
  }
}

JSClassDef js_tokenizer_class = {
    .class_name = "Tokenizer",
    .finalizer = js_tokenizer_finalizer,
};

const JSCFunctionListEntry js_tokenizer_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("encode", 1, js_tokenizer_method, TOKENIZER_ENCODE),
    JS_CFUNC_MAGIC_DEF("decode", 1, js_tokenizer_method, TOKENIZER_DECODE),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Tokenizer", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_tokenizer_static_funcs[] = {
    JS_CFUNC_MAGIC_DEF("load", 1, js_tokenizer_static, TOKENIZER_STATIC_LOAD),
};
#endif /* HAVE_OPENCV_DNN_TOKENIZER */

/* cv::dnn::Model and its convenience subclasses (Classification/Detection/
 * Segmentation/Keypoints/TextRecognition/TextDetection). Identical API surface
 * on OpenCV 4.x and 5.x (unlike the rest of this file, nothing here needs a
 * capability guard). All eight classes share the same construction shape
 * (fromModelFile(model, config="") or fromNet(existingNet)) and the same base
 * proxy methods inherited from Model (setInputSize/Mean/Scale/Crop/SwapRB,
 * setOutputNames, setInputParams, predict, setPreferableBackend/Target,
 * enableWinograd) - factored out once here instead of seven times. */

enum {
  MODEL_SET_INPUT_SIZE,
  MODEL_SET_INPUT_MEAN,
  MODEL_SET_INPUT_SCALE,
  MODEL_SET_INPUT_CROP,
  MODEL_SET_INPUT_SWAP_RB,
  MODEL_SET_OUTPUT_NAMES,
  MODEL_SET_INPUT_PARAMS,
  MODEL_PREDICT,
  MODEL_SET_PREFERABLE_BACKEND,
  MODEL_SET_PREFERABLE_TARGET,
  MODEL_ENABLE_WINOGRAD,
  MODEL_METHOD_COUNT,
};

template<class ModelT>
static bool
js_model_base_method(JSContext* ctx, ModelT& model, int argc, JSValueConst argv[], int magic, JSValue& ret) {
  try {
    switch(magic) {
      case MODEL_SET_INPUT_SIZE: {
        if(argc >= 2 && JS_IsNumber(argv[0]) && JS_IsNumber(argv[1])) {
          int32_t w, h;
          js_value_to(ctx, argv[0], w);
          js_value_to(ctx, argv[1], h);
          model.setInputSize(cv::Size(w, h));
        } else {
          cv::Size sz;
          js_value_to(ctx, argv[0], sz);
          model.setInputSize(sz);
        }
        break;
      }

      case MODEL_SET_INPUT_MEAN: {
        cv::Scalar mean;
        js_scalar_read(ctx, argv[0], mean);
        model.setInputMean(mean);
        break;
      }

      case MODEL_SET_INPUT_SCALE: {
        cv::Scalar scale;
        js_scalar_read(ctx, argv[0], scale);
        model.setInputScale(scale);
        break;
      }

      case MODEL_SET_INPUT_CROP: {
        bool crop = false;
        js_value_to(ctx, argv[0], crop);
        model.setInputCrop(crop);
        break;
      }

      case MODEL_SET_INPUT_SWAP_RB: {
        bool swapRB = false;
        js_value_to(ctx, argv[0], swapRB);
        model.setInputSwapRB(swapRB);
        break;
      }

      case MODEL_SET_OUTPUT_NAMES: {
        std::vector<cv::String> names;
        js_value_to(ctx, argv[0], names);
        model.setOutputNames(names);
        break;
      }

      case MODEL_SET_INPUT_PARAMS: {
        double scale = 1.0;
        cv::Size size;
        cv::Scalar mean;
        bool swapRB = false, crop = false;

        if(argc > 0)
          js_value_to(ctx, argv[0], scale);
        if(argc > 1)
          js_value_to(ctx, argv[1], size);
        if(argc > 2)
          js_scalar_read(ctx, argv[2], mean);
        if(argc > 3)
          js_value_to(ctx, argv[3], swapRB);
        if(argc > 4)
          js_value_to(ctx, argv[4], crop);

        model.setInputParams(scale, size, mean, swapRB, crop);
        break;
      }

      case MODEL_PREDICT: {
        JSInputArray frame = js_input_array(ctx, argv[0]);
        std::vector<cv::Mat> outs;

        model.predict(frame, outs);

        js_array_clear(ctx, argv[1]);
        js_array_copy(ctx, argv[1], outs);
        break;
      }

      case MODEL_SET_PREFERABLE_BACKEND: {
        int32_t backendId = 0;
        js_value_to(ctx, argv[0], backendId);
        model.setPreferableBackend(cv::dnn::Backend(backendId));
        break;
      }

      case MODEL_SET_PREFERABLE_TARGET: {
        int32_t targetId = 0;
        js_value_to(ctx, argv[0], targetId);
        model.setPreferableTarget(cv::dnn::Target(targetId));
        break;
      }

      case MODEL_ENABLE_WINOGRAD: {
        bool useWinograd = true;
        js_value_to(ctx, argv[0], useWinograd);
        model.enableWinograd(useWinograd);
        break;
      }

      default: return false;
    }
  } catch(const cv::Exception& e) {
    ret = js_cv_throw(ctx, e);
    return true;
  }

  return true;
}

#define DNN_MODEL_BASE_PROTO_FUNCS(method_fn)                                                        \
  JS_CFUNC_MAGIC_DEF("setInputSize", 1, method_fn, MODEL_SET_INPUT_SIZE),                             \
      JS_CFUNC_MAGIC_DEF("setInputMean", 1, method_fn, MODEL_SET_INPUT_MEAN),                         \
      JS_CFUNC_MAGIC_DEF("setInputScale", 1, method_fn, MODEL_SET_INPUT_SCALE),                       \
      JS_CFUNC_MAGIC_DEF("setInputCrop", 1, method_fn, MODEL_SET_INPUT_CROP),                         \
      JS_CFUNC_MAGIC_DEF("setInputSwapRB", 1, method_fn, MODEL_SET_INPUT_SWAP_RB),                    \
      JS_CFUNC_MAGIC_DEF("setOutputNames", 1, method_fn, MODEL_SET_OUTPUT_NAMES),                     \
      JS_CFUNC_MAGIC_DEF("setInputParams", 0, method_fn, MODEL_SET_INPUT_PARAMS),                     \
      JS_CFUNC_MAGIC_DEF("predict", 2, method_fn, MODEL_PREDICT),                                     \
      JS_CFUNC_MAGIC_DEF("setPreferableBackend", 1, method_fn, MODEL_SET_PREFERABLE_BACKEND),         \
      JS_CFUNC_MAGIC_DEF("setPreferableTarget", 1, method_fn, MODEL_SET_PREFERABLE_TARGET),           \
      JS_CFUNC_MAGIC_DEF("enableWinograd", 1, method_fn, MODEL_ENABLE_WINOGRAD)

/* Mechanical skeleton (class id/proto/ctor/data accessors/finalizer) shared by
 * all eight Model-family classes - construction shape is identical:
 * new X(net) from an existing dnn.Net, or new X(model[, config]) from files. */
#define DEFINE_DNN_MODEL_SKELETON(tag, CppType, JsName)                                                    \
  using JS##tag##Data = CppType;                                                                            \
  extern "C" {                                                                                               \
  thread_local JSValue tag##_proto, tag##_class;                                                             \
  thread_local JSClassID js_##tag##_class_id;                                                                \
  }                                                                                                            \
  static JSValue js_##tag##_wrap(JSContext* ctx, JSValueConst proto, JS##tag##Data* data) {                  \
    JSValue ret = JS_NewObjectProtoClass(ctx, proto, js_##tag##_class_id);                                    \
    JS_SetOpaque(ret, data);                                                                                   \
    return ret;                                                                                                \
  }                                                                                                             \
  static JS##tag##Data* js_##tag##_data(JSValueConst val) {                                                    \
    return static_cast<JS##tag##Data*>(JS_GetOpaque(val, js_##tag##_class_id));                                \
  }                                                                                                              \
  static JS##tag##Data* js_##tag##_data2(JSContext* ctx, JSValueConst val) {                                    \
    return static_cast<JS##tag##Data*>(JS_GetOpaque2(ctx, val, js_##tag##_class_id));                           \
  }                                                                                                                \
  static JSValue js_##tag##_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) { \
    JS##tag##Data* data;                                                                                           \
    JSValue obj = JS_UNDEFINED, proto;                                                                             \
    JSNetData* net;                                                                                                 \
    if(!(data = js_allocate<JS##tag##Data>(ctx)))                                                                   \
      return JS_EXCEPTION;                                                                                           \
    try {                                                                                                             \
      if(argc > 0 && (net = js_net_data(argv[0]))) {                                                                  \
        new(data) JS##tag##Data(*net);                                                                                 \
      } else {                                                                                                          \
        std::string model, config;                                                                                      \
        if(argc > 0)                                                                                                      \
          js_value_to(ctx, argv[0], model);                                                                                \
        if(argc > 1)                                                                                                        \
          js_value_to(ctx, argv[1], config);                                                                                 \
        new(data) JS##tag##Data(model, config);                                                                               \
      }                                                                                                                        \
    } catch(const cv::Exception& e) {                                                                                           \
      js_deallocate(ctx, data);                                                                                                  \
      return js_cv_throw(ctx, e);                                                                                                 \
    }                                                                                                                              \
    proto = JS_GetPropertyStr(ctx, new_target, "prototype");                                                                       \
    if(JS_IsException(proto))                                                                                                       \
      goto fail;                                                                                                                     \
    obj = JS_NewObjectProtoClass(ctx, proto, js_##tag##_class_id);                                                                    \
    JS_FreeValue(ctx, proto);                                                                                                          \
    if(JS_IsException(obj))                                                                                                             \
      goto fail;                                                                                                                          \
    JS_SetOpaque(obj, data);                                                                                                               \
    return obj;                                                                                                                             \
  fail:                                                                                                                                      \
    data->~JS##tag##Data();                                                                                                                   \
    js_deallocate(ctx, data);                                                                                                                  \
    JS_FreeValue(ctx, obj);                                                                                                                     \
    return JS_EXCEPTION;                                                                                                                         \
  }                                                                                                                                                \
  static void js_##tag##_finalizer(JSRuntime* rt, JSValue val) {                                                                                   \
    JS##tag##Data* data;                                                                                                                             \
    if((data = js_##tag##_data(val))) {                                                                                                               \
      data->~JS##tag##Data();                                                                                                                          \
      js_deallocate(rt, data);                                                                                                                          \
    }                                                                                                                                                     \
  }                                                                                                                                                         \
  JSClassDef js_##tag##_class = {                                                                                                                           \
      .class_name = JsName,                                                                                                                                  \
      .finalizer = js_##tag##_finalizer,                                                                                                                       \
  }

/* Model - the base class itself, no methods beyond the shared base set. */
DEFINE_DNN_MODEL_SKELETON(model, cv::dnn::Model, "Model");

static JSValue
js_model_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSmodelData* data;
  JSValue ret = JS_UNDEFINED;

  if(!(data = js_model_data2(ctx, this_val)))
    return JS_EXCEPTION;

  js_model_base_method(ctx, *data, argc, argv, magic, ret);
  return ret;
}

const JSCFunctionListEntry js_model_proto_funcs[] = {
    DNN_MODEL_BASE_PROTO_FUNCS(js_model_method),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Model", JS_PROP_CONFIGURABLE),
};

/* ClassificationModel */
DEFINE_DNN_MODEL_SKELETON(classification_model, cv::dnn::ClassificationModel, "ClassificationModel");

enum {
  CLASSIFICATION_MODEL_CLASSIFY = MODEL_METHOD_COUNT,
  CLASSIFICATION_MODEL_SET_ENABLE_SOFTMAX,
  CLASSIFICATION_MODEL_GET_ENABLE_SOFTMAX,
};

static JSValue
js_classification_model_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSclassification_modelData* data;
  JSValue ret = JS_UNDEFINED;

  if(!(data = js_classification_model_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(js_model_base_method(ctx, *data, argc, argv, magic, ret))
    return ret;

  try {
    switch(magic) {
      case CLASSIFICATION_MODEL_CLASSIFY: {
        JSInputArray frame = js_input_array(ctx, argv[0]);
        int classId = -1;
        float conf = 0;

        data->classify(frame, classId, conf);

        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "classId", js_value_from(ctx, classId));
        JS_SetPropertyStr(ctx, obj, "confidence", js_value_from(ctx, conf));
        ret = obj;
        break;
      }

      case CLASSIFICATION_MODEL_SET_ENABLE_SOFTMAX: {
        bool enable = true;
        js_value_to(ctx, argv[0], enable);
        data->setEnableSoftmaxPostProcessing(enable);
        break;
      }

      case CLASSIFICATION_MODEL_GET_ENABLE_SOFTMAX: {
        ret = js_value_from(ctx, data->getEnableSoftmaxPostProcessing());
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

const JSCFunctionListEntry js_classification_model_proto_funcs[] = {
    DNN_MODEL_BASE_PROTO_FUNCS(js_classification_model_method),
    JS_CFUNC_MAGIC_DEF("classify", 1, js_classification_model_method, CLASSIFICATION_MODEL_CLASSIFY),
    JS_CFUNC_MAGIC_DEF("setEnableSoftmaxPostProcessing", 1, js_classification_model_method, CLASSIFICATION_MODEL_SET_ENABLE_SOFTMAX),
    JS_CFUNC_MAGIC_DEF("getEnableSoftmaxPostProcessing", 0, js_classification_model_method, CLASSIFICATION_MODEL_GET_ENABLE_SOFTMAX),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "ClassificationModel", JS_PROP_CONFIGURABLE),
};

/* DetectionModel */
DEFINE_DNN_MODEL_SKELETON(detection_model, cv::dnn::DetectionModel, "DetectionModel");

enum {
  DETECTION_MODEL_DETECT = MODEL_METHOD_COUNT,
  DETECTION_MODEL_SET_NMS_ACROSS_CLASSES,
  DETECTION_MODEL_GET_NMS_ACROSS_CLASSES,
};

static JSValue
js_detection_model_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSdetection_modelData* data;
  JSValue ret = JS_UNDEFINED;

  if(!(data = js_detection_model_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(js_model_base_method(ctx, *data, argc, argv, magic, ret))
    return ret;

  try {
    switch(magic) {
      case DETECTION_MODEL_DETECT: {
        JSInputArray frame = js_input_array(ctx, argv[0]);
        std::vector<int> classIds;
        std::vector<float> confidences;
        std::vector<cv::Rect> boxes;
        float confThreshold = 0.5f, nmsThreshold = 0.0f;

        if(argc > 4)
          js_value_to(ctx, argv[4], confThreshold);
        if(argc > 5)
          js_value_to(ctx, argv[5], nmsThreshold);

        data->detect(frame, classIds, confidences, boxes, confThreshold, nmsThreshold);

        js_array_clear(ctx, argv[1]);
        js_array_copy(ctx, argv[1], classIds);
        js_array_clear(ctx, argv[2]);
        js_array_copy(ctx, argv[2], confidences);
        js_array_clear(ctx, argv[3]);
        js_array_copy(ctx, argv[3], boxes);
        break;
      }

      case DETECTION_MODEL_SET_NMS_ACROSS_CLASSES: {
        bool value = false;
        js_value_to(ctx, argv[0], value);
        data->setNmsAcrossClasses(value);
        break;
      }

      case DETECTION_MODEL_GET_NMS_ACROSS_CLASSES: {
        ret = js_value_from(ctx, data->getNmsAcrossClasses());
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

const JSCFunctionListEntry js_detection_model_proto_funcs[] = {
    DNN_MODEL_BASE_PROTO_FUNCS(js_detection_model_method),
    JS_CFUNC_MAGIC_DEF("detect", 4, js_detection_model_method, DETECTION_MODEL_DETECT),
    JS_CFUNC_MAGIC_DEF("setNmsAcrossClasses", 1, js_detection_model_method, DETECTION_MODEL_SET_NMS_ACROSS_CLASSES),
    JS_CFUNC_MAGIC_DEF("getNmsAcrossClasses", 0, js_detection_model_method, DETECTION_MODEL_GET_NMS_ACROSS_CLASSES),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "DetectionModel", JS_PROP_CONFIGURABLE),
};

/* SegmentationModel */
DEFINE_DNN_MODEL_SKELETON(segmentation_model, cv::dnn::SegmentationModel, "SegmentationModel");

enum {
  SEGMENTATION_MODEL_SEGMENT = MODEL_METHOD_COUNT,
};

static JSValue
js_segmentation_model_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSsegmentation_modelData* data;
  JSValue ret = JS_UNDEFINED;

  if(!(data = js_segmentation_model_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(js_model_base_method(ctx, *data, argc, argv, magic, ret))
    return ret;

  try {
    switch(magic) {
      case SEGMENTATION_MODEL_SEGMENT: {
        JSInputArray frame = js_input_array(ctx, argv[0]);
        JSOutputArray mask = js_cv_outputarray(ctx, argv[1]);

        data->segment(frame, mask);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

const JSCFunctionListEntry js_segmentation_model_proto_funcs[] = {
    DNN_MODEL_BASE_PROTO_FUNCS(js_segmentation_model_method),
    JS_CFUNC_MAGIC_DEF("segment", 2, js_segmentation_model_method, SEGMENTATION_MODEL_SEGMENT),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "SegmentationModel", JS_PROP_CONFIGURABLE),
};

/* KeypointsModel */
DEFINE_DNN_MODEL_SKELETON(keypoints_model, cv::dnn::KeypointsModel, "KeypointsModel");

enum {
  KEYPOINTS_MODEL_ESTIMATE = MODEL_METHOD_COUNT,
};

static JSValue
js_keypoints_model_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSkeypoints_modelData* data;
  JSValue ret = JS_UNDEFINED;

  if(!(data = js_keypoints_model_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(js_model_base_method(ctx, *data, argc, argv, magic, ret))
    return ret;

  try {
    switch(magic) {
      case KEYPOINTS_MODEL_ESTIMATE: {
        JSInputArray frame = js_input_array(ctx, argv[0]);
        float thresh = 0.5f;

        if(argc > 1)
          js_value_to(ctx, argv[1], thresh);

        ret = js_value_from(ctx, data->estimate(frame, thresh));
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

const JSCFunctionListEntry js_keypoints_model_proto_funcs[] = {
    DNN_MODEL_BASE_PROTO_FUNCS(js_keypoints_model_method),
    JS_CFUNC_MAGIC_DEF("estimate", 1, js_keypoints_model_method, KEYPOINTS_MODEL_ESTIMATE),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "KeypointsModel", JS_PROP_CONFIGURABLE),
};

/* TextRecognitionModel */
DEFINE_DNN_MODEL_SKELETON(text_recognition_model, cv::dnn::TextRecognitionModel, "TextRecognitionModel");

enum {
  TEXT_RECOGNITION_MODEL_RECOGNIZE = MODEL_METHOD_COUNT,
  TEXT_RECOGNITION_MODEL_SET_DECODE_TYPE,
  TEXT_RECOGNITION_MODEL_GET_DECODE_TYPE,
  TEXT_RECOGNITION_MODEL_SET_DECODE_OPTS_CTC,
  TEXT_RECOGNITION_MODEL_SET_VOCABULARY,
  TEXT_RECOGNITION_MODEL_GET_VOCABULARY,
};

static JSValue
js_text_recognition_model_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JStext_recognition_modelData* data;
  JSValue ret = JS_UNDEFINED;

  if(!(data = js_text_recognition_model_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(js_model_base_method(ctx, *data, argc, argv, magic, ret))
    return ret;

  try {
    switch(magic) {
      case TEXT_RECOGNITION_MODEL_RECOGNIZE: {
        JSInputArray frame = js_input_array(ctx, argv[0]);

        if(argc > 1) {
          std::vector<cv::Rect> roiRects;
          std::vector<std::string> results;

          js_value_to(ctx, argv[1], roiRects);

          data->recognize(frame, roiRects, results);

          js_array_clear(ctx, argv[2]);
          js_array_copy(ctx, argv[2], results);
        } else {
          ret = js_value_from(ctx, data->recognize(frame));
        }

        break;
      }

      case TEXT_RECOGNITION_MODEL_SET_DECODE_TYPE: {
        std::string decodeType;
        js_value_to(ctx, argv[0], decodeType);
        data->setDecodeType(decodeType);
        break;
      }

      case TEXT_RECOGNITION_MODEL_GET_DECODE_TYPE: {
        ret = js_value_from(ctx, data->getDecodeType());
        break;
      }

      case TEXT_RECOGNITION_MODEL_SET_DECODE_OPTS_CTC: {
        int32_t beamSize, vocPruneSize = 0;
        js_value_to(ctx, argv[0], beamSize);
        if(argc > 1)
          js_value_to(ctx, argv[1], vocPruneSize);
        data->setDecodeOptsCTCPrefixBeamSearch(beamSize, vocPruneSize);
        break;
      }

      case TEXT_RECOGNITION_MODEL_SET_VOCABULARY: {
        std::vector<std::string> vocabulary;
        js_value_to(ctx, argv[0], vocabulary);
        data->setVocabulary(vocabulary);
        break;
      }

      case TEXT_RECOGNITION_MODEL_GET_VOCABULARY: {
        ret = js_value_from(ctx, data->getVocabulary());
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

const JSCFunctionListEntry js_text_recognition_model_proto_funcs[] = {
    DNN_MODEL_BASE_PROTO_FUNCS(js_text_recognition_model_method),
    JS_CFUNC_MAGIC_DEF("recognize", 1, js_text_recognition_model_method, TEXT_RECOGNITION_MODEL_RECOGNIZE),
    JS_CFUNC_MAGIC_DEF("setDecodeType", 1, js_text_recognition_model_method, TEXT_RECOGNITION_MODEL_SET_DECODE_TYPE),
    JS_CFUNC_MAGIC_DEF("getDecodeType", 0, js_text_recognition_model_method, TEXT_RECOGNITION_MODEL_GET_DECODE_TYPE),
    JS_CFUNC_MAGIC_DEF("setDecodeOptsCTCPrefixBeamSearch", 1, js_text_recognition_model_method, TEXT_RECOGNITION_MODEL_SET_DECODE_OPTS_CTC),
    JS_CFUNC_MAGIC_DEF("setVocabulary", 1, js_text_recognition_model_method, TEXT_RECOGNITION_MODEL_SET_VOCABULARY),
    JS_CFUNC_MAGIC_DEF("getVocabulary", 0, js_text_recognition_model_method, TEXT_RECOGNITION_MODEL_GET_VOCABULARY),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "TextRecognitionModel", JS_PROP_CONFIGURABLE),
};

/* TextDetectionModel_EAST / TextDetectionModel_DB share detect()/
 * detectTextRectangles() from their common (unbound - protected ctor, not
 * directly constructible) TextDetectionModel base. */
enum {
  TEXT_DETECTION_MODEL_DETECT = MODEL_METHOD_COUNT,
  TEXT_DETECTION_MODEL_DETECTTEXTRECTANGLES,
  TEXT_DETECTION_MODEL_METHOD_COUNT,
};

template<class ModelT>
static bool
js_text_detection_model_method(JSContext* ctx, ModelT& model, int argc, JSValueConst argv[], int magic, JSValue& ret) {
  try {
    switch(magic) {
      case TEXT_DETECTION_MODEL_DETECT: {
        JSInputArray frame = js_input_array(ctx, argv[0]);
        std::vector<std::vector<cv::Point>> detections;

        if(argc > 2) {
          std::vector<float> confidences;
          model.detect(frame, detections, confidences);
          js_array_clear(ctx, argv[2]);
          js_array_copy(ctx, argv[2], confidences);
        } else {
          model.detect(frame, detections);
        }

        js_array_clear(ctx, argv[1]);
        js_array_copy(ctx, argv[1], detections);
        break;
      }

      case TEXT_DETECTION_MODEL_DETECTTEXTRECTANGLES: {
        JSInputArray frame = js_input_array(ctx, argv[0]);
        std::vector<cv::RotatedRect> detections;

        if(argc > 2) {
          std::vector<float> confidences;
          model.detectTextRectangles(frame, detections, confidences);
          js_array_clear(ctx, argv[2]);
          js_array_copy(ctx, argv[2], confidences);
        } else {
          model.detectTextRectangles(frame, detections);
        }

        js_array_clear(ctx, argv[1]);
        js_array_copy(ctx, argv[1], detections);
        break;
      }

      default: return false;
    }
  } catch(const cv::Exception& e) {
    ret = js_cv_throw(ctx, e);
    return true;
  }

  return true;
}

#define DNN_TEXT_DETECTION_MODEL_PROTO_FUNCS(method_fn)                                       \
  JS_CFUNC_MAGIC_DEF("detect", 2, method_fn, TEXT_DETECTION_MODEL_DETECT),                     \
      JS_CFUNC_MAGIC_DEF("detectTextRectangles", 2, method_fn, TEXT_DETECTION_MODEL_DETECTTEXTRECTANGLES)

/* TextDetectionModel_EAST */
DEFINE_DNN_MODEL_SKELETON(text_detection_model_east, cv::dnn::TextDetectionModel_EAST, "TextDetectionModel_EAST");

enum {
  TEXT_DETECTION_MODEL_EAST_SET_CONFIDENCE_THRESHOLD = TEXT_DETECTION_MODEL_METHOD_COUNT,
  TEXT_DETECTION_MODEL_EAST_GET_CONFIDENCE_THRESHOLD,
  TEXT_DETECTION_MODEL_EAST_SET_NMS_THRESHOLD,
  TEXT_DETECTION_MODEL_EAST_GET_NMS_THRESHOLD,
};

static JSValue
js_text_detection_model_east_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JStext_detection_model_eastData* data;
  JSValue ret = JS_UNDEFINED;

  if(!(data = js_text_detection_model_east_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(js_model_base_method(ctx, *data, argc, argv, magic, ret))
    return ret;
  if(js_text_detection_model_method(ctx, *data, argc, argv, magic, ret))
    return ret;

  try {
    switch(magic) {
      case TEXT_DETECTION_MODEL_EAST_SET_CONFIDENCE_THRESHOLD: {
        float confThreshold;
        js_value_to(ctx, argv[0], confThreshold);
        data->setConfidenceThreshold(confThreshold);
        break;
      }

      case TEXT_DETECTION_MODEL_EAST_GET_CONFIDENCE_THRESHOLD: {
        ret = js_value_from(ctx, data->getConfidenceThreshold());
        break;
      }

      case TEXT_DETECTION_MODEL_EAST_SET_NMS_THRESHOLD: {
        float nmsThreshold;
        js_value_to(ctx, argv[0], nmsThreshold);
        data->setNMSThreshold(nmsThreshold);
        break;
      }

      case TEXT_DETECTION_MODEL_EAST_GET_NMS_THRESHOLD: {
        ret = js_value_from(ctx, data->getNMSThreshold());
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

const JSCFunctionListEntry js_text_detection_model_east_proto_funcs[] = {
    DNN_MODEL_BASE_PROTO_FUNCS(js_text_detection_model_east_method),
    DNN_TEXT_DETECTION_MODEL_PROTO_FUNCS(js_text_detection_model_east_method),
    JS_CFUNC_MAGIC_DEF("setConfidenceThreshold", 1, js_text_detection_model_east_method, TEXT_DETECTION_MODEL_EAST_SET_CONFIDENCE_THRESHOLD),
    JS_CFUNC_MAGIC_DEF("getConfidenceThreshold", 0, js_text_detection_model_east_method, TEXT_DETECTION_MODEL_EAST_GET_CONFIDENCE_THRESHOLD),
    JS_CFUNC_MAGIC_DEF("setNMSThreshold", 1, js_text_detection_model_east_method, TEXT_DETECTION_MODEL_EAST_SET_NMS_THRESHOLD),
    JS_CFUNC_MAGIC_DEF("getNMSThreshold", 0, js_text_detection_model_east_method, TEXT_DETECTION_MODEL_EAST_GET_NMS_THRESHOLD),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "TextDetectionModel_EAST", JS_PROP_CONFIGURABLE),
};

/* TextDetectionModel_DB */
DEFINE_DNN_MODEL_SKELETON(text_detection_model_db, cv::dnn::TextDetectionModel_DB, "TextDetectionModel_DB");

enum {
  TEXT_DETECTION_MODEL_DB_SET_BINARY_THRESHOLD = TEXT_DETECTION_MODEL_METHOD_COUNT,
  TEXT_DETECTION_MODEL_DB_GET_BINARY_THRESHOLD,
  TEXT_DETECTION_MODEL_DB_SET_POLYGON_THRESHOLD,
  TEXT_DETECTION_MODEL_DB_GET_POLYGON_THRESHOLD,
  TEXT_DETECTION_MODEL_DB_SET_UNCLIP_RATIO,
  TEXT_DETECTION_MODEL_DB_GET_UNCLIP_RATIO,
  TEXT_DETECTION_MODEL_DB_SET_MAX_CANDIDATES,
  TEXT_DETECTION_MODEL_DB_GET_MAX_CANDIDATES,
};

static JSValue
js_text_detection_model_db_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JStext_detection_model_dbData* data;
  JSValue ret = JS_UNDEFINED;

  if(!(data = js_text_detection_model_db_data2(ctx, this_val)))
    return JS_EXCEPTION;

  if(js_model_base_method(ctx, *data, argc, argv, magic, ret))
    return ret;
  if(js_text_detection_model_method(ctx, *data, argc, argv, magic, ret))
    return ret;

  try {
    switch(magic) {
      case TEXT_DETECTION_MODEL_DB_SET_BINARY_THRESHOLD: {
        float v;
        js_value_to(ctx, argv[0], v);
        data->setBinaryThreshold(v);
        break;
      }

      case TEXT_DETECTION_MODEL_DB_GET_BINARY_THRESHOLD: {
        ret = js_value_from(ctx, data->getBinaryThreshold());
        break;
      }

      case TEXT_DETECTION_MODEL_DB_SET_POLYGON_THRESHOLD: {
        float v;
        js_value_to(ctx, argv[0], v);
        data->setPolygonThreshold(v);
        break;
      }

      case TEXT_DETECTION_MODEL_DB_GET_POLYGON_THRESHOLD: {
        ret = js_value_from(ctx, data->getPolygonThreshold());
        break;
      }

      case TEXT_DETECTION_MODEL_DB_SET_UNCLIP_RATIO: {
        double v;
        js_value_to(ctx, argv[0], v);
        data->setUnclipRatio(v);
        break;
      }

      case TEXT_DETECTION_MODEL_DB_GET_UNCLIP_RATIO: {
        ret = js_value_from(ctx, data->getUnclipRatio());
        break;
      }

      case TEXT_DETECTION_MODEL_DB_SET_MAX_CANDIDATES: {
        int32_t v;
        js_value_to(ctx, argv[0], v);
        data->setMaxCandidates(v);
        break;
      }

      case TEXT_DETECTION_MODEL_DB_GET_MAX_CANDIDATES: {
        ret = js_value_from(ctx, data->getMaxCandidates());
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

const JSCFunctionListEntry js_text_detection_model_db_proto_funcs[] = {
    DNN_MODEL_BASE_PROTO_FUNCS(js_text_detection_model_db_method),
    DNN_TEXT_DETECTION_MODEL_PROTO_FUNCS(js_text_detection_model_db_method),
    JS_CFUNC_MAGIC_DEF("setBinaryThreshold", 1, js_text_detection_model_db_method, TEXT_DETECTION_MODEL_DB_SET_BINARY_THRESHOLD),
    JS_CFUNC_MAGIC_DEF("getBinaryThreshold", 0, js_text_detection_model_db_method, TEXT_DETECTION_MODEL_DB_GET_BINARY_THRESHOLD),
    JS_CFUNC_MAGIC_DEF("setPolygonThreshold", 1, js_text_detection_model_db_method, TEXT_DETECTION_MODEL_DB_SET_POLYGON_THRESHOLD),
    JS_CFUNC_MAGIC_DEF("getPolygonThreshold", 0, js_text_detection_model_db_method, TEXT_DETECTION_MODEL_DB_GET_POLYGON_THRESHOLD),
    JS_CFUNC_MAGIC_DEF("setUnclipRatio", 1, js_text_detection_model_db_method, TEXT_DETECTION_MODEL_DB_SET_UNCLIP_RATIO),
    JS_CFUNC_MAGIC_DEF("getUnclipRatio", 0, js_text_detection_model_db_method, TEXT_DETECTION_MODEL_DB_GET_UNCLIP_RATIO),
    JS_CFUNC_MAGIC_DEF("setMaxCandidates", 1, js_text_detection_model_db_method, TEXT_DETECTION_MODEL_DB_SET_MAX_CANDIDATES),
    JS_CFUNC_MAGIC_DEF("getMaxCandidates", 0, js_text_detection_model_db_method, TEXT_DETECTION_MODEL_DB_GET_MAX_CANDIDATES),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "TextDetectionModel_DB", JS_PROP_CONFIGURABLE),
};

#define REGISTER_DNN_MODEL_CLASS(tag, JsName)                                                             \
  do {                                                                                                     \
    JS_NewClassID(&js_##tag##_class_id);                                                                    \
    JS_NewClass(JS_GetRuntime(ctx), js_##tag##_class_id, &js_##tag##_class);                                  \
    tag##_proto = JS_NewObject(ctx);                                                                           \
    JS_SetPropertyFunctionList(ctx, tag##_proto, js_##tag##_proto_funcs, countof(js_##tag##_proto_funcs));       \
    JS_SetClassProto(ctx, js_##tag##_class_id, tag##_proto);                                                       \
    tag##_class = JS_NewCFunction2(ctx, js_##tag##_constructor, JsName, 0, JS_CFUNC_constructor, 0);                 \
    JS_SetConstructor(ctx, tag##_class, tag##_proto);                                                                  \
    JS_SetPropertyStr(ctx, dnn_object, JsName, tag##_class);                                                             \
  } while(0)

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

#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
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
#endif

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

#ifdef HAVE_OPENCV_DNN_NEW_ENGINE
          int32_t engine = cv::dnn::ENGINE_AUTO;
          if(argc > argi)
            js_value_to(ctx, argv[argi++], engine);

          ret = js_net_new(ctx, cv::dnn::readNetFromONNX(reinterpret_cast<const char*>(ptr), size, engine));
        } else {
          std::string onnxFile;

          js_value_to(ctx, argv[0], onnxFile);

          int32_t engine = cv::dnn::ENGINE_AUTO;
          if(argc > 1)
            js_value_to(ctx, argv[1], engine);

          ret = js_net_new(ctx, cv::dnn::readNetFromONNX(onnxFile, engine));
        }
#else
          ret = js_net_new(ctx, cv::dnn::readNetFromONNX(reinterpret_cast<const char*>(ptr), size));
        } else {
          std::string onnxFile;

          js_value_to(ctx, argv[0], onnxFile);

          ret = js_net_new(ctx, cv::dnn::readNetFromONNX(onnxFile));
        }
#endif

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

#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
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
#endif

      case DNN_READTENSORFROMONNX: {
        std::string path;

        js_value_to(ctx, argv[0], path);

        ret = js_value_from(ctx, cv::dnn::readTensorFromONNX(path));
        break;
      }

#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
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
#endif

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

// Functions that also exist in opencv.js - exported at module level
const JSCFunctionListEntry js_dnn_flat_funcs[] = {
    JS_CFUNC_MAGIC_DEF("blobFromImage", 1, js_dnn_func, DNN_BLOBFROMIMAGE),
    JS_CFUNC_MAGIC_DEF("readNet", 1, js_dnn_func, DNN_READNET),
    JS_CFUNC_MAGIC_DEF("readNetFromONNX", 1, js_dnn_func, DNN_READNETFROMONNX),
    JS_CFUNC_MAGIC_DEF("readNetFromTensorflow", 1, js_dnn_func, DNN_READNETFROMTENSORFLOW),
    JS_CFUNC_MAGIC_DEF("readNetFromTFLite", 1, js_dnn_func, DNN_READNETFROMTFLITE),
};

// Functions that are qjs-opencv extensions - stay in dnn namespace
const JSCFunctionListEntry js_dnn_dnn_funcs[] = {
    JS_PROP_INT32_DEF("DNN_BACKEND_DEFAULT", cv::dnn::DNN_BACKEND_DEFAULT, JS_PROP_ENUMERABLE),
#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
    JS_PROP_INT32_DEF("DNN_BACKEND_HALIDE", cv::dnn::DNN_BACKEND_HALIDE, JS_PROP_ENUMERABLE),
#endif
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
    JS_PROP_INT32_DEF("DNN_LAYOUT_UNKNOWN", qjs_dnn_compat::DNN_LAYOUT_UNKNOWN, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_ND", qjs_dnn_compat::DNN_LAYOUT_ND, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_NCHW", qjs_dnn_compat::DNN_LAYOUT_NCHW, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_NCDHW", qjs_dnn_compat::DNN_LAYOUT_NCDHW, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_NHWC", qjs_dnn_compat::DNN_LAYOUT_NHWC, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_NDHWC", qjs_dnn_compat::DNN_LAYOUT_NDHWC, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_LAYOUT_PLANAR", qjs_dnn_compat::DNN_LAYOUT_PLANAR, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_PMODE_NULL", cv::dnn::DNN_PMODE_NULL, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_PMODE_CROP_CENTER", cv::dnn::DNN_PMODE_CROP_CENTER, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DNN_PMODE_LETTERBOX", cv::dnn::DNN_PMODE_LETTERBOX, JS_PROP_ENUMERABLE),
#ifdef HAVE_OPENCV_DNN_NEW_ENGINE
    JS_PROP_INT32_DEF("ENGINE_CLASSIC", cv::dnn::ENGINE_CLASSIC, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("ENGINE_NEW", cv::dnn::ENGINE_NEW, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("ENGINE_AUTO", cv::dnn::ENGINE_AUTO, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("ENGINE_ORT", cv::dnn::ENGINE_ORT, JS_PROP_ENUMERABLE),
#endif

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
#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
    JS_CFUNC_MAGIC_DEF("readNetFromCaffe", 1, js_dnn_func, DNN_READNETFROMCAFFE),
    JS_CFUNC_MAGIC_DEF("readNetFromDarknet", 1, js_dnn_func, DNN_READNETFROMDARKNET),
#endif
    JS_CFUNC_MAGIC_DEF("readNetFromModelOptimizer", 1, js_dnn_func, DNN_READNETFROMMODELOPTIMIZER),
#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
    JS_CFUNC_MAGIC_DEF("readNetFromTorch", 1, js_dnn_func, DNN_READNETFROMTORCH),
#endif
    JS_CFUNC_MAGIC_DEF("readTensorFromONNX", 1, js_dnn_func, DNN_READTENSORFROMONNX),
#ifndef HAVE_OPENCV_DNN_NEW_ENGINE
    JS_CFUNC_MAGIC_DEF("readTorchBlob", 1, js_dnn_func, DNN_READTORCHBLOB),
    JS_CFUNC_MAGIC_DEF("shrinkCaffeModel", 1, js_dnn_func, DNN_SHRINKCAFFEMODEL),
#endif
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

  /* create the Layer class */
  JS_NewClassID(&js_layer_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_layer_class_id, &js_layer_class);

  layer_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, layer_proto, js_layer_proto_funcs, countof(js_layer_proto_funcs));
  JS_SetClassProto(ctx, js_layer_class_id, layer_proto);

  layer_class = JS_NewCFunction2(ctx, js_layer_constructor, "Layer", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, layer_class, layer_proto);

  JS_SetPropertyStr(ctx, dnn_object, "Net", net_class);
  JS_SetPropertyStr(ctx, dnn_object, "Image2BlobParams", imageblob2params_class);
  JS_SetPropertyStr(ctx, dnn_object, "Layer", layer_class);

#ifdef HAVE_OPENCV_DNN_TOKENIZER
  /* create the Tokenizer class */
  JS_NewClassID(&js_tokenizer_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_tokenizer_class_id, &js_tokenizer_class);

  tokenizer_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, tokenizer_proto, js_tokenizer_proto_funcs, countof(js_tokenizer_proto_funcs));
  JS_SetClassProto(ctx, js_tokenizer_class_id, tokenizer_proto);

  tokenizer_class = JS_NewCFunction2(ctx, js_tokenizer_constructor, "Tokenizer", 0, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, tokenizer_class, tokenizer_proto);
  JS_SetPropertyFunctionList(ctx, tokenizer_class, js_tokenizer_static_funcs, countof(js_tokenizer_static_funcs));

  JS_SetPropertyStr(ctx, dnn_object, "Tokenizer", tokenizer_class);
#endif

  /* create the Model family classes (Model + 7 convenience subclasses) */
  REGISTER_DNN_MODEL_CLASS(model, "Model");
  REGISTER_DNN_MODEL_CLASS(classification_model, "ClassificationModel");
  REGISTER_DNN_MODEL_CLASS(detection_model, "DetectionModel");
  REGISTER_DNN_MODEL_CLASS(segmentation_model, "SegmentationModel");
  REGISTER_DNN_MODEL_CLASS(keypoints_model, "KeypointsModel");
  REGISTER_DNN_MODEL_CLASS(text_recognition_model, "TextRecognitionModel");
  REGISTER_DNN_MODEL_CLASS(text_detection_model_east, "TextDetectionModel_EAST");
  REGISTER_DNN_MODEL_CLASS(text_detection_model_db, "TextDetectionModel_DB");

  JS_SetPropertyFunctionList(ctx, dnn_object, js_dnn_dnn_funcs, countof(js_dnn_dnn_funcs));

  if(m) {
    JS_SetModuleExport(ctx, m, "dnn", dnn_object);
    JS_SetModuleExportList(ctx, m, js_dnn_flat_funcs, countof(js_dnn_flat_funcs));
  }

  return 0;
}

extern "C" void
js_dnn_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "dnn");
  JS_AddModuleExportList(ctx, m, js_dnn_flat_funcs, countof(js_dnn_flat_funcs));
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
