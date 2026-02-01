#include "js_cv.hpp"
#include "js_umat.hpp"
#include "js_affine3.hpp"
#include "jsbindings.hpp"
#include <quickjs.h>

#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>


typedef cv::ogl::Texture2D JSTexture2DData;

extern "C" {
thread_local JSValue texture2d_proto = JS_UNDEFINED, texture2d_class = JS_UNDEFINED, texture2d_params_proto = JS_UNDEFINED;
thread_local JSClassID js_texture2d_class_id = 0;
}

JSTexture2DData*
js_texture2d_data(JSValueConst val) {
  return static_cast<JSTexture2DData*>(JS_GetOpaque(val, js_texture2d_class_id));
}

JSTexture2DData*
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

static JSValue
js_texture2d_new(JSContext* ctx, const JSTexture2DData& arg) {
  JSTexture2DData* ptr;

  if(!(ptr = js_allocate<JSTexture2DData>(ctx)))
    return JS_EXCEPTION;

  new(ptr) JSTexture2DData();

  JSValue obj = JS_NewObjectProtoClass(ctx, texture2d_proto, js_texture2d_class_id);

  JS_SetOpaque(obj, ptr);

  return obj;
}

void
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

JSClassDef js_texture2d_class = {
    .class_name = "Texture2D",
    .finalizer = js_texture2d_finalizer,
};

const JSCFunctionListEntry js_texture2d_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("bind", 0, js_texture2d_method, TEXTURE2D_BIND),
};

const JSCFunctionListEntry js_texture2d_static_funcs[] = {};


enum {
  OPENGL_CALIBRATE = 0,
  OPENGL_DISTORT_POINTS,
  OPENGL_ESTIMATE_NEW_CAMERA_MATRIX_FOR_UNDISTORT_RECTIFY,
  OPENGL_INIT_UNDISTORT_RECTIFY_MAP,
  OPENGL_PROJECT_POINTS,
  OPENGL_STEREO_CALIBRATE,
  OPENGL_STEREO_RECTIFY,
  OPENGL_UNDISTORT_IMAGE,
  OPENGL_UNDISTORT_POINTS,
};

