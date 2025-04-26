#include "js_rect.hpp"
#include "js_alloc.hpp"
#include "js_array.hpp"
#include "js_point.hpp"
#include "js_size.hpp"
#include "js_typed_array.hpp"
#include "js_contour.hpp"
#include "jsbindings.hpp"
#include <quickjs.h>
#include "util.hpp"
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

extern "C" {
thread_local JSValue rect_proto = JS_UNDEFINED, rect_class = JS_UNDEFINED;
thread_local JSClassID js_rect_class_id = 0;
}

extern "C" JSValue
js_rect_new(JSContext* ctx, JSValueConst proto, double x, double y, double w, double h) {
  JSValue ret;
  JSRectData<double>* s;

  if(JS_IsUndefined(rect_proto))
    js_rect_init(ctx, NULL);

  ret = JS_NewObjectProtoClass(ctx, proto, js_rect_class_id);

  s = js_allocate<JSRectData<double>>(ctx);

  new(s) JSRectData<double>();
  s->x = x <= DBL_EPSILON ? 0 : x;
  s->y = y <= DBL_EPSILON ? 0 : y;
  s->width = w <= DBL_EPSILON ? 0 : w;
  s->height = h <= DBL_EPSILON ? 0 : h;

  JS_SetOpaque(ret, s);
  return ret;
}

JSValue
js_rect_new(JSContext* ctx, double x, double y, double w, double h) {
  return js_rect_new(ctx, rect_proto, x, y, w, h);
}

JSValue
js_rect_wrap(JSContext* ctx, const JSRectData<double>& rect) {
  return js_rect_new(ctx, rect.x, rect.y, rect.width, rect.height);
}

static JSValue
js_rect_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  double x = 0, y = 0, w = 0, h = 0;
  JSRectData<double> rect = {0, 0, 0, 0};
  int optind = 0;
  JSValue proto;

  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    return JS_EXCEPTION;

  if(argc > 0) {
    if(!js_rect_read(ctx, argv[0], &rect)) {
      JSPointData<double> point, point2;
      JSSizeData<double> size;
      while(optind < argc) {
        if(JS_IsNumber(argv[optind]) && JS_IsNumber(argv[optind + 1])) {
          if(optind == 0) {
            JS_ToFloat64(ctx, &x, argv[optind]);
            JS_ToFloat64(ctx, &y, argv[optind + 1]);
          } else {
            JS_ToFloat64(ctx, &w, argv[optind]);
            JS_ToFloat64(ctx, &h, argv[optind + 1]);
          }
          optind += 2;
        } else if(js_point_read(ctx, argv[optind], optind == 0 ? &point : &point2)) {
          if(optind == 0) {
            x = point.x;
            y = point.y;
          } else {
            w = fabs(point2.x - x);
            h = fabs(point2.y - y);
            x = fmin(point2.x, x);
            y = fmin(point2.y, y);
          }
          optind++;
        }

        else if(js_size_read(ctx, argv[optind], &size)) {
          w = size.width;
          h = size.height;
          optind++;
        } else {
          optind++;
        }
      }
    } else {
      x = rect.x;
      y = rect.y;
      w = rect.width;
      h = rect.height;
    }
  }

  return js_rect_new(ctx, proto, x, y, w, h);
}

JSRectData<double>*
js_rect_data(JSValueConst val) {
  return static_cast<JSRectData<double>*>(JS_GetOpaque(val, js_rect_class_id));
}

