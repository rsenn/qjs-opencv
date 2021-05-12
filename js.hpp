#ifndef JS_HPP
#define JS_HPP

#include <errno.h>

extern "C" {
#include "quickjs/quickjs-atom.h"
}

enum {
  JS_NULL,
#define DEF(name, str) JS_ATOM_##name,
#include "quickjs-atom.h"
#undef DEF
  JS_ATOM_END,
};
#include "quickjs/quickjs.h"

#include <unordered_map>
#include <vector>
#include <type_traits>
#include <functional>
#include <string>
#include <cstring>
#include <iostream>
#include <iterator>

struct jsiter;

struct jsatom {

  static jsatom
  create(JSContext* ctx, const char* str) {
    return jsatom(JS_NewAtom(ctx, str));
  }
  static jsatom
  create(JSContext* ctx, uint32_t num) {
    return jsatom(JS_NewAtomUInt32(ctx, num));
  }
  static jsatom
  create(JSContext* ctx, const char* x, size_t n) {
    return jsatom(JS_NewAtomLen(ctx, x, n));
  }
  static jsatom
  create(JSContext* ctx, const jsatom& atom) {
    return jsatom(JS_DupAtom(ctx, atom));
  }
  static jsatom
  create(JSContext* ctx, const JSValueConst& value) {
    return jsatom(JS_ValueToAtom(ctx, value));
  }
  static jsatom
  create(JSContext* ctx, const std::string& str) {
    return create(ctx, str.data(), str.size());
  }

  //~jsatom() { destroy(); }

  static void
  destroy(JSContext* ctx, jsatom& a) {
    JS_FreeAtom(ctx, a._a);
    a._a = JS_ATOM_NULL;
  }

  operator JSAtom() const { return _a; }

  const char*
  to_cstring(JSContext* _ctx) const {
    return JS_AtomToCString(_ctx, _a);
  }

  JSValue
  to_value(JSContext* _ctx) const {
    return JS_AtomToValue(_ctx, _a);
  }

  JSValue
  to_string(JSContext* _ctx) const {
    return JS_AtomToString(_ctx, _a);
  }

private:
  jsatom(JSAtom a) : _a(a) {}

  static jsatom
  create(JSAtom a) {
    jsatom ret(a);
    return ret;
  }

  JSAtom _a;
};

struct jsrt {
  typedef JSValue value;
  typedef JSValueConst const_value;
  typedef JSAtom atom;

  static value _true, _false, _null, _undefined;

  bool init(int argc, char* argv[]);
  bool create(JSContext* ctx = 0);

  jsrt() {}
  jsrt(JSContext* c) : ctx(JS_DupContext(c)) {}
  ~jsrt();

  value
  new_string(const char* str) const {
    return JS_NewString(ctx, str);
  }

  typedef value c_function(jsrt* rt, const_value this_val, int argc, const_value* argv);

  value eval_buf(const char* buf, int buf_len, const char* filename, int eval_flags);
  value eval_file(const char* filename, int module = -1);

  // value add_function(const char* name, JSCFunction* fn, int args = 0);

  template<class T> void get_number(const_value val, T& ref) const;
  void get_string(const_value val, std::string& str) const;
  void get_string(const_value val, const char*& cstr) const;

  template<class T> void get_int_array(const_value val, T& ref) const;
  template<class T> void get_point(const_value val, T& ref) const;
  template<class T> void get_point_array(const_value val, std::vector<T>& ref) const;
  template<class T> void get_rect(const_value val, T& ref) const;
  template<class T> void get_color(const_value val, T& ref) const;

  void
  get_boolean(const_value val, bool& ref) {
    bool b = JS_ToBool(ctx, val);
    ref = b;
  }
  bool
  get_boolean(const_value val) {
    bool b;
    get_boolean(val, b);
    return b;
  }

  value create_array(int32_t size = -1);
  value create_object();
  template<class T> value create(T arg);
  template<class T> value create_point(T x, T y);

  template<class T>
  value
  get_property(const_value obj, T prop) const {
    throw std::runtime_error("template specialization");
  }

  value get_property_atom(const_value obj, atom) const;
  value get_property_symbol(const_value obj, const char* symbol);

  bool has_property(const_value obj, const jsatom& atom) const;

