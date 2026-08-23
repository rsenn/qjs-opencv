#include "js_range.hpp"
#include "include/js_alloc.hpp"
#include "include/js_array.hpp"
#include "include/js_typed_array.hpp"

extern "C" {
thread_local JSValue range_proto = JS_UNDEFINED, range_class = JS_UNDEFINED;
thread_local JSClassID js_range_class_id = 0;
}

JSValue
js_range_create(JSContext* ctx, JSValueConst proto) {
  JSValue ret;
  JSRangeData* s;

  if(JS_IsUndefined(range_proto))
    js_range_init(ctx, NULL);

  if(JS_IsUndefined(proto))
    proto = range_proto;

  ret = JS_NewObjectProtoClass(ctx, proto, js_range_class_id);

  s = js_allocate<JSRangeData>(ctx);

  new(s) JSRangeData();

  JS_SetOpaque(ret, s);
  return ret;
}

template<class T>
static inline int
js_range_arg(JSContext* ctx, JSRangeData* out, int argc, JSValueConst argv[]) {
  int ret = 0;

  if(js_range_read(ctx, argv[0], out)) {
    ret = 1;
  } else {
    int32_t start, end;

    if(argc >= 1) {
      js_value_to(ctx, argv[ret++], start);

      if(argc >= 2)
        js_value_to(ctx, argv[ret++], end);
    }

    if(ret == 1)
      end = start;

    out->start = start;
    out->end = end;
  }

  return ret;
}

template<class T>
static inline BOOL
js_range_argument(JSContext* ctx, int argc, JSValueConst argv[], int& argind, JSRangeData* out) {
  int ret = 0;
  JSRangeData* rg;

  if((rg = js_range_data(argv[argind]))) {
    *out = *rg;
    argind++;
    return TRUE;
  } else if(js_range_read(ctx, argv[argind], out)) {
    argind++;
    return TRUE;
  }

  if(argind + 1 < argc && JS_IsNumber(argv[argind]) && JS_IsNumber(argv[argind + 1])) {
    if(js_number_read(ctx, argv[argind], &out->start) && js_number_read(ctx, argv[argind + 1], &out->end)) {
      argind += 2;
      return TRUE;
    }
  }

  return FALSE;
}

JSValue
js_range_new(JSContext* ctx, JSValueConst proto, int start, int end) {
  JSValue ret = js_range_create(ctx, proto);
  JSRangeData* s = js_range_data(ret);

  s->start = start;
  s->end = end;

  return ret;
}

JSRangeData*
js_range_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSRangeData*>(JS_GetOpaque2(ctx, val, js_range_class_id));
}

JSRangeData*
js_range_data(JSValueConst val) {
  return static_cast<JSRangeData*>(JS_GetOpaque(val, js_range_class_id));
}

JSValue
js_range_clone(JSContext* ctx, JSValueConst proto, const JSRangeData& range) {
  return js_range_new(ctx, proto, range.start, range.end);
}

extern "C" {

static JSValue
js_range_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSRangeData *rg, *other;
  JSValue obj, proto;

  if(!(rg = js_allocate<JSRangeData>(ctx)))
    return JS_EXCEPTION;

  new(rg) JSRangeData();

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_range_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, rg);

  if(argc > 0) {
    if(!js_range_read(ctx, argv[0], rg)) {
      if(!js_value_to(ctx, argv[0], rg->start))
        return JS_EXCEPTION;

      // Default end to 0 if only 1 argument provided (opencv.js compatibility)
      if(argc < 2) {
        rg->end = 0.0;
      } else if(!js_value_to(ctx, argv[1], rg->end)) {
        return JS_EXCEPTION;
      }
    }
  }

  return obj;

fail:
  js_deallocate(ctx, rg);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

enum {
  RANGE_PROP_START = 0,
  RANGE_PROP_END,
};

static JSValue
js_range_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSRangeData* s;

  if(!(s = js_range_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case RANGE_PROP_START: return js_value_from(ctx, s->start);
    case RANGE_PROP_END: return js_value_from(ctx, s->end);
  }

  return JS_UNDEFINED;
}

static JSValue
js_range_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSRangeData* s;

  if(!(s = js_range_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case RANGE_PROP_START: js_value_to(ctx, val, s->start); break;
    case RANGE_PROP_END: js_value_to(ctx, val, s->end); break;
  }

  return JS_UNDEFINED;
}

enum {
  RANGE_METHOD_SIZE,
  RANGE_METHOD_EMPTY,
  RANGE_METHOD_CLONE,
};

