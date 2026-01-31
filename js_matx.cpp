#include "js_matx.hpp"
#include "js_alloc.hpp"
#include "include/js_array.hpp"
#include "js_point.hpp"
#include "js_size.hpp"
#include "js_typed_array.hpp"
#include "js_contour.hpp"
#include "jsbindings.hpp"
#include <quickjs.h>
#include "include/util.hpp"
#include <float.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>
#include <opencv2/core/types.hpp>
#include <ostream>
#include <string>
#include <vector> 


static JSValue
js_matx_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  double x = 0, y = 0, w = 0, h = 0;
  JSMatxData<double> matx = {0, 0, 0, 0};
  int optind = 0;
  JSValue proto;

  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    return JS_EXCEPTION;

  

  return js_matx_new(ctx, proto, x, y, w, h);
}

JSMatxData<double>*
js_matx_data(JSValueConst val) {
  return static_cast<JSMatxData<double>*>(JS_GetOpaque(val, js_matx_class_id));
}

JSMatxData<double>*
js_matx_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSMatxData<double>*>(JS_GetOpaque2(ctx, val, js_matx_class_id));
}

enum {
  PROP_X = 0,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_POS,
  PROP_SIZE,
  PROP_TOPLEFT,
  PROP_BOTTOM_RIGHT,
  PROP_EMPTY,
  PROP_AREA
};

static JSValue
js_matx_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSValue ret = JS_UNDEFINED;
  JSMatxData<double>* s;

  if((s = static_cast<JSMatxData<double>*>(JS_GetOpaque /*2*/ (/*ctx, */ this_val, js_matx_class_id))) == nullptr)
    return JS_UNDEFINED;

  switch(magic) {
    case PROP_X:
    case PROP_X1: {
      ret = JS_NewFloat64(ctx, s->x);
      break;
    }

    case PROP_Y:
    case PROP_Y1: {
      ret = JS_NewFloat64(ctx, s->y);
      break;
    }

    case PROP_WIDTH: {
      ret = JS_NewFloat64(ctx, s->width);
      break;
    }

    case PROP_HEIGHT: {
      ret = JS_NewFloat64(ctx, s->height);
      break;
    }

    case PROP_X2: {
      ret = JS_NewFloat64(ctx, s->x + s->width);
      break;
    }

    case PROP_Y2: {
      ret = JS_NewFloat64(ctx, s->y + s->height);
      break;
    }

    case PROP_POS: {
      ret = js_point_new(ctx, s->x, s->y);
      break;
    }

    case PROP_SIZE: {
      ret = js_size_new(ctx, s->width, s->height);
      break;
    }

    case PROP_TOPLEFT: {
      ret = js_point_new(ctx, s->x, s->y);
      break;
    }

    case PROP_BOTTOM_RIGHT: {
      ret = js_point_new(ctx, s->x + s->width, s->y + s->height);
      break;
    }

    case PROP_EMPTY: {
      ret = JS_NewBool(ctx, s->empty());
      break;
    }

    case PROP_AREA: {
      ret = JS_NewFloat64(ctx, s->area());
      break;
    }
  }

  return ret;
}

static JSValue
js_matx_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSMatxData<double>* s;
  double v;
  if((s = static_cast<JSMatxData<double>*>(JS_GetOpaque2(ctx, this_val, js_matx_class_id))) == nullptr)
    return JS_EXCEPTION;

  if(JS_ToFloat64(ctx, &v, val))
    return JS_EXCEPTION;

  switch(magic) {
    case PROP_X: {
      s->x = v;
      break;
    }

    case PROP_Y: {
      s->y = v;
      break;
    }

    case PROP_WIDTH: {
      s->width = v;
      break;
    }

    case PROP_HEIGHT: {
      s->height = v;
      break;
    }

    case PROP_X1: {
      double x2 = s->x + s->width;

      s->x = v;
      s->width = x2 - v;
      break;
    }

    case PROP_Y1: {
      double y2 = s->y + s->height;

      s->y = v;
      s->height = y2 - v;
      break;
    }

    case PROP_X2: {
      s->width = v - s->x;
      break;
    }

    case PROP_Y2: {
      s->height = v - s->y;
      break;
    }

    case PROP_POS: {
      JSPointData<double> point;
      js_point_read(ctx, val, &point);
      s->x = point.x;
      s->y = point.y;
      break;
    }

    case PROP_SIZE: {
      JSSizeData<double> size;
      js_size_read(ctx, val, &size);
      s->width = size.width;
      s->height = size.height;
      break;
    }
  }

  return JS_UNDEFINED;
}

