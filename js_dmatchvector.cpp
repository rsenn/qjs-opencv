#include "js_dmatchvector.hpp"
#include "js_dmatch.hpp"
#include "include/js_vector.hpp"
#include <quickjs.h>
#include <stdio.h>

extern "C" int js_dmatchvector_init(JSContext* ctx, JSModuleDef* m) {
    int result = js_register_vector<cv::DMatch>(ctx, m, "DMatchVector");
    JSValue& ctor = js_vector_get_ctor<cv::DMatch>();
    fprintf(stderr, "DEBUG: js_dmatchvector_init result=%d, ctor undefined=%d\n", 
            result, JS_IsUndefined(ctor));
    js_set_vector_export<cv::DMatch>(ctx, m, "DMatchVector");
    return result;
}

extern "C" void js_dmatchvector_export(JSContext* ctx, JSModuleDef* m) {
    JSValue& ctor = js_vector_get_ctor<cv::DMatch>();
    fprintf(stderr, "DEBUG: js_dmatchvector_export ctor undefined=%d\n", 
            JS_IsUndefined(ctor));
    js_export_vector<cv::DMatch>(ctx, m, "DMatchVector");
}
