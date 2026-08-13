#include "js_primitivevectors.hpp"
#include "include/js_vector.hpp"
#include <quickjs.h>
#include <string>

extern "C" int js_primitivevectors_init(JSContext* ctx, JSModuleDef* m) {
    int result = 0;

    result = js_register_vector<int>(ctx, m, "IntVector");
    js_set_vector_export<int>(ctx, m, "IntVector");
    if (result == 0) {
        result = js_register_vector<float>(ctx, m, "FloatVector");
        js_set_vector_export<float>(ctx, m, "FloatVector");
    }
    if (result == 0) {
        result = js_register_vector<double>(ctx, m, "DoubleVector");
        js_set_vector_export<double>(ctx, m, "DoubleVector");
    }
    if (result == 0) {
        result = js_register_vector<char>(ctx, m, "CharVector");
        js_set_vector_export<char>(ctx, m, "CharVector");
    }
    if (result == 0) {
        result = js_register_vector<std::string>(ctx, m, "StringVector");
        js_set_vector_export<std::string>(ctx, m, "StringVector");
    }

    return result;
}

extern "C" void js_primitivevectors_export(JSContext* ctx, JSModuleDef* m) {
    js_export_vector<int>(ctx, m, "IntVector");
    js_export_vector<float>(ctx, m, "FloatVector");
    js_export_vector<double>(ctx, m, "DoubleVector");
    js_export_vector<char>(ctx, m, "CharVector");
    js_export_vector<std::string>(ctx, m, "StringVector");
}
