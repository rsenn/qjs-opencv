#include "js_cv.hpp"
#include "js_umat.hpp"
#include "js_affine3.hpp"
#include "jsbindings.hpp"
#include "util.hpp"
#include <quickjs.h>

#include <opencv2/core/opengl.hpp>

typedef cv::ogl::Buffer JSBufferData;

static JSValue buffer_proto = JS_UNDEFINED, buffer_class = JS_UNDEFINED, buffer_params_proto = JS_UNDEFINED;
static JSClassID js_buffer_class_id = 0;

static JSBufferData*
js_buffer_data(JSValueConst val) {
  return static_cast<JSBufferData*>(JS_GetOpaque(val, js_buffer_class_id));
}

static JSBufferData*
js_buffer_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSBufferData*>(JS_GetOpaque2(ctx, val, js_buffer_class_id));
}

static JSValue
js_buffer_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSBufferData* tx;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(tx = js_allocate<JSBufferData>(ctx)))
    return JS_EXCEPTION;

  try {
    new(tx) JSBufferData();
  } catch(const cv::Exception& e) {
    js_cv_throw(ctx, e);
    goto fail;
  }

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_buffer_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, tx);

  return obj;

fail:
  js_deallocate(ctx, tx);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

/*static JSValue
js_buffer_new(JSContext* ctx, const JSBufferData& arg) {
  JSBufferData* ptr;

  if(!(ptr = js_allocate<JSBufferData>(ctx)))
    return JS_EXCEPTION;

  new(ptr) JSBufferData();

  JSValue obj = JS_NewObjectProtoClass(ctx, buffer_proto, js_buffer_class_id);

  JS_SetOpaque(obj, ptr);

  return obj;
}*/

static void
js_buffer_finalizer(JSRuntime* rt, JSValue val) {
  JSBufferData* tx;

  if((tx = js_buffer_data(val))) {
    tx->~Buffer();

    js_deallocate(rt, tx);
  }
}

enum {
  BUFFER_BIND = 0,
};

static JSValue
js_buffer_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSBufferData* b;
  JSValue ret = JS_UNDEFINED;

  if(!(b = js_buffer_data2(ctx, this_val)))
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case BUFFER_BIND: {
        int32_t target = -1;
        JS_ToInt32(ctx, &target, argv[0]);
        b->bind(cv::ogl::Buffer::Target(target));
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

static JSClassDef js_buffer_class = {
    .class_name = "Buffer",
    .finalizer = js_buffer_finalizer,
};

static const JSCFunctionListEntry js_buffer_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("bind", 1, js_buffer_method, BUFFER_BIND),
};

static const JSCFunctionListEntry js_buffer_static_funcs[] = {
    JS_PROP_INT32_DEF("ARRAY_BUFFER", cv::ogl::Buffer::ARRAY_BUFFER, JS_PROP_CONFIGURABLE),

};

typedef cv::ogl::Texture2D JSTexture2DData;

static JSValue texture2d_proto = JS_UNDEFINED, texture2d_class = JS_UNDEFINED, texture2d_params_proto = JS_UNDEFINED;
static JSClassID js_texture2d_class_id = 0;

static JSTexture2DData*
js_texture2d_data(JSValueConst val) {
  return static_cast<JSTexture2DData*>(JS_GetOpaque(val, js_texture2d_class_id));
}

static JSTexture2DData*
js_texture2d_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSTexture2DData*>(JS_GetOpaque2(ctx, val, js_texture2d_class_id));
}

static JSValue
js_texture2d_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSTexture2DData* tx;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(tx = js_allocate<JSTexture2DData>(ctx)))
    return JS_EXCEPTION;

  new(tx) JSTexture2DData();

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_texture2d_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, tx);

  return obj;

fail:
  js_deallocate(ctx, tx);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

/*static JSValue
js_texture2d_new(JSContext* ctx, const JSTexture2DData& arg) {
  JSTexture2DData* ptr;

  if(!(ptr = js_allocate<JSTexture2DData>(ctx)))
    return JS_EXCEPTION;

  new(ptr) JSTexture2DData();

  JSValue obj = JS_NewObjectProtoClass(ctx, texture2d_proto, js_texture2d_class_id);

  JS_SetOpaque(obj, ptr);

  return obj;
}*/

static void
js_texture2d_finalizer(JSRuntime* rt, JSValue val) {
  JSTexture2DData* tx;

  if((tx = js_texture2d_data(val))) {
    tx->~Texture2D();

    js_deallocate(rt, tx);
  }
}

