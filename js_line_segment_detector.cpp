#include "js_alloc.hpp"
#include "js_array.hpp"
#include "js_line.hpp"
#include "js_size.hpp"
#include "js_umat.hpp"
#include "jsbindings.hpp"
#include <opencv2/core/cvstd_wrapper.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/matx.hpp>
#include <quickjs.h>
#include <stddef.h>
#include <cstdint>
#include <opencv2/imgproc.hpp>
#include <vector>

typedef cv::Ptr<cv::LineSegmentDetector> JSLineSegmentDetector;

extern "C" int js_line_segment_detector_init(JSContext*, JSModuleDef*);

extern "C" {
JSValue line_segment_detector_proto = JS_UNDEFINED, line_segment_detector_class = JS_UNDEFINED;
thread_local JSClassID js_line_segment_detector_class_id = 0;
}

JSValue
js_line_segment_detector_new(JSContext* ctx,
                             JSValueConst proto,
                             int refine = cv::LSD_REFINE_STD,
                             double scale = 0.8,
                             double sigma_scale = 0.6,
                             double quant = 2.0,
                             double ang_th = 22.5,
                             double log_eps = 0,
                             double density_th = 0.7,
                             int n_bins = 1024) {
  JSValue ret;
  JSLineSegmentDetector* s;
  cv::Ptr<cv::LineSegmentDetector> ptr;

  ret = JS_NewObjectProtoClass(ctx, proto, js_line_segment_detector_class_id);
  s = js_allocate<JSLineSegmentDetector>(ctx);

  ptr = cv::createLineSegmentDetector(refine, scale, sigma_scale, quant, ang_th, log_eps, density_th, n_bins);
  *s = ptr;
  JS_SetOpaque(ret, s);

  return ret;
}

static JSValue
js_line_segment_detector_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {

  JSValue proto;
  int32_t refine = cv::LSD_REFINE_STD, n_bins = 1024;
  double scale = 0.8, sigma_scale = 0.6, quant = 2.0, ang_th = 22.5, log_eps = 0, density_th = 0.7;

  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    return JS_EXCEPTION;

  if(argc >= 1)
    JS_ToInt32(ctx, &refine, argv[0]);
  if(argc >= 2)
    JS_ToFloat64(ctx, &scale, argv[1]);
  if(argc >= 3)
    JS_ToFloat64(ctx, &sigma_scale, argv[2]);
  if(argc >= 4)
    JS_ToFloat64(ctx, &quant, argv[3]);
  if(argc >= 5)
    JS_ToFloat64(ctx, &ang_th, argv[4]);
  if(argc >= 6)
    JS_ToFloat64(ctx, &log_eps, argv[5]);
  if(argc >= 7)
    JS_ToFloat64(ctx, &density_th, argv[6]);
  if(argc >= 8)
    JS_ToInt32(ctx, &n_bins, argv[7]);

  return js_line_segment_detector_new(ctx, proto, refine, scale, sigma_scale, quant, ang_th, log_eps, density_th, n_bins);
}

JSLineSegmentDetector*
js_line_segment_detector_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSLineSegmentDetector*>(JS_GetOpaque2(ctx, val, js_line_segment_detector_class_id));
}

void
js_line_segment_detector_finalizer(JSRuntime* rt, JSValue val) {
  JSLineSegmentDetector* s = static_cast<JSLineSegmentDetector*>(JS_GetOpaque(val, js_line_segment_detector_class_id));
  cv::LineSegmentDetector* lsd = s->get();

  lsd->~LineSegmentDetector();

  js_deallocate(rt, s);
}

