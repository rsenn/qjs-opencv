#include "js_point.hpp"
#include "js_alloc.hpp"
#include "include/js_array.hpp"
#include "js_rect.hpp"
#include "js_size.hpp"
#include "js_typed_array.hpp"
#include "jsbindings.hpp"
#include "include/util.hpp"
#include "include/geometry.hpp"
#include <quickjs.h>
#include <math.h>
#include <cctype>
#include <cstdlib>
#include <array>
#include <new>
#include <ostream>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795029
#endif

std::vector<JSPointData<double>*> points;

extern "C" {

thread_local JSValue point_proto = JS_UNDEFINED, point_class = JS_UNDEFINED;
thread_local JSClassID js_point_class_id = 0;

JSValue
js_point_create(JSContext* ctx, JSValueConst proto) {
  JSValue ret;
  JSPointData<double>* s;

  if(JS_IsUndefined(point_proto))
    js_point_init(ctx, NULL);

  if(JS_IsUndefined(proto))
    proto = point_proto;

  ret = JS_NewObjectProtoClass(ctx, proto, js_point_class_id);

  s = js_allocate<JSPointData<double>>(ctx);

  new(s) JSPointData<double>();

  JS_SetOpaque(ret, s);
  return ret;
}
}

template<class T>
static inline int
js_point_arg(JSContext* ctx, JSPointData<T>* out, int argc, JSValueConst argv[]) {
  int ret = 0;
  if(js_point_read(ctx, argv[0], out)) {
    ret = 1;
  } else {
    double x, y;
    if(argc >= 1) {
      JS_ToFloat64(ctx, &x, argv[ret++]);
      if(argc >= 2)
        JS_ToFloat64(ctx, &y, argv[ret++]);
    }
    if(ret == 1)
      y = x;
    out->x = x;
    out->y = y;
  }

  return ret;
}

template<class T>
static inline BOOL
js_point_argument(JSContext* ctx, int argc, JSValueConst argv[], int& argind, JSPointData<T>* out) {
  int ret = 0;

  JSPointData<T>* pt;

  if((pt = js_point_data(argv[argind]))) {
    *out = *pt;
    argind++;
    return TRUE;
  } else if(js_point_read(ctx, argv[argind], out)) {
    argind++;
    return TRUE;
  }

  if(argind + 1 < argc && JS_IsNumber(argv[argind]) && JS_IsNumber(argv[argind + 1])) {
    if(js_number_read(ctx, argv[argind], &out->x) && js_number_read(ctx, argv[argind + 1], &out->y)) {
      argind += 2;
      return TRUE;
    }
  }

  return FALSE;
}

JSValue
js_point_new(JSContext* ctx, JSValueConst proto, double x, double y) {
  JSValue ret = js_point_create(ctx, proto);
  JSPointData<double>* s = js_point_data(ret);

  s->x = x;
  s->y = y;

  return ret;
}

JSPointData<double>*
js_point_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSPointData<double>*>(JS_GetOpaque2(ctx, val, js_point_class_id));
}

JSPointData<double>*
js_point_data(JSValueConst val) {
  return static_cast<JSPointData<double>*>(JS_GetOpaque(val, js_point_class_id));
}

JSValue
js_point_clone(JSContext* ctx, JSValueConst proto, const JSPointData<double>& point) {
  return js_point_new(ctx, proto, point.x, point.y);
}

