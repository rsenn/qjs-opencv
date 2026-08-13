#ifndef JS_KEYPOINTVECTOR_HPP
#define JS_KEYPOINTVECTOR_HPP

#include <quickjs.h>

#ifdef __cplusplus
extern "C" {
#endif

int js_keypointvector_init(JSContext* ctx, JSModuleDef* m);
void js_keypointvector_export(JSContext* ctx, JSModuleDef* m);

#ifdef __cplusplus
}
#endif

#endif // JS_KEYPOINTVECTOR_HPP
