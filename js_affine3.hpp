#ifndef JS_AFFINE3_HPP
#define JS_AFFINE3_HPP

#include "jsbindings.hpp"
#include <quickjs.h>
#include <array>

extern "C" {

extern JSValue affine3_proto, affine3_class;
extern thread_local VISIBLE JSClassID js_affine3_class_id;

VISIBLE JSValue js_affine3_new(JSContext* ctx, double x1, double y1, double x2, double y2);
JSModuleDef* js_init_module_affine3(JSContext*, const char*);

VISIBLE JSAffine3Data<double>* js_affine3_data2(JSContext*, JSValueConst val);
VISIBLE JSAffine3Data<double>* js_affine3_data(JSValueConst val);

VISIBLE int js_affine3_init(JSContext*, JSModuleDef*);
}
 
extern "C" int js_affine3_init(JSContext*, JSModuleDef*);

VISIBLE JSValue js_affine3_wrap(JSContext* ctx, const JSAffine3Data<double>& affine3);
VISIBLE JSValue js_affine3_wrap(JSContext* ctx, const JSAffine3Data<int>& affine3);

VISIBLE JSAffine3Data<double>* js_affine3_data(JSValueConst val);
VISIBLE JSAffine3Data<double>* js_affine3_data2(JSContext*, JSValueConst val);

#endif /* defined(JS_AFFINE3_HPP) */