  bool
  has_property(const_value obj, const std::string& name) const {
    jsatom atom = jsatom::create(ctx, name);
    bool ret = has_property(obj, atom);
    jsatom::destroy(ctx, atom);
    return ret;
  }
  bool
  has_property(const_value obj, uint32_t index) const {
    jsatom atom = jsatom::create(ctx, index);
    bool ret = has_property(obj, atom);
    jsatom::destroy(ctx, atom);
    return ret;
  }
  bool
  has_property(const_value obj, const const_value& prop) const {
    jsatom atom = jsatom::create(ctx, prop);
    bool ret = has_property(obj, atom);
    jsatom::destroy(ctx, atom);
    return ret;
  }

  template<class T>
  void
  set_property(const_value obj, T prop, value val) {}

  void set_property(const_value obj, const jsatom& atom, value val, int flags);
  void
  set_property(const_value obj, const std::string& name, value val, int flags) {
    jsatom atom = jsatom::create(ctx, name);
    set_property(obj, atom, val, flags);
    jsatom::destroy(ctx, atom);
  }

  void
  set_property(const_value obj, uint32_t index, value val, int flags) {
    jsatom atom = jsatom::create(ctx, index);
    set_property(obj, atom, val, flags);
    jsatom::destroy(ctx, atom);
  }

  void
  set_property(const_value obj, const const_value& prop, value val, int flags) {
    jsatom atom = jsatom::create(ctx, prop);
    set_property(obj, atom, val, flags);
    jsatom::destroy(ctx, atom);
  }

  value get_constructor(const_value obj) const;
  bool has_constructor(const_value obj) const;

  value get_prototype(const_value obj) const;
  bool has_prototype(const_value obj) const;

  std::string function_name(const_value fn) const;

  std::string
  class_name(const_value obj) const {
    return function_name(get_constructor(obj));
  }

  value get_global(const char* name) const;
  void set_global(const char* name, value v);

  value get_symbol(const char* name) const;
  value
  get_iterator(const_value obj, const char* symbol = "iterator") {
    return get_property_symbol(obj, symbol);
  }
  value call_iterator(const_value obj, const char* symbol = "iterator");
  value call_iterator_next(const_value obj, const char* symbol = "iterator");

  const_value
  global_object() const {
    value globalThis = JS_GetGlobalObject(ctx);
    return globalThis;
  }
  value
  global_object() {
    value globalThis = JS_GetGlobalObject(ctx);
    return globalThis;
  }

  value call(const_value func, size_t argc, value argv[]) const;
  value call(const_value func, std::vector<value>& args) const;
  value call(const char* name, size_t argc, value argv[]) const;
  value call(const_value func, const_value this_arg, size_t argc, value argv[]) const;

  std::string to_str(const_value val);

  template<class T> T to(const_value val);
  template<class T> value from(const T& val);

  const_value prototype(const_value obj) const;

  void property_names(const_value obj, std::vector<const char*>& out, bool enum_only = false, bool recursive = false) const;

  std::vector<const char*> property_names(const_value obj, bool enum_only = true, bool recursive = true) const;

  void dump_error() const;

  void dump_exception(JSValueConst exception_val, bool is_throw) const;

  bool is_number(const_value val) const;
  bool is_big_int(const_value val) const;
  bool is_big_float(const_value val) const;
  bool is_big_decimal(const_value val) const;
  bool is_bool(const_value val) const;
  bool is_null(const_value val) const;
  bool is_undefined(const_value val) const;
  bool is_exception(const_value val) const;
  bool is_uninitialized(const_value val) const;
  bool is_string(const_value val) const;
  bool is_symbol(const_value val) const;
  bool is_object(const_value val) const;
  bool is_error(const_value val) const;
  bool is_function(const_value val) const;
  bool is_constructor(const_value val) const;
  bool is_array(const_value val) const;
  bool is_extensible(const_value val) const;

  bool is_promise(const_value val);
  bool is_point(const_value val) const;
  bool is_rect(const_value val) const;
  bool is_color(const_value val) const;
  bool is_array_like(const_value val) const;

  bool is_iterable(const_value val);
  bool is_iterator(const_value val) const;

  // int tag(const_value val) const;
  int tag(value val) const;
  void* obj(const_value val) const;

  std::string
  typestr(const_value val) const {
    if(is_number(val))
      return "number";
    else if(is_bool(val))
      return "boolean";
    else if(is_symbol(val))
      return "symbol";
    else if(is_undefined(val))
      return "undefined";

    /*   else if(is_constructor(val))
       return "constructor";*/
    else if(is_object(val) || is_null(val)) {
      if(!is_null(val) && obj(val)) {
        if(is_function(val))
          return "function";
        else if(is_array(val))
          return "array";
        else if(is_point(val))
          return "point";
        else if(is_rect(val))
          return "rect";
        else if(is_color(val))
          return "color";
      }
      return "object";
    }
    return "unknown";
  }