extern "C" {

static JSValue
js_point_cross(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSPointData<double>*s, *other;
  double retval;

  if(!(s = js_point_data2(ctx, this_val)) || !(other = js_point_data2(ctx, argv[0])))
    return JS_EXCEPTION;

  retval = s->cross(*other);

  return JS_NewFloat64(ctx, retval);
}

/*static JSValue
js_point_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSPointData<double> point = {0, 0};
  JSValue proto;

  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    return JS_EXCEPTION;

  if(argc > 0) {
    if(!js_point_read(ctx, argv[0], &point)) {
      if(JS_ToFloat64(ctx, &point.x, argv[0]))
        return JS_EXCEPTION;

      if(argc < 2 || JS_ToFloat64(ctx, &point.y, argv[1]))
        return JS_EXCEPTION;
    }
  }

  return js_point_new(ctx, proto, point);
}*/

static JSValue
js_point_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSPointData<double>*pt, *other;
  JSValue obj, proto;

  if(!(pt = js_allocate<JSPointData<double>>(ctx)))
    return JS_EXCEPTION;

  new(pt) JSPointData<double>();

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_point_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, pt);

  if(argc > 0) {
    if(!js_point_read(ctx, argv[0], pt)) {
      if(JS_ToFloat64(ctx, &pt->x, argv[0]))
        return JS_EXCEPTION;

      if(argc < 2 || JS_ToFloat64(ctx, &pt->y, argv[1]))
        return JS_EXCEPTION;
    }
  }

  return obj;

fail:
  js_deallocate(ctx, pt);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

static JSValue
js_point_ddot(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSPointData<double>* s = js_point_data2(ctx, this_val);
  JSPointData<double>* other = js_point_data2(ctx, argv[0]);
  double retval;

  if(!s || !other)
    return JS_EXCEPTION;

  retval = s->ddot(*other);

  return JS_NewFloat64(ctx, retval);
}

static JSValue
js_point_difference(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret;
  int argind = 0;
  JSPointData<double>*s, arg, point;

  if(!(s = js_point_data2(ctx, this_val)))
    return JS_EXCEPTION;

  point = *s;

  while(argind < argc) {
    if(!js_point_argument(ctx, argc, argv, argind, &arg))
      break;
    point.x = arg.x - point.x;
    point.y = arg.y - point.y;
  }

  ret = js_point_new(ctx, JS_GetPrototype(ctx, this_val), point.x, point.y);
  return ret;
}

static JSValue
js_point_adjacent(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;
  BOOL result = FALSE;
  int argind = 0;
  JSPointData<double>*s, arg, point;

  if(!(s = js_point_data2(ctx, this_val)))
    return JS_EXCEPTION;

  point = *s;

  while(argind < argc) {
    if(!js_point_argument(ctx, argc, argv, argind, &arg))
      break;

    if((result = point_adjacent<int>(point, arg)))
      break;
  }

  ret = JS_NewBool(ctx, result);
  return ret;
}

enum { POINT_PROP_X = 0, POINT_PROP_Y };