static JSValue
js_opengl_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
    switch(magic) {
      case OPENGL_CALIBRATE: {
        JSInputArray objectPoints = js_input_array(ctx, argv[0]);
        JSInputArray imagePoints = js_input_array(ctx, argv[1]);
        JSSizeData<int> image_size;
        js_value_to(ctx, argv[2], image_size);

        JSInputOutputArray K = js_cv_inputoutputarray(ctx, argv[3]), D = js_cv_inputoutputarray(ctx, argv[4]);
        JSOutputArray rvecs = js_cv_outputarray(ctx, argv[5]), tvecs = js_cv_outputarray(ctx, argv[6]);
        int32_t flags = 0;
        cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, DBL_EPSILON);

        if(argc > 7)
          JS_ToInt32(ctx, &flags, argv[7]);

        cv::opengl::calibrate(objectPoints, imagePoints, image_size, K, D, rvecs, tvecs, flags, criteria);

        break;
      }

      case OPENGL_DISTORT_POINTS: {
        JSInputArray undistorted = js_input_array(ctx, argv[0]);
        JSOutputArray distorted = js_cv_outputarray(ctx, argv[1]);
        JSInputArray K = js_input_array(ctx, argv[2]), D = js_input_array(ctx, argv[3]);
        double alpha = 0;

        if(argc > 4)
          JS_ToFloat64(ctx, &alpha, argv[4]);

        break;
      }

      case OPENGL_ESTIMATE_NEW_CAMERA_MATRIX_FOR_UNDISTORT_RECTIFY: {
        JSInputArray K = js_input_array(ctx, argv[0]), D = js_input_array(ctx, argv[1]);
        JSSizeData<int> image_size;
        js_value_to(ctx, argv[2], image_size);
        JSInputArray R = js_input_array(ctx, argv[3]);
        JSOutputArray P = js_cv_outputarray(ctx, argv[4]);
        double balance = 0.0;

        if(argc > 5)
          JS_ToFloat64(ctx, &balance, argv[5]);

        JSSizeData<int> new_size;

        if(argc > 6)
          js_value_to(ctx, argv[6], new_size);

        double fov_scale = 1.0;

        if(argc > 7)
          JS_ToFloat64(ctx, &fov_scale, argv[7]);

        cv::opengl::estimateNewCameraMatrixForUndistortRectify(K, D, image_size, R, P, balance, new_size, fov_scale);
        break;
      }

      case OPENGL_INIT_UNDISTORT_RECTIFY_MAP: {
        JSInputArray K = js_input_array(ctx, argv[0]), D = js_input_array(ctx, argv[1]);
        JSInputArray R = js_input_array(ctx, argv[2]), P = js_input_array(ctx, argv[3]);
        JSSizeData<int> size;
        js_value_to(ctx, argv[4], size);
        int32_t m1type;
        js_value_to(ctx, argv[5], m1type);
        JSOutputArray map1 = js_cv_outputarray(ctx, argv[6]), map2 = js_cv_outputarray(ctx, argv[7]);

        cv::opengl::initUndistortRectifyMap(K, D, R, P, size, m1type, map1, map2);
        break;
      }

      case OPENGL_PROJECT_POINTS: {
        JSInputArray objectPoints = js_input_array(ctx, argv[0]);
        JSOutputArray imagePoints = js_cv_outputarray(ctx, argv[1]);
        cv::Affine3<double>* affine;

        if(argc > 2)
          affine = js_affine3_data(argv[2]);

        if(affine) {

          JSInputArray K = js_input_array(ctx, argv[3]), D = js_input_array(ctx, argv[4]);
          double alpha = 0.0;

          if(argc > 5)
            JS_ToFloat64(ctx, &alpha, argv[5]);

          JSOutputArray jacobian = cv::noArray();

          if(argc > 6)
            jacobian = js_cv_outputarray(ctx, argv[6]);

          cv::opengl::projectPoints(objectPoints, imagePoints, *affine, K, D, alpha, jacobian);
        } else {
          JSInputArray rvec = js_input_array(ctx, argv[3]), tvec = js_input_array(ctx, argv[4]);
          JSInputArray K = js_input_array(ctx, argv[5]), D = js_input_array(ctx, argv[6]);
          double alpha = 0.0;

          if(argc > 7)
            JS_ToFloat64(ctx, &alpha, argv[7]);
          JSOutputArray jacobian = cv::noArray();

          if(argc > 8)
            jacobian = js_cv_outputarray(ctx, argv[8]);

          cv::opengl::projectPoints(objectPoints, imagePoints, rvec, tvec, K, D, alpha, jacobian);
        }

        break;
      }

      case OPENGL_STEREO_CALIBRATE: {
        JSInputArray objectPoints = js_input_array(ctx, argv[0]), imagePoints1 = js_input_array(ctx, argv[1]), imagePoints2 = js_input_array(ctx, argv[2]);
        JSInputOutputArray K1 = js_cv_inputoutputarray(ctx, argv[3]), D1 = js_cv_inputoutputarray(ctx, argv[4]), K2 = js_cv_inputoutputarray(ctx, argv[5]),
                           D2 = js_cv_inputoutputarray(ctx, argv[6]);
        JSSizeData<int> image_size;
        js_value_to(ctx, argv[7], image_size);
        JSOutputArray R = js_cv_outputarray(ctx, argv[8]), T = js_cv_outputarray(ctx, argv[9]);

        if(argc <= 10 || JS_IsNumber(argv[10])) {
          int32_t flags = cv::opengl::CALIB_FIX_INTRINSIC;

          if(argc > 10)
            JS_ToInt32(ctx, &flags, argv[10]);
          cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, DBL_EPSILON);

          ret = JS_NewFloat64(ctx, cv::opengl::stereoCalibrate(objectPoints, imagePoints1, imagePoints2, K1, D1, K2, D2, image_size, R, T, flags, criteria));
        } else {
          JSOutputArray rvecs = js_cv_outputarray(ctx, argv[10]), tvecs = js_cv_outputarray(ctx, argv[11]);
          int32_t flags = cv::opengl::CALIB_FIX_INTRINSIC;

          if(argc > 12)
            JS_ToInt32(ctx, &flags, argv[12]);
          cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, DBL_EPSILON);

          ret = JS_NewFloat64(
              ctx, cv::opengl::stereoCalibrate(objectPoints, imagePoints1, imagePoints2, K1, D1, K2, D2, image_size, R, T, rvecs, tvecs, flags, criteria));
        }

        break;
      }

      case OPENGL_STEREO_RECTIFY: {
        JSInputArray K1 = js_input_array(ctx, argv[0]), D1 = js_input_array(ctx, argv[1]), K2 = js_input_array(ctx, argv[2]), D2 = js_input_array(ctx, argv[3]);
        JSSizeData<int> image_size;
        js_value_to(ctx, argv[4], image_size);

        JSInputArray R = js_input_array(ctx, argv[5]), tvec = js_input_array(ctx, argv[6]);
        JSOutputArray R1 = js_cv_outputarray(ctx, argv[7]), R2 = js_cv_outputarray(ctx, argv[8]), P1 = js_cv_outputarray(ctx, argv[9]),
                      P2 = js_cv_outputarray(ctx, argv[10]), Q = js_cv_outputarray(ctx, argv[11]);
        int32_t flags;

        if(argc > 12)
          JS_ToInt32(ctx, &flags, argv[12]);
        JSSizeData<int> new_image_size;

        if(argc > 13)
          js_value_to(ctx, argv[13], new_image_size);

        double balance = 0.0, fov_scale = 1.0;

        if(argc > 14)
          JS_ToFloat64(ctx, &balance, argv[14]);
        if(argc > 15)
          JS_ToFloat64(ctx, &fov_scale, argv[15]);

        cv::opengl::stereoRectify(K1, D1, K2, D2, image_size, R, tvec, R1, R2, P1, P2, Q, flags, new_image_size, balance, fov_scale);

        break;
      }

      case OPENGL_UNDISTORT_IMAGE: {
        JSInputArray distorted = js_input_array(ctx, argv[0]);
        JSOutputArray undistorted = js_cv_outputarray(ctx, argv[1]);
        JSInputArray K = js_input_array(ctx, argv[2]), D = js_input_array(ctx, argv[3]), Knew = cv::noArray();

        if(argc > 4)
          Knew = js_input_array(ctx, argv[4]);

        JSSizeData<int> new_size;

        if(argc > 5)
          js_value_to(ctx, argv[5], new_size);

        cv::opengl::undistortImage(distorted, undistorted, K, D, Knew, new_size);
        break;
      }

      case OPENGL_UNDISTORT_POINTS: {
        JSInputArray distorted = js_input_array(ctx, argv[0]);
        JSOutputArray undistorted = js_cv_outputarray(ctx, argv[1]);
        JSInputArray K = js_input_array(ctx, argv[2]), D = js_input_array(ctx, argv[3]), R = cv::noArray(), P = cv::noArray();

        if(argc > 4)
          R = js_input_array(ctx, argv[4]);
        if(argc > 5)
          P = js_input_array(ctx, argv[5]);

        cv::TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, DBL_EPSILON);

        cv::opengl::undistortPoints(distorted, undistorted, K, D, R, P, criteria);
        break;
      }
    }
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

