#include "js_cv.hpp"
#include "js_mat.hpp"
#include "js_umat.hpp"
#include "js_size.hpp"
#include "include/jsbindings.hpp"
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
  JSSizeData<int> asize;
  cv::ogl::Buffer::Target target = cv::ogl::Buffer::ARRAY_BUFFER;
  int32_t abufId = -1, atype = -1;
  int i = 0;
  bool autoRelease = false;

  if(!(tx = js_allocate<JSBufferData>(ctx)))
    return JS_EXCEPTION;

  if(i < argc) {
    if(JS_IsNumber(argv[i])) {
      asize.width = js_value_to<uint32_t>(ctx, argv[i++]);
      asize.height = js_value_to<uint32_t>(ctx, argv[i++]);
    } else if(!js_size_read(ctx, argv[i], &asize)) {
      i++;
    } else {
      JS_ThrowTypeError(ctx, "argument 1 must be Number or cv::Size");
      goto fail;
    }
  }

  if(!(tx = js_allocate<JSBufferData>(ctx)))
    return JS_EXCEPTION;

  try {
    if(argc == 0) {
      new(tx) JSBufferData();
    } else if(i > 0) {
      if(i < argc)
        atype = js_value_to<int32_t>(ctx, argv[i++]);

      if(i < argc && JS_IsNumber(argv[i]))
        abufId = js_value_to<int32_t>(ctx, argv[i++]);

      if(i < argc)
        autoRelease = js_value_to<BOOL>(ctx, argv[i++]);

      if((abufId & 0xffffff00) == 0x8800) {
        target = cv::ogl::Buffer::Target(abufId);
        new(tx) JSBufferData(asize, atype, target, autoRelease);
      } else {
        new(tx) JSBufferData(asize, atype, abufId, autoRelease);
      }

    } else {
      JSInputArray arr = js_input_array(ctx, argv[0]);

      if(argc > 1)
        target = cv::ogl::Buffer::Target(js_value_to<int32_t>(ctx, argv[1]));

      if(argc > 2)
        autoRelease = js_value_to<BOOL>(ctx, argv[2]);

      new(tx) JSBufferData(arr, target, autoRelease);
    }

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
  BUFFER_UNBIND = 0,
};