static JSValue
js_point_get_xy(JSContext* ctx, JSValueConst this_val, int magic) {
  JSPointData<double>* s;

  if(!(s = js_point_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case POINT_PROP_X: return JS_NewFloat64(ctx, s->x);
    case POINT_PROP_Y: return JS_NewFloat64(ctx, s->y);
  }

  return JS_UNDEFINED;
}

static JSValue
js_point_set_xy(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSPointData<double>* s;

  if(!(s = js_point_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case POINT_PROP_X: JS_ToFloat64(ctx, &s->x, val); break;

    case POINT_PROP_Y: JS_ToFloat64(ctx, &s->y, val); break;
  }

  return JS_UNDEFINED;
}

static JSValue
js_point_inside(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSPointData<double>* s = js_point_data2(ctx, this_val);
  JSRectData<double> r = js_rect_get(ctx, argv[0]);
  bool retval;
  if(!s /*|| !r*/)
    return JS_EXCEPTION;

  retval = s->inside(r);

  return JS_NewBool(ctx, retval);
}

enum { POINT_METHOD_MAG = 0, POINT_METHOD_NORM, POINT_METHOD_ABS, POINT_METHOD_ANGLE, POINT_METHOD_ROTATE, POINT_METHOD_MOD, POINT_METHOD_CLONE };

static JSValue
js_point_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSPointData<double>* s;
  JSValue ret = JS_UNDEFINED;

  if(!(s = js_point_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case POINT_METHOD_MAG: {
      ret = JS_NewFloat64(ctx, sqrt(s->x * s->x + s->y * s->y));
      break;
    }

    case POINT_METHOD_NORM: {
      double magnitude = sqrt(s->x * s->x + s->y * s->y);
      JSPointData<double> normalized(s->x / magnitude, s->y / magnitude);

      ret = js_point_clone(ctx, JS_GetPrototype(ctx, this_val), normalized);
      break;
    }

    case POINT_METHOD_ABS: {
      ret = js_point_new(ctx, JS_GetPrototype(ctx, this_val), fabs(s->x), fabs(s->y));
      break;
    }

    case POINT_METHOD_ANGLE: {
      JSPointData<double> point(*s), *arg = nullptr;
      BOOL deg = FALSE;
      int i = 0;

      if(i < argc && (arg = js_point_data(argv[i])))
        i++;

      while(i < argc) {
        if(JS_IsBool(argv[i]))
          deg = JS_ToBool(ctx, argv[i]);

        i++;
      }

      if(arg)
        point = sub(*arg, point);

      double phi = std::atan2(point.y, point.x);

      if(deg)
        phi = phi * 180 / M_PI;

      ret = JS_NewFloat64(ctx, phi);
      break;
    }

    case POINT_METHOD_ROTATE: {
      JSPointData<double> point;
      double theta = 0;

      JS_ToFloat64(ctx, &theta, argv[0]);

      point.x = s->x * cos(theta) - s->y * sin(theta);
      point.y = s->x * sin(theta) + s->y * cos(theta);
      ret = js_point_clone(ctx, JS_GetPrototype(ctx, this_val), point);
      break;
    }

    case POINT_METHOD_MOD: {
      JSPointData<double> other;

      if(js_point_read(ctx, argv[0], &other)) {
        double x = fmod(s->x, other.x);
        double y = fmod(s->y, other.y);

        if(x < 0)
          x += other.x;
        if(y < 0)
          y += other.y;

        ret = js_point_new(ctx, JS_GetPrototype(ctx, this_val), x, y);
      }
      break;
    }

    case POINT_METHOD_CLONE: {
      ret = js_point_new(ctx, JS_GetPrototype(ctx, this_val), s->x, s->y);
      break;
    }
  }

  return ret;
}

enum {
  POINT_ARITH_ADD = 0,
  POINT_ARITH_SUB,
  POINT_ARITH_MUL,
  POINT_ARITH_DIV,
  POINT_ARITH_MOD,
  POINT_ARITH_SUM,
  POINT_ARITH_DIFF,
  POINT_ARITH_PROD,
  POINT_ARITH_QUOT
};

static JSValue
js_point_arith(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSPointData<double> other, point, *s;
  int argind = 0;
  BOOL method = TRUE;

  if(!(s = js_point_data(this_val))) {
    if(!js_point_read(ctx, argv[0], &point)) {
      return JS_ThrowTypeError(ctx, "argument must be a Point somehow");
    }
    argind++;
    method = FALSE;
  } else {
    point = *s;
  } /*else {

   if(magic < POINT_ARITH_SUM) {
     if(!(s = js_point_data2(ctx, magic >= POINT_ARITH_SUM ? argv[0] : this_val)))
       return JS_EXCEPTION;

     point = *s;
   } else {
     if(!js_point_read(ctx, argv[0], &point)) {
       return JS_ThrowTypeError(ctx, "argument must be a Point somehow");
     }
   }*/

  while(argind < argc) {
    if(!js_point_argument(ctx, argc, argv, argind, &other)) {
      if(js_number_read(ctx, argv[argind], &other.x)) {
        other.y = other.x;
        argind++;
      } else {
        break;
      }
    }
    switch(magic % POINT_ARITH_SUM) {
      case POINT_ARITH_ADD: point = add(point, other); break;
      case POINT_ARITH_SUB: point = sub(point, other); break;
      case POINT_ARITH_MUL: point = mul(point, *reinterpret_cast<JSSizeData<double>*>(&other)); break;
      case POINT_ARITH_DIV: point = div(point, *reinterpret_cast<JSSizeData<double>*>(&other)); break;
    }
  }

  if(!method || magic >= POINT_ARITH_SUM)
    return js_point_new(ctx, point_proto, point.x, point.y);

  *s = point;

  return JS_DupValue(ctx, this_val);
}

static JSValue
js_point_to_string(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSPointData<double>* s = js_point_data2(ctx, this_val);
  std::ostringstream os;
  JSValue xv, yv;
  const char* delim = ",";
  double x = -1, y = -1;
  /* if(!s)
     return JS_EXCEPTION;*/
  if(argc > 0)
    delim = JS_ToCString(ctx, argv[0]);

  xv = JS_GetPropertyStr(ctx, this_val, "x");
  yv = JS_GetPropertyStr(ctx, this_val, "y");

  if(JS_IsNumber(xv) && JS_IsNumber(yv)) {
    JS_ToFloat64(ctx, &x, xv);
    JS_ToFloat64(ctx, &y, yv);
  } else if(s) {
    x = s->x;
    y = s->y;
  }

  switch(magic) {
    case 0: {
      os << x << "," << y;
      break;
    }

    case 1: {
      os << "[" << x << "," << y << "]";
      break;
    }

    case 2: {

      os << "{x:" << x << ",y:" << y << "}";
      break;
    }
  }

  return JS_NewString(ctx, os.str().c_str());
}

static JSValue
js_point_to_array(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSPointData<double>* s = js_point_data2(ctx, this_val);
  std::array<double, 2> arr;

  arr[0] = s->x;
  arr[1] = s->y;

  return js_array_from(ctx, arr.cbegin(), arr.cend());
}

static JSValue
js_point_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSPointData<double>* s = js_point_data2(ctx, this_val);
  JSValue obj = JS_NewObjectClass(ctx, js_point_class_id);

  JS_DefinePropertyValueStr(ctx, obj, "x", JS_NewFloat64(ctx, s->x), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "y", JS_NewFloat64(ctx, s->y), JS_PROP_ENUMERABLE);

  return obj;
}

static JSAtom iterator_symbol;

static JSValue
js_point_symbol_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue arr, iter;
  arr = js_point_to_array(ctx, this_val, argc, argv);

  if(iterator_symbol == 0)
    iterator_symbol = js_symbol_atom(ctx, "iterator");

  if(!js_is_function(ctx, (iter = JS_GetProperty(ctx, arr, iterator_symbol))))
    return JS_EXCEPTION;
  return JS_Call(ctx, iter, arr, 0, argv);
}

