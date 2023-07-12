#include "js_affine3.hpp"
#include "js_alloc.hpp"
#include "js_array.hpp"
#include "js_typed_array.hpp"
#include "jsbindings.hpp"
#include "util.hpp"
#include <quickjs.h>
/*#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <utility>*/

enum { PROP_A = 0, PROP_B, PROP_SLOPE, PROP_PIVOT, PROP_TO, PROP_ANGLE, PROP_ASPECT, PROP_LENGTH };
typedef cv::Affine3<double>::Vec3 vector_type;
typedef cv::Affine3<double>::Mat3 mat3_type;

extern "C" {
JSValue affine3_proto = JS_UNDEFINED, affine3_class = JS_UNDEFINED;
thread_local JSClassID js_affine3_class_id = 0;

cv::Affine3<double>*
js_affine3_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<cv::Affine3<double>*>(JS_GetOpaque2(ctx, val, js_affine3_class_id));
}
cv::Affine3<double>*
js_affine3_data(JSValueConst val) {
  return static_cast<cv::Affine3<double>*>(JS_GetOpaque(val, js_affine3_class_id));
}

static JSValue js_affine3_buffer(JSContext* ctx, JSValueConst this_val);
}

JSValue
js_affine3_wrap(JSContext* ctx, const cv::Affine3<double>& affine3) {
  JSValue ret;
  cv::Affine3<double>* aff;

  ret = JS_NewObjectProtoClass(ctx, affine3_proto, js_affine3_class_id);
  aff = js_allocate<cv::Affine3<double>>(ctx);
  *aff = affine3;

  JS_SetOpaque(ret, aff);
  return ret;
}

extern "C" {
JSValue
js_affine3_new(JSContext* ctx) {
  JSValue ret;
  cv::Affine3<double>* aff;

  if(JS_IsUndefined(affine3_proto))
    js_affine3_init(ctx, NULL);

  ret = JS_NewObjectProtoClass(ctx, affine3_proto, js_affine3_class_id);

  aff = js_allocate<cv::Affine3<double>>(ctx);

  new(aff) cv::Affine3<double>();

  JS_SetOpaque(ret, aff);
  return ret;
}

static JSValue
js_affine3_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Affine3<double>*aff, *other;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;

  aff = js_allocate<cv::Affine3<double>>(ctx);
  if(!aff)
    return JS_EXCEPTION;

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_affine3_class_id);
  JS_FreeValue(ctx, proto);
  if(JS_IsException(obj))
    goto fail;

  if(argc > 0) {
    cv::Mat* mat;
    mat3_type matx;
    vector_type rvec, t = vector_type::all(0);

    if(argc > 1)
      if(js_array_to(ctx, argv[1], t) < 3)
        t = vector_type::all(0);

    if((other = js_affine3_data(argv[0]))) {
      new(aff) cv::Affine3<double>(*other);
    } else if((mat = js_mat_data_nothrow(argv[0]))) {
      new(aff) cv::Affine3<double>(*mat);
    } else if(js_array_to(ctx, argv[0], matx) >= 3) {
      new(aff) cv::Affine3<double>(matx, t);
    } else if(js_array_to(ctx, argv[0], rvec) >= 3) {
      new(aff) cv::Affine3<double>(rvec, t);
    } else {
      JS_ThrowTypeError(ctx, "argument 1 must be one of: Affine3, Array, Mat");
      goto fail;
    }
  } else {
    new(aff) cv::Affine3<double>();
  }

  JS_SetOpaque(obj, aff);

  return obj;
fail:
  js_deallocate(ctx, aff);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

static JSValue
js_affine3_get(JSContext* ctx, JSValueConst this_val, int magic) {
  cv::Affine3<double>* aff;

  if(!(aff = js_affine3_data2(ctx, this_val)))
    return JS_UNDEFINED;

  JSValue buffer = js_affine3_buffer(ctx, this_val);

  return js_typedarray<double>::from_buffer(ctx, buffer, magic * 4 * sizeof(double), 4);
}