static JSValue
js_buffer_function(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
    switch(magic) {
      case BUFFER_UNBIND: {
        int32_t target = -1;
        JS_ToInt32(ctx, &target, argv[0]);
        cv::ogl::Buffer::unbind(cv::ogl::Buffer::Target(target));
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

enum {
  BUFFER_ID = 0,
  BUFFER_CHANNELS,
  BUFFER_COLS,
  BUFFER_ROWS,
  BUFFER_DEPTH,
  BUFFER_ELEMSIZE,
  BUFFER_ELEMSIZE1,
  BUFFER_EMPTY,
  BUFFER_SIZE,
  BUFFER_TYPE,
};

static JSValue
js_buffer_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSBufferData* b;
  JSValue ret = JS_UNDEFINED;

  if(!(b = js_buffer_data2(ctx, this_val)))
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case BUFFER_ID: {
        ret = JS_NewInt32(ctx, b->bufId());
        break;
      }

      case BUFFER_CHANNELS: {
        ret = JS_NewUint32(ctx, b->channels());
        break;
      }

      case BUFFER_COLS: {
        ret = JS_NewUint32(ctx, b->cols());
        break;
      }

      case BUFFER_ROWS: {
        ret = JS_NewUint32(ctx, b->rows());
        break;
      }

      case BUFFER_DEPTH: {
        ret = JS_NewUint32(ctx, b->depth());
        break;
      }

      case BUFFER_ELEMSIZE: {
        ret = JS_NewUint32(ctx, b->elemSize());
        break;
      }

      case BUFFER_ELEMSIZE1: {
        ret = JS_NewUint32(ctx, b->elemSize1());
        break;
      }

      case BUFFER_EMPTY: {
        ret = JS_NewBool(ctx, b->empty());
        break;
      }

      case BUFFER_SIZE: {
        ret = js_size_new(ctx, b->size());
        break;
      }

      case BUFFER_TYPE: {
        ret = JS_NewInt32(ctx, b->type());
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

enum {
  BUFFER_BIND = 0,
  BUFFER_CLONE,
  BUFFER_COPY_FROM,
  BUFFER_COPY_TO,
  BUFFER_CREATE,
  BUFFER_RELEASE,
  BUFFER_SET_AUTO_RELEASE,
  BUFFER_MAP_HOST,
  BUFFER_UNMAP_HOST,
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

      case BUFFER_CLONE: {
        cv::ogl::Buffer::Target target = cv::ogl::Buffer::ARRAY_BUFFER;
        bool autoRelease = false;
        int i = 0;

        if(i < argc)
          target = cv::ogl::Buffer::Target(js_value_to<int32_t>(ctx, argv[i++]));

        if(i < argc)
          autoRelease = js_value_to<BOOL>(ctx, argv[i++]);

        b->clone(target, autoRelease);
        break;
      }

      case BUFFER_COPY_FROM: {
        JSInputArray arr = js_input_array(ctx, argv[0]);
        cv::ogl::Buffer::Target target = cv::ogl::Buffer::ARRAY_BUFFER;
        bool autoRelease = false;

        if(argc > 1)
          target = cv::ogl::Buffer::Target(js_value_to<int32_t>(ctx, argv[1]));

        if(argc > 2)
          autoRelease = js_value_to<BOOL>(ctx, argv[2]);

        b->copyFrom(arr, target, autoRelease);
        break;
      }

      case BUFFER_COPY_TO: {
        JSOutputArray arr = js_cv_outputarray(ctx, argv[0]);

        b->copyTo(arr);
        break;
      }

      case BUFFER_CREATE: {
        JSSizeData<int> asize;
        int32_t atype;
        cv::ogl::Buffer::Target target = cv::ogl::Buffer::ARRAY_BUFFER;
        bool autoRelease = false;
        int i = 0;

        if(i < argc) {
          if(JS_IsNumber(argv[i])) {
            asize.height = js_value_to<uint32_t>(ctx, argv[i++]);
            asize.width = js_value_to<uint32_t>(ctx, argv[i++]);
          } else if(!js_size_read(ctx, argv[i], &asize)) {
            i++;
          } else {
            ret = JS_ThrowTypeError(ctx, "argument 1 must be Number or cv::Size");
            break;
          }
        }

        if(i < argc)
          atype = js_value_to<int32_t>(ctx, argv[i++]);

        if(i < argc && JS_IsNumber(argv[i]))
          target = cv::ogl::Buffer::Target(js_value_to<int32_t>(ctx, argv[i++]));

        if(i < argc)
          autoRelease = js_value_to<BOOL>(ctx, argv[i++]);

        b->create(asize, atype, target, autoRelease);
        break;
      }

      case BUFFER_RELEASE: {
        b->release();
        break;
      }

      case BUFFER_SET_AUTO_RELEASE: {
        bool autoRelease = false;
        if(argc > 0)
          autoRelease = js_value_to<BOOL>(ctx, argv[0]);

        b->setAutoRelease(autoRelease);
        break;
      }

      case BUFFER_MAP_HOST: {
        ret = js_mat_wrap(ctx, b->mapHost(cv::ogl::Buffer::Access(js_value_to<int32_t>(ctx, argv[0]))));
        break;
      }

      case BUFFER_UNMAP_HOST: {
        b->unmapHost();
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
    JS_CFUNC_MAGIC_DEF("clone", 0, js_buffer_method, BUFFER_CLONE),
    JS_CFUNC_MAGIC_DEF("copyFrom", 1, js_buffer_method, BUFFER_COPY_FROM),
    JS_CFUNC_MAGIC_DEF("copyTo", 1, js_buffer_method, BUFFER_COPY_TO),
    JS_CFUNC_MAGIC_DEF("create", 2, js_buffer_method, BUFFER_CREATE),

    // XXX: TODO: [un]mapDevice()

    JS_CFUNC_MAGIC_DEF("release", 0, js_buffer_method, BUFFER_RELEASE),
    JS_CFUNC_MAGIC_DEF("setAutoRelease", 1, js_buffer_method, BUFFER_SET_AUTO_RELEASE),
    JS_CFUNC_MAGIC_DEF("mapHost", 1, js_buffer_method, BUFFER_MAP_HOST),
    JS_CFUNC_MAGIC_DEF("unmapHost", 0, js_buffer_method, BUFFER_UNMAP_HOST),

    JS_CGETSET_MAGIC_DEF("id", js_buffer_get, 0, BUFFER_ID),
    JS_CGETSET_MAGIC_DEF("channels", js_buffer_get, 0, BUFFER_CHANNELS),
    JS_CGETSET_MAGIC_DEF("cols", js_buffer_get, 0, BUFFER_COLS),
    JS_CGETSET_MAGIC_DEF("rows", js_buffer_get, 0, BUFFER_ROWS),
    JS_CGETSET_MAGIC_DEF("depth", js_buffer_get, 0, BUFFER_DEPTH),
    JS_CGETSET_MAGIC_DEF("elemSize", js_buffer_get, 0, BUFFER_ELEMSIZE),
    JS_CGETSET_MAGIC_DEF("elemSize1", js_buffer_get, 0, BUFFER_ELEMSIZE1),
    JS_CGETSET_MAGIC_DEF("empty", js_buffer_get, 0, BUFFER_EMPTY),
    JS_CGETSET_MAGIC_DEF("size", js_buffer_get, 0, BUFFER_SIZE),
    JS_CGETSET_MAGIC_DEF("type", js_buffer_get, 0, BUFFER_TYPE),
};

static const JSCFunctionListEntry js_buffer_static_funcs[] = {
    JS_CFUNC_MAGIC_DEF("unbind", 1, js_buffer_function, BUFFER_UNBIND),

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
  JSSizeData<int> asize;
  int i = 0;
  cv::ogl::Texture2D::Format format;
  int32_t atexId = -1;
  bool autoRelease = false;

  if(!(tx = js_allocate<JSTexture2DData>(ctx)))
    return JS_EXCEPTION;

  if(i < argc) {
    if(JS_IsNumber(argv[i])) {
      asize.width = js_value_to<uint32_t>(ctx, argv[i++]);
      asize.height = js_value_to<uint32_t>(ctx, argv[i++]);
    } else if(!js_size_read(ctx, argv[i], &asize)) {
      i++;
    } else {
      JS_ThrowTypeError(ctx, "argument 1 must be Number or cv::Size");
      goto fail;
    }
  }

  try {
    if(argc == 0) {
      new(tx) JSTexture2DData();
    } else if(i) {

      if(i < argc)
        format = cv::ogl::Texture2D::Format(js_value_to<int32_t>(ctx, argv[i++]));

      if(i < argc && JS_IsNumber(argv[i]))
        atexId = js_value_to<int32_t>(ctx, argv[i++]);

      if(i < argc)
        autoRelease = js_value_to<BOOL>(ctx, argv[i++]);

      if(atexId != -1)
        new(tx) JSTexture2DData(asize, format, atexId, autoRelease);
      else
        new(tx) JSTexture2DData(asize, format, autoRelease);

    } else {

      JSInputArray arr = js_input_array(ctx, argv[i++]);

      if(i < argc)
        autoRelease = js_value_to<BOOL>(ctx, argv[i++]);

      new(tx) JSTexture2DData(arr, autoRelease);
    }
  } catch(const cv::Exception& e) {
    js_cv_throw(ctx, e);
    goto fail;
  }

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

enum {
  TEXTURE2D_ID = 0,
  TEXTURE2D_COLS,
  TEXTURE2D_ROWS,
  TEXTURE2D_EMPTY,
  TEXTURE2D_SIZE,
  TEXTURE2D_FORMAT,
};

static JSValue
js_texture2d_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSTexture2DData* tx;
  JSValue ret = JS_UNDEFINED;

  if(!(tx = js_texture2d_data2(ctx, this_val)))
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case TEXTURE2D_ID: {
        ret = JS_NewUint32(ctx, tx->texId());
        break;
      }

      case TEXTURE2D_COLS: {
        ret = JS_NewUint32(ctx, tx->cols());
        break;
      }

      case TEXTURE2D_ROWS: {
        ret = JS_NewUint32(ctx, tx->rows());
        break;
      }

      case TEXTURE2D_EMPTY: {
        ret = JS_NewBool(ctx, tx->empty());
        break;
      }

      case TEXTURE2D_SIZE: {
        ret = js_size_new(ctx, tx->size());
        break;
      }

      case TEXTURE2D_FORMAT: {
        ret = JS_NewInt32(ctx, tx->format());
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

enum {
  TEXTURE2D_BIND = 0,
  TEXTURE2D_COPY_FROM,
  TEXTURE2D_COPY_TO,
  TEXTURE2D_CREATE,
  TEXTURE2D_RELEASE,
  TEXTURE2D_SET_AUTO_RELEASE,
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

      case TEXTURE2D_COPY_FROM: {
        JSInputArray arr = js_input_array(ctx, argv[0]);
        bool autoRelease = false;

        if(argc > 1)
          autoRelease = js_value_to<BOOL>(ctx, argv[1]);

        tx->copyFrom(arr, autoRelease);
        break;
      }

      case TEXTURE2D_COPY_TO: {
        JSOutputArray arr = js_cv_outputarray(ctx, argv[0]);
        int32_t ddepth = CV_32F;
        bool autoRelease = false;

        if(argc > 1)
          ddepth = js_value_to<int32_t>(ctx, argv[1]);

        if(argc > 2)
          autoRelease = js_value_to<BOOL>(ctx, argv[2]);

        tx->copyTo(arr, ddepth, autoRelease);
        break;
      }

      case TEXTURE2D_CREATE: {
        JSSizeData<int> asize;
        cv::ogl::Texture2D::Format format;
        bool autoRelease = false;
        int i = 0;

        if(i < argc) {
          if(JS_IsNumber(argv[i])) {
            asize.height = js_value_to<uint32_t>(ctx, argv[i++]);
            asize.width = js_value_to<uint32_t>(ctx, argv[i++]);
          } else if(!js_size_read(ctx, argv[i], &asize)) {
            i++;
          } else {
            ret = JS_ThrowTypeError(ctx, "argument 1 must be Number or cv::Size");
            break;
          }
        }

        if(i < argc)
          format = cv::ogl::Texture2D::Format(js_value_to<int32_t>(ctx, argv[i++]));

        if(i < argc)
          autoRelease = js_value_to<BOOL>(ctx, argv[i++]);

        tx->create(asize, format, autoRelease);
        break;
      }

      case TEXTURE2D_RELEASE: {
        tx->release();
        break;
      }

      case TEXTURE2D_SET_AUTO_RELEASE: {
        bool autoRelease = js_value_to<BOOL>(ctx, argv[0]);

        tx->setAutoRelease(autoRelease);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

static void
js_texture2d_finalizer(JSRuntime* rt, JSValue val) {
  JSTexture2DData* tx;

  if((tx = js_texture2d_data(val))) {
    tx->~Texture2D();

    js_deallocate(rt, tx);
  }
}

static JSClassDef js_texture2d_class = {
    .class_name = "Texture2D",
    .finalizer = js_texture2d_finalizer,
};

static const JSCFunctionListEntry js_texture2d_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("bind", 0, js_texture2d_method, TEXTURE2D_BIND),
    JS_CFUNC_MAGIC_DEF("copyFrom", 1, js_texture2d_method, TEXTURE2D_COPY_FROM),
    JS_CFUNC_MAGIC_DEF("copyTo", 1, js_texture2d_method, TEXTURE2D_COPY_TO),
    JS_CFUNC_MAGIC_DEF("create", 2, js_texture2d_method, TEXTURE2D_CREATE),
    JS_CFUNC_MAGIC_DEF("release", 0, js_texture2d_method, TEXTURE2D_RELEASE),
    JS_CFUNC_MAGIC_DEF("setAutoRelease", 1, js_texture2d_method, TEXTURE2D_SET_AUTO_RELEASE),
    JS_CGETSET_MAGIC_DEF("id", js_texture2d_get, 0, TEXTURE2D_ID),
    JS_CGETSET_MAGIC_DEF("cols", js_texture2d_get, 0, TEXTURE2D_COLS),
    JS_CGETSET_MAGIC_DEF("rows", js_texture2d_get, 0, TEXTURE2D_ROWS),
    JS_CGETSET_MAGIC_DEF("empty", js_texture2d_get, 0, TEXTURE2D_EMPTY),
    JS_CGETSET_MAGIC_DEF("size", js_texture2d_get, 0, TEXTURE2D_SIZE),
    JS_CGETSET_MAGIC_DEF("format", js_texture2d_get, 0, TEXTURE2D_FORMAT),
};

static const JSCFunctionListEntry js_texture2d_static_funcs[] = {
    JS_PROP_INT32_DEF("NONE", cv::ogl::Texture2D::NONE, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("DEPTH_COMPONENT", cv::ogl::Texture2D::DEPTH_COMPONENT, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("RGB", cv::ogl::Texture2D::RGB, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("RGBA", cv::ogl::Texture2D::RGBA, JS_PROP_CONFIGURABLE),
};

typedef cv::ogl::Arrays JSArraysData;

static JSValue arrays_proto = JS_UNDEFINED, arrays_class = JS_UNDEFINED, arrays_params_proto = JS_UNDEFINED;
static JSClassID js_arrays_class_id = 0;

static JSArraysData*
js_arrays_data(JSValueConst val) {
  return static_cast<JSArraysData*>(JS_GetOpaque(val, js_arrays_class_id));
}

static JSArraysData*
js_arrays_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSArraysData*>(JS_GetOpaque2(ctx, val, js_arrays_class_id));
}

static JSValue
js_arrays_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSArraysData* a;
  JSValue obj = JS_UNDEFINED, proto;

  if(!(a = js_allocate<JSArraysData>(ctx)))
    return JS_EXCEPTION;

  try {
    new(a) JSArraysData();
  } catch(const cv::Exception& e) {
    js_cv_throw(ctx, e);
    goto fail;
  }

  /* using new_target to get the prototype is necessary when the class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  obj = JS_NewObjectProtoClass(ctx, proto, js_arrays_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, a);

  return obj;

fail:
  js_deallocate(ctx, a);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

enum {
  ARRAYS_EMPTY = 0,
  ARRAYS_SIZE,
};

static JSValue
js_arrays_get(JSContext* ctx, JSValueConst this_val, int magic) {
  JSArraysData* a;
  JSValue ret = JS_UNDEFINED;

  if(!(a = js_arrays_data2(ctx, this_val)))
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case ARRAYS_EMPTY: {
        ret = JS_NewBool(ctx, a->empty());
        break;
      }

      case ARRAYS_SIZE: {
        ret = JS_NewInt32(ctx, a->size());
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

enum {
  ARRAYS_BIND = 0,
  ARRAYS_RESET_COLOR_ARRAY,
  ARRAYS_RESET_NORMAL_ARRAY,
  ARRAYS_RESET_TEXCOORD_ARRAY,
  ARRAYS_RESET_VERTEX_ARRAY,
  ARRAYS_RELEASE,
  ARRAYS_SET_AUTO_RELEASE,
  ARRAYS_SET_COLOR_ARRAY,
  ARRAYS_SET_NORMAL_ARRAY,
  ARRAYS_SET_TEXCOORD_ARRAY,
  ARRAYS_SET_VERTEX_ARRAY,
};

static JSValue
js_arrays_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSArraysData* a;
  JSValue ret = JS_UNDEFINED;

  if(!(a = js_arrays_data2(ctx, this_val)))
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case ARRAYS_BIND: {
        a->bind();
        break;
      }

      case ARRAYS_RESET_COLOR_ARRAY: {
        a->resetColorArray();
        break;
      }

      case ARRAYS_RESET_NORMAL_ARRAY: {
        a->resetNormalArray();
        break;
      }

      case ARRAYS_RESET_TEXCOORD_ARRAY: {
        a->resetTexCoordArray();
        break;
      }

      case ARRAYS_RESET_VERTEX_ARRAY: {
        a->resetVertexArray();
        break;
      }

      case ARRAYS_RELEASE: {
        a->release();
        break;
      }

      case ARRAYS_SET_AUTO_RELEASE: {
        bool autoRelease = js_value_to<BOOL>(ctx, argv[0]);

        a->setAutoRelease(autoRelease);
        break;
      }

      case ARRAYS_SET_COLOR_ARRAY: {
        JSInputArray arr = js_input_array(ctx, argv[0]);
        a->setColorArray(arr);
        break;
      }

      case ARRAYS_SET_NORMAL_ARRAY: {
        JSInputArray arr = js_input_array(ctx, argv[0]);
        a->setNormalArray(arr);
        break;
      }

      case ARRAYS_SET_TEXCOORD_ARRAY: {
        JSInputArray arr = js_input_array(ctx, argv[0]);
        a->setTexCoordArray(arr);
        break;
      }

      case ARRAYS_SET_VERTEX_ARRAY: {
        JSInputArray arr = js_input_array(ctx, argv[0]);
        a->setVertexArray(arr);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

static void
js_arrays_finalizer(JSRuntime* rt, JSValue val) {
  JSArraysData* a;

  if((a = js_arrays_data(val))) {
    a->~Arrays();

    js_deallocate(rt, a);
  }
}

static JSClassDef js_arrays_class = {
    .class_name = "Arrays",
    .finalizer = js_arrays_finalizer,
};

static const JSCFunctionListEntry js_arrays_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("bind", 0, js_arrays_method, ARRAYS_BIND),
    JS_CGETSET_MAGIC_DEF("empty", js_arrays_get, 0, ARRAYS_EMPTY),
    JS_CFUNC_MAGIC_DEF("release", 0, js_arrays_method, ARRAYS_RELEASE),
    JS_CFUNC_MAGIC_DEF("resetColorArray", 0, js_arrays_method, ARRAYS_RESET_COLOR_ARRAY),
    JS_CFUNC_MAGIC_DEF("resetNormalArray", 0, js_arrays_method, ARRAYS_RESET_NORMAL_ARRAY),
    JS_CFUNC_MAGIC_DEF("resetTexCoordArray", 0, js_arrays_method, ARRAYS_RESET_TEXCOORD_ARRAY),
    JS_CFUNC_MAGIC_DEF("resetVertexArray", 0, js_arrays_method, ARRAYS_RESET_VERTEX_ARRAY),
    JS_CFUNC_MAGIC_DEF("setAutoRelease", 1, js_arrays_method, ARRAYS_SET_AUTO_RELEASE),
    JS_CFUNC_MAGIC_DEF("setColorArray", 1, js_arrays_method, ARRAYS_SET_COLOR_ARRAY),
    JS_CFUNC_MAGIC_DEF("setNormalArray", 1, js_arrays_method, ARRAYS_SET_NORMAL_ARRAY),
    JS_CFUNC_MAGIC_DEF("setTexCoordArray", 1, js_arrays_method, ARRAYS_SET_TEXCOORD_ARRAY),
    JS_CFUNC_MAGIC_DEF("setVertexArray", 1, js_arrays_method, ARRAYS_SET_VERTEX_ARRAY),
    JS_CGETSET_MAGIC_DEF("size", js_arrays_get, 0, ARRAYS_SIZE),
};

static const JSCFunctionListEntry js_arrays_static_funcs[] = {};

enum {
  OPENGL_CONVERT_FROM_GL_TEXTURE_2D = 0,
  OPENGL_CONVERT_TO_GL_TEXTURE_2D,
  OPENGL_MAP_GL_BUFFER,
  OPENGL_RENDER,
  OPENGL_UNMAP_GL_BUFFER,
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

      case OPENGL_MAP_GL_BUFFER: {
        JSBufferData* b;
        int32_t accessFlag = cv::ACCESS_READ | cv::ACCESS_WRITE;

        if(!(b = js_buffer_data2(ctx, argv[0])))
          return JS_EXCEPTION;

        if(argc > 1)
          accessFlag = js_value_to<int32_t>(ctx, argv[1]);

        ret = js_umat_wrap(ctx, cv::ogl::mapGLBuffer(*b, cv::AccessFlag(accessFlag)));
        break;
      }

      case OPENGL_RENDER: {
        JSTexture2DData* tx;
        JSArraysData* ar;

        if((tx = js_texture2d_data(argv[0]))) {
          JSRectData<double> wndRect(0, 0, 1, 1), texRect(0, 0, 1, 1);

          if(argc > 1)
            js_rect_read(ctx, argv[1], &wndRect);

          if(argc > 2)
            js_rect_read(ctx, argv[2], &texRect);

          cv::ogl::render(*tx, wndRect, texRect);
        } else if((ar = js_arrays_data(argv[0]))) {
          int32_t mode = cv::ogl::POINTS;
          cv::Scalar color = cv::Scalar::all(255);

          if(argc <= 1 || JS_IsNumber(argv[1])) {

            if(argc > 1)
              mode = js_value_to<int32_t>(ctx, argv[1]);

            if(argc > 2)
              js_value_to(ctx, argv[2], color);

            cv::ogl::render(*ar, mode, color);
          } else {
            JSInputArray indices = js_input_array(ctx, argv[1]);

            if(argc > 2)
              mode = js_value_to<int32_t>(ctx, argv[2]);

            if(argc > 3)
              js_value_to(ctx, argv[3], color);

            cv::ogl::render(*ar, indices, mode, color);
          }
        }

        break;
      }

      case OPENGL_UNMAP_GL_BUFFER: {
        JSUMatData* um;

        if(!(um = js_umat_data2(ctx, argv[0])))
          return JS_EXCEPTION;

        cv::ogl::unmapGLBuffer(*um);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

js_function_list_t js_opengl_ogl_funcs{
    JS_CFUNC_MAGIC_DEF("convertFromGLTexture2D", 2, js_opengl_func, OPENGL_CONVERT_FROM_GL_TEXTURE_2D),
    JS_CFUNC_MAGIC_DEF("convertToGLTexture2D", 2, js_opengl_func, OPENGL_CONVERT_TO_GL_TEXTURE_2D),
    JS_CFUNC_MAGIC_DEF("mapGLBuffer", 1, js_opengl_func, OPENGL_MAP_GL_BUFFER),
    JS_CFUNC_MAGIC_DEF("render", 1, js_opengl_func, OPENGL_RENDER),
    // XXX: TODO: setGIDevice() function
    JS_CFUNC_MAGIC_DEF("unmapGLBuffer", 1, js_opengl_func, OPENGL_UNMAP_GL_BUFFER),

    // XXX: TODO: setOpenGlDrawCallback etc.

    JS_PROP_INT32_DEF("POINTS", cv::ogl::POINTS, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("LINES", cv::ogl::LINES, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("LINE_LOOP", cv::ogl::LINE_LOOP, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("LINE_STRIP", cv::ogl::LINE_STRIP, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("TRIANGLES", cv::ogl::TRIANGLES, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("TRIANGLE_STRIP", cv::ogl::TRIANGLE_STRIP, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("TRIANGLE_FAN", cv::ogl::TRIANGLE_FAN, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("QUADS", cv::ogl::QUADS, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("QUAD_STRIP", cv::ogl::QUAD_STRIP, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("POLYGON", cv::ogl::POLYGON, JS_PROP_CONFIGURABLE),
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

  JS_NewClassID(&js_arrays_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_arrays_class_id, &js_arrays_class);

  arrays_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, arrays_proto, js_arrays_proto_funcs, countof(js_arrays_proto_funcs));
  JS_SetClassProto(ctx, js_arrays_class_id, arrays_proto);

  arrays_class = JS_NewCFunction2(ctx, js_arrays_constructor, "Arrays", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, arrays_class, arrays_proto);
  JS_SetPropertyFunctionList(ctx, arrays_class, js_arrays_static_funcs, countof(js_arrays_static_funcs));

  JS_SetPropertyStr(ctx, ogl_object, "Arrays", arrays_class);

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