static JSValue
js_line_segment_detector_compare_segments(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSLineSegmentDetector* s;
  JSSizeData<int> size;
  std::vector<cv::Vec4i> lines1, lines2;
  JSInputOutputArray image;
  int32_t ret;

  if((s = js_line_segment_detector_data2(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  js_size_read(ctx, argv[0], &size);

  js_array_to(ctx, argv[1], lines1);
  js_array_to(ctx, argv[2], lines2);

  image = argc >= 4 ? js_umat_or_mat(ctx, argv[3]) : cv::noArray();

  ret = (*s)->compareSegments(size, lines1, lines2, image);

  return JS_NewInt32(ctx, ret);
}

static JSValue
js_line_segment_detector_detect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSLineSegmentDetector* s;
  JSInputArray image;
  std::vector<cv::Vec4f> lines;
  std::vector<float> width, prec;
  std::vector<int32_t> nfa;
  JSOutputArray linearr = js_cv_outputarray(ctx, argv[1]);
  bool copyArray = !linearr.isVector() && !linearr.isMat();

  if((s = js_line_segment_detector_data2(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  if(copyArray)
    linearr = JSOutputArray(lines);

  image = js_umat_or_mat(ctx, argv[0]);

  try {
    (*s)->detect(image, linearr, width, prec, nfa);
  } catch(const cv::Exception& e) { return JS_ThrowInternalError(ctx, "cv::Exception: %s", e.what()); }

  if(copyArray) {
    size_t i, length = lines.size();

    js_array_clear(ctx, argv[1]);

    for(i = 0; i < length; i++) {
      JSLineData<float> line(js_line_from(lines[i]));

      JS_SetPropertyUint32(ctx, argv[1], i, js_line_new(ctx, line));
    }
  }

  if(argc >= 3 && js_is_array(ctx, argv[2])) {
    js_array_clear(ctx, argv[2]);
    js_array_copy(ctx, argv[2], width);
  }
  if(argc >= 4 && js_is_array(ctx, argv[3])) {
    js_array_clear(ctx, argv[3]);
    js_array_copy(ctx, argv[3], prec);
  }
  if(argc >= 5 && js_is_array(ctx, argv[4])) {
    js_array_clear(ctx, argv[4]);
    js_array_copy(ctx, argv[4], nfa);
  }

  return JS_UNDEFINED;
}

static JSValue
js_line_segment_detector_draw_segments(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSLineSegmentDetector* s;
  JSInputOutputArray image;
  JSInputArray linearr;
  std::vector<cv::Vec4f> lines;

  if((s = js_line_segment_detector_data2(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  image = js_umat_or_mat(ctx, argv[0]);
  linearr = js_input_array(ctx, argv[1]);

  if(!linearr.isMat() && !linearr.isVector()) {
    linearr = JSInputArray(lines);
    js_array_to(ctx, argv[1], lines);
  }

  (*s)->drawSegments(image, linearr);

  return JS_UNDEFINED;
}

JSClassDef js_line_segment_detector_class = {
    .class_name = "LineSegmentDetector",
    .finalizer = js_line_segment_detector_finalizer,
};

const JSCFunctionListEntry js_line_segment_detector_proto_funcs[] = {
    JS_CFUNC_DEF("compareSegments", 3, js_line_segment_detector_compare_segments),
    JS_CFUNC_DEF("detect", 2, js_line_segment_detector_detect),
    JS_CFUNC_DEF("drawSegments", 2, js_line_segment_detector_draw_segments),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "LineSegmentDetector", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_line_segment_detector_static_funcs[] = {};

extern "C" int
js_line_segment_detector_init(JSContext* ctx, JSModuleDef* m) {

  if(js_line_segment_detector_class_id == 0) {
    /* create the LineSegmentDetector class */
    JS_NewClassID(&js_line_segment_detector_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_line_segment_detector_class_id, &js_line_segment_detector_class);

    line_segment_detector_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, line_segment_detector_proto, js_line_segment_detector_proto_funcs, countof(js_line_segment_detector_proto_funcs));
    JS_SetClassProto(ctx, js_line_segment_detector_class_id, line_segment_detector_proto);

    line_segment_detector_class = JS_NewCFunction2(ctx, js_line_segment_detector_ctor, "LineSegmentDetector", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, line_segment_detector_class, line_segment_detector_proto);
    JS_SetPropertyFunctionList(ctx, line_segment_detector_class, js_line_segment_detector_static_funcs, countof(js_line_segment_detector_static_funcs));
  }

  if(m)
    JS_SetModuleExport(ctx, m, "LineSegmentDetector", line_segment_detector_class);

  return 0;
}

extern "C" void
js_line_segment_detector_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "LineSegmentDetector");
}

void
js_line_segment_detector_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(line_segment_detector_class))
    js_line_segment_detector_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "LineSegmentDetector", line_segment_detector_class);
}

#if defined(JS_LINE_SEGMENT_DETECTOR_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_line_segment_detector
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  if(!(m = JS_NewCModule(ctx, module_name, &js_line_segment_detector_init)))
    return NULL;
  js_line_segment_detector_export(ctx, m);
  return m;
}
