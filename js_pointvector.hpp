#ifndef JS_POINTVECTOR_HPP
#define JS_POINTVECTOR_HPP

#include <quickjs.h>

#ifdef __cplusplus
extern "C" {
#endif

int js_pointvector_init(JSContext* ctx, JSModuleDef* m);
void js_pointvector_export(JSContext* ctx, JSModuleDef* m);

#ifdef __cplusplus
}
#endif

#endif // JS_POINTVECTOR_HPP
