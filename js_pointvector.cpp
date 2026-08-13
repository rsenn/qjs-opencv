#include "js_pointvector.hpp"
#include "include/js_vector.hpp"
#include <quickjs.h>

extern "C" int js_pointvector_init(JSContext* ctx, JSModuleDef* m) {
    // Register the vector type
    int result = js_register_vector<cv::Point>(ctx, m, "PointVector");

    if (result == 0 && m) {
        // Get the constructor and export it
        JSValue& ctor = js_vector_get_ctor<cv::Point>();
        if (!JS_IsUndefined(ctor)) {
            JS_SetModuleExport(ctx, m, "PointVector", JS_DupValue(ctx, ctor));
        }
    }

    return result;
}

extern "C" void js_pointvector_export(JSContext* ctx, JSModuleDef* m) {
    if (m) {
        JS_AddModuleExport(ctx, m, "PointVector");
    }
}
