#ifndef JS_CV_HPP
#define JS_CV_HPP

#include "include/jsbindings.hpp"
#include <quickjs.h>

extern "C" JSValue cv_class;

extern "C" int js_cv_init(JSContext*, JSModuleDef*);

template<class E>
static JSValue
js_cv_throw(JSContext* ctx, const E& e) {
  const char *msg, *what = e.what(), *loc = 0;
  size_t loclen;

  if((msg = strstr(what, "/opencv")))
    what = msg + 1;

  /*if((msg = strstr(what, "/opencv"))) {
    loc = msg + 1;
    if((what = strstr(loc, ": "))) {
      loclen = what - loc;
      what += 2;
    }
  }

  if((msg = strstr(what, "error: ("))) {
    if((msg = strstr(msg, ") ")))
      what = msg + 2;
  }*/

  JSValue ret = JS_NewError(ctx);

  if(loc)
    JS_DefinePropertyValueStr(ctx, ret, "location", JS_NewStringLen(ctx, loc, loclen), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);

  JS_DefinePropertyValueStr(ctx, ret, "message", JS_NewString(ctx, what), JS_PROP_CONFIGURABLE);

  return JS_Throw(ctx, ret);
}

#endif /* defined(JS_CV_HPP) */
