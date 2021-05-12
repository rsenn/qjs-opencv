#include "jsbindings.hpp"
#include "js_size.hpp"
#include "js_array.hpp"
#include "js_alloc.hpp"
#include "util.hpp"

enum js_size_fit_t { JS_SIZE_FIT_WIDTH = 1, JS_SIZE_FIT_HEIGHT = 2, JS_SIZE_FIT_INSIDE, JS_SIZE_FIT_OUTSIDE };

extern "C" {
JSValue size_proto = JS_UNDEFINED, size_class = JS_UNDEFINED;
JSClassID js_size_class_id = 0;
}

template<class T>
static inline JSSizeData<T>
js_size_fit(const JSSizeData<T>& size, T to, js_size_fit_t mode) {
  JSSizeData<T> ret;
  switch(mode) {
    case JS_SIZE_FIT_WIDTH: {
      ret.width = to;
      ret.height = size.height * (to / size.width);
      break;
    }
    case JS_SIZE_FIT_HEIGHT: {
      ret.width = size.width * (to / size.height);
      ret.height = to;
      break;
    }
  }
  return ret;
}

template<class T>
static JSSizeData<T>
js_size_fit(const JSSizeData<T>& size, const JSSizeData<T>& to, js_size_fit_t mode) {
  JSSizeData<T> ret;
  switch(mode) {
    case JS_SIZE_FIT_INSIDE:
    case JS_SIZE_FIT_OUTSIDE: {
      ret = js_size_fit(size, to, JS_SIZE_FIT_WIDTH);
      if(mode == JS_SIZE_FIT_INSIDE && ret.height > to.height)
        ret = js_size_fit(size, to.height, JS_SIZE_FIT_HEIGHT);

      break;
    }
    default: ret = js_size_fit(size, mode == JS_SIZE_FIT_WIDTH ? to.width : to.height, mode); break;
  }
  return ret;
}

