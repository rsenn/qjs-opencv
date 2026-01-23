#include "js_alloc.hpp"
#include "js_point.hpp"
#include "js_rect.hpp"
#include "js_size.hpp"
#include "jsbindings.hpp"
#include <opencv2/core/types.hpp>
#include <quickjs.h>
#include <stddef.h>
#include <new>

enum { ROTATED_RECT_PROP_CENTER = 0, ROTATED_RECT_PROP_SIZE, ROTATED_RECT_PROP_ANGLE };
enum { ROTATED_RECT_METHOD_BOUNDING_RECT = 0, ROTATED_RECT_METHOD_BOUNDING_RECT2F, ROTATED_RECT_METHOD_POINTS };

extern "C" {
thread_local JSValue rotated_rect_proto = JS_UNDEFINED, rotated_rect_class = JS_UNDEFINED;
thread_local JSClassID js_rotated_rect_class_id = 0;
}

JSRotatedRectData*
js_rotated_rect_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSRotatedRectData*>(JS_GetOpaque2(ctx, val, js_rotated_rect_class_id));
}
JSRotatedRectData*
js_rotated_rect_data(JSValueConst val) {
  return static_cast<JSRotatedRectData*>(JS_GetOpaque(val, js_rotated_rect_class_id));
}

extern "C" int js_rotated_rect_init(JSContext* ctx, JSModuleDef* m);

JSValue
js_rotated_rect_new(JSContext* ctx, JSValueConst proto, const JSPointData<float>& center, const JSSizeData<float>& size, float angle) {
  JSValue ret;
  JSRotatedRectData* rr;

  if(JS_IsUndefined(rotated_rect_proto))
    js_rotated_rect_init(ctx, NULL);

  if(JS_IsUndefined(proto))
    proto = rotated_rect_proto;

  ret = JS_NewObjectProtoClass(ctx, proto, js_rotated_rect_class_id);

  rr = js_allocate<JSRotatedRectData>(ctx);

  rr->center = center;
  rr->size = size;
  rr->angle = angle;

  JS_SetOpaque(ret, rr);
  return ret;
}

extern "C" {
static JSValue
js_rotated_rect_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSRotatedRectData* rr;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;

  rr = js_allocate<JSRotatedRectData>(ctx);

  if(!rr)
    return JS_EXCEPTION;

  /* using new_target to get the prototype is necessary when the
     class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_rotated_rect_class_id);
  JS_FreeValue(ctx, proto);
  if(JS_IsException(obj))
    goto fail;

  {
    JSPointData<float> point1, point2, point3;
    JSSizeData<float> size;
    double angle;

    if(argc >= 1)
      js_point_read(ctx, argv[0], &point1);

    if(argc >= 3 && js_size_read(ctx, argv[1], &size)) {
      JS_ToFloat64(ctx, &angle, argv[2]);
      new(rr) cv::RotatedRect(point1, size, angle);
    } else if(argc >= 3 && js_point_read(ctx, argv[1], &point2) && js_point_read(ctx, argv[2], &point3)) {
      new(rr) cv::RotatedRect(point1, point2, point3);
    } else {
      new(rr) cv::RotatedRect();
    }
  }

  JS_SetOpaque(obj, rr);
  return obj;
fail:
  js_deallocate(ctx, rr);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

static JSValue
js_rotated_rect_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSRotatedRectData* rr;
  JSValue ret = JS_UNDEFINED;

  if(!(rr = js_rotated_rect_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case ROTATED_RECT_PROP_CENTER: {
      ret = js_point_new(ctx, rr->center);
      break;
    }

    case ROTATED_RECT_PROP_SIZE: {
      ret = js_size_new(ctx, rr->size);
      break;
    }

    case ROTATED_RECT_PROP_ANGLE: {
      ret = JS_NewFloat64(ctx, rr->angle);
      break;
    }
  }

  return ret;
}

static JSValue
js_rotated_rect_set(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic) {
  JSRotatedRectData* rr;
  JSValue ret = JS_UNDEFINED;

  if(!(rr = js_rotated_rect_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case ROTATED_RECT_PROP_CENTER: {
      JSPointData<float> center;
      if(js_point_read(ctx, value, &center))
        rr->center = center;
      break;
    }

    case ROTATED_RECT_PROP_SIZE: {
      JSSizeData<float> size;
      if(js_size_read(ctx, value, &size))
        rr->size = size;
      break;
    }

    case ROTATED_RECT_PROP_ANGLE: {
      double angle;
      if(!JS_ToFloat64(ctx, &angle, value))
        rr->angle = angle;
      break;
    }
  }

  return ret;
}

static JSValue
js_rotated_rect_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSRotatedRectData* rr;
  JSValue ret = JS_UNDEFINED;

  if(!(rr = js_rotated_rect_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case ROTATED_RECT_METHOD_BOUNDING_RECT: {
      JSRectData<int> rect = rr->boundingRect();
      ret = js_rect_new(ctx, rect);
      break;
    }

    case ROTATED_RECT_METHOD_BOUNDING_RECT2F: {
      JSRectData<float> rect = rr->boundingRect2f();
      ret = js_rect_new(ctx, rect);
      break;
    }

    case ROTATED_RECT_METHOD_POINTS: {
      JSValue result;
      JSPointData<float> pts[4];
      rr->points(pts);
      if(js_is_array(ctx, argv[0])) {
        result = JS_DupValue(ctx, argv[0]);
      } else {
        ret = JS_NewArray(ctx);
        result = JS_DupValue(ctx, ret);
      }
      for(size_t i = 0; i < 4; i++)
        JS_SetPropertyUint32(ctx, result, i, js_point_new(ctx, pts[i]));
      JS_FreeValue(ctx, result);
      break;
    }
  }

  return ret;
}

static JSValue
js_rotated_rect_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSRotatedRectData* rr = js_rotated_rect_data2(ctx, this_val);
  JSValue obj = JS_NewObjectClass(ctx, js_rotated_rect_class_id);

  JS_DefinePropertyValueStr(ctx, obj, "center", js_point_new(ctx, rr->center), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "size", js_size_new(ctx, rr->size), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "angle", JS_NewFloat64(ctx, rr->angle), JS_PROP_ENUMERABLE);
  return obj;
}

static JSValue
js_rotated_rect_from(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSRotatedRectData rr;
  JSValue ret = JS_EXCEPTION;

  return ret;
}

void
js_rotated_rect_finalizer(JSRuntime* rt, JSValue val) {
  JSRotatedRectData* rr;

  if((rr = static_cast<JSRotatedRectData*>(JS_GetOpaque(val, js_rotated_rect_class_id))))
    /* Note: 'rr' can be NULL in case JS_SetOpaque() was not called */
    js_deallocate(rt, rr);
}

