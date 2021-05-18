#include "jsbindings.hpp"
#include "js_alloc.hpp"

#include <opencv2/imgproc.hpp>

extern "C" VISIBLE int js_line_segment_detector_init(JSContext*, JSModuleDef*);

extern "C" {
JSValue line_segment_detector_proto = JS_UNDEFINED, line_segment_detector_class = JS_UNDEFINED;
JSClassID js_line_segment_detector_class_id = 0;
}

JSValue
js_line_segment_detector_new(JSContext* ctx) {
  JSValue ret;

  cv::Ptr<cv::LineSegmentDetector>* s;

  ret = JS_NewObjectProtoClass(ctx, line_segment_detector_proto, js_line_segment_detector_class_id);
  s = js_allocate<cv::Ptr<cv::LineSegmentDetector>>(ctx);

  *s = cv::createLineSegmentDetector();

  JS_SetOpaque(ret, s);

  return ret;
}

static JSValue
js_line_segment_detector_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  return js_line_segment_detector_new(ctx);
}

cv::LineSegmentDetector*
js_line_segment_detector_data(JSContext* ctx, JSValueConst val) {
  return static_cast<cv::LineSegmentDetector*>(JS_GetOpaque2(ctx, val, js_line_segment_detector_class_id));
}

void
js_line_segment_detector_finalizer(JSRuntime* rt, JSValue val) {
  cv::LineSegmentDetector* s = static_cast<cv::LineSegmentDetector*>(JS_GetOpaque(val, js_line_segment_detector_class_id));

  s->~LineSegmentDetector();
  js_deallocate(rt, s);
}

static JSValue
js_line_segment_detector_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  cv::LineSegmentDetector* s;
  JSValue ret = JS_UNDEFINED;

  if((s = js_line_segment_detector_data(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  switch(magic) {}
  return ret;
}

JSClassDef js_line_segment_detector_class = {
    .class_name = "LineSegmentDetector",
    .finalizer = js_line_segment_detector_finalizer,
};

const JSCFunctionListEntry js_line_segment_detector_proto_funcs[] = {
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
    JS_SetPropertyFunctionList(ctx,
                               line_segment_detector_proto,
                               js_line_segment_detector_proto_funcs,
                               countof(js_line_segment_detector_proto_funcs));
    JS_SetClassProto(ctx, js_line_segment_detector_class_id, line_segment_detector_proto);

    line_segment_detector_class =
        JS_NewCFunction2(ctx, js_line_segment_detector_ctor, "LineSegmentDetector", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, line_segment_detector_class, line_segment_detector_proto);
    JS_SetPropertyFunctionList(ctx,
                               line_segment_detector_class,
                               js_line_segment_detector_static_funcs,
                               countof(js_line_segment_detector_static_funcs));
  }

  if(m)
    JS_SetModuleExport(ctx, m, "LineSegmentDetector", line_segment_detector_class);

  return 0;
}

extern "C" VISIBLE void
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
  m = JS_NewCModule(ctx, module_name, &js_line_segment_detector_init);
  if(!m)
    return NULL;
  js_line_segment_detector_export(ctx, m);
  return m;
}