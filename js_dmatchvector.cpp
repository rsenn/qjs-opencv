#include "js_dmatchvector.hpp"
#include "include/js_vector.hpp"
#include <quickjs.h>

extern "C" int js_dmatchvector_init(JSContext* ctx, JSModuleDef* m) {
    int result = js_register_vector<cv::DMatch>(ctx, m, "DMatchVector");

    if (result == 0 && m) {
        JSValue& ctor = js_vector_get_ctor<cv::DMatch>();
        if (!JS_IsUndefined(ctor)) {
            JS_SetModuleExport(ctx, m, "DMatchVector", JS_DupValue(ctx, ctor));
        }
    }

    return result;
}

extern "C" void js_dmatchvector_export(JSContext* ctx, JSModuleDef* m) {
    if (m) {
        JS_AddModuleExport(ctx, m, "DMatchVector");
    }
}