JSRectData<double>*
js_rect_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSRectData<double>*>(JS_GetOpaque2(ctx, val, js_rect_class_id));
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
js_rect_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSValue ret = JS_UNDEFINED;
  JSRectData<double>* s;

  if((s = static_cast<JSRectData<double>*>(JS_GetOpaque /*2*/ (/*ctx, */ this_val, js_rect_class_id))) == nullptr)
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
js_rect_set(JSContext* ctx, JSValueConst this_val, JSValueConst val, int magic) {
  JSRectData<double>* s;
  double v;
  if((s = static_cast<JSRectData<double>*>(JS_GetOpaque2(ctx, this_val, js_rect_class_id))) == nullptr)
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
js_rect_to_string(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSRectData<double> rect, *s;
  std::ostringstream os;
  std::array<const char*, 3> delims = {
      ",",
      "∣" /*｜⧸⦁⎮∥∣⸾⼁❘❙⟊⍿⎸⏐｜│￨︲︱❘|｜*/,
      "×" /*"𝅃🅧𝚡🅧🅇𝘹𝚡𝘹𝐱ꭗ𝐗𝑿𝅃𝅃xˣₓ⒳Ⓧⓧ✕✘✗⨉⨯⨂✖⨻⦁⋅⊗⊠∗×⨯×"*/};

  for(size_t i = 0; i < argc; i++) {
    delims[i] = JS_ToCString(ctx, argv[i]);
  }

  if((s = static_cast<JSRectData<double>*>(JS_GetOpaque2(ctx, this_val, js_rect_class_id))) != nullptr) {
    rect = *s;
  } else {
    js_rect_read(ctx, this_val, &rect);
  }

  os << rect.x << delims[0] << rect.y << delims[1] << rect.width << delims[2] << rect.height;

  return JS_NewString(ctx, os.str().c_str());
}

static JSValue
js_rect_to_source(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSRectData<double> rect, *s;
  std::ostringstream os;

  if((s = static_cast<JSRectData<double>*>(JS_GetOpaque2(ctx, this_val, js_rect_class_id))) != nullptr) {
    rect = *s;
  } else {
    js_rect_read(ctx, this_val, &rect);
  }

  os << "{x:" << rect.x << ",y:" << rect.y << ",width:" << rect.width << ",height:" << rect.height << "}";

  return JS_NewString(ctx, os.str().c_str());
}

enum { FUNC_EQUALS = 0, FUNC_ROUND, FUNC_TOOBJECT, FUNC_TOARRAY, FUNC_CONTOUR };

static JSValue
js_rect_funcs(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSRectData<double> rect, other, *s, *a;
  JSValue ret = JS_EXCEPTION;

  if((s = js_rect_data2(ctx, this_val)) == nullptr)
    return ret;

  rect = *s;

  switch(magic) {
    case FUNC_EQUALS: {
      js_rect_read(ctx, argv[0], &other);
      bool equals = rect.x == other.x && rect.y == other.y && rect.width == other.width && rect.height == other.height;
      ret = JS_NewBool(ctx, equals);
      break;
    }

    case FUNC_ROUND: {
      double x, y, width, height, f;
      int32_t precision = 0;
      if(argc > 0)
        JS_ToInt32(ctx, &precision, argv[0]);
      f = std::pow(10, precision);
      x = std::round(rect.x * f) / f;
      y = std::round(rect.y * f) / f;
      width = std::round(rect.width * f) / f;
      height = std::round(rect.height * f) / f;
      ret = js_rect_new(ctx, x, y, width, height);
      break;
    }

    case FUNC_TOOBJECT: {
      ret = JS_NewObject(ctx);

      JS_SetPropertyStr(ctx, ret, "x", JS_NewFloat64(ctx, rect.x));
      JS_SetPropertyStr(ctx, ret, "y", JS_NewFloat64(ctx, rect.y));
      JS_SetPropertyStr(ctx, ret, "width", JS_NewFloat64(ctx, rect.width));
      JS_SetPropertyStr(ctx, ret, "height", JS_NewFloat64(ctx, rect.height));
      break;
    }

    case FUNC_TOARRAY: {
      std::array<double, 4> array{rect.x, rect.y, rect.width, rect.height};

      ret = js_array_from(ctx, array.cbegin(), array.cend());
      break;
    }

    case FUNC_CONTOUR: {
      JSContourData<double> c = {
          JSPointData<double>(rect.x, rect.y),
          JSPointData<double>(rect.x + rect.width, rect.y),
          JSPointData<double>(rect.x + rect.width, rect.y + rect.height),
          JSPointData<double>(rect.x, rect.y + rect.height),
      };

      ret = js_contour_new(ctx, c);
      break;
    }
  }

  return ret;
}

static JSValue
js_rect_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSRectData<double>* r = js_rect_data2(ctx, this_val);
  JSValue obj = JS_NewObjectClass(ctx, js_rect_class_id);

  JS_DefinePropertyValueStr(ctx, obj, "x", JS_NewFloat64(ctx, r->x), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "y", JS_NewFloat64(ctx, r->y), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "width", JS_NewFloat64(ctx, r->width), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "height", JS_NewFloat64(ctx, r->height), JS_PROP_ENUMERABLE);
  return obj;
}

enum { METHOD_CONTAINS = 0, METHOD_BR, METHOD_TL, METHOD_SIZE, METHOD_INSET, METHOD_OUTSET, METHOD_HSPLIT, METHOD_VSPLIT, METHOD_MERGE, METHOD_CLONE };

