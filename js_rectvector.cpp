#include "js_rectvector.hpp"
#include "include/js_vector.hpp"
#include <quickjs.h>

extern "C" int js_rectvector_init(JSContext* ctx, JSModuleDef* m) {
    int result = js_register_vector<cv::Rect>(ctx, m, "RectVector");
    js_set_vector_export<cv::Rect>(ctx, m, "RectVector");
    return result;
}

extern "C" void js_rectvector_export(JSContext* ctx, JSModuleDef* m) {
    js_export_vector<cv::Rect>(ctx, m, "RectVector");
}