  std::string
  to_string(const_value arg) const {
    std::string ret;
    get_string(arg, ret);
    return ret;
  }

  atom new_atom(const char*, size_t) const;
  atom new_atom(const char*) const;
  atom new_atom(uint32_t) const;
  atom value_to_atom(const const_value& v) const;

  void free_atom(const atom& a) const;

  value atom_to_value(const atom& a) const;
  value atom_to_string(const atom& a) const;
  const char* atom_to_cstring(const atom& a) const;

  void
  free_value(const value& v) const {
    JS_FreeValue(ctx, v);
  }

protected:
  value get_undefined() const;
  value get_null() const;
  value get_true() const;
  value get_false() const;

public:
  JSContext* ctx;

private:
  JSRuntime*
  get_runtime() const {
    return JS_GetRuntime(ctx);
  }

public:
  int32_t get_length(const const_value& v) const;
  jsiter begin(JSValue& v);
  jsiter end(JSValue& v);

private:
  friend class jsiter;

  std::function<JSValue(JSValue, uint32_t)> index() const;
  std::function<JSValue(uint32_t)> index(const JSValueConst&) const;
};

inline void
jsrt::get_string(const_value val, std::string& str) const {
  const char* s = JS_ToCString(ctx, val);
  str = std::string(s);
  JS_FreeCString(ctx, s);
}

inline void
jsrt::get_string(const_value val, const char*& str) const {
  const char* s = JS_ToCString(ctx, val);
  str = strdup(s);
  JS_FreeCString(ctx, s);
}

template<>
inline void
jsrt::get_number<int32_t>(const_value val, int32_t& ref) const {
  int32_t i = -1;
  JS_ToInt32(ctx, &i, val);
  ref = i;
}

template<>
inline void
jsrt::get_number<int64_t>(const_value val, int64_t& ref) const {
  int64_t i = 0;
  JS_ToInt64(ctx, &i, val);
  ref = i;
}

template<>
inline void
jsrt::get_number<double>(const_value val, double& ref) const {
  double f = 0;
  JS_ToFloat64(ctx, &f, val);
  ref = f;
}

template<>
inline void
jsrt::get_number<float>(const_value val, float& ref) const {
  double f = 0;
  get_number(val, f);
  ref = f;
}

template<>
inline void
jsrt::get_number<uint32_t>(const_value val, uint32_t& ref) const {
  int64_t i = 0;
  get_number<int64_t>(val, i);
  ref = static_cast<uint32_t>(i);
}

template<class T>
inline void
jsrt::get_int_array(const_value val, T& ref) const {
  uint32_t i, n, length = 0, arr = JS_IsArray(ctx, val);
  if(arr) {
    get_number(get_property<const char*>(val, "length"), length);
    for(i = 0; i < length; i++) {
      value v = get_property<uint32_t>(val, i);
      get_number(v, n);
      ref[i] = n;
    }
  }
}

template<class T>
inline void
jsrt::get_point(const_value val, T& ref) const {
  const_value vx = _undefined, vy = _undefined;

  if(is_array(val)) {
    uint32_t length;
    get_number(get_property<const char*>(val, "length"), length);
    if(length >= 2) {
      vx = get_property<uint32_t>(val, 0);
      vy = get_property<uint32_t>(val, 1);
    } else {
      return;
    }
  } else if(is_object(val) && has_property(val, "x") && has_property(val, "y")) {
    vx = get_property<const char*>(val, "x");
    vy = get_property<const char*>(val, "y");
  }

  get_number(vx, ref.x);
  get_number(vy, ref.y);
}

template<class T>
inline void
jsrt::get_rect(const_value val, T& ref) const {
  if(is_object(val)) {
    double vw, vh;

    get_point(val, ref);

    get_number(get_property<const char*>(val, "width"), vw);
    get_number(get_property<const char*>(val, "height"), vh);

    ref.width = vw;
    ref.height = vh;
  }
}