static JSValue
js_rect_method(JSContext* ctx, JSValueConst rect, int argc, JSValueConst argv[], int magic) {
  JSRectData<double>* s = static_cast<JSRectData<double>*>(JS_GetOpaque2(ctx, rect, js_rect_class_id));
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
      JSRectData<double> rect = *s;
      if(argc >= 1) {
        double n;
        JS_ToFloat64(ctx, &n, argv[0]);
        rect.x += n;
        rect.width -= n * 2;
        rect.y += n;
        rect.height -= n * 2;

      } else if(argc >= 2) {
        double h, v;
        JS_ToFloat64(ctx, &h, argv[0]);
        JS_ToFloat64(ctx, &v, argv[1]);
        rect.x += h;
        rect.width -= h * 2;
        rect.y += v;
        rect.height -= v * 2;

      } else if(argc >= 4) {
        double t, r, b, l;
        JS_ToFloat64(ctx, &t, argv[0]);
        JS_ToFloat64(ctx, &r, argv[1]);
        JS_ToFloat64(ctx, &b, argv[2]);
        JS_ToFloat64(ctx, &l, argv[3]);
        rect.x += l;
        rect.width -= l + r;
        rect.y += t;
        rect.height -= t + b;
      }
      ret = js_rect_wrap(ctx, rect);
      break;
    }

    case METHOD_OUTSET: {
      JSRectData<double> rect = *s;
      if(argc >= 1) {
        double n;
        JS_ToFloat64(ctx, &n, argv[0]);
        rect.x -= n;
        rect.width += n * 2;
        rect.y -= n;
        rect.height += n * 2;

      } else if(argc >= 2) {
        double h, v;
        JS_ToFloat64(ctx, &h, argv[0]);
        JS_ToFloat64(ctx, &v, argv[1]);
        rect.x -= h;
        rect.width += h * 2;
        rect.y -= v;
        rect.height += v * 2;

      } else if(argc >= 4) {
        double t, r, b, l;
        JS_ToFloat64(ctx, &t, argv[0]);
        JS_ToFloat64(ctx, &r, argv[1]);
        JS_ToFloat64(ctx, &b, argv[2]);
        JS_ToFloat64(ctx, &l, argv[3]);
        rect.x -= l;
        rect.width += l + r;
        rect.y -= t;
        rect.height += t + b;
      }
      ret = js_rect_wrap(ctx, rect);
      break;
    }

    case METHOD_HSPLIT: {
      std::vector<JSRectData<double>> rects;
      double x1 = s->x, x2 = s->x + s->width;
      auto args = argument_range(argc, argv);
      JSRectData<double>* prev;
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
      rects.push_back(*s);
      prev = &rects.back();
      for(double h : breakpoints) {
        // printf("hsplit h=%lf\n", h);
        prev->width = h - (prev->x - x1);
        rects.push_back(*s);
        rects.back().x = x1 + h;
        prev = &rects.back();
        prev->width = s->width - h;
      }
      ret = js_array_from(ctx, rects);
      break;
    }

    case METHOD_VSPLIT: {
      std::vector<JSRectData<double>> rects;
      double y1 = s->y, y2 = s->y + s->height;
      auto args = argument_range(argc, argv);
      JSRectData<double>* prev;
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
      rects.push_back(*s);
      prev = &rects.back();

      for(double v : breakpoints) {
        // printf("hsplit v=%lf\n", v);
        prev->height = v - (prev->y - y1);
        rects.push_back(*s);
        rects.back().y = y1 + v;
        prev = &rects.back();
        prev->height = s->height - v;
      }

      ret = js_array_from(ctx, rects);
      break;
    }

    case METHOD_MERGE: {

      double x1 = s->x, x2 = s->x + s->width, y1 = s->y, y2 = s->y + s->height;
      size_t i;
      for(i = 0; i < argc; i++) {
        JSRectData<double> rect;
        js_rect_read(ctx, argv[i], &rect);
        if(x1 > rect.x)
          x1 = rect.x;
        if(x2 < rect.x + rect.width)
          x2 = rect.x + rect.width;
        if(y1 > rect.y)
          y1 = rect.y;
        if(y2 < rect.y + rect.height)
          y2 = rect.y + rect.height;
      }

      ret = js_rect_new(ctx, x1, y1, x2 - x1, y2 - y1);
      break;
    }

    case METHOD_CLONE: {
      ret = js_rect_new(ctx, s->x, s->y, s->width, s->height);
      break;
    }
  }

  return ret;
}

static JSAtom iterator_symbol;

static JSValue
js_rect_symbol_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue arr, iter;
  arr = js_rect_funcs(ctx, this_val, argc, argv, 3);

  if(iterator_symbol == 0)
    iterator_symbol = js_symbol_atom(ctx, "iterator");

  if(!JS_IsFunction(ctx, (iter = JS_GetProperty(ctx, arr, iterator_symbol))))
    return JS_EXCEPTION;

  return JS_Call(ctx, iter, arr, 0, argv);
}

static JSValue
js_rect_from(JSContext* ctx, JSValueConst rect, int argc, JSValueConst argv[]) {
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
    ret = js_rect_new(ctx, array[0], array[1], array[2], array[3]);

  return ret;
}

