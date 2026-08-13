#include "js_primitivevectors.hpp"
#include "include/js_vector.hpp"
#include <quickjs.h>
#include <string>

extern "C" int js_primitivevectors_init(JSContext* ctx, JSModuleDef* m) {
    int result = 0;
    
    // IntVector
    result = js_register_vector<int>(ctx, m, "IntVector");
    if (result == 0 && m) {
        JSValue& ctor = js_vector_get_ctor<int>();
        if (!JS_IsUndefined(ctor)) {
            JS_SetModuleExport(ctx, m, "IntVector", JS_DupValue(ctx, ctor));
        }
    }
    
    // FloatVector
    if (result == 0) {
        result = js_register_vector<float>(ctx, m, "FloatVector");
        if (result == 0 && m) {
            JSValue& ctor = js_vector_get_ctor<float>();
            if (!JS_IsUndefined(ctor)) {
                JS_SetModuleExport(ctx, m, "FloatVector", JS_DupValue(ctx, ctor));
            }
        }
    }
    
    // DoubleVector
    if (result == 0) {
        result = js_register_vector<double>(ctx, m, "DoubleVector");
        if (result == 0 && m) {
            JSValue& ctor = js_vector_get_ctor<double>();
            if (!JS_IsUndefined(ctor)) {
                JS_SetModuleExport(ctx, m, "DoubleVector", JS_DupValue(ctx, ctor));
            }
        }
    }
    
    // CharVector
    if (result == 0) {
        result = js_register_vector<char>(ctx, m, "CharVector");
        if (result == 0 && m) {
            JSValue& ctor = js_vector_get_ctor<char>();
            if (!JS_IsUndefined(ctor)) {
                JS_SetModuleExport(ctx, m, "CharVector", JS_DupValue(ctx, ctor));
            }
        }
    }
    
    // StringVector
    if (result == 0) {
        result = js_register_vector<std::string>(ctx, m, "StringVector");
        if (result == 0 && m) {
            JSValue& ctor = js_vector_get_ctor<std::string>();
            if (!JS_IsUndefined(ctor)) {
                JS_SetModuleExport(ctx, m, "StringVector", JS_DupValue(ctx, ctor));
            }
        }
    }

    return result;
}

extern "C" void js_primitivevectors_export(JSContext* ctx, JSModuleDef* m) {
    if (m) {
        JS_AddModuleExport(ctx, m, "IntVector");
        JS_AddModuleExport(ctx, m, "FloatVector");
        JS_AddModuleExport(ctx, m, "DoubleVector");
        JS_AddModuleExport(ctx, m, "CharVector");
        JS_AddModuleExport(ctx, m, "StringVector");
    }
}