static JSValue
js_matx_to_string(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSMatxData<double> matx, *s;
  std::ostringstream os;
  std::array<const char*, 3> delims = {
      ",",
      "∣" /*｜⧸⦁⎮∥∣⸾⼁❘❙⟊⍿⎸⏐｜│￨︲︱❘|｜*/,
      "×" /*"𝅃🅧𝚡🅧🅇𝘹𝚡𝘹𝐱ꭗ𝐗𝑿𝅃𝅃xˣₓ⒳Ⓧⓧ✕✘✗⨉⨯⨂✖⨻⦁⋅⊗⊠∗×⨯×"*/};

  for(size_t i = 0; i < argc; i++) {
    delims[i] = JS_ToCString(ctx, argv[i]);
  }

  if((s = static_cast<JSMatxData<double>*>(JS_GetOpaque2(ctx, this_val, js_matx_class_id))) != nullptr) {
    matx = *s;
  } else {
    js_matx_read(ctx, this_val, &matx);
  }

  os << matx.x << delims[0] << matx.y << delims[1] << matx.width << delims[2] << matx.height;

  return JS_NewString(ctx, os.str().c_str());
}

static JSValue
js_matx_to_source(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSMatxData<double> matx, *s;
  std::ostringstream os;

  if((s = static_cast<JSMatxData<double>*>(JS_GetOpaque2(ctx, this_val, js_matx_class_id))) != nullptr) {
    matx = *s;
  } else {
    js_matx_read(ctx, this_val, &matx);
  }

  os << "{x:" << matx.x << ",y:" << matx.y << ",width:" << matx.width << ",height:" << matx.height << "}";

  return JS_NewString(ctx, os.str().c_str());
}

enum { FUNC_EQUALS = 0, FUNC_ROUND, FUNC_TOOBJECT, FUNC_TOARRAY, FUNC_CONTOUR };

static JSValue
js_matx_funcs(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSMatxData<double> matx, other, *s, *a;
  JSValue ret = JS_EXCEPTION;

  if((s = js_matx_data2(ctx, this_val)) == nullptr)
    return ret;

  matx = *s;

  switch(magic) {
    case FUNC_EQUALS: {
      js_matx_read(ctx, argv[0], &other);
      bool equals = matx.x == other.x && matx.y == other.y && matx.width == other.width && matx.height == other.height;
      ret = JS_NewBool(ctx, equals);
      break;
    }

    case FUNC_ROUND: {
      double x, y, width, height, f;
      int32_t precision = 0;
      if(argc > 0)
        JS_ToInt32(ctx, &precision, argv[0]);
      f = std::pow(10, precision);
      x = std::round(matx.x * f) / f;
      y = std::round(matx.y * f) / f;
      width = std::round(matx.width * f) / f;
      height = std::round(matx.height * f) / f;
      ret = js_matx_new(ctx, x, y, width, height);
      break;
    }

    case FUNC_TOOBJECT: {
      ret = JS_NewObject(ctx);

      JS_SetPropertyStr(ctx, ret, "x", JS_NewFloat64(ctx, matx.x));
      JS_SetPropertyStr(ctx, ret, "y", JS_NewFloat64(ctx, matx.y));
      JS_SetPropertyStr(ctx, ret, "width", JS_NewFloat64(ctx, matx.width));
      JS_SetPropertyStr(ctx, ret, "height", JS_NewFloat64(ctx, matx.height));
      break;
    }

    case FUNC_TOARRAY: {
      std::array<double, 4> array{matx.x, matx.y, matx.width, matx.height};

      ret = js_array_from(ctx, array.cbegin(), array.cend());
      break;
    }

    case FUNC_CONTOUR: {
      JSContourData<double> c = {
          JSPointData<double>(matx.x, matx.y),
          JSPointData<double>(matx.x + matx.width, matx.y),
          JSPointData<double>(matx.x + matx.width, matx.y + matx.height),
          JSPointData<double>(matx.x, matx.y + matx.height),
      };

      ret = js_contour_new(ctx, c);
      break;
    }
  }

  return ret;
}

static JSValue
js_matx_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSMatxData<double>* r = js_matx_data2(ctx, this_val);
  JSValue obj = JS_NewObjectClass(ctx, js_matx_class_id);

  JS_DefinePropertyValueStr(ctx, obj, "x", JS_NewFloat64(ctx, r->x), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "y", JS_NewFloat64(ctx, r->y), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "width", JS_NewFloat64(ctx, r->width), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "height", JS_NewFloat64(ctx, r->height), JS_PROP_ENUMERABLE);
  return obj;
}