static JSValue
js_point_round(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSPointData<double> point, *s = js_point_data2(ctx, this_val);
  double x, y;
  double prec = 1;
  point = *s;
  JSValue ret = JS_UNDEFINED;

  switch(magic) {
    case 0:
      if(argc > 0)
        JS_ToFloat64(ctx, &prec, argv[0]);

      x = round(s->x / prec);
      y = round(s->y / prec);
      ret = js_point_new(ctx, x * prec, y * prec);
      break;

    case 1:
      if(argc > 0)
        JS_ToFloat64(ctx, &prec, argv[0]);

      x = floor(s->x / prec);
      y = floor(s->y / prec);
      ret = js_point_new(ctx, x * prec, y * prec);
      break;

    case 2:
      if(argc > 0)
        JS_ToFloat64(ctx, &prec, argv[0]);

      x = ceil(s->x / prec);
      y = ceil(s->y / prec);
      ret = js_point_new(ctx, x * prec, y * prec);
      break;
  }

  return ret;
}

static JSValue
js_point_from(JSContext* ctx, JSValueConst point, int argc, JSValueConst argv[]) {
  std::array<double, 2> array;
  JSValue ret = JS_EXCEPTION;

  if(JS_IsString(argv[0])) {
    const char* str = JS_ToCString(ctx, argv[0]);
    char* endptr = nullptr;
    for(size_t i = 0; i < 2; i++) {
      while(!isdigit(*str) && *str != '-' && *str != '+' && !(*str == '.' && isdigit(str[1])))
        str++;
      if(*str == '\0')
        break;
      array[i] = strtod(str, &endptr);
      str = endptr;
    }
  } else if(js_is_array(ctx, argv[0])) {
    js_array_to(ctx, argv[0], array);
  }
  if(array[0] > 0 && array[1] > 0)
    ret = js_point_new(ctx, array[0], array[1]);

  return ret;
}