static JSValue
js_size_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  JSSizeData<double> size, *s;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;

  s = js_allocate<JSSizeData<double>>(ctx);
  if(!s)
    return JS_EXCEPTION;
  new(s) JSSizeData<double>();

  if(argc > 0) {
    if(js_size_read(ctx, argv[0], &size)) {
      *s = size;
    } else {
      if(JS_ToFloat64(ctx, &size.width, argv[0]))
        goto fail;
      if(argc < 2 || JS_ToFloat64(ctx, &size.height, argv[1]))
        goto fail;

      *s = size;
    }
  }

  /* using new_target to get the prototype is necessary when the
     class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_size_class_id);
  JS_FreeValue(ctx, proto);
  if(JS_IsException(obj))
    goto fail;
  JS_SetOpaque(obj, s);
  return obj;
fail:
  js_deallocate(ctx, s);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

VISIBLE JSSizeData<double>*
js_size_data(JSContext* ctx, JSValueConst val) {
  return static_cast<JSSizeData<double>*>(JS_GetOpaque2(ctx, val, js_size_class_id));
}

static JSValue
js_size_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSSizeData<double>* s = js_size_data(ctx, this_val);
  if(!s)
    return JS_EXCEPTION;
  if(magic == 0)
    return JS_NewFloat64(ctx, s->width);
  else if(magic == 1)
    return JS_NewFloat64(ctx, s->height);
  else if(magic == 2)
    return JS_NewFloat64(ctx, s->aspectRatio());
  else if(magic == 3)
    return JS_NewBool(ctx, s->empty());
  else if(magic == 4)
    return JS_NewFloat64(ctx, s->area());
  return JS_UNDEFINED;
}

VISIBLE JSValue
js_size_new(JSContext* ctx, double w, double h) {
  JSValue ret;
  JSSizeData<double>* s;

  if(JS_IsUndefined(size_proto))
    js_size_init(ctx, NULL);

  ret = JS_NewObjectProtoClass(ctx, size_proto, js_size_class_id);

  s = js_allocate<JSSizeData<double>>(ctx);
  s->width = w <= DBL_EPSILON ? 0 : w;
  s->height = h <= DBL_EPSILON ? 0 : h;

  JS_SetOpaque(ret, s);
  return ret;
}

VISIBLE JSValue
js_size_wrap(JSContext* ctx, const JSSizeData<double>& sz) {
  return js_size_new(ctx, sz.width, sz.height);
}

static JSValue
js_size_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSSizeData<double>* s = js_size_data(ctx, this_val);
  double v;
  if(!s)
    return JS_EXCEPTION;
  if(JS_ToFloat64(ctx, &v, val))
    return JS_EXCEPTION;
  if(magic == 0)
    s->width = v;
  else
    s->height = v;
  return JS_UNDEFINED;
}

static JSValue
js_size_to_string(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSSizeData<double> size, *s;
  std::ostringstream os;
  const char* delim = "×";
  if((s = js_size_data(ctx, this_val)) == nullptr) {
    js_size_read(ctx, this_val, &size);
  } else {
    size = *s;
  }

  if(argc > 0)
    delim = JS_ToCString(ctx, argv[0]);

  os << size.width << delim << size.height;

  return JS_NewString(ctx, os.str().c_str());
}
static JSValue
js_size_to_source(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSSizeData<double> size, *s;
  std::ostringstream os;
  if((s = js_size_data(ctx, this_val)) == nullptr) {
    js_size_read(ctx, this_val, &size);
  } else {
    size = *s;
  }
  os << "{width:" << size.width << ",height:" << size.height << "}";

  return JS_NewString(ctx, os.str().c_str());
}

static JSValue
js_size_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSSizeData<double>* s = js_size_data(ctx, this_val);
  JSValue obj = JS_NewObjectProto(ctx, size_proto);

  JS_DefinePropertyValueStr(ctx, obj, "width", JS_NewFloat64(ctx, s->width), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "height", JS_NewFloat64(ctx, s->height), JS_PROP_ENUMERABLE);
  return obj;
}

static JSValue
js_size_funcs(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSSizeData<double> size, *s, *a;
  JSValue ret = JS_UNDEFINED;
  if((s = js_size_data(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  size = *s;

  if(magic == 0) {
    a = js_size_data(ctx, argv[0]);
    bool equals = s->width == a->width && s->height == a->height;
    ret = JS_NewBool(ctx, equals);
  } else if(magic == 1) {
    double width, height, f;
    int32_t precision = 0;
    if(argc > 0)
      JS_ToInt32(ctx, &precision, argv[0]);
    f = std::pow(10, precision);
    width = std::round(size.width * f) / f;
    height = std::round(size.height * f) / f;
    ret = js_size_new(ctx, width, height);
  } else if(magic == 2) {
    ret = JS_NewObject(ctx);

    JS_SetPropertyStr(ctx, ret, "width", JS_NewFloat64(ctx, size.width));
    JS_SetPropertyStr(ctx, ret, "height", JS_NewFloat64(ctx, size.height));
  } else if(magic == 3) {
    ret = JS_NewArray(ctx);

    JS_SetPropertyUint32(ctx, ret, 0, JS_NewFloat64(ctx, size.width));
    JS_SetPropertyUint32(ctx, ret, 1, JS_NewFloat64(ctx, size.height));
  } else if(magic == 4 || magic == 5 || magic == 6 || magic == 7) {
    JSSizeData<double> other, result;
    double arg;

    js_size_fit_t fit_type = js_size_fit_t(magic - 3);
    if(magic >= 6) {}

    if(js_size_read(ctx, argv[0], &other)) {
      result = js_size_fit(size, other, fit_type);

    } else if(fit_type == JS_SIZE_FIT_WIDTH || fit_type == JS_SIZE_FIT_HEIGHT) {
      JS_ToFloat64(ctx, &arg, argv[0]);
      result = js_size_fit(size, arg, fit_type);
    }

    if(!result.empty())
      ret = js_size_new(ctx, result.width, result.height);
  }
  return ret;
}

static JSValue
js_size_add(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSSizeData<double>*s, size;

  if((s = js_size_data(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  if(js_size_read(ctx, argv[0], &size)) {
    s->width += size.width;
    s->height += size.height;
  } else if(argc >= 2 && JS_IsNumber(argv[0]) && JS_IsNumber(argv[1])) {
    double width, height;

    JS_ToFloat64(ctx, &width, argv[0]);
    JS_ToFloat64(ctx, &height, argv[1]);
    s->width += width;
    s->height += height;
  }

  return JS_DupValue(ctx, this_val);
}

static JSValue
js_size_sub(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSSizeData<double>*s, size;

  if((s = js_size_data(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  if(js_size_read(ctx, argv[0], &size)) {
    s->width -= size.width;
    s->height -= size.height;
  } else if(argc >= 2 && JS_IsNumber(argv[0]) && JS_IsNumber(argv[1])) {
    double width, height;

    JS_ToFloat64(ctx, &width, argv[0]);
    JS_ToFloat64(ctx, &height, argv[1]);
    s->width -= width;
    s->height -= height;
  }

  return JS_DupValue(ctx, this_val);
}

static JSValue
js_size_mul(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSSizeData<double> size, *s = js_size_data(ctx, this_val);
  double factor;
  JS_ToFloat64(ctx, &factor, argv[0]);

  size = *s;
  size.width *= factor;
  size.height *= factor;

  return js_size_wrap(ctx, size);
}

static JSValue
js_size_div(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSSizeData<double> size, other, *s, *a = nullptr;
  JSValue ret = JS_EXCEPTION;
  if((s = js_size_data(ctx, this_val)) != nullptr) {
    size = *s;

    if(JS_IsNumber(argv[0])) {
      double divider;
      JS_ToFloat64(ctx, &divider, argv[0]);
      size.width /= divider;
      size.height /= divider;
      ret = js_size_wrap(ctx, size);
    } else {
      if((a = js_size_data(ctx, argv[0])) != nullptr)
        other = *a;
      else {
        js_size_read(ctx, argv[0], &other);
        a = &other;
      }
      std::array<double, 2> result{size.width / other.width, size.height / other.height};
      ret = JS_NewArray(ctx);

      JS_SetPropertyUint32(ctx, ret, 0, JS_NewFloat64(ctx, result[0]));
      JS_SetPropertyUint32(ctx, ret, 1, JS_NewFloat64(ctx, result[1]));
    }
  }
  return ret;
}

static JSAtom iterator_symbol;

static JSValue
js_size_symbol_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSValue arr, iter;
  arr = js_size_funcs(ctx, this_val, argc, argv, 3);

  if(iterator_symbol == 0)
    iterator_symbol = js_symbol_atom(ctx, "iterator");

  if(!JS_IsFunction(ctx, (iter = JS_GetProperty(ctx, arr, iterator_symbol))))
    return JS_EXCEPTION;
  return JS_Call(ctx, iter, arr, 0, argv);
}

static JSValue
js_size_from(JSContext* ctx, JSValueConst size, int argc, JSValueConst* argv) {
  std::array<double, 2> array;
  JSValue ret = JS_EXCEPTION;

  if(JS_IsString(argv[0])) {
    const char* str = JS_ToCString(ctx, argv[0]);
    char* endptr = nullptr;
    for(size_t i = 0; i < 2; i++) {
      while(!isdigit(*str) && *str != '-' && *str != '+' && !(*str == '.' && isdigit(str[1]))) str++;
      if(*str == '\0')
        break;
      array[i] = strtod(str, &endptr);
      str = endptr;
    }
  } else if(js_is_array(ctx, argv[0])) {
    js_array_to<double, 2>(ctx, argv[0], array);
  }
  if(array[0] > 0 && array[1] > 0)
    ret = js_size_new(ctx, array[0], array[1]);
  return ret;
}

void
js_size_finalizer(JSRuntime* rt, JSValue val) {
  JSSizeData<double>* s = static_cast<JSSizeData<double>*>(JS_GetOpaque(val, js_size_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */
  if(s)
    js_deallocate(rt, s);
}

