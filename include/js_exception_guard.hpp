#ifndef JS_EXCEPTION_GUARD_HPP
#define JS_EXCEPTION_GUARD_HPP

/**
 * @file js_exception_guard.hpp
 *
 * Turns a C++ exception escaping a binding into a catchable JS exception.
 *
 * QuickJS calls every registered native function through C frames. A
 * `cv::Exception` thrown by an OpenCV precondition assert and left uncaught
 * unwinds straight through those frames, which is undefined behaviour and in
 * practice reaches `std::terminate()` - SIGABRT, the whole qjsm process gone,
 * with no way for JS `try`/`catch` to see it. Most - but far from all -
 * `js_*.cpp` files wrapped their own calls in `try`/`catch(const
 * cv::Exception&)`; 24 files had no guard at all (BUGS:
 * uncaught-cv-exception-aborts-process-in-unguarded-modules).
 *
 * Rather than retrofit try/catch into every binding - which is invasive and
 * regresses the moment someone adds a function without one - the guard is
 * applied once, at registration: the `JS_CFUNC_*` / `JS_CGETSET_*` / `JS_CTOR_*`
 * macros below shadow QuickJS's (and util.hpp's) so that the function pointer
 * stored in each JSCFunctionListEntry is a template trampoline wrapping the
 * real one. The macro signatures and the resulting entries are otherwise
 * identical, so existing tables need no edits.
 *
 * This header is force-included into every translation unit of the module by
 * CMakeLists.txt (`-include js_exception_guard.hpp`); it must not be included
 * manually.
 *
 * Per-binding try/catch blocks still work and still run first - they are inside
 * the trampoline. This is only the backstop for what they miss.
 */

#include "util.hpp"

#include <opencv2/core/base.hpp>

#include <exception>
#include <new>

/**
 * Converts the in-flight C++ exception into a pending JS exception.
 * Only ever called from inside a `catch(...)` block.
 */
static inline JSValue
js_guard_rethrow_as_js(JSContext* ctx) {
  try {
    throw;
  } catch(const cv::Exception& e) {
    /* e.what() is multi-line and already names file/line/function. err/msg are
       the terse parts, which read better as a JS Error message. */
    return JS_ThrowInternalError(ctx, "OpenCV(%s): %s", CV_VERSION, e.msg.empty() ? e.what() : e.msg.c_str());
  } catch(const std::bad_alloc&) { return JS_ThrowOutOfMemory(ctx); } catch(const std::exception& e) {
    return JS_ThrowInternalError(ctx, "%s", e.what());
  } catch(...) { return JS_ThrowInternalError(ctx, "unknown C++ exception"); }
}

/* The trampolines. Each takes the wrapped function as a non-type template
   parameter, so the indirection is resolved at compile time and the entry in
   the function list is a plain function pointer, exactly as before.

   A null `F` is legal and reachable: JS_CGETSET_DEF entries routinely pass 0
   for a read-only property's setter. Those instantiations are never called -
   QuickJS checks the pointer - but must still compile, hence the `if
   constexpr` guards. */

template<JSValue (*F)(JSContext*, JSValueConst, int, JSValueConst*)>
static JSValue
js_guarded(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  if constexpr(F == nullptr)
    return JS_UNDEFINED;
  else
    try {
      return F(ctx, this_val, argc, argv);
    } catch(...) { return js_guard_rethrow_as_js(ctx); }
}

template<JSValue (*F)(JSContext*, JSValueConst, int, JSValueConst*, int)>
static JSValue
js_guarded_magic(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  if constexpr(F == nullptr)
    return JS_UNDEFINED;
  else
    try {
      return F(ctx, this_val, argc, argv, magic);
    } catch(...) { return js_guard_rethrow_as_js(ctx); }
}

template<JSValue (*F)(JSContext*, JSValueConst)>
static JSValue
js_guarded_getter(JSContext* ctx, JSValueConst this_val) {
  if constexpr(F == nullptr)
    return JS_UNDEFINED;
  else
    try {
      return F(ctx, this_val);
    } catch(...) { return js_guard_rethrow_as_js(ctx); }
}

template<JSValue (*F)(JSContext*, JSValueConst, JSValueConst)>
static JSValue
js_guarded_setter(JSContext* ctx, JSValueConst this_val, JSValueConst val) {
  if constexpr(F == nullptr)
    return JS_UNDEFINED;
  else
    try {
      return F(ctx, this_val, val);
    } catch(...) { return js_guard_rethrow_as_js(ctx); }
}

template<JSValue (*F)(JSContext*, JSValueConst, int)>
static JSValue
js_guarded_getter_magic(JSContext* ctx, JSValueConst this_val, int magic) {
  if constexpr(F == nullptr)
    return JS_UNDEFINED;
  else
    try {
      return F(ctx, this_val, magic);
    } catch(...) { return js_guard_rethrow_as_js(ctx); }
}

template<JSValue (*F)(JSContext*, JSValueConst, JSValueConst, int)>
static JSValue
js_guarded_setter_magic(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  if constexpr(F == nullptr)
    return JS_UNDEFINED;
  else
    try {
      return F(ctx, this_val, val, magic);
    } catch(...) { return js_guard_rethrow_as_js(ctx); }
}

/* Selects the trampoline for `F`, or a null pointer when `F` is itself null -
   a read-only property passes 0 for its setter, and wrapping that would make
   the property look writable. It has to be a specialised trait rather than the
   obvious `F ? &js_guarded_x<F> : nullptr`, because in the null case GCC never
   gets a target type for the address-of and rejects the whole expression. */