static JSValue
js_point_fromangle(JSContext* ctx, JSValueConst point, int argc, JSValueConst argv[]) {
  JSValue ret = JS_EXCEPTION;
  double phi = 0;
  JSSizeData<double> f = {1.0, 1.0};
  BOOL deg = FALSE;

  JS_ToFloat64(ctx, &phi, argv[0]);

  if(argc >= 2) {
    if(!js_size_read(ctx, argv[1], &f)) {
      JS_ToFloat64(ctx, &f.width, argv[1]);
      f.height = f.width;
    }
  }

  if(argc >= 3)
    deg = JS_ToBool(ctx, argv[2]);

  if(deg)
    phi = phi * M_PI / 180.0;

  return js_point_new(ctx, std::cos(phi) * f.width, std::sin(phi) * f.height);
}

static JSValue
js_point_diff(JSContext* ctx, JSValueConst point, int argc, JSValueConst argv[]) {
  JSValue ret = JS_NewArray(ctx);
  int argind = 0, i = 0, outind = 0;
  JSPointData<double> pt, prev;

  while(argind < argc) {
    js_point_arg(ctx, argc, argv, argind, pt);
    if(i > 0)
      JS_SetPropertyUint32(ctx, ret, outind++, js_point_new(ctx, pt.x - prev.x, pt.y - prev.y));
    prev = pt;
    ++i;
  }

  return ret;
}

static JSValue
js_point_distance(JSContext* ctx, JSValueConst point, int argc, JSValueConst argv[]) {
  int argind = 0, i = 0, outind = 0;
  JSPointData<double> pt, prev;
  double d = 0;

  while(argind < argc) {
    js_point_arg(ctx, argc, argv, argind, pt);

    if(i > 0)
      d += distance(prev, pt);

    prev = pt;
    ++i;
  }

  return JS_NewFloat64(ctx, d);
}

static void
js_point_finalizer(JSRuntime* rt, JSValue val) {
  JSPointData<double>* s;

  if((s = static_cast<JSPointData<double>*>(JS_GetOpaque(val, js_point_class_id)))) {
    js_deallocate(rt, s);
  }
}

