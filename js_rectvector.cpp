#include "js_rectvector.hpp"
#include "include/js_vector.hpp"
#include <quickjs.h>

extern "C" int js_rectvector_init(JSContext* ctx, JSModuleDef* m) {
    int result = js_register_vector<cv::Rect>(ctx, m, "RectVector");

    if (result == 0 && m) {
        JSValue& ctor = js_vector_get_ctor<cv::Rect>();
        if (!JS_IsUndefined(ctor)) {
            JS_SetModuleExport(ctx, m, "RectVector", JS_DupValue(ctx, ctor));
        }
    }

    return result;
}

extern "C" void js_rectvector_export(JSContext* ctx, JSModuleDef* m) {
    if (m) {
        JS_AddModuleExport(ctx, m, "RectVector");
    }
}
