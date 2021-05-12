#include "js.hpp"
#include "jsbindings.hpp"
#include "util.hpp"
#include <cstring>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <cassert>
#include <regex>

extern "C" {
#include "quickjs/quickjs.h"
#include "quickjs/quickjs-libc.h"
#include "quickjs/cutils.h"

jsrt js;

char* normalize_module(JSContext* ctx, const char* module_base_name, const char* module_name, void* opaque);

struct JSString {
  JSRefCountHeader header; /* must come first, 32-bit */
  uint32_t len : 31;
  uint8_t is_wide_char : 1; /* 0 = 8 bits, 1 = 16 bits characters */
  /* for JS_ATOM_TYPE_SYMBOL: hash = 0, atom_type = 3,
     for JS_ATOM_TYPE_PRIVATE: hash = 1, atom_type = 3
     XXX: could change encoding to have one more bit in hash */
  uint32_t hash : 30;
  uint8_t atom_type : 2; /* != 0 if atom, JS_ATOM_TYPE_x */
  uint32_t hash_next;    /* atom_index for JS_ATOM_TYPE_SYMBOL */
#ifdef DUMP_LEAKS
  struct list_head link; /* string list */
#endif
  union {
    uint8_t str8[0]; /* 8 bit strings will get an extra null terminator */
    uint16_t str16[0];
  } u;
};

typedef struct JSString JSAtomStruct;
};

static JSValue
js_print(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  int i;
  const char* str;

  for(i = 0; i < argc; i++) {
    if(i != 0)
      putchar(' ');
    str = JS_ToCString(ctx, argv[i]);
    if(!str)
      return JS_EXCEPTION;
    fputs(str, stdout);
    JS_FreeCString(ctx, str);
  }
  putchar('\n');
  return JS_UNDEFINED;
}

struct JSProperty;
struct JSShapeProperty;

jsrt::value jsrt::_undefined = JS_UNDEFINED;
jsrt::value jsrt::_true = JS_TRUE;
jsrt::value jsrt::_false = JS_FALSE;
jsrt::value jsrt::_null = JS_NULL;

bool
jsrt::init(int argc, char* argv[]) {
  int load_std = 0;

  if(ctx == nullptr)
    if(!create())
      return false;

  // JS_AddIntrinsicBaseObjects(ctx);
  js_std_add_helpers(ctx, argc, argv);

  /* system modules */
  js_init_module_std(ctx, "std");
  js_init_module_os(ctx, "os");

  /* make 'std' and 'os' visible to non module code */
  if(load_std) {
    const char* str = "import * as std from 'std';\n"
                      "import * as os from 'os';\n"
                      "globalThis.std = std;\n"
                      "globalThis.os = os;\n";
    eval_buf(str, strlen(str), "<input>", JS_EVAL_TYPE_MODULE);
  }

  /* loader for ES6 modules */
  JS_SetModuleLoaderFunc(get_runtime(), &normalize_module, js_module_loader, this);

  // global.get();

  /*  this->_undefined = get_undefined();
    this->_null = get_null();
    this->_true = get_true();
    this->_false = get_false();*/

  return ctx != nullptr;
}

bool
jsrt::create(JSContext* _ctx) {
  assert(ctx == nullptr);
  if(_ctx) {
    // rt = JS_GetRuntime(ctx = _ctx);
  } else {
    JSRuntime* rt;
    if((rt = JS_NewRuntime())) {
      js_std_init_handlers(rt);

      ctx = JS_NewContext(rt);
    }
  }
  return ctx != nullptr;
}

/*jsrt::value*
jsrt::get_function(const char* name) {
  auto it = funcmap.find(name);
  if(it != funcmap.end()) {
    std::pair<JSCFunction*, value>& val = it->second;
    return &val.second;
  }
  return nullptr;
}*/

std::string
jsrt::to_str(const_value val) {
  std::string ret;
  if(JS_IsFunction(ctx, val))
    ret = "Function";
  else if(JS_IsNumber(val))
    ret = "Number";
  else if(JS_IsBool(val))
    ret = "Boolean";
  else if(JS_IsString(val))
    ret = "String";
  else if(js_is_array(ctx, val))
    ret = "Array";
  else if(JS_IsObject(val))
    ret = "Object";
  else if(JS_IsSymbol(val))
    ret = "Symbol";
  else if(JS_IsException(val))
    ret = "Exception";
  else if(JS_IsUninitialized(val))
    ret = "Uninitialized";
  else if(JS_IsUndefined(val))
    ret = "undefined";

  return ret;
}

