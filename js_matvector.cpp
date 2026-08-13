#include "js_matvector.hpp"
#include "include/js_vector.hpp"
#include <quickjs.h>
#include <cstdio>

extern "C" int js_matvector_init(JSContext* ctx, JSModuleDef* m) {
    // Register the vector type (this creates the constructor internally)
    int result = js_register_vector<cv::Mat>(ctx, m, "MatVector");

    if (result == 0 && m) {
        // Get the constructor that was stored by js_register_vector
        JSValue& ctor = js_vector_get_ctor<cv::Mat>();
        
        // Export during init phase
        if (!JS_IsUndefined(ctor)) {
            JS_SetModuleExport(ctx, m, "MatVector", JS_DupValue(ctx, ctor));
        }
    }

    return result;
}

extern "C" void js_matvector_export(JSContext* ctx, JSModuleDef* m) {
    // Just declare the export during export phase
    if (m) {
        JS_AddModuleExport(ctx, m, "MatVector");
    }
}
