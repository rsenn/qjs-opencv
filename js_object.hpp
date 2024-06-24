#ifndef JS_OBJECT_HPP

#include "util.hpp"
#include <quickjs.h>
#include <cstdint>
#include <map>
#include <string>
#include <utility>

class js_object {
public:
  template<class T>
  static int64_t
  to_map(JSContext* ctx, JSValueConst obj, std::map<std::string, T>& out, int flags = JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK) {
    int64_t i = 0;
    JSPropertyEnum* names = 0;
    uint32_t plen = 0;
    JS_GetOwnPropertyNames(ctx, &names, &plen, obj, flags);
    for(auto penum : range_view<JSPropertyEnum>(names, plen)) {
      JSValue value = JS_GetProperty(ctx, obj, penum.atom);
      const char* name = JS_AtomToCString(ctx, penum.atom);
      T prop;
      js_value_to(ctx, value, prop);
      out[name] = prop;
      JS_FreeCString(ctx, name);
      ++i;
    }

    return i;
  }

  template<class T>
  static JSValue
  from_map(JSContext* ctx, const std::map<std::string, T>& in) {
    typedef std::pair<std::string, T> entry_type;
    JSValue obj = JS_NewObject(ctx);

    for(entry_type entry : in) {
      T prop = entry.second;
      JS_DefinePropertyValueStr(ctx, obj, entry.first.c_str(), js_value_from(ctx, prop), JS_PROP_C_W_E);
    }

    return obj;
  }
};

template<class T>
static inline int64_t
js_object_to(JSContext* ctx, JSValueConst obj, std::map<std::string, T>& map) {
  return js_object::to_map<T>(ctx, obj, map);
}

template<class Container>
static inline JSValue
js_object_from(JSContext* ctx, const Container& v) {
  return js_object::from_map(ctx, v);
}

#endif // defined(JS_OBJECT_HPP)