js_function_list_t js_opengl_ogl_funcs{
    JS_CFUNC_MAGIC_DEF("convertFromGLTexture2D", 2, js_opengl_func, OPENGL_CONVERT_FROM_GL_TEXTURE_2D),
 };

js_function_list_t js_opengl_static_funcs{
    JS_OBJECT_DEF("ogl", js_opengl_ogl_funcs.data(), int(js_opengl_ogl_funcs.size()), JS_PROP_C_W_E),
};

extern "C" int
js_opengl_init(JSContext* ctx, JSModuleDef* m) {

JS_NewClassID(&js_texture2d_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_texture2d_class_id, &js_texture2d_class);

  texture2d_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, texture2d_proto, js_texture2d_proto_funcs, countof(js_texture2d_proto_funcs));
  JS_SetClassProto(ctx, js_texture2d_class_id, texture2d_proto);

  texture2d_class = JS_NewCFunction2(ctx, js_texture2d_constructor, "Texture2D", 0, JS_CFUNC_constructor, 0);
  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, texture2d_class, texture2d_proto);
  JS_SetPropertyFunctionList(ctx, texture2d_class, js_texture2d_static_funcs, countof(js_texture2d_static_funcs));


  if(m) {
    JS_SetModuleExportList(ctx, m, js_opengl_static_funcs.data(), js_opengl_static_funcs.size());
  }

  return 0;
}

extern "C" void
js_opengl_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_opengl_static_funcs.data(), js_opengl_static_funcs.size());
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
