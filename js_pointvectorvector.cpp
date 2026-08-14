#include "js_pointvectorvector.hpp"
#include "include/js_vector.hpp"
#include <quickjs.h>

extern "C" int js_pointvectorvector_init(JSContext* ctx, JSModuleDef* m) {
    int result = js_register_vector<std::vector<cv::Point>>(ctx, m, "PointVectorVector");
    js_set_vector_export<std::vector<cv::Point>>(ctx, m, "PointVectorVector");
    return result;
}

extern "C" void js_pointvectorvector_export(JSContext* ctx, JSModuleDef* m) {
    js_export_vector<std::vector<cv::Point>>(ctx, m, "PointVectorVector");
}
