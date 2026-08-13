#ifndef JS_DMATCH_HPP
#define JS_DMATCH_HPP

#include <quickjs.h>
#include <opencv2/features2d.hpp>
#include "include/jsbindings.hpp"

extern "C" {
extern thread_local JSValue dmatch_proto, dmatch_class;
extern thread_local JSClassID js_dmatch_class_id;
}

typedef cv::DMatch JSDMatchData;

JSDMatchData* js_dmatch_data2(JSContext* ctx, JSValueConst val);
JSDMatchData* js_dmatch_data(JSValueConst val);

JSValue js_dmatch_new(JSContext* ctx, const JSDMatchData& dm);
JSValue js_dmatch_wrap(JSContext* ctx, const JSDMatchData& dm);

extern "C" int js_dmatch_init(JSContext* ctx, JSModuleDef* m);
extern "C" void js_dmatch_export(JSContext* ctx, JSModuleDef* m);

#endif /* JS_DMATCH_HPP */
