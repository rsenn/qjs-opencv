#include "js_mat.hpp"
#include "js_cv.hpp"
#include <opencv2/alphamat.hpp>

extern "C" int js_alphamat_init(JSContext*, JSModuleDef*);

/* cv.alphamat.infoFlow(image, trimap, result): image 8-bit 3-channel,
 * trimap 8-bit 1-channel (0 background, 128 unknown, 255 foreground),
 * result is filled with the 8-bit alpha matte. OpenCV does not check
 * its input and crashes on a wrong type or size, so it is checked here. */
static JSValue
js_alphamat_info_flow(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;

  if(argc < 3)
    return JS_ThrowTypeError(ctx, "alphamat.infoFlow: expected image, trimap, result");

  try {
    cv::Mat* image = js_mat_data2(ctx, argv[0]);
    cv::Mat* trimap = js_mat_data2(ctx, argv[1]);
    cv::Mat* result = js_mat_data2(ctx, argv[2]);

    if(!image || !trimap || !result)
      return JS_EXCEPTION;

    if(image->type() != CV_8UC3)
      return JS_ThrowTypeError(ctx, "alphamat.infoFlow: image must be CV_8UC3");

    if(trimap->type() != CV_8UC1)
      return JS_ThrowTypeError(ctx, "alphamat.infoFlow: trimap must be CV_8UC1");

    if(image->size() != trimap->size())
      return JS_ThrowRangeError(ctx, "alphamat.infoFlow: image and trimap sizes differ");

    cv::alphamat::infoFlow(*image, *trimap, *result);
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

static const JSCFunctionListEntry js_alphamat_funcs[] = {
    JS_CFUNC_DEF("infoFlow", 3, js_alphamat_info_flow),
};

static const JSCFunctionListEntry js_alphamat_static_funcs[] = {
    JS_OBJECT_DEF("alphamat", js_alphamat_funcs, countof(js_alphamat_funcs), JS_PROP_C_W_E),
};

extern "C" int
js_alphamat_init(JSContext* ctx, JSModuleDef* m) {
  if(m)
    JS_SetModuleExportList(ctx, m, js_alphamat_static_funcs, countof(js_alphamat_static_funcs));

  return 0;
}

extern "C" void
js_alphamat_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_alphamat_static_funcs, countof(js_alphamat_static_funcs));
}