void
js_rect_finalizer(JSRuntime* rt, JSValue val) {
  JSRectData<double>* s = static_cast<JSRectData<double>*>(JS_GetOpaque(val, js_rect_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  // s->~JSRectData<double>();
  if(s)
    js_deallocate(rt, s);
}

JSClassDef js_rect_class = {
    .class_name = "Rect",
    .finalizer = js_rect_finalizer,
};

const JSCFunctionListEntry js_rect_proto_funcs[] = {
    JS_CGETSET_ENUMERABLE_DEF("x", js_rect_get, js_rect_set, PROP_X),
    JS_CGETSET_ENUMERABLE_DEF("y", js_rect_get, js_rect_set, PROP_Y),
    JS_CGETSET_ENUMERABLE_DEF("width", js_rect_get, js_rect_set, PROP_WIDTH),
    JS_CGETSET_ENUMERABLE_DEF("height", js_rect_get, js_rect_set, PROP_HEIGHT),
    JS_CGETSET_MAGIC_DEF("x1", js_rect_get, js_rect_set, PROP_X1),
    JS_CGETSET_MAGIC_DEF("y1", js_rect_get, js_rect_set, PROP_Y1),
    JS_CGETSET_MAGIC_DEF("x2", js_rect_get, js_rect_set, PROP_X2),
    JS_CGETSET_MAGIC_DEF("y2", js_rect_get, js_rect_set, PROP_Y2),
    JS_CGETSET_MAGIC_DEF("position", js_rect_get, js_rect_set, PROP_POS),
    JS_CGETSET_MAGIC_DEF("size", js_rect_get, js_rect_set, PROP_SIZE),
    JS_CGETSET_MAGIC_DEF("tl", js_rect_get, 0, PROP_TOPLEFT),
    JS_CGETSET_MAGIC_DEF("br", js_rect_get, 0, PROP_BOTTOM_RIGHT),
    JS_CFUNC_MAGIC_DEF("contains", 0, js_rect_method, METHOD_CONTAINS),
    JS_CGETSET_MAGIC_DEF("empty", js_rect_get, 0, PROP_EMPTY),
    JS_CGETSET_MAGIC_DEF("area", js_rect_get, 0, PROP_AREA),
    JS_CFUNC_MAGIC_DEF("inset", 1, js_rect_method, METHOD_INSET),
    JS_CFUNC_MAGIC_DEF("outset", 1, js_rect_method, METHOD_OUTSET),
    JS_CFUNC_MAGIC_DEF("hsplit", 1, js_rect_method, METHOD_HSPLIT),
    JS_CFUNC_MAGIC_DEF("vsplit", 1, js_rect_method, METHOD_VSPLIT),
    JS_CFUNC_MAGIC_DEF("merge", 1, js_rect_method, METHOD_MERGE),
    JS_CFUNC_MAGIC_DEF("clone", 0, js_rect_method, METHOD_CLONE),
    JS_CFUNC_DEF("toString", 0, js_rect_to_string),
    JS_CFUNC_DEF("toSource", 0, js_rect_to_source),
    JS_CFUNC_MAGIC_DEF("equals", 1, js_rect_funcs, FUNC_EQUALS),
    JS_CFUNC_MAGIC_DEF("round", 0, js_rect_funcs, FUNC_ROUND),
    JS_CFUNC_MAGIC_DEF("contour", 0, js_rect_funcs, FUNC_CONTOUR),
    JS_CFUNC_MAGIC_DEF("toObject", 0, js_rect_funcs, FUNC_TOOBJECT),
    JS_CFUNC_MAGIC_DEF("toArray", 0, js_rect_funcs, FUNC_TOARRAY),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Rect", JS_PROP_CONFIGURABLE),
};
const JSCFunctionListEntry js_rect_static_funcs[] = {
    JS_CFUNC_DEF("from", 1, js_rect_from),
};

extern "C" int
js_rect_init(JSContext* ctx, JSModuleDef* m) {

  if(js_rect_class_id == 0) {
    /* create the Rect class */
    JS_NewClassID(&js_rect_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_rect_class_id, &js_rect_class);

    rect_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, rect_proto, js_rect_proto_funcs, countof(js_rect_proto_funcs));
    JS_SetClassProto(ctx, js_rect_class_id, rect_proto);

    rect_class = JS_NewCFunction2(ctx, js_rect_constructor, "Rect", 0, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, rect_class, rect_proto);
    JS_SetPropertyFunctionList(ctx, rect_class, js_rect_static_funcs, countof(js_rect_static_funcs));

    // js_set_inspect_method(ctx, rect_proto, js_rect_inspect);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Rect", rect_class);

  return 0;
}

extern "C" void
js_rect_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Rect");
}

#ifdef JS_RECT_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_rect
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  if(!(m = JS_NewCModule(ctx, module_name, &js_rect_init)))
    return NULL;
  js_rect_export(ctx, m);
  return m;
}
