#ifndef JS_LIBCAMERA_APP_HPP
#define JS_LIBCAMERA_APP_HPP 1

#include "jsbindings.hpp"
#include <quickjs.h>

extern "C" {
extern thread_local JSValue libcamera_app_proto, libcamera_app_class, libcamera_app_options_proto, libcamera_app_options_class;
extern thread_local JSClassID js_libcamera_app_class_id, js_libcamera_app_options_class_id;
}

extern "C" int js_libcamera_app_init(JSContext*, JSModuleDef*);

class LibcameraApp;
class Options;

typedef LibcameraApp JSLibcameraAppData;
typedef Options JSLibcameraAppOptionsData;

JSValue js_libcamera_app_options_wrap(JSContext* ctx, JSLibcameraAppOptionsData* opt);

JSLibcameraAppData* js_libcamera_app_data2(JSContext*, JSValueConst val);
JSLibcameraAppData* js_libcamera_app_data(JSValueConst val);

#endif /* defined(JS_LIBCAMERA_APP_HPP) */
