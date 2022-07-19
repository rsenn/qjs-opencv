#ifndef JS_LIBCAMERA_APP_HPP
#define JS_LIBCAMERA_APP_HPP

#include "jsbindings.hpp"
#include <quickjs.h>
#include <lccv.hpp>

extern "C" {
extern JSValue libcamera_app_proto, libcamera_app_class, libcamera_app_options_proto, libcamera_app_options_class;
extern thread_local VISIBLE JSClassID js_libcamera_app_class_id, js_libcamera_app_options_class_id;
}

extern "C" VISIBLE int js_libcamera_app_init(JSContext*, JSModuleDef*);

class LibcameraApp;
class Options;

typedef LibcameraApp JSLibcameraAppData;
typedef Options JSLibcameraAppOptionsData;

VISIBLE JSValue js_libcamera_app_options_wrap(JSContext* ctx, JSLibcameraAppOptionsData* opt);

VISIBLE JSLibcameraAppData* js_libcamera_app_data2(JSContext*, JSValueConst val);
VISIBLE JSLibcameraAppData* js_libcamera_app_data(JSValueConst val);

#endif /* defined(JS_LIBCAMERA_APP_HPP) */