template<class T>
inline void
jsrt::get_color(const_value val, T& ref) const {
  const_value vr = _undefined, vg = _undefined, vb = _undefined, va = _undefined;

  if(is_object(val) && has_property(val, "r") && has_property(val, "g") && has_property(val, "b")) {
    vr = get_property<const char*>(val, "r");
    vg = get_property<const char*>(val, "g");
    vb = get_property<const char*>(val, "b");
    va = get_property<const char*>(val, "a");
  } else if(is_array_like(val)) {
    uint32_t length;
    get_number(get_property<const char*>(val, "length"), length);
    if(length >= 3) {
      vr = get_property<uint32_t>(val, 0);
      vg = get_property<uint32_t>(val, 1);
      vb = get_property<uint32_t>(val, 2);
      va = get_property<uint32_t>(val, 3);
    } else {
      return;
    }
  }
  get_number(vb, ref[0]);
  get_number(vg, ref[1]);
  get_number(vr, ref[2]);
  get_number(va, ref[3]);
}

template<class T>
inline void
jsrt::get_point_array(const_value val, std::vector<T>& ref) const {
  if(is_array_like(val)) {
    uint32_t i, length;

    get_number(get_property<const char*>(val, "length"), length);
    ref.resize(length);

    for(i = 0; i < length; i++) {
      JSValueConst pt = get_property<uint32_t>(val, i);

      get_point(pt, ref[i]);
    }
  }
}

template<>
inline jsrt::value
jsrt::create<double>(double num) {
  return JS_NewFloat64(ctx, num);
}

template<>
inline jsrt::value
jsrt::create<float>(float num) {
  return create(static_cast<double>(num));
}

template<>
inline jsrt::value
jsrt::create<int>(int num) {
  return JS_NewInt32(ctx, num);
}

template<>
inline jsrt::value
jsrt::create<const char*>(const char* str) {
  return JS_NewString(ctx, str);
}

template<>
inline jsrt::value
jsrt::create<bool>(bool b) {
  return b ? get_true() : get_false();
}

inline jsrt::value
jsrt::create_array(int32_t size) {
  value ret = JS_NewArray(ctx);
  if(size >= 0)
    JS_SetPropertyStr(ctx, ret, "length", JS_NewInt32(ctx, size));

  return ret;
}

inline jsrt::value
jsrt::create_object() {
  return JS_NewObject(ctx);
}

template<class T>
inline jsrt::value
jsrt::create(T arg) {
  return std::is_pointer<T>::value && arg == nullptr ? get_null() : get_undefined();
}

template<class T>
inline jsrt::value
jsrt::create_point(T x, T y) {
  value obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "x", create(x));
  JS_SetPropertyStr(ctx, obj, "y", create(y));
  return obj;
}

template<>
inline jsrt::value
jsrt::get_property<uint32_t>(const_value obj, uint32_t index) const {
  return JS_GetPropertyUint32(ctx, obj, index);
}

template<>
inline jsrt::value
jsrt::get_property<int>(const_value obj, int index) const {
  return JS_GetPropertyUint32(ctx, obj, index);
}

template<>
inline jsrt::value
jsrt::get_property<const std::string&>(const_value obj, const std::string& name) const {
  return JS_GetPropertyStr(ctx, obj, name.c_str());
}

template<>
inline jsrt::value
jsrt::get_property<const char*>(const_value obj, const char* name) const {
  return JS_GetPropertyStr(ctx, obj, name);
}

inline jsrt::value
jsrt::get_property_atom(const_value obj, atom a) const {
  return JS_GetProperty(ctx, obj, a);
}

template<>
inline jsrt::value
jsrt::get_property<jsrt::value>(const_value obj, value prop) const {
  return get_property_atom(obj, value_to_atom(prop));
}

inline jsrt::value
jsrt::get_constructor(jsrt::const_value obj) const {
  return get_property<const char*>(get_prototype(obj), "constructor");
}

inline bool
jsrt::has_constructor(jsrt::const_value obj) const {
  return !is_undefined(get_constructor(obj));
}

inline jsrt::value
jsrt::get_prototype(jsrt::const_value obj) const {
  return JS_GetPrototype(ctx, obj);
}

inline bool
jsrt::has_prototype(jsrt::const_value obj) const {
  return !is_undefined(get_prototype(obj));
}

inline std::string
jsrt::function_name(jsrt::const_value fn) const {
  return to_string(get_property<const char*>(fn, "name"));
}

inline bool
jsrt::has_property(const_value obj, const jsatom& atom) const {
  return JS_HasProperty(ctx, obj, atom);
}

template<>
inline void
jsrt::set_property<const char*>(const_value obj, const char* name, value val) {
  // JS_SetPropertyStr(ctx, obj, name, val);
  JS_DefinePropertyValueStr(ctx, obj, name, val, JS_PROP_C_W_E);
}