JSClassDef js_rotated_rect_class = {
    .class_name = "RotatedRect",
    .finalizer = js_rotated_rect_finalizer,
};

const JSCFunctionListEntry js_rotated_rect_proto_funcs[] = {

    JS_CGETSET_MAGIC_DEF("center", js_rotated_rect_get, js_rotated_rect_set, ROTATED_RECT_PROP_CENTER),
    JS_CGETSET_MAGIC_DEF("size", js_rotated_rect_get, js_rotated_rect_set, ROTATED_RECT_PROP_SIZE),
    JS_CGETSET_MAGIC_DEF("angle", js_rotated_rect_get, js_rotated_rect_set, ROTATED_RECT_PROP_ANGLE),
    JS_CFUNC_MAGIC_DEF("boundingRect", 0, js_rotated_rect_method, ROTATED_RECT_METHOD_BOUNDING_RECT),
    JS_CFUNC_MAGIC_DEF("boundingRect2f", 0, js_rotated_rect_method, ROTATED_RECT_METHOD_BOUNDING_RECT2F),
    JS_CFUNC_MAGIC_DEF("points", 1, js_rotated_rect_method, ROTATED_RECT_METHOD_POINTS),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "RotatedRect", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_rotated_rect_static_funcs[] = {
    JS_CFUNC_DEF("from", 1, js_rotated_rect_from),
};

int
js_rotated_rect_init(JSContext* ctx, JSModuleDef* m) {

  /*if(js_rotated_rect_class_id == 0)*/ {
    /* create the RotatedRect class */
    JS_NewClassID(&js_rotated_rect_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_rotated_rect_class_id, &js_rotated_rect_class);

    rotated_rect_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, rotated_rect_proto, js_rotated_rect_proto_funcs, countof(js_rotated_rect_proto_funcs));
    JS_SetClassProto(ctx, js_rotated_rect_class_id, rotated_rect_proto);

    rotated_rect_class = JS_NewCFunction2(ctx, js_rotated_rect_constructor, "RotatedRect", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, rotated_rect_class, rotated_rect_proto);
    JS_SetPropertyFunctionList(ctx, rotated_rect_class, js_rotated_rect_static_funcs, countof(js_rotated_rect_static_funcs));

    // js_object_inspect(ctx, rotated_rect_proto, js_rotated_rect_inspect);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "RotatedRect", rotated_rect_class);

  return 0;
}

extern "C" void
js_rotated_rect_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "RotatedRect");
}

#ifdef JS_ROTATED_RECT_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_rotated_rect
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  if(!(m = JS_NewCModule(ctx, module_name, &js_rotated_rect_init)))
    return NULL;
  js_rotated_rect_export(ctx, m);
  return m;
}
}
