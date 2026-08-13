#ifndef JS_DMATCHVECTOR_HPP
#define JS_DMATCHVECTOR_HPP

#include <quickjs.h>

#ifdef __cplusplus
extern "C" {
#endif

int js_dmatchvector_init(JSContext* ctx, JSModuleDef* m);
void js_dmatchvector_export(JSContext* ctx, JSModuleDef* m);

#ifdef __cplusplus
}
#endif

#endif // JS_DMATCHVECTOR_HPP
