#ifndef JS_MATVECTOR_HPP
#define JS_MATVECTOR_HPP

#include <quickjs.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern "C" int js_matvector_init(JSContext*, JSModuleDef*);

extern const JSCFunctionListEntry js_matvector_funcs[];
extern size_t js_matvector_func_count;

#ifdef __cplusplus
}
#endif

#endif /* JS_MATVECTOR_HPP */
