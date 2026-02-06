#include "cutils.h"
#include "js_alloc.hpp"
#include "include/js_array.hpp"
#include "js_line.hpp"
#include "js_umat.hpp"
#include "jsbindings.hpp"
#include <opencv2/core/cvstd_wrapper.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/matx.hpp>
#include <quickjs.h>
#include <stddef.h>
#ifdef HAVE_OPENCV2_XIMGPROC_HPP
#include <opencv2/ximgproc.hpp>
#include <opencv2/ximgproc/fast_line_detector.hpp>
#include <cstdint>
#include <vector>

typedef cv::Ptr<cv::ximgproc::FastLineDetector> JSFastLineDetector;

extern "C" int js_fast_line_detector_init(JSContext*, JSModuleDef*);

extern "C" {
thread_local JSValue fast_line_detector_proto = JS_UNDEFINED, fast_line_detector_class = JS_UNDEFINED;
thread_local JSClassID js_fast_line_detector_class_id = 0;
}

JSValue
js_fast_line_detector_new(JSContext* ctx,
                          JSValueConst proto,
                          int length_threshold = 10,
                          float distance_threshold = 1.414213562f,
                          double canny_th1 = 50.0,
                          double canny_th2 = 50.0,
                          int canny_aperture_size = 3,
                          bool do_merge = false) {
  JSValue ret;
  JSFastLineDetector* s;
  cv::Ptr<cv::ximgproc::FastLineDetector> ptr;

  ret = JS_NewObjectProtoClass(ctx, proto, js_fast_line_detector_class_id);
  s = js_allocate<JSFastLineDetector>(ctx);

  ptr = cv::ximgproc::createFastLineDetector(length_threshold, distance_threshold, canny_th1, canny_th2, canny_aperture_size, do_merge);
  *s = ptr;
  JS_SetOpaque(ret, s);

  return ret;
}

static JSValue
js_fast_line_detector_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {

  JSValue proto;
  int32_t length_threshold = 10;
  double distance_threshold = 1.414213562;
  double canny_th1 = 50.0;
  double canny_th2 = 50.0;
  int32_t canny_aperture_size = 3;
  BOOL do_merge = FALSE;

  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    return JS_EXCEPTION;

  if(argc >= 1)
    JS_ToInt32(ctx, &length_threshold, argv[0]);
  if(argc >= 2)
    JS_ToFloat64(ctx, &distance_threshold, argv[1]);
  if(argc >= 3)
    JS_ToFloat64(ctx, &canny_th1, argv[2]);
  if(argc >= 4)
    JS_ToFloat64(ctx, &canny_th1, argv[3]);
  if(argc >= 5)
    JS_ToInt32(ctx, &canny_aperture_size, argv[4]);
  if(argc >= 6)
    do_merge = JS_ToBool(ctx, argv[5]);

  return js_fast_line_detector_new(ctx, proto, length_threshold, distance_threshold, canny_th1, canny_th2, canny_aperture_size, do_merge);
}

JSFastLineDetector*
js_fast_line_detector_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSFastLineDetector*>(JS_GetOpaque2(ctx, val, js_fast_line_detector_class_id));
}

void
js_fast_line_detector_finalizer(JSRuntime* rt, JSValue val) {
  JSFastLineDetector* s = static_cast<JSFastLineDetector*>(JS_GetOpaque(val, js_fast_line_detector_class_id));
  cv::ximgproc::FastLineDetector* lsd = s->get();

  lsd->~FastLineDetector();

  js_deallocate(rt, s);
}

static JSValue
js_fast_line_detector_detect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSFastLineDetector* s;
  JSInputArray image;
  JSOutputArray lines;

  if((s = js_fast_line_detector_data2(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  image = js_umat_or_mat(ctx, argv[0]);
  lines = js_cv_outputarray(ctx, argv[1]);

  (*s)->detect(image, lines);

  /*if(argc >= 2 && js_is_array(ctx, argv[1])) {
    size_t i, length = lines.size();

    for(i = 0; i < length; i++) {
      JSLineData<float> line(js_line_from(lines[i]));

      JS_SetPropertyUint32(ctx, argv[1], i, js_line_new(ctx, line));
    }
  }*/

  return JS_UNDEFINED;
}

static JSValue
js_fast_line_detector_draw_segments(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSFastLineDetector* s;
  JSInputOutputArray image;
  // std::vector<cv::Vec4f> lines;
  cv::Mat mat;
  JSInputArray lines;
  BOOL draw_arrow = FALSE;
  cv::Scalar linecolor{255, 255, 255, 255};
  int32_t linethickness = 1;

  if((s = js_fast_line_detector_data2(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  image = js_umat_or_mat(ctx, argv[0]);
  lines = js_input_array(ctx, argv[1]);

  // js_array_to(ctx, argv[1], lines);

  if(argc > 2)
    js_value_to(ctx, argv[2], draw_arrow);
  if(argc > 3)
    js_color_read(ctx, argv[3], &linecolor);
  if(argc > 4)
    js_value_to(ctx, argv[4], linethickness);

  (*s)->drawSegments(image, lines, draw_arrow, linecolor, linethickness);

  return JS_UNDEFINED;
}

JSClassDef js_fast_line_detector_class = {
    .class_name = "FastLineDetector",
    .finalizer = js_fast_line_detector_finalizer,
};

const JSCFunctionListEntry js_fast_line_detector_proto_funcs[] = {
    JS_CFUNC_DEF("detect", 2, js_fast_line_detector_detect),
    JS_CFUNC_DEF("drawSegments", 2, js_fast_line_detector_draw_segments),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "FastLineDetector", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_fast_line_detector_static_funcs[] = {};

extern "C" int
js_fast_line_detector_init(JSContext* ctx, JSModuleDef* m) {

  if(js_fast_line_detector_class_id == 0) {
    /* create the FastLineDetector class */
    JS_NewClassID(&js_fast_line_detector_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_fast_line_detector_class_id, &js_fast_line_detector_class);

    fast_line_detector_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, fast_line_detector_proto, js_fast_line_detector_proto_funcs, countof(js_fast_line_detector_proto_funcs));
    JS_SetClassProto(ctx, js_fast_line_detector_class_id, fast_line_detector_proto);

    fast_line_detector_class = JS_NewCFunction2(ctx, js_fast_line_detector_constructor, "FastLineDetector", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, fast_line_detector_class, fast_line_detector_proto);
    JS_SetPropertyFunctionList(ctx, fast_line_detector_class, js_fast_line_detector_static_funcs, countof(js_fast_line_detector_static_funcs));
  }

  if(m)
    JS_SetModuleExport(ctx, m, "FastLineDetector", fast_line_detector_class);

  return 0;
}

extern "C" void
js_fast_line_detector_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "FastLineDetector");
}

void
js_fast_line_detector_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(fast_line_detector_class))
    js_fast_line_detector_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "FastLineDetector", fast_line_detector_class);
}

#if defined(JS_FAST_LINE_DETECTOR_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_fast_line_detector
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_fast_line_detector_init)))
    return NULL;

  js_fast_line_detector_export(ctx, m);
  return m;
}
#endif