enum {
  TEXTURE2D_BIND = 0,
};

static JSValue
js_texture2d_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSTexture2DData* tx;
  JSValue ret = JS_UNDEFINED;

  if(!(tx = js_texture2d_data2(ctx, this_val)))
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case TEXTURE2D_BIND: {
        tx->bind();
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

static JSClassDef js_texture2d_class = {
    .class_name = "Texture2D",
    .finalizer = js_texture2d_finalizer,
};

static const JSCFunctionListEntry js_texture2d_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("bind", 0, js_texture2d_method, TEXTURE2D_BIND),
};

static const JSCFunctionListEntry js_texture2d_static_funcs[] = {};

enum {
  OPENGL_CONVERT_FROM_GL_TEXTURE_2D = 0,
  OPENGL_CONVERT_TO_GL_TEXTURE_2D,
};

static JSValue
js_opengl_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
    switch(magic) {
      case OPENGL_CONVERT_FROM_GL_TEXTURE_2D: {
        JSTexture2DData* tx;

        if(!(tx = js_texture2d_data2(ctx, argv[0])))
          return JS_EXCEPTION;

        JSOutputArray dst = js_cv_outputarray(ctx, argv[1]);

        cv::ogl::convertFromGLTexture2D(*tx, dst);
        break;
      }
      case OPENGL_CONVERT_TO_GL_TEXTURE_2D: {
        JSInputArray src = js_input_array(ctx, argv[0]);
        JSTexture2DData* tx;

        if(!(tx = js_texture2d_data2(ctx, argv[1])))
          return JS_EXCEPTION;

        cv::ogl::convertToGLTexture2D(src, *tx);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

js_function_list_t js_opengl_ogl_funcs{
    JS_CFUNC_MAGIC_DEF("convertFromGLTexture2D", 2, js_opengl_func, OPENGL_CONVERT_FROM_GL_TEXTURE_2D),
    JS_CFUNC_MAGIC_DEF("convertToGLTexture2D", 2, js_opengl_func, OPENGL_CONVERT_TO_GL_TEXTURE_2D)
};

/*js_function_list_t js_opengl_static_funcs{
    JS_OBJECT_DEF("ogl", js_opengl_ogl_funcs.data(), int(js_opengl_ogl_funcs.size()), JS_PROP_C_W_E),
};*/

static JSValue ogl_object;

extern "C" int
js_opengl_init(JSContext* ctx, JSModuleDef* m) {

  ogl_object = JS_NewObjectProto(ctx, JS_NULL);

  JS_SetPropertyFunctionList(ctx, ogl_object, js_opengl_ogl_funcs.data(), js_opengl_ogl_funcs.size());

  JS_NewClassID(&js_buffer_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_buffer_class_id, &js_buffer_class);

  buffer_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, buffer_proto, js_buffer_proto_funcs, countof(js_buffer_proto_funcs));
  JS_SetClassProto(ctx, js_buffer_class_id, buffer_proto);

  buffer_class = JS_NewCFunction2(ctx, js_buffer_constructor, "Buffer", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, buffer_class, buffer_proto);
  JS_SetPropertyFunctionList(ctx, buffer_class, js_buffer_static_funcs, countof(js_buffer_static_funcs));

  JS_SetPropertyStr(ctx, ogl_object, "Buffer", buffer_class);

  JS_NewClassID(&js_texture2d_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_texture2d_class_id, &js_texture2d_class);

  texture2d_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, texture2d_proto, js_texture2d_proto_funcs, countof(js_texture2d_proto_funcs));
  JS_SetClassProto(ctx, js_texture2d_class_id, texture2d_proto);

  texture2d_class = JS_NewCFunction2(ctx, js_texture2d_constructor, "Texture2D", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, texture2d_class, texture2d_proto);
  JS_SetPropertyFunctionList(ctx, texture2d_class, js_texture2d_static_funcs, countof(js_texture2d_static_funcs));

  JS_SetPropertyStr(ctx, ogl_object, "Texture2D", texture2d_class);

  if(m) {
    JS_SetModuleExport(ctx, m, "ogl", ogl_object);
  }

  return 0;
}

extern "C" void
js_opengl_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "ogl");
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_opengl
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_opengl_init)))
    return NULL;

  js_opengl_export(ctx, m);
  return m;
}
