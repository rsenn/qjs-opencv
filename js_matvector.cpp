#include "js_matvector.hpp"
#include "include/js_vector.hpp"
#include <quickjs.h>

extern "C" int js_matvector_init(JSContext* ctx, JSModuleDef* m) {
    int result = js_register_vector<cv::Mat>(ctx, m, "MatVector");
    js_set_vector_export<cv::Mat>(ctx, m, "MatVector");
    return result;
}

extern "C" void js_matvector_export(JSContext* ctx, JSModuleDef* m) {
    js_export_vector<cv::Mat>(ctx, m, "MatVector");
}