jsrt::const_value
jsrt::prototype(const_value obj) const {
  return JS_GetPrototype(ctx, obj);
}

std::vector<const char*>
jsrt::property_names(const_value obj, bool enum_only, bool recursive) const {
  std::vector<const char*> ret;
  property_names(obj, ret, enum_only);
  return ret;
}

void
jsrt::property_names(const_value obj, std::vector<const char*>& out, bool enum_only, bool recursive) const {
  JSPropertyEnum* props;
  uint32_t nprops;
  while(JS_IsObject(obj)) {
    props = nullptr;
    nprops = 0;
    JS_GetOwnPropertyNames(
        ctx, &props, &nprops, obj, JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK | (enum_only ? JS_GPN_ENUM_ONLY : 0));
    for(uint32_t i = 0; i < nprops; i++) {
      const char* s = JS_AtomToCString(ctx, props[i].atom);
      out.push_back(s);
    }
    if(!recursive)
      break;

    obj = prototype(obj);
  }
}

bool
jsrt::is_point(const_value val) const {
  if(is_array(val)) {
    int32_t length = -1;
    get_number(get_property<const char*>(val, "length"), length);
    if(length == 2)
      return true;
  } else if(is_object(val)) {
    JSValue x, y;
    x = get_property<const char*>(val, "x");
    y = get_property<const char*>(val, "y");
    if(is_number(x) && is_number(y))
      return true;
  }

  return false;
}

bool
jsrt::is_rect(const_value val) const {
  if(is_object(val)) {
    JSValue x, y, w, h;
    x = get_property<const char*>(val, "x");
    y = get_property<const char*>(val, "y");
    w = get_property<const char*>(val, "width");
    h = get_property<const char*>(val, "height");
    if(is_number(x) && is_number(y))
      if(is_number(w) && is_number(h))
        return true;
  }

  return false;
}

bool
jsrt::is_color(const_value val) const {
  JSValue b = _undefined, g = _undefined, r = _undefined, a = _undefined;

  if(is_array_like(val)) {
    uint32_t length;
    get_number(get_property<const char*>(val, "length"), length);

    if(length == 3 || length == 4) {
      b = get_property<uint32_t>(val, 0);
      g = get_property<uint32_t>(val, 1);
      r = get_property<uint32_t>(val, 2);
      a = length > 3 ? get_property<uint32_t>(val, 3) : const_cast<jsrt*>(this)->create<int32_t>(255);
    } else {
      return false;
    }
  } else if(is_object(val)) {
    b = get_property<const char*>(val, "b");
    g = get_property<const char*>(val, "g");
    r = get_property<const char*>(val, "r");
    a = get_property<const char*>(val, "a");
  }

  if(is_number(b) && is_number(g) && is_number(r) && is_number(a))
    return true;
  return false;
}

/*
jsrt::global::global(jsrt& rt) : val(JS_UNDEFINED), js(rt) {}

bool
jsrt::global::get() const {
  if(JS_IsUndefined((const_value)val)) {
    if(js.ctx) {
      value global = JS_GetGlobalObject(js.ctx);

      JS_SetPropertyStr(js.ctx, global, "global", JS_DupValue(js.ctx, global));
      JS_FreeValue(js.ctx, global);
    }

    if(js.ctx) {
      val = JS_GetGlobalObject(js.ctx);
      return true;
    }
  }
  return false;
}

jsrt::global::global(global&& o) noexcept : val(std::move(o.val)), js(o.js) {}

jsrt::global::~global() {
  if(js.ctx)
    JS_FreeValue(js.ctx, val);
}*/

jsrt::value
jsrt::get_undefined() const {
  return JS_UNDEFINED;
}

jsrt::value
jsrt::get_null() const {
  return JS_NULL;
}

jsrt::value
jsrt::get_true() const {
  return JS_TRUE;
}

jsrt::value
jsrt::get_false() const {
  return JS_FALSE;
}

