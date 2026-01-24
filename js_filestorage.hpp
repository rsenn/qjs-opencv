#ifndef JS_FILESTORAGE_HPP
#define JS_FILESTORAGE_HPP

#include <quickjs.h>
#include <opencv2/core/persistence.hpp>

using JSFileStorageData = cv::FileStorage;

extern "C" int js_filestorage_init(JSContext*, JSModuleDef*);

extern "C" {
JSFileStorageData* js_filestorage_data(JSValueConst val);
JSFileStorageData* js_filestorage_data2(JSContext*, JSValueConst val);
int js_filestorage_init(JSContext*, JSModuleDef*);
JSModuleDef* js_init_module_filestorage(JSContext*, const char*);
}

extern "C" int js_filestorage_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_FILESTORAGE_HPP) */
