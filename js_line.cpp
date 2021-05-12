#include "jsbindings.hpp"
#include "js_point.hpp"
#include "js_alloc.hpp"
#include "js_line.hpp"
#include "js_array.hpp"
#include "js_typed_array.hpp"
#include "util.hpp"
#include "line.hpp"

enum { PROP_SLOPE = 0, PROP_PIVOT, PROP_TO, PROP_ANGLE, PROP_LENGTH };
enum { METHOD_SWAP = 0, METHOD_AT, METHOD_INTERSECT, METHOD_ENDPOINT_DISTANCES, METHOD_DISTANCE };

extern "C" {

JSValue line_proto = JS_UNDEFINED, line_class = JS_UNDEFINED;
JSClassID js_line_class_id = 0;

VISIBLE JSValue
js_line_new(JSContext* ctx, double x1, double y1, double x2, double y2) {
  JSValue ret;
  JSLineData<double>* ln;

  if(JS_IsUndefined(line_proto))
    js_line_init(ctx, NULL);

  ret = JS_NewObjectProtoClass(ctx, line_proto, js_line_class_id);

  ln = js_allocate<JSLineData<double>>(ctx);

  ln->array[0] = x1;
  ln->array[1] = y1;
  ln->array[2] = x2;
  ln->array[3] = y2;

  JS_SetOpaque(ret, ln);
  return ret;
}

static JSValue
js_line_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  JSLineData<double>* ln;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;
  auto args = argument_range(std::min(4, argc), argv);

  ln = js_allocate<JSLineData<double>>(ctx);
  if(!ln)
    return JS_EXCEPTION;

  if(argc >= 4 && std::ranges::all_of(args, JS_IsNumber)) {
    if(JS_ToFloat64(ctx, &ln->array[0], argv[0]))
      goto fail;
    if(JS_ToFloat64(ctx, &ln->array[1], argv[1]))
      goto fail;
    if(JS_ToFloat64(ctx, &ln->array[2], argv[2]))
      goto fail;
    if(JS_ToFloat64(ctx, &ln->array[3], argv[3]))
      goto fail;
  } else if(!js_line_read(ctx, argv[0], ln)) {
    return JS_ThrowTypeError(ctx, "argument 1 is not a valid line");
  }
  /* using new_target to get the prototype is necessary when the
     class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_line_class_id);
  JS_FreeValue(ctx, proto);
  if(JS_IsException(obj))
    goto fail;
  JS_SetOpaque(obj, ln);
  return obj;
fail:
  js_deallocate(ctx, ln);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSLineData<double>*
js_line_data(JSContext* ctx, JSValueConst val) {
  return static_cast<JSLineData<double>*>(JS_GetOpaque2(ctx, val, js_line_class_id));
}

static JSValue
js_line_get_xy12(JSContext* ctx, JSValueConst this_val, int magic) {
  JSValue ret = JS_UNDEFINED;
  JSLineData<double>* ln = static_cast<JSLineData<double>*>(JS_GetOpaque2(ctx, this_val, js_line_class_id));
  if(!ln)
    ret = JS_EXCEPTION;
  else if(magic == 0)
    ret = JS_NewFloat64(ctx, ln->array[0]);
  else if(magic == 1)
    ret = JS_NewFloat64(ctx, ln->array[1]);
  else if(magic == 2)
    ret = JS_NewFloat64(ctx, ln->array[2]);
  else if(magic == 3)
    ret = JS_NewFloat64(ctx, ln->array[3]);
  return ret;
}

static JSValue
js_line_get_ab(JSContext* ctx, JSValueConst this_val, int magic) {
  JSValue ret = JS_UNDEFINED;
  JSLineData<double>* ln = static_cast<JSLineData<double>*>(JS_GetOpaque2(ctx, this_val, js_line_class_id));
  if(!ln)
    ret = JS_EXCEPTION;
  else if(magic == 0)
    ret = js_point_new(ctx, ln->array[0], ln->array[1]);
  else if(magic == 1)
    ret = js_point_new(ctx, ln->array[2], ln->array[3]);

  return ret;
}

static JSValue
js_line_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSLineData<double>* ln;

  if(!(ln = static_cast<JSLineData<double>*>(JS_GetOpaque2(ctx, this_val, js_line_class_id))))
    return JS_EXCEPTION;
  switch(magic) {
    case PROP_SLOPE: {
      Line<double> line(ln->array);
      return js_point_wrap(ctx, line.slope());
    }
    case PROP_PIVOT: {
      JSPointData<double> pivot(ln->x1, ln->y1);
      return js_point_wrap(ctx, pivot);
    }
    case PROP_TO: {
      JSPointData<double> to(ln->x2, ln->y2);
      return js_point_wrap(ctx, to);
    }
    case PROP_ANGLE: {
      Line<double> line(ln->array);
      return JS_NewFloat64(ctx, std::atan2(ln->x2 - ln->x1, ln->y2 - ln->y1));
    }
    case PROP_LENGTH: {
      Line<double> line(ln->array);
      return JS_NewFloat64(ctx, line.length());
    }
  }
  return JS_UNDEFINED;
}

static JSValue
js_line_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSLineData<double>* ln;
  double v;
  if(!(ln = static_cast<JSLineData<double>*>(JS_GetOpaque2(ctx, this_val, js_line_class_id))))
    return JS_EXCEPTION;
  switch(magic) {
    case PROP_PIVOT: {
      JSPointData<double> pivot;
      js_point_read(ctx, val, &pivot);

      ln->x1 = pivot.x;
      ln->y1 = pivot.y;
      break;
    }
    case PROP_TO: {
      JSPointData<double> to;
      js_point_read(ctx, val, &to);

      ln->x2 = to.x;
      ln->y2 = to.y;
      break;
    }
  }

  return JS_UNDEFINED;
}

static JSValue
js_line_set_xy12(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSLineData<double>* ln = static_cast<JSLineData<double>*>(JS_GetOpaque2(ctx, this_val, js_line_class_id));
  double v;
  if(!ln)
    return JS_EXCEPTION;
  if(JS_ToFloat64(ctx, &v, val))
    return JS_EXCEPTION;
  if(magic == 0)
    ln->array[0] = v;
  else if(magic == 1)
    ln->array[1] = v;
  else if(magic == 2)
    ln->array[2] = v;
  else if(magic == 3)
    ln->array[3] = v;

  return JS_UNDEFINED;
}

static JSValue
js_line_set_ab(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSLineData<double>* ln;
  JSPointData<double> pt;

  if(!(ln = static_cast<JSLineData<double>*>(JS_GetOpaque2(ctx, this_val, js_line_class_id))))
    return JS_EXCEPTION;

  js_point_read(ctx, val, &pt);

  if(magic == 0) {
    ln->array[0] = pt.x;
    ln->array[1] = pt.y;

  } else if(magic == 1) {
    ln->array[2] = pt.x;
    ln->array[3] = pt.y;
  }

  return JS_UNDEFINED;
}

static JSValue
js_line_points(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSLineData<double>* ln;

  if(!(ln = static_cast<JSLineData<double>*>(JS_GetOpaque2(ctx, this_val, js_line_class_id))))
    return JS_EXCEPTION;

  return js_array_from(ctx, ln->points);
}

static JSValue
js_line_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSLineData<double>* ln = js_line_data(ctx, this_val);
  JSValue obj = JS_NewObjectProto(ctx, line_proto);

  JS_DefinePropertyValueStr(ctx, obj, "x1", JS_NewFloat64(ctx, ln->x1), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "y1", JS_NewFloat64(ctx, ln->y1), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "x2", JS_NewFloat64(ctx, ln->x2), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "y2", JS_NewFloat64(ctx, ln->y2), JS_PROP_ENUMERABLE);
  return obj;
}

static JSValue
js_line_methods(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSLineData<double>* ln;
  JSValue ret = JS_UNDEFINED;
  if(!(ln = static_cast<JSLineData<double>*>(JS_GetOpaque2(ctx, this_val, js_line_class_id))))
    return JS_EXCEPTION;

  switch(magic) {
    case METHOD_SWAP: {
      JSPointData<double> a = ln->points[0], b = ln->points[1];
      ln->points[0] = b;
      ln->points[1] = a;
      ret = JS_DupValue(ctx, this_val);
      break;
    }
    case METHOD_AT: {
      double sigma;
      JSPointData<double> p;
      js_value_to(ctx, argv[0], sigma);

      sigma = fmin(fmax(sigma, 1), 0);

      p.x = ln->points[0].x * (1.0 - sigma) + ln->points[1].x * sigma;
      p.y = ln->points[0].y * (1.0 - sigma) + ln->points[1].y * sigma;

      ret = js_point_wrap(ctx, p);
      break;
    }
    case METHOD_INTERSECT: {
      std::array<double, 4> arg = js_line_get<double>(ctx, argv[0]);
      JSPointData<double>* point = nullptr;

      if(argc > 1)
        point = js_point_data(ctx, argv[1]);

      Line<double> line = Line(ln->array);
      Line<double> other = Line(arg);

      ret = JS_NewBool(ctx, line.intersect(other, point));
      break;
    }
    case METHOD_ENDPOINT_DISTANCES: {
      JSPointData<double> pt;
      js_point_read(ctx, argv[0], &pt);
      Line<double> line = Line(ln->array);
      auto distances = line.endpoint_distances(pt);
      ret = JS_NewArray(ctx);
      JS_SetPropertyUint32(ctx, ret, 0, js_number_new(ctx, distances.first));
      JS_SetPropertyUint32(ctx, ret, 1, js_number_new(ctx, distances.second));
      break;
    }
    case METHOD_DISTANCE: {
      cv::Point2d pt;
      js_point_read(ctx, argv[0], &pt);
      Line<double> line = Line(ln->array);
      ret = js_number_new(ctx, line.distance(pt));
      break;
    }
  }
  return ret;
}

static JSValue
js_line_toarray(JSContext* ctx, JSValueConst line, int argc, JSValueConst* arg) {
  JSLineData<double>* ln;

  if(!(ln = static_cast<JSLineData<double>*>(JS_GetOpaque2(ctx, line, js_line_class_id))))
    return JS_EXCEPTION;

  return js_typedarray_from(ctx, ln->array);
}

static JSValue
js_call_method(JSContext* ctx, JSValue obj, const char* name, int argc, JSValueConst* argv) {
  JSValue fn, ret = JS_UNDEFINED;

  fn = JS_GetPropertyStr(ctx, obj, name);
  if(!JS_IsUndefined(fn))
    ret = JS_Call(ctx, fn, obj, argc, argv);
  return ret;
}

#define JS_LINE_AS_POINTS 0x01
#define JS_LINE_AS_VECTOR 0x02

#define JS_LINE_GET_ITERATOR 0x80
#define JS_LINE_TO_STRING 0x40

static JSValue
js_line_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSLineData<double>* ln = static_cast<JSLineData<double>*>(JS_GetOpaque2(ctx, this_val, js_line_class_id));
  JSValue method, ret = JS_UNDEFINED;

  if(magic & JS_LINE_AS_POINTS)
    ret = js_line_points(ctx, this_val, argc, argv);
  if(magic & JS_LINE_AS_VECTOR)
    ret = js_line_toarray(ctx, this_val, argc, argv);

  if(magic & JS_LINE_GET_ITERATOR)
    ret = js_call_method(ctx, ret, "values", 0, NULL);

  if(magic & JS_LINE_TO_STRING) {
    JSValueConst args[1] = {0};
    args[0] = JS_NewString(ctx, " ");

    ret = js_call_method(ctx, ret, "join", 1, args);
  }

  return ret;
}
/*
static JSValue
js_line_symbol_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSValue arr, iter;
  jsrt js(ctx);
  arr = js_line_to_array(ctx, this_val, argc, argv);

  if(JS_IsUndefined(iterator_symbol))
    iterator_symbol = js.get_symbol("iterator");

  if(!JS_IsFunction(ctx, (iter = js.get_property(arr, iterator_symbol))))
    return JS_EXCEPTION;
  return JS_Call(ctx, iter, arr, 0, argv);
}*/

static JSValue
js_line_from(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSLineData<double> line = {0, 0, 0, 0};
  JSValue ret = JS_EXCEPTION;

  if(JS_IsString(argv[0])) {
    const char* str = JS_ToCString(ctx, argv[0]);
    char* endptr = nullptr;
    for(size_t i = 0; i < 4; i++) {
      while(!isdigit(*str) && *str != '-' && *str != '+' && !(*str == '.' && isdigit(str[1]))) str++;
      if(*str == '\0')
        break;
      line.array[i] = strtod(str, &endptr);
      str = endptr;
    }
  } else {
    js_line_read(ctx, argv[0], &line);
  }
  if(line.array[2] > 0 && line.array[3] > 0)
    ret = js_line_new(ctx, line.array[0], line.array[1], line.array[2], line.array[3]);
  return ret;
}

void
js_line_finalizer(JSRuntime* rt, JSValue val) {
  JSLineData<double>* ln;

  if((ln = static_cast<JSLineData<double>*>(JS_GetOpaque(val, js_line_class_id))))
    /* Note: 'ln' can be NULL in case JS_SetOpaque() was not called */
    js_deallocate(rt, ln);
}

JSClassDef js_line_class = {
    .class_name = "Line",
    .finalizer = js_line_finalizer,
};

const JSCFunctionListEntry js_line_proto_funcs[] = {
    JS_CGETSET_ENUMERABLE_DEF("x1", js_line_get_xy12, js_line_set_xy12, 0),
    JS_CGETSET_ENUMERABLE_DEF("y1", js_line_get_xy12, js_line_set_xy12, 1),
    JS_CGETSET_ENUMERABLE_DEF("x2", js_line_get_xy12, js_line_set_xy12, 2),
    JS_CGETSET_ENUMERABLE_DEF("y2", js_line_get_xy12, js_line_set_xy12, 3),
    JS_CGETSET_MAGIC_DEF("a", js_line_get_ab, js_line_set_ab, 0),
    JS_CGETSET_MAGIC_DEF("b", js_line_get_ab, js_line_set_ab, 1),
    JS_CGETSET_MAGIC_DEF("0", js_line_get_ab, js_line_set_ab, 0),
    JS_CGETSET_MAGIC_DEF("1", js_line_get_ab, js_line_set_ab, 1),
    JS_CGETSET_MAGIC_DEF("slope", js_line_get, 0, PROP_SLOPE),
    JS_CGETSET_MAGIC_DEF("angle", js_line_get, 0, PROP_ANGLE),
    JS_CGETSET_MAGIC_DEF("length", js_line_get, 0, PROP_LENGTH),
    JS_CGETSET_MAGIC_DEF("pivot", js_line_get, js_line_set, PROP_PIVOT),
    JS_CGETSET_MAGIC_DEF("to", js_line_get, js_line_set, PROP_TO),
    JS_CFUNC_MAGIC_DEF("swap", 0, js_line_methods, METHOD_SWAP),
    JS_CFUNC_MAGIC_DEF("at", 1, js_line_methods, METHOD_AT),
    JS_CFUNC_MAGIC_DEF("intersect", 1, js_line_methods, METHOD_INTERSECT),
    JS_CFUNC_MAGIC_DEF("endpointDistances", 1, js_line_methods, METHOD_ENDPOINT_DISTANCES),
    JS_CFUNC_MAGIC_DEF("distance", 1, js_line_methods, METHOD_DISTANCE),
    JS_CFUNC_DEF("toArray", 0, js_line_toarray),
    JS_CFUNC_MAGIC_DEF("toPoints", 0, js_line_iterator, JS_LINE_AS_POINTS),
    JS_CFUNC_MAGIC_DEF("toString", 0, js_line_iterator, JS_LINE_AS_POINTS | JS_LINE_TO_STRING),
    JS_CFUNC_MAGIC_DEF("values", 0, js_line_iterator, JS_LINE_AS_VECTOR | JS_LINE_GET_ITERATOR),
    JS_ALIAS_DEF("start", "pivot"),
    JS_ALIAS_DEF("end", "to"),
    JS_ALIAS_DEF("[Symbol.iterator]", "values"),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Line", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_line_static_funcs[] = {JS_CFUNC_DEF("from", 1, js_line_from)};

int
js_line_init(JSContext* ctx, JSModuleDef* m) {

  if(js_line_class_id == 0) {
    /* create the Line class */
    JS_NewClassID(&js_line_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_line_class_id, &js_line_class);

    line_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, line_proto, js_line_proto_funcs, countof(js_line_proto_funcs));
    JS_SetClassProto(ctx, js_line_class_id, line_proto);

    line_class = JS_NewCFunction2(ctx, js_line_ctor, "Line", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, line_class, line_proto);
    JS_SetPropertyFunctionList(ctx, line_class, js_line_static_funcs, countof(js_line_static_funcs));

    js_set_inspect_method(ctx, line_proto, js_line_inspect);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Line", line_class);

  return 0;
}

void
js_line_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(line_class))
    js_line_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "Line", line_class);
}

#ifdef JS_LINE_MODULE
#define JS_INIT_MODULE /*VISIBLE*/ js_init_module
#else
#define JS_INIT_MODULE /*VISIBLE*/ js_init_module_line
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_line_init);
  if(!m)
    return NULL;
  JS_AddModuleExport(ctx, m, "Line");
  return m;
}
}