enum {
  METHOD_CONCATENATE,
  METHOD_INV,
  METHOD_LINEAR,
  METHOD_ROTATE,
  METHOD_ROTATION,
  METHOD_TRANSLATE,
  METHOD_TRANSLATION,
  METHOD_RVEC,
  METHOD_GETMAT,
};

static JSValue
js_affine3_methods(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  cv::Affine3<double>* aff;
  JSValue ret = JS_UNDEFINED;

  if(!(aff = js_affine3_data2(ctx, this_val)))
    return JS_EXCEPTION;

  switch(magic) {
    case METHOD_CONCATENATE: {
      cv::Affine3<double> result, *other;

      if(!(other = js_affine3_data2(ctx, argv[0])))
        return JS_EXCEPTION;

      result = aff->concatenate(*other);
      ret = js_affine3_wrap(ctx, result);
      break;
    }
    case METHOD_INV: {
      cv::Affine3<double> result;
      int32_t method = cv::DECOMP_SVD;

      if(argc > 0)
        JS_ToInt32(ctx, &method, argv[0]);
      result = aff->inv(method);
      ret = js_affine3_wrap(ctx, result);
      break;
    }
    case METHOD_LINEAR: {
      mat3_type matx;

      if(argc > 0) {
        if(js_array_to(ctx, argv[0], matx) < 3) {
          ret = JS_ThrowTypeError(ctx, "argument 1 must be a 3x3 matrix");
          break;
        }
        aff->linear(matx);
      } else {
        matx = aff->linear();
        ret = js_array_from(ctx, matx);
      }
      break;
    }
    case METHOD_ROTATE: {
      cv::Affine3<double> result;
      mat3_type matx;

      if(js_array_to(ctx, argv[0], matx) >= 3) {
        result = aff->rotate(matx);
      } else {
        vector_type vec;
        js_array_to(ctx, argv[0], vec);

        result = aff->rotate(vec);
      }

      ret = js_affine3_wrap(ctx, result);
      break;
    }
    case METHOD_ROTATION: {
      if(argc > 0) {
        cv::Mat* mat;
        mat3_type matx;

        if((mat = js_mat_data_nothrow(argv[0]))) {
          aff->rotation(*mat);
        } else if(js_array_to(ctx, argv[0], matx) >= 3) {
          aff->rotation(matx);
        } else {
          vector_type vec;
          js_array_to(ctx, argv[0], vec);

          aff->rotation(vec);
        }
      } else {
        mat3_type mat = aff->rotation();
        ret = js_array_from(ctx, mat);
      }

      break;
    }
    case METHOD_TRANSLATE: {
      cv::Affine3<double> result;
      vector_type vec;
      js_array_to(ctx, argv[0], vec);

      result = aff->translate(vec);
      ret = js_affine3_wrap(ctx, result);
      break;
    }
    case METHOD_TRANSLATION: {
      vector_type vec;
      if(argc > 0) {
        js_array_to(ctx, argv[0], vec);

        aff->translation(vec);
      } else {
        vec = aff->translation();
        ret = js_array_from(ctx, vec);
      }
      break;
    }
    case METHOD_RVEC: {
      vector_type vec = aff->rvec();
      ret = js_array_from(ctx, vec);
      break;
    }
    case METHOD_GETMAT: {
      vector_type vec = aff->rvec();
      ret = js_array_from(ctx, vec);
      break;
    }
  }
  return ret;
}

static JSValue
js_affine3_array(JSContext* ctx, JSValueConst this_val) {
  cv::Affine3<double>* aff;

  if(!(aff = js_affine3_data2(ctx, this_val)))
    return JS_EXCEPTION;

  JSValue buffer = js_affine3_buffer(ctx, this_val);

  return js_typedarray<double>::from_buffer(ctx, buffer, 0, 4 * 4);
}

static JSValue
js_affine3_identity(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  return js_affine3_wrap(ctx, cv::Affine3<double>::Identity());
}