template<>
inline void
jsrt::set_property<uint32_t>(const_value obj, uint32_t index, value val) {
  JS_SetPropertyUint32(ctx, obj, index, val);
}

template<>
inline void
jsrt::set_property<const jsatom&>(const_value obj, const jsatom& atom, value val) {
  JS_SetProperty(ctx, obj, atom, val);
}

inline void
jsrt::set_property(const_value obj, const jsatom& atom, value val, int flags) {
  JS_SetPropertyInternal(ctx, obj, atom, val, flags);
}

template<class T>
inline jsrt::value
vector_to_js(jsrt& js, const T& v, size_t n, const std::function<jsrt::value(const typename T::value_type&)>& fn) {
  using std::placeholders::_1;
  jsrt::value ret = js.create_array(n);
  for(uint32_t i = 0; i < n; i++) js.set_property(ret, i, fn(v[i]));
  return ret;
}

template<class T>
inline jsrt::value
vector_to_js(jsrt& js, const T& v, size_t n) {
  return vector_to_js(v, n, std::bind(&jsrt::create<typename T::value_type>, &js, std::placeholders::_1));
}

template<class T>
inline jsrt::value
vector_to_js(jsrt& js, const T& v) {
  return vector_to_js(js, v, v.size());
}

template<class P>
inline jsrt::value
vector_to_js(jsrt& js, const std::vector<P>& v, const std::function<jsrt::value(const P&)>& fn) {
  return vector_to_js(js, v, v.size(), fn);
}

template<class P>
inline jsrt::value
pointer_to_js(jsrt& js, const P* v, size_t n, const std::function<jsrt::value(const P&)>& fn) {
  jsrt::value ret = js.create_array(n);
  for(uint32_t i = 0; i < n; i++) js.set_property(ret, i, fn(v[i]));
  return ret;
}

template<class P>
inline jsrt::value
pointer_to_js(jsrt& js, const P* v, size_t n) {
  std::function<jsrt::value(const P&)> fn([&](const P& v) -> jsrt::value { return js.create(v); });

  return pointer_to_js(js, v, n, fn);
}

template<class P>
inline jsrt::value
vector_to_js(jsrt& js, const std::vector<P>& v, jsrt::value (*fn)(const P&)) {
  uint32_t i, n = v.size();
  jsrt::value ret = js.create_array(n);
  for(i = 0; i < n; i++) js.set_property(ret, i, fn(v[i]));
  return ret;
}

template<class P>
inline jsrt::value
vector_to_js(jsrt& js, const std::vector<P>& v) {
  using std::placeholders::_1;
  return vector_to_js<P>(v, std::bind(&jsrt::create<P>, &js, _1));
}

inline std::string
to_string(const char* s) {
  return std::string(s);
}

inline bool
jsrt::is_number(const_value val) const {
  return JS_IsNumber(val);
}

inline bool
jsrt::is_big_int(const_value val) const {
  return JS_IsBigInt(ctx, val);
}

inline bool
jsrt::is_big_float(const_value val) const {
  return JS_IsBigFloat(val);
}

inline bool
jsrt::is_big_decimal(const_value val) const {
  return JS_IsBigDecimal(val);
}

inline bool
jsrt::is_bool(const_value val) const {
  return JS_IsBool(val);
}

inline bool
jsrt::is_null(const_value val) const {
  return JS_IsNull(val);
}

inline bool
jsrt::is_undefined(const_value val) const {
  return JS_IsUndefined(val);
}

inline bool
jsrt::is_exception(const_value val) const {
  return JS_IsException(val);
}

inline bool
jsrt::is_uninitialized(const_value val) const {
  return JS_IsUninitialized(val);
}

inline bool
jsrt::is_string(const_value val) const {
  return JS_IsString(val);
}

inline bool
jsrt::is_symbol(const_value val) const {
  return JS_IsSymbol(val);
}

inline bool
jsrt::is_object(const_value val) const {
  return JS_IsObject(val);
}

inline bool
jsrt::is_error(const_value val) const {
  return JS_IsError(ctx, val);
}

inline bool
jsrt::is_function(const_value val) const {
  return JS_IsFunction(ctx, val);
}

inline bool
jsrt::is_constructor(const_value val) const {
  return JS_IsConstructor(ctx, val);
}

inline bool
jsrt::is_array(const_value val) const {
  return JS_IsArray(ctx, val);
}

