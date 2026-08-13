#include "js_keypointvector.hpp"
#include "js_keypoint.hpp"  // Must include before js_vector.hpp to get JSConverter<cv::KeyPoint>
#include "include/js_vector.hpp"
#include <quickjs.h>

extern "C" int js_keypointvector_init(JSContext* ctx, JSModuleDef* m) {
    int result = js_register_vector<cv::KeyPoint>(ctx, m, "KeyPointVector");
    js_set_vector_export<cv::KeyPoint>(ctx, m, "KeyPointVector");
    return result;
}

extern "C" void js_keypointvector_export(JSContext* ctx, JSModuleDef* m) {
    js_export_vector<cv::KeyPoint>(ctx, m, "KeyPointVector");
}