JSClassDef js_size_class = {
    .class_name = "Size",
    .finalizer = js_size_finalizer,
};

const JSCFunctionListEntry js_size_proto_funcs[] = {JS_CGETSET_ENUMERABLE_DEF("width", js_size_get, js_size_set, 0),
                                                    JS_CGETSET_ENUMERABLE_DEF("height", js_size_get, js_size_set, 1),
                                                    JS_CGETSET_ENUMERABLE_DEF("aspect", js_size_get, 0, 2),
                                                    JS_CGETSET_ENUMERABLE_DEF("empty", js_size_get, 0, 3),
                                                    JS_CGETSET_ENUMERABLE_DEF("area", js_size_get, 0, 4),
                                                    JS_CFUNC_MAGIC_DEF("equals", 1, js_size_funcs, 0),
                                                    JS_CFUNC_MAGIC_DEF("round", 0, js_size_funcs, 1),
                                                    JS_CFUNC_MAGIC_DEF("toObject", 0, js_size_funcs, 2),
                                                    JS_CFUNC_MAGIC_DEF("toArray", 0, js_size_funcs, 3),
                                                    JS_CFUNC_MAGIC_DEF("fitWidth", 0, js_size_funcs, 4),
                                                    JS_CFUNC_MAGIC_DEF("fitHeight", 0, js_size_funcs, 5),
                                                    JS_CFUNC_MAGIC_DEF("fitInside", 0, js_size_funcs, 6),
                                                    JS_CFUNC_MAGIC_DEF("fitOutside", 0, js_size_funcs, 7),
                                                    JS_CFUNC_DEF("toString", 0, js_size_to_string),
                                                    JS_CFUNC_DEF("toSource", 0, js_size_to_source),
                                                    JS_CFUNC_DEF("add", 1, js_size_add),
                                                    JS_CFUNC_DEF("sub", 1, js_size_sub),
                                                    JS_CFUNC_DEF("mul", 1, js_size_mul),
                                                    JS_CFUNC_DEF("div", 1, js_size_div),
                                                    JS_ALIAS_DEF("values", "toArray"),
                                                    JS_CFUNC_DEF("[Symbol.iterator]", 0, js_size_symbol_iterator),
                                                    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Size", JS_PROP_CONFIGURABLE)};

const JSCFunctionListEntry js_size_static_funcs[] = {JS_CFUNC_DEF("from", 1, js_size_from)};

int
js_size_init(JSContext* ctx, JSModuleDef* m) {

  if(js_size_class_id == 0) {
    /* create the Size class */
    JS_NewClassID(&js_size_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_size_class_id, &js_size_class);

    size_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, size_proto, js_size_proto_funcs, countof(js_size_proto_funcs));

    JS_SetClassProto(ctx, js_size_class_id, size_proto);

    size_class = JS_NewCFunction2(ctx, js_size_ctor, "Size", 0, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, size_class, size_proto);
    JS_SetPropertyFunctionList(ctx, size_class, js_size_static_funcs, countof(js_size_static_funcs));

    js_set_inspect_method(ctx, size_proto, js_size_inspect);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Size", size_class);
  /*else
    JS_SetPropertyStr(ctx, *static_cast<JSValue*>(m), name, size_class);*/
  return 0;
}

extern "C" VISIBLE void
js_size_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Size");
}

void
js_size_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(size_class))
    js_size_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "Size", size_class);
}

#ifdef JS_SIZE_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_size
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_size_init);
  if(!m)
    return NULL;
  js_size_export(ctx, m);
  return m;
}