extern "C" {

JSClassDef js_point_class = {
    .class_name = "Point",
    .finalizer = js_point_finalizer,
};

const JSCFunctionListEntry js_point_proto_funcs[] = {
    JS_CGETSET_ENUMERABLE_DEF("x", js_point_get_xy, js_point_set_xy, POINT_PROP_X),
    JS_CGETSET_ENUMERABLE_DEF("y", js_point_get_xy, js_point_set_xy, POINT_PROP_Y),
    JS_CFUNC_DEF("cross", 1, js_point_cross),
    JS_CFUNC_DEF("dot", 1, js_point_ddot),
    JS_CFUNC_DEF("inside", 1, js_point_inside),
    JS_CFUNC_DEF("difference", 1, js_point_difference),
    JS_CFUNC_DEF("adjacent", 1, js_point_adjacent),
    JS_CFUNC_MAGIC_DEF("add", 1, js_point_arith, POINT_ARITH_ADD),
    JS_CFUNC_MAGIC_DEF("sub", 1, js_point_arith, POINT_ARITH_SUB),
    JS_CFUNC_MAGIC_DEF("mul", 1, js_point_arith, POINT_ARITH_MUL),
    JS_CFUNC_MAGIC_DEF("div", 1, js_point_arith, POINT_ARITH_DIV),
    JS_CFUNC_MAGIC_DEF("sum", 1, js_point_arith, POINT_ARITH_SUM),
    JS_CFUNC_MAGIC_DEF("diff", 1, js_point_arith, POINT_ARITH_DIFF),
    JS_CFUNC_MAGIC_DEF("prod", 1, js_point_arith, POINT_ARITH_PROD),
    JS_CFUNC_MAGIC_DEF("quot", 1, js_point_arith, POINT_ARITH_QUOT),
    JS_CFUNC_MAGIC_DEF("mag", 0, js_point_method, POINT_METHOD_MAG),
    JS_CFUNC_MAGIC_DEF("norm", 0, js_point_method, POINT_METHOD_NORM),
    JS_CFUNC_MAGIC_DEF("abs", 0, js_point_method, POINT_METHOD_ABS),
    JS_CFUNC_MAGIC_DEF("angle", 0, js_point_method, POINT_METHOD_ANGLE),
    JS_CFUNC_MAGIC_DEF("round", 0, js_point_round, 0),
    JS_CFUNC_MAGIC_DEF("floor", 0, js_point_round, 1),
    JS_CFUNC_MAGIC_DEF("ceil", 0, js_point_round, 2),
    JS_CFUNC_MAGIC_DEF("toString", 0, js_point_to_string, 0),
    JS_CFUNC_MAGIC_DEF("rotate", 1, js_point_method, POINT_METHOD_ROTATE),
    JS_CFUNC_MAGIC_DEF("mod", 1, js_point_method, POINT_METHOD_MOD),
    JS_CFUNC_MAGIC_DEF("clone", 0, js_point_method, POINT_METHOD_CLONE),
    JS_CFUNC_DEF("toArray", 0, js_point_to_array),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, js_point_symbol_iterator),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Point", JS_PROP_CONFIGURABLE),

    // JS_CFUNC_MAGIC_DEF("[Symbol.toStringTag]", 0, js_point_to_string, 1),
};
const JSCFunctionListEntry js_point_static_funcs[] = {
    JS_CFUNC_DEF("from", 1, js_point_from),
    JS_CFUNC_DEF("fromAngle", 1, js_point_fromangle),
    // JS_CFUNC_DEF("diff", 2, js_point_diff),
    JS_CFUNC_DEF("distance", 2, js_point_distance),
    JS_CFUNC_MAGIC_DEF("sum", 1, js_point_arith, POINT_ARITH_SUM),
    JS_CFUNC_MAGIC_DEF("diff", 1, js_point_arith, POINT_ARITH_DIFF),
    JS_CFUNC_MAGIC_DEF("prod", 1, js_point_arith, POINT_ARITH_PROD),
    JS_CFUNC_MAGIC_DEF("quot", 1, js_point_arith, POINT_ARITH_QUOT),
};

int
js_point_init(JSContext* ctx, JSModuleDef* m) {

  if(js_point_class_id == 0) {
    /* create the Point class */
    JS_NewClassID(&js_point_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_point_class_id, &js_point_class);

    point_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, point_proto, js_point_proto_funcs, countof(js_point_proto_funcs));
    JS_SetClassProto(ctx, js_point_class_id, point_proto);

    point_class = JS_NewCFunction2(ctx, js_point_constructor, "Point", 0, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetPropertyFunctionList(ctx, point_class, js_point_static_funcs, countof(js_point_static_funcs));

    JS_SetConstructor(ctx, point_class, point_proto);

    // js_object_inspect(ctx, point_proto, js_point_inspect);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Point", point_class);
  /* else
     JS_SetPropertyStr(ctx, *static_cast<JSValue*>(m), name, point_class);*/
  return 0;
}

extern "C" void
js_point_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Point");
}

#if defined(JS_POINT_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_point
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_point_init)))
    return NULL;

  js_point_export(ctx, m);
  return m;
}
}
