#include "js_keypointvector.hpp"
#include "js_keypoint.hpp"  // Must include before js_vector.hpp to get JSConverter<cv::KeyPoint>
#include "include/js_vector.hpp"
#include <quickjs.h>

extern "C" int js_keypointvector_init(JSContext* ctx, JSModuleDef* m) {
    int result = js_register_vector<cv::KeyPoint>(ctx, m, "KeyPointVector");

    if (result == 0 && m) {
        JSValue& ctor = js_vector_get_ctor<cv::KeyPoint>();
        if (!JS_IsUndefined(ctor)) {
            JS_SetModuleExport(ctx, m, "KeyPointVector", JS_DupValue(ctx, ctor));
        }
    }

    return result;
}

extern "C" void js_keypointvector_export(JSContext* ctx, JSModuleDef* m) {
    if (m) {
        JS_AddModuleExport(ctx, m, "KeyPointVector");
    }
}
