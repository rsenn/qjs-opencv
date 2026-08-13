#ifndef JS_RECTVECTOR_HPP
#define JS_RECTVECTOR_HPP

#include <quickjs.h>

#ifdef __cplusplus
extern "C" {
#endif

int js_rectvector_init(JSContext* ctx, JSModuleDef* m);
void js_rectvector_export(JSContext* ctx, JSModuleDef* m);

#ifdef __cplusplus
}
#endif

#endif // JS_RECTVECTOR_HPP
