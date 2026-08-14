#ifndef JS_POINTVECTORVECTOR_HPP
#define JS_POINTVECTORVECTOR_HPP

#include <quickjs.h>

#ifdef __cplusplus
extern "C" {
#endif

extern "C" int js_pointvectorvector_init(JSContext* ctx, JSModuleDef* m);
extern "C" void js_pointvectorvector_export(JSContext* ctx, JSModuleDef* m);

#ifdef __cplusplus
}
#endif

#endif /* JS_POINTVECTORVECTOR_HPP */
