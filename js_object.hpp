#ifndef JS_OBJECT_HPP


class js_object {
public:
  template<class T>
  static int64_t
  to_map(JSContext* ctx, JSValueConst obj, std::map<std::string, T>& out) {
    int64_t i = 0;
    JSPropertyEnum* names;
    uint32_t plen;

    JS_GetOwnPropertyNames(ctx, &names, &plen, obj, JS_GPN_ENUM_ONLY);

    for(auto name : range_view<JSPropertyEnum>(names, plen)) {
      JSValue value = JS_GetProperty(ctx, obj, name.atom);
      T prop;
      js_value_to(ctx, value, prop);
      out[name] = prop;
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
      JS_SetPropertyStr(ctx, obj, entry.first.c_str(), js_value_from(ctx, prop) /*, JS_PROP_C_W_E*/);
    }

    return obj;
  }
};

#endif // defined(JS_OBJECT_HPP)