enum { METHOD_CONTAINS = 0, METHOD_BR, METHOD_TL, METHOD_SIZE, METHOD_INSET, METHOD_OUTSET, METHOD_HSPLIT, METHOD_VSPLIT, METHOD_MERGE, METHOD_CLONE };

static JSValue
js_matx_method(JSContext* ctx, JSValueConst matx, int argc, JSValueConst argv[], int magic) {
  JSMatxData<double>* s = static_cast<JSMatxData<double>*>(JS_GetOpaque2(ctx, matx, js_matx_class_id));
  JSValue ret = JS_UNDEFINED;
  JSPointData<double> point = js_point_get(ctx, argv[0]);

  switch(magic) {
    case METHOD_CONTAINS: {
      ret = JS_NewBool(ctx, s->contains(point));
      break;
    }

    case METHOD_BR:
    case METHOD_TL: {
      JSPointData<double> pt = magic == 3 ? s->br() : s->tl();

      ret = js_point_new(ctx, pt.x, pt.y);
      break;
    }

    case METHOD_SIZE: {
      cv::Size2d sz = s->size();
      ret = js_size_new(ctx, sz.width, sz.height);
      break;
    }

    case METHOD_INSET: {
      JSMatxData<double> matx = *s;
      if(argc >= 1) {
        double n;
        JS_ToFloat64(ctx, &n, argv[0]);
        matx.x += n;
        matx.width -= n * 2;
        matx.y += n;
        matx.height -= n * 2;

      } else if(argc >= 2) {
        double h, v;
        JS_ToFloat64(ctx, &h, argv[0]);
        JS_ToFloat64(ctx, &v, argv[1]);
        matx.x += h;
        matx.width -= h * 2;
        matx.y += v;
        matx.height -= v * 2;

      } else if(argc >= 4) {
        double t, r, b, l;
        JS_ToFloat64(ctx, &t, argv[0]);
        JS_ToFloat64(ctx, &r, argv[1]);
        JS_ToFloat64(ctx, &b, argv[2]);
        JS_ToFloat64(ctx, &l, argv[3]);
        matx.x += l;
        matx.width -= l + r;
        matx.y += t;
        matx.height -= t + b;
      }
      ret = js_matx_wrap(ctx, matx);
      break;
    }

    case METHOD_OUTSET: {
      JSMatxData<double> matx = *s;
      if(argc >= 1) {
        double n;
        JS_ToFloat64(ctx, &n, argv[0]);
        matx.x -= n;
        matx.width += n * 2;
        matx.y -= n;
        matx.height += n * 2;

      } else if(argc >= 2) {
        double h, v;
        JS_ToFloat64(ctx, &h, argv[0]);
        JS_ToFloat64(ctx, &v, argv[1]);
        matx.x -= h;
        matx.width += h * 2;
        matx.y -= v;
        matx.height += v * 2;

      } else if(argc >= 4) {
        double t, r, b, l;
        JS_ToFloat64(ctx, &t, argv[0]);
        JS_ToFloat64(ctx, &r, argv[1]);
        JS_ToFloat64(ctx, &b, argv[2]);
        JS_ToFloat64(ctx, &l, argv[3]);
        matx.x -= l;
        matx.width += l + r;
        matx.y -= t;
        matx.height += t + b;
      }
      ret = js_matx_wrap(ctx, matx);
      break;
    }

    case METHOD_HSPLIT: {
      std::vector<JSMatxData<double>> matxs;
      double x1 = s->x, x2 = s->x + s->width;
      auto args = argument_range(argc, argv);
      JSMatxData<double>* prev;
      std::vector<double> breakpoints;
      for(JSValueConst const& arg : args) {
        double d;
        js_value_to(ctx, arg, d);
        if(d < 0)
          d = s->width + d;
        if(d > 0 && d < s->width)
          breakpoints.push_back(d);
      }
      std::sort(breakpoints.begin(), breakpoints.end());
      matxs.push_back(*s);
      prev = &matxs.back();
      for(double h : breakpoints) {
        // printf("hsplit h=%lf\n", h);
        prev->width = h - (prev->x - x1);
        matxs.push_back(*s);
        matxs.back().x = x1 + h;
        prev = &matxs.back();
        prev->width = s->width - h;
      }
      ret = js_array_from(ctx, matxs);
      break;
    }

    case METHOD_VSPLIT: {
      std::vector<JSMatxData<double>> matxs;
      double y1 = s->y, y2 = s->y + s->height;
      auto args = argument_range(argc, argv);
      JSMatxData<double>* prev;
      std::vector<double> breakpoints;

      for(JSValueConst const& arg : args) {
        double d;

        js_value_to(ctx, arg, d);

        if(d < 0)
          d = s->height + d;

        if(d > 0 && d < s->height)
          breakpoints.push_back(d);
      }

      std::sort(breakpoints.begin(), breakpoints.end());
      matxs.push_back(*s);
      prev = &matxs.back();

      for(double v : breakpoints) {
        // printf("hsplit v=%lf\n", v);
        prev->height = v - (prev->y - y1);
        matxs.push_back(*s);
        matxs.back().y = y1 + v;
        prev = &matxs.back();
        prev->height = s->height - v;
      }

      ret = js_array_from(ctx, matxs);
      break;
    }

    case METHOD_MERGE: {

      double x1 = s->x, x2 = s->x + s->width, y1 = s->y, y2 = s->y + s->height;
      size_t i;
      for(i = 0; i < argc; i++) {
        JSMatxData<double> matx;
        js_matx_read(ctx, argv[i], &matx);
        if(x1 > matx.x)
          x1 = matx.x;
        if(x2 < matx.x + matx.width)
          x2 = matx.x + matx.width;
        if(y1 > matx.y)
          y1 = matx.y;
        if(y2 < matx.y + matx.height)
          y2 = matx.y + matx.height;
      }

      ret = js_matx_new(ctx, x1, y1, x2 - x1, y2 - y1);
      break;
    }

    case METHOD_CLONE: {
      ret = js_matx_new(ctx, s->x, s->y, s->width, s->height);
      break;
    }
  }

  return ret;
}