static JSValue
js_affine3_buffer(JSContext* ctx, JSValueConst this_val) {
  cv::Affine3<double>* aff;

  if(!(aff = js_affine3_data2(ctx, this_val)))
    return JS_EXCEPTION;

  auto& array = mat_array(aff->matrix);

  return js_arraybuffer_from(ctx, array.begin(), array.end(), this_val);
}

static JSValue
js_call_method(JSContext* ctx, JSValue obj, const char* name, int argc, JSValueConst argv[]) {
  JSValue fn, ret = JS_UNDEFINED;

  fn = JS_GetPropertyStr(ctx, obj, name);
  if(!JS_IsUndefined(fn))
    ret = JS_Call(ctx, fn, obj, argc, argv);
  return ret;
}

static JSValue
js_affine3_from(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  cv::Affine3<double> affine3;
  JSValue ret = JS_EXCEPTION;

  return ret;
}

void
js_affine3_finalizer(JSRuntime* rt, JSValue val) {
  cv::Affine3<double>* aff;

  if((aff = js_affine3_data(val)))
    /* Note: 'aff' can be NULL in case JS_SetOpaque() was not called */
    js_deallocate(rt, aff);
}

JSClassDef js_affine3_class = {
    .class_name = "Affine3",
    .finalizer = js_affine3_finalizer,
};

const JSCFunctionListEntry js_affine3_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("0", js_affine3_get, 0, 0),
    JS_CGETSET_MAGIC_DEF("1", js_affine3_get, 0, 1),
    JS_CGETSET_MAGIC_DEF("2", js_affine3_get, 0, 2),
    JS_CGETSET_MAGIC_DEF("3", js_affine3_get, 0, 3),
    JS_CGETSET_DEF("buffer", js_affine3_buffer, 0),
    JS_CGETSET_DEF("array", js_affine3_array, 0),

    JS_CFUNC_MAGIC_DEF("concatenate", 1, js_affine3_methods, METHOD_CONCATENATE),
    JS_CFUNC_MAGIC_DEF("inv", 0, js_affine3_methods, METHOD_INV),
    JS_CFUNC_MAGIC_DEF("linear", 0, js_affine3_methods, METHOD_LINEAR),
    JS_CFUNC_MAGIC_DEF("rotate", 1, js_affine3_methods, METHOD_ROTATE),
    JS_CFUNC_MAGIC_DEF("rotation", 0, js_affine3_methods, METHOD_ROTATION),
    JS_CFUNC_MAGIC_DEF("translate", 1, js_affine3_methods, METHOD_TRANSLATE),
    JS_CFUNC_MAGIC_DEF("translation", 0, js_affine3_methods, METHOD_TRANSLATION),
    JS_CFUNC_MAGIC_DEF("rvec", 0, js_affine3_methods, METHOD_RVEC),
    JS_CFUNC_MAGIC_DEF("getMat", 0, js_affine3_methods, METHOD_GETMAT),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Affine3", JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("length", 4, 0),
};

const JSCFunctionListEntry js_affine3_static_funcs[] = {
    JS_CFUNC_DEF("Identity", 0, js_affine3_identity),
};

int
js_affine3_init(JSContext* ctx, JSModuleDef* m) {

  if(js_affine3_class_id == 0) {
    /* create the Affine3 class */
    JS_NewClassID(&js_affine3_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_affine3_class_id, &js_affine3_class);

    affine3_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, affine3_proto, js_affine3_proto_funcs, countof(js_affine3_proto_funcs));
    JS_SetClassProto(ctx, js_affine3_class_id, affine3_proto);

    affine3_class = JS_NewCFunction2(ctx, js_affine3_constructor, "Affine3", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, affine3_class, affine3_proto);
    JS_SetPropertyFunctionList(ctx, affine3_class, js_affine3_static_funcs, countof(js_affine3_static_funcs));

    // js_set_inspect_method(ctx, affine3_proto, js_affine3_inspect);
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Affine3", affine3_class);

  return 0;
}

extern "C" void
js_affine3_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Affine3");
}

#ifdef JS_AFFINE3_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_affine3
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_affine3_init);
  if(!m)
    return NULL;
  js_affine3_export(ctx, m);
  return m;
}
}
