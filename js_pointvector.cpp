#include "js_pointvector.hpp"
#include "include/js_vector.hpp"
#include <quickjs.h>

extern "C" int js_pointvector_init(JSContext* ctx, JSModuleDef* m) {
    int result = js_register_vector<cv::Point>(ctx, m, "PointVector");
    js_set_vector_export<cv::Point>(ctx, m, "PointVector");
    return result;
}

extern "C" void js_pointvector_export(JSContext* ctx, JSModuleDef* m) {
    js_export_vector<cv::Point>(ctx, m, "PointVector");
}
