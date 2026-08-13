#ifndef JS_PRIMITIVEVECTORS_HPP
#define JS_PRIMITIVEVECTORS_HPP

#include <quickjs.h>

#ifdef __cplusplus
extern "C" {
#endif

int js_primitivevectors_init(JSContext* ctx, JSModuleDef* m);
void js_primitivevectors_export(JSContext* ctx, JSModuleDef* m);

#ifdef __cplusplus
}
#endif

#endif // JS_PRIMITIVEVECTORS_HPP