static JSValue
js_range_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSRangeData* s;
  JSValue ret = JS_UNDEFINED;

  if(!(s = js_range_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case RANGE_METHOD_SIZE: {
      ret = js_value_from(ctx, uint32_t(s->size()));
      break;
    }

    case RANGE_METHOD_EMPTY: {
      ret = js_value_from(ctx, s->empty());
      break;
    }

    case RANGE_METHOD_CLONE: {
      ret = js_range_new(ctx, JS_GetPrototype(ctx, this_val), s->start, s->end);
      break;
    }
  }

  return ret;
}

static JSValue
js_range_to_array(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSRangeData* s = js_range_data2(ctx, this_val);
  std::array<int32_t, 2> arr{s->start, s->end};

  return js_typedarray_from(ctx, arr.cbegin(), arr.cend());
}

static JSAtom iterator_symbol;

static JSValue
js_range_symbol_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue arr, iter;
  arr = js_range_to_array(ctx, this_val, argc, argv);

  if(iterator_symbol == 0)
    iterator_symbol = js_symbol_atom(ctx, "iterator");

  if(!js_is_function(ctx, (iter = JS_GetProperty(ctx, arr, iterator_symbol))))
    return JS_EXCEPTION;

  return JS_Call(ctx, iter, arr, 0, argv);
}

static JSValue
js_range_from(JSContext* ctx, JSValueConst range, int argc, JSValueConst argv[]) {
  std::array<int32_t, 2> array;
  JSValue ret = JS_EXCEPTION;

  if(JS_IsString(argv[0])) {
    const char* str = JS_ToCString(ctx, argv[0]);
    char* endptr = nullptr;

    for(size_t i = 0; i < 2; i++) {
      while(!isdigit(*str) && *str != '-' && *str != '+' && !(*str == '.' && isdigit(str[1])))
        str++;

      if(*str == '\0')
        break;

      array[i] = strtol(str, &endptr, 10);
      str = endptr;
    }
  } else if(js_is_array(ctx, argv[0])) {
    js_array_to(ctx, argv[0], array);
  }

  return js_range_new(ctx, array[0], array[1]);
}

static JSValue
js_range_all(JSContext* ctx, JSValueConst range, int argc, JSValueConst argv[]) {
  return js_range_clone(ctx, cv::Range::all());
}

static void
js_range_finalizer(JSRuntime* rt, JSValue val) {
  JSRangeData* s;

  if((s = static_cast<JSRangeData*>(JS_GetOpaque(val, js_range_class_id)))) {
    js_deallocate(rt, s);
  }
}

JSClassDef js_range_class = {
    .class_name = "Range",
    .finalizer = js_range_finalizer,
};

const JSCFunctionListEntry js_range_proto_funcs[] = {
    JS_CGETSET_ENUMERABLE_DEF("start", js_range_get, js_range_set, RANGE_PROP_START),
    JS_CGETSET_ENUMERABLE_DEF("end", js_range_get, js_range_set, RANGE_PROP_END),
    JS_CFUNC_MAGIC_DEF("size", 0, js_range_method, RANGE_METHOD_SIZE),
    JS_CFUNC_MAGIC_DEF("empty", 0, js_range_method, RANGE_METHOD_EMPTY),
    JS_CFUNC_MAGIC_DEF("clone", 0, js_range_method, RANGE_METHOD_CLONE),
    JS_CFUNC_DEF("toArray", 0, js_range_to_array),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, js_range_symbol_iterator),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Range", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_range_static_funcs[] = {
    JS_CFUNC_DEF("from", 1, js_range_from),
    JS_CFUNC_DEF("all", 0, js_range_all),
};

int
js_range_init(JSContext* ctx, JSModuleDef* m) {

  /* create the Range class */
  JS_NewClassID(&js_range_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_range_class_id, &js_range_class);

  range_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, range_proto, js_range_proto_funcs, countof(js_range_proto_funcs));
  JS_SetClassProto(ctx, js_range_class_id, range_proto);

  range_class = JS_NewCFunction2(ctx, js_range_constructor, "Range", 0, JS_CFUNC_constructor, 0);

  /* set proto.constructor and ctor.prototype */
  JS_SetPropertyFunctionList(ctx, range_class, js_range_static_funcs, countof(js_range_static_funcs));
  JS_SetConstructor(ctx, range_class, range_proto);

  if(m)
    JS_SetModuleExport(ctx, m, "Range", range_class);

  return 0;
}

extern "C" void
js_range_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Range");
}

#if defined(JS_RANGE_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_range
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_range_init)))
    return NULL;

  js_range_export(ctx, m);
  return m;
}
}
