#include "js_cv.hpp"
#include "js_umat.hpp"
#include "include/jsbindings.hpp"
#include <quickjs.h>

#include <opencv2/dnn.hpp>

enum {
  DNN_LAYOUT_NCHW,
  DNN_IMAGE2BLOBPARAMS,
  DNN_IMAGEPADDINGMODE,
  DNN_NET,
  DNN_NMSBOXES,
  DNN_READNET,
};

static JSValue
js_dnn_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
    switch(magic) {}
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

js_function_list_t js_dnn_dnn_funcs{
    JS_PROP_INT32_DEF("DNN_LAYOUT_NCHW", cv::dnn::DNN_LAYOUT_NCHW, JS_PROP_ENUMERABLE),
    JS_CFUNC_MAGIC_DEF("Image2BlobParams", 0, js_dnn_func, "DNN_IMAGE2BLOBPARAMS"),
    JS_CFUNC_MAGIC_DEF("ImagePaddingMode", 0, js_dnn_func, "DNN_IMAGEPADDINGMODE"),
    JS_CFUNC_MAGIC_DEF("Net", 0, js_dnn_func, "DNN_NET"),
    JS_CFUNC_MAGIC_DEF("NMSBoxes", 0, js_dnn_func, "DNN_NMSBOXES"),
    JS_CFUNC_MAGIC_DEF("readNet", 0, js_dnn_func, "DNN_READNET"),
};

js_function_list_t js_dnn_static_funcs{
    JS_OBJECT_DEF("dnn", js_dnn_dnn_funcs.data(), int(js_dnn_dnn_funcs.size()), JS_PROP_C_W_E),
};

extern "C" int
js_dnn_init(JSContext* ctx, JSModuleDef* m) {

  if(m) {
    JS_SetModuleExportList(ctx, m, js_dnn_static_funcs.data(), js_dnn_static_funcs.size());
  }

  return 0;
}

extern "C" void
js_dnn_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_dnn_static_funcs.data(), js_dnn_static_funcs.size());
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_dnn
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_dnn_init)))
    return NULL;

  js_dnn_export(ctx, m);
  return m;
}