inline bool
jsrt::is_extensible(const_value val) const {
  return JS_IsExtensible(ctx, val);
}

inline bool
jsrt::is_array_like(const_value val) const {
  if(is_array(val))
    return true;

  if(has_property(val, "length")) {
    if(is_number(get_property<const char*>(val, "length")))
      return true;
  }
  return false;
}

/**
 * Array iterator
 */
struct jsiter {
  JSValue
  operator*() const {
    if(p < n)
      return i((uint32_t)p);
    else
      return JS_UNDEFINED;
  }
  jsiter
  operator++() {
    jsiter ret = *this;
    if(p < n)
      p++;
    return ret;
  }
  jsiter&
  operator++(int) {
    if(p < n)
      ++p;
    return *this;
  }
  bool
  operator==(const jsiter& o) const {
    return p == o.p && n == o.n;
  }
  bool
  operator<(const jsiter& o) const {
    return p < o.p;
  }
  bool
  operator>(const jsiter& o) const {
    return p > o.p;
  }
  bool
  operator<=(const jsiter& o) const {
    return !(*this > o);
  }
  bool
  operator>=(const jsiter& o) const {
    return !(*this < o);
  }
  bool
  operator!=(const jsiter& o) const {
    return !(*this == o);
  }

  ptrdiff_t
  operator-(const jsiter& o) const {
    return p - o.p;
  }

  jsiter
  operator-(size_t o) const {
    return jsiter(i, n, p - o);
  }

  jsiter
  operator+(size_t o) const {
    return jsiter(i, n, p + o);
  }

protected:
  std::function<JSValue(uint32_t)> i;
  uint32_t n;
  uint32_t p;

private:
  friend class jsrt;

  jsiter(std::function<JSValue(uint32_t)> index, size_t len, size_t pos) : i(index), n(len), p(pos) {}
  jsiter(jsrt& js, const JSValue& arr, size_t len) : i(js.index(arr)), n(len), p(0) {}
  jsiter(jsrt& js, const JSValue& arr, size_t len, size_t pos) : i(js.index(arr)), n(len), p(pos) {}
};

inline int32_t
jsrt::get_length(const jsrt::const_value& a) const {
  uint32_t length;
  JSValue v = get_property<const char*>(a, "length");
  if(is_undefined(v))
    return -1;
  get_number(v, length);
  return length;
}

inline jsiter
jsrt::begin(JSValue& v) {
  uint32_t n = get_length(v);
  return jsiter(*this, v, n, 0);
}

inline jsiter
jsrt::end(JSValue& v) {
  uint32_t n = get_length(v);
  return jsiter(*this, v, n, n);
}

inline std::function<JSValue(JSValue, uint32_t)>
jsrt::index() const {
  return std::bind(&jsrt::get_property<uint32_t>, this, std::placeholders::_1, std::placeholders::_2);
}

inline std::function<JSValue(uint32_t)>
jsrt::index(const JSValueConst& a) const {
  return std::bind(&jsrt::get_property<uint32_t>, this, a, std::placeholders::_1);
}

inline bool
jsrt::is_iterable(const_value val) {
  bool ret = false;
  value sym = get_symbol("iterator");
  if(is_object(val) && has_property(val, sym)) {
    value iter = get_property<value>(val, sym);
    ret = is_function(iter);
  }
  free_value(sym);
  return ret;
}

inline bool
jsrt::is_iterator(const_value val) const {
  if(is_object(val) && has_property(val, "next")) {
    value next = get_property<const char*>(val, "next");
    return is_function(next);
  }
  return false;
}

extern "C" jsrt js;

template<>
inline std::string
jsrt::to<std::string>(jsrt::const_value val) {
  return jsrt::to_string(val);
}

template<>
inline int32_t
jsrt::to<int32_t>(jsrt::const_value val) {
  int32_t ret;
  jsrt::get_number(val, ret);
  return ret;
}

template<>
inline uint32_t
jsrt::to<uint32_t>(jsrt::const_value val) {
  uint32_t ret;
  jsrt::get_number(val, ret);
  return ret;
}

template<>
inline float
jsrt::to<float>(jsrt::const_value val) {
  float ret;
  jsrt::get_number(val, ret);
  return ret;
}

template<>
inline double
jsrt::to<double>(jsrt::const_value val) {
  double ret;
  jsrt::get_number(val, ret);
  return ret;
}

template<>
inline jsrt::value
jsrt::from<std::string>(const std::string& value) {
  return new_string(value.c_str());
}

#endif // defined JS_HPP