#define JS_GUARD_TRAIT(trait, wrapper, ...) \
  typedef JSValue (*trait##_fn)(__VA_ARGS__); \
  template<trait##_fn F> struct trait { \
    static constexpr trait##_fn ptr = &wrapper<F>; \
  }; \
  template<> struct trait<nullptr> { \
    static constexpr trait##_fn ptr = nullptr; \
  };

/* The tables spell an absent getter/setter as a plain `0`, which C++17 will not
   accept directly as a pointer template argument; the cast makes it the null
   pointer constant the trait matches on, and is a no-op for a real function. */
#define JS_GUARD(trait, f) trait<(trait##_fn)(f)>::ptr

JS_GUARD_TRAIT(js_guard, js_guarded, JSContext*, JSValueConst, int, JSValueConst*)
JS_GUARD_TRAIT(js_guard_magic, js_guarded_magic, JSContext*, JSValueConst, int, JSValueConst*, int)
JS_GUARD_TRAIT(js_guard_getter, js_guarded_getter, JSContext*, JSValueConst)
JS_GUARD_TRAIT(js_guard_setter, js_guarded_setter, JSContext*, JSValueConst, JSValueConst)
JS_GUARD_TRAIT(js_guard_getter_magic, js_guarded_getter_magic, JSContext*, JSValueConst, int)
JS_GUARD_TRAIT(js_guard_setter_magic, js_guarded_setter_magic, JSContext*, JSValueConst, JSValueConst, int)

#undef JS_CFUNC_DEF
#define JS_CFUNC_DEF(n, length, func1) \
  { \
    .name = n, .prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, .def_type = JS_DEF_CFUNC, .magic = 0, .u = { \
      .func = {length, JS_CFUNC_generic, {.generic = JS_GUARD(js_guard, func1)}} \
    } \
  }

#undef JS_CFUNC_MAGIC_DEF
#define JS_CFUNC_MAGIC_DEF(n, length, func1, m) \
  { \
    .name = n, .prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, .def_type = JS_DEF_CFUNC, .magic = m, .u = { \
      .func = {length, JS_CFUNC_generic_magic, {.generic_magic = JS_GUARD(js_guard_magic, func1)}} \
    } \
  }

#undef JS_CGETSET_DEF
#define JS_CGETSET_DEF(n, fgetter, fsetter) \
  { \
    .name = n, .prop_flags = JS_PROP_CONFIGURABLE, .def_type = JS_DEF_CGETSET, .magic = 0, .u = { \
      .getset = {.get = {.getter = JS_GUARD(js_guard_getter, fgetter)}, .set = {.setter = JS_GUARD(js_guard_setter, fsetter)}} \
    } \
  }

#undef JS_CGETSET_MAGIC_DEF
#define JS_CGETSET_MAGIC_DEF(n, fgetter, fsetter, m) \
  { \
    .name = n, .prop_flags = JS_PROP_CONFIGURABLE, .def_type = JS_DEF_CGETSET_MAGIC, .magic = m, .u = { \
      .getset = {.get = {.getter_magic = JS_GUARD(js_guard_getter_magic, fgetter)}, .set = {.setter_magic = JS_GUARD(js_guard_setter_magic, fsetter)}} \
    } \
  }

#undef JS_CGETSET_ENUMERABLE_DEF
#define JS_CGETSET_ENUMERABLE_DEF(prop_name, fgetter, fsetter, magic_num) \
  { \
    .name = prop_name, .prop_flags = JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE, .def_type = JS_DEF_CGETSET_MAGIC, .magic = magic_num, .u = { \
      .getset = {.get = {.getter_magic = JS_GUARD(js_guard_getter_magic, fgetter)}, .set = {.setter_magic = JS_GUARD(js_guard_setter_magic, fsetter)}} \
    } \
  }

/* The only two cprotos this project uses with the SPECIAL macros are
   `constructor` (JSCFunction shape) and `constructor_or_func_magic`
   (constructor_magic shape); both are dispatched below by wrapping with the
   matching trampoline. */
#undef JS_CFUNC_SPECIAL_DEF
#define JS_CFUNC_SPECIAL_DEF(n, length, cproto, func1) \
  { \
    .name = n, .prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, .def_type = JS_DEF_CFUNC, .magic = 0, .u = { \
      .func = {length, JS_CFUNC_##cproto, {.cproto = JS_GUARD_SPECIAL_##cproto(func1)}} \
    } \
  }

#undef JS_CFUNC_SPECIAL_MAGIC_DEF
#define JS_CFUNC_SPECIAL_MAGIC_DEF(n, len, cp, func1, magic_num) \
  { \
    .name = n, .prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, .def_type = JS_DEF_CFUNC, .magic = magic_num, .u = { \
      .func = {.length = len, .cproto = JS_CFUNC_##cp, .cfunc = {.constructor_magic = JS_GUARD_SPECIAL_##cp(func1)}} \
    } \
  }

#define JS_GUARD_SPECIAL_constructor(f) (JS_GUARD(js_guard, f))
#define JS_GUARD_SPECIAL_constructor_or_func_magic(f) (JS_GUARD(js_guard_magic, f))

/* util.hpp defines these in terms of the SPECIAL macros, so they pick the guard
   up automatically at the point of use - but only if they expand to the
   definitions above rather than the ones util.hpp saw. Redefining them here
   keeps that independent of include order. */
#undef JS_CTOR_MAGIC_DEF
#define JS_CTOR_MAGIC_DEF(n, len, func1, magic_num) JS_CFUNC_SPECIAL_MAGIC_DEF(n, len, constructor_or_func_magic, func1, magic_num)

#undef JS_CTOR_DEF
#define JS_CTOR_DEF(n, len, func1) JS_CFUNC_SPECIAL_DEF(n, len, constructor_or_func_magic, func1)

#endif /* JS_EXCEPTION_GUARD_HPP */