static JSAtom iterator_symbol;

static JSValue
js_matx_symbol_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue arr, iter;
  arr = js_matx_funcs(ctx, this_val, argc, argv, 3);

  if(iterator_symbol == 0)
    iterator_symbol = js_symbol_atom(ctx, "iterator");

  if(!js_is_function(ctx, (iter = JS_GetProperty(ctx, arr, iterator_symbol))))
    return JS_EXCEPTION;

  return JS_Call(ctx, iter, arr, 0, argv);
}

static JSValue
js_matx_from(JSContext* ctx, JSValueConst matx, int argc, JSValueConst argv[]) {
  std::array<double, 4> array;
  JSValue ret = JS_EXCEPTION;

  if(JS_IsString(argv[0])) {
    const char* str = JS_ToCString(ctx, argv[0]);
    char* endptr = nullptr;
    for(size_t i = 0; i < 4; i++) {
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

  if(array[2] > 0 && array[3] > 0)
    ret = js_matx_new(ctx, array[0], array[1], array[2], array[3]);

  return ret;
}

void
js_matx_finalizer(JSRuntime* rt, JSValue val) {
  JSMatxData<double>* s = static_cast<JSMatxData<double>*>(JS_GetOpaque(val, js_matx_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  // s->~JSMatxData<double>();
  if(s)
    js_deallocate(rt, s);
}

JSClassDef js_matx_class = {
    .class_name = "Matx",
    .finalizer = js_matx_finalizer,
};

const JSCFunctionListEntry js_matx_proto_funcs[] = { 
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Matx", JS_PROP_CONFIGURABLE),
};
const JSCFunctionListEntry js_matx_static_funcs[] = {
    JS_CFUNC_DEF("from", 1, js_matx_from),
};

extern "C" int
js_matx_init(JSContext* ctx, JSModuleDef* m) {

  if(js_matx_class_id == 0) {
    /* create the Matx class */
    JS_NewClassID(&js_matx_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_matx_class_id, &js_matx_class);

    matx_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, matx_proto, js_matx_proto_funcs, countof(js_matx_proto_funcs));
    JS_SetClassProto(ctx, js_matx_class_id, matx_proto);

    matx_class = JS_NewCFunction2(ctx, js_matx_constructor, "Matx", 0, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, matx_class, matx_proto);
    JS_SetPropertyFunctionList(ctx, matx_class, js_matx_static_funcs, countof(js_matx_static_funcs));

    // js_object_inspect(ctx, matx_proto, js_matx_inspect);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Matx", matx_class);

  return 0;
}

extern "C" void
js_matx_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Matx");
}

#ifdef JS_MATX_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_matx
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_matx_init)))
    return NULL;

  js_matx_export(ctx, m);
  return m;
}
