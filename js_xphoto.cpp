#include "js_mat.hpp"
#include "js_cv.hpp"
#include "include/js_inputoutputarray.hpp"
#include <opencv2/xphoto.hpp>

extern "C" int js_xphoto_init(JSContext*, JSModuleDef*);

/* cv.xphoto.inpaint(src, mask, dst[, algorithm]): src 8-bit, mask 8-bit
 * 1-channel with 0 marking the pixels to fill. INPAINT_SHIFTMAP copies
 * patches and expects a Lab-like colour space, the FSR variants
 * interpolate and take 1-channel or 3-channel BGR. */
static JSValue
js_xphoto_inpaint(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSValue ret = JS_UNDEFINED;

  if(argc < 3)
    return JS_ThrowTypeError(ctx, "xphoto.inpaint: expected src, mask, dst");

  try {
    cv::Mat* src = js_mat_data2(ctx, argv[0]);
    cv::Mat* mask = js_mat_data2(ctx, argv[1]);
    cv::Mat* dst = js_mat_data2(ctx, argv[2]);
    int32_t algorithm = cv::xphoto::INPAINT_SHIFTMAP;

    if(!src || !mask || !dst)
      return JS_EXCEPTION;

    if(argc > 3 && JS_ToInt32(ctx, &algorithm, argv[3]))
      return JS_EXCEPTION;

    if(mask->type() != CV_8UC1 || mask->size() != src->size())
      return JS_ThrowTypeError(ctx, "xphoto.inpaint: mask must be CV_8UC1 and as large as src");

    if(src->depth() != CV_8U)
      return JS_ThrowTypeError(ctx, "xphoto.inpaint: src must be 8-bit");

    cv::xphoto::inpaint(*src, *mask, *dst, algorithm);
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

static const JSCFunctionListEntry js_xphoto_funcs[] = {
    JS_CFUNC_DEF("inpaint", 3, js_xphoto_inpaint),
    JS_PROP_INT32_DEF("INPAINT_SHIFTMAP", cv::xphoto::INPAINT_SHIFTMAP, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("INPAINT_FSR_BEST", cv::xphoto::INPAINT_FSR_BEST, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("INPAINT_FSR_FAST", cv::xphoto::INPAINT_FSR_FAST, JS_PROP_ENUMERABLE),
};

static const JSCFunctionListEntry js_xphoto_static_funcs[] = {
    JS_OBJECT_DEF("xphoto", js_xphoto_funcs, countof(js_xphoto_funcs), JS_PROP_C_W_E),
};

extern "C" int
js_xphoto_init(JSContext* ctx, JSModuleDef* m) {
  if(m)
    JS_SetModuleExportList(ctx, m, js_xphoto_static_funcs, countof(js_xphoto_static_funcs));

  return 0;
}

extern "C" void
js_xphoto_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_xphoto_static_funcs, countof(js_xphoto_static_funcs));
}