jsrt::~jsrt() {
  if(ctx)
    JS_FreeContext(ctx);
  ctx = nullptr;
}

JSValue
jsrt::eval_buf(const char* buf, int buf_len, const char* filename, int eval_flags) {
  jsrt::value val;
  if((eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE) {
    val = JS_Eval(ctx, buf, buf_len, filename, eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
    if(!JS_IsException(val)) {
      js_module_set_import_meta(ctx, val, TRUE, TRUE);
      val = JS_EvalFunction(ctx, val);
    }
  } else {
    val = JS_Eval(ctx, buf, buf_len, filename, eval_flags);
  }
  if(JS_IsException(val)) {
    js_std_dump_error(ctx);
  } else {
  }
  return val;
}

jsrt::value
jsrt::eval_file(const char* filename, int module) {
  char* buf;
  int eval_flags;
  jsrt::value ret;
  size_t buf_len;
  buf = reinterpret_cast<char*>(js_load_file(ctx, &buf_len, filename));
  if(!buf) {
    perror(filename);
    exit(1);
  }
  if(module < 0)
    module = (str_end(filename, ".mjs") || JS_DetectModule((const char*)buf, buf_len));

  eval_flags = module ? JS_EVAL_TYPE_MODULE : JS_EVAL_TYPE_GLOBAL;

  /*  std::string script(buf, buf_len);
    std::cerr << "Script: " << script << std::endl;
 */
  ret = eval_buf(buf, buf_len, filename, eval_flags);
  js_free(ctx, buf);
  return ret;
}

void
jsrt::set_global(const char* name, jsrt::value val) {
  value globalThis = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, globalThis, name, val);
}

jsrt::value
jsrt::get_global(const char* name) const {
  value globalThis = JS_GetGlobalObject(ctx);
  value ret = JS_GetPropertyStr(ctx, globalThis, name);
  return ret;
}

bool
jsrt::is_promise(const_value val) {
  jsrt::value promise = get_global("Promise");
  jsrt::value promise_proto = get_property<const char*>(promise, "prototype");
  return is_object(val) && (JS_IsInstanceOf(ctx, val, promise) || JS_IsInstanceOf(ctx, val, promise_proto));
}

jsrt::value
jsrt::call(const char* name, size_t argc, value argv[]) const {
  const_value func = get_global(name);
  return call(func, argc, argv);
}

jsrt::value
jsrt::call(const_value func, std::vector<value>& args) const {
  return call(func, args.size(), const_cast<value*>(args.data()));
}

jsrt::value
jsrt::call(const_value func, const_value this_arg, size_t argc, value argv[]) const {
  value ret = JS_Call(ctx, func, this_arg, argc, const_cast<const_value*>(argv));
  if(JS_IsException(ret))
    dump_error();
  return ret;
}

jsrt::value
jsrt::call(const_value func, size_t argc, value argv[]) const {
  return call(func, global_object(), argc, argv);
}

extern "C" char*
normalize_module(JSContext* ctx, const char* module_base_name, const char* module_name, void* opaque) {
  using std::filesystem::exists;
  using std::filesystem::path;
  using std::filesystem::weakly_canonical;

  char* name;
  jsrt* js = static_cast<jsrt*>(opaque);

  std::string module_dir = std::string(CONFIG_PREFIX) + "/lib/quickjs";

  /*std::cerr << "module_base_name: " << module_base_name << std::endl;
  std::cerr << "module_name: " << module_name << std::endl;
  std::cerr << "module_dir: " << module_dir << std::endl;*/

  if(module_name[0] == '.' && module_name[1] == '/')
    module_name += 2;

  const char* module_ext = strrchr(module_name, '.');
  // std::cerr << "module_ext: " << module_ext << std::endl;

  std::string module;
  path module_path;
  if((module_ext && !strcmp(".so", module_ext)) || module_ext == nullptr) {
    module = module_dir + "/" + module_name;
    module_path = path(module);
  } else {
    module = module_base_name;
    module_path = path(module).replace_filename(path(module_name, module_name + strlen(module_name)));
  }

  std::string module_pathstr;

  // std::cerr << "module_path: " << module_path.string() << std::endl;

  bool present = exists(module_path);
  /*  std::cerr << "exists module_path: " << present << std::endl;*/
  module_pathstr = module_path.string();

  if(!present) {
    module_path = weakly_canonical(module_path);
    module_pathstr = module_path.string();

    present = exists(module_path);
  }
  /*std::cerr << "module_pathstr: " << module_pathstr << std::endl;
  std::cerr << "module_name: " << module_name << std::endl;
  std::cerr << "present: " << present << std::endl;*/

  if(true) {

    /*
    module_pathstr.resize(module_pathstr.size()+1);
    */
    const char* s = module_pathstr.c_str();
    name = static_cast<char*>(js_strdup(ctx, s));

    return name;
  }
  return 0;
}

void
jsrt::dump_error() const {
  JSValue exception_val;

  exception_val = JS_GetException(ctx);
  dump_exception(exception_val, true);
  JS_FreeValue(ctx, exception_val);
}

void
jsrt::dump_exception(JSValueConst exception_val, bool is_throw) const {
  JSValue val;
  const char* stack;
  bool is_error;

  is_error = JS_IsError(ctx, exception_val);
  /*  if(is_throw && !is_error)
      printf("Throw: ");*/
  js_print(ctx, JS_NULL, 1, (JSValueConst*)&exception_val);
  if(is_error) {
    val = JS_GetPropertyStr(ctx, exception_val, "stack");
    if(!JS_IsUndefined(val)) {
      stack = JS_ToCString(ctx, val);
      printf("%s\n", stack);
      JS_FreeCString(ctx, stack);
    }
    JS_FreeValue(ctx, val);
  }
}

jsrt::atom
jsrt::new_atom(const char* buf, size_t len) const {
  return JS_NewAtomLen(ctx, buf, len);
}

jsrt::atom
jsrt::new_atom(const char* str) const {
  return JS_NewAtom(ctx, str);
}

jsrt::atom
jsrt::new_atom(uint32_t n) const {
  return JS_NewAtomUInt32(ctx, n);
}

void
jsrt::free_atom(const jsrt::atom& a) const {
  JS_FreeAtom(ctx, a);
}

jsrt::value
jsrt::atom_to_value(const jsrt::atom& a) const {
  return JS_AtomToValue(ctx, a);
}

jsrt::value
jsrt::atom_to_string(const jsrt::atom& a) const {
  return JS_AtomToString(ctx, a);
}

const char*
jsrt::atom_to_cstring(const jsrt::atom& a) const {
  return JS_AtomToCString(ctx, a);
}

jsrt::atom
jsrt::value_to_atom(const const_value& v) const {
  return JS_ValueToAtom(ctx, v);
}

jsrt::value
jsrt::get_symbol(const char* name) const {
  value ctor = get_global("Symbol");
  if(has_property(ctor, name))
    return get_property<const char*>(ctor, name);

  const_value for_fn = get_property<const char*>(ctor, "for");
  value arg = new_string(name);
  return call(for_fn, ctor, 1, &arg);
}

jsrt::value
jsrt::get_property_symbol(const_value obj, const char* symbol) {
  value sym = get_symbol(symbol);
  value ret = get_property<value>(obj, sym);
  free_value(sym);
  return ret;
}

jsrt::value
jsrt::call_iterator(const_value obj, const char* symbol) {
  value fn = get_iterator(obj, symbol);
  value ret = _undefined;

  if(is_function(fn))
    ret = call(fn, obj, 0, nullptr);

  free_value(fn);

  return ret;
}

jsrt::value
jsrt::call_iterator_next(const_value obj, const char* symbol) {
  value iter = get_iterator(obj, symbol);
  value ret = _undefined, next = _undefined;

  if(is_object(iter) && !is_null(iter)) {
    next = get_property<const char*>(iter, "next");

    if(is_function(next) && has_property(next, "bind")) {
      ret = call(get_property<const char*>(next, "bind"), iter, 0, nullptr);
      //   free_value(next);
    }
  }
  // free_value(iter);
  return ret;
}

int
jsrt::tag(value val) const {
  return JS_VALUE_GET_TAG(val);
}

void*
jsrt::obj(const_value val) const {
  return JS_VALUE_GET_OBJ(val);
}
