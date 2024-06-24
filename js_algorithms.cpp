 
#include "jsbindings.hpp"
#include "js_umat.hpp"
#include <opencv2/core.hpp> 
#include "algorithms/dominant_colors_grabber.hpp"
#include "algorithms/palette.hpp"
#include "algorithms/pixel_neighborhood.hpp"
#include "algorithms/skeletonization.hpp"
#include "algorithms/trace_skeleton.hpp"
#include <quickjs.h>
#include "util.hpp"
 
static JSValue
js_cv_skeletonization(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSInputOutputArray src, dst;
  cv::Mat output;
  src = js_umat_or_mat(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(js_is_noarray(src) || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  output = skeletonization(src);
  output.copyTo(dst);

  return JS_UNDEFINED;
}

static JSValue
js_cv_pixel_neighborhood(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSMatData* src;
  JSOutputArray dst;
  cv::Mat output;
  int count;

  src = js_mat_data2(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(src == nullptr || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  if(argc > 2) {
    int32_t count;

    JS_ToInt32(ctx, &count, argv[2]);
    output = magic ? pixel_neighborhood_cross_if(*src, count) : pixel_neighborhood_if(*src, count);
  } else {
    output = magic ? pixel_neighborhood_cross(*src) : pixel_neighborhood(*src);
  }

  dst.getMatRef() = output;
  //  output.copyTo();

  return JS_UNDEFINED;
}

static JSValue
js_cv_pixel_find_value(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSMatData* src;
  std::vector<JSPointData<int>> output;
  uint32_t value;

  src = js_mat_data2(ctx, argv[0]);

  if(src == nullptr)
    return JS_ThrowInternalError(ctx, "src not an array!");

  JS_ToUint32(ctx, &value, argv[1]);
  output = pixel_find_value(*src, value);

  return js_array_from(ctx, output);
}
 
static JSValue
js_cv_trace_skeleton(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSContoursData<double> contours;
  uint32_t count;
  JSInputOutputArray src = js_umat_or_mat(ctx, argv[0]);
  cv::Mat* mat;

  if(!(mat = js_mat_data2(ctx, argv[0])))
    return JS_EXCEPTION;

  /*  if(src.empty())
      return JS_ThrowInternalError(ctx, "argument 1 must be Mat or UMat");
  */

  cv::Mat *neighborhood = 0, *mapping = 0;

  if(argc > 2)
    neighborhood = js_mat_data(argv[2]);
  if(argc > 3)
    mapping = js_mat_data(argv[3]);

  count = trace_skeleton(*mat, contours, neighborhood, mapping);

  if(argc >= 2) {
    if(!js_is_array(ctx, argv[1]))
      return JS_ThrowTypeError(ctx, "argument 2 must be array");

    js_array_copy(ctx, argv[1], contours);

    return JS_NewUint32(ctx, count);
  }

  return js_array_from(ctx, contours);
}

static JSValue
js_cv_palette_generate(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {

  JSInputArray src = js_umat_or_mat(ctx, argv[0]);

  dominant_colors_grabber dcg;
  int32_t mode = 0, count = 0;
  enum color_space cs;
  enum dist_type dt;
  cv::Vec3i params[] = {{1, 1, 1}, {1, 2, 2}, {25, 0, 0}, {25, 0, 0}, {15, 0, 0}, {15, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  std::vector<cv::Scalar> palette;
  std::vector<cv::Vec3b> result;
  JSValue ret = JS_UNDEFINED;

  if(argc >= 2 && JS_IsNumber(argv[1]))
    JS_ToInt32(ctx, &mode, argv[1]);
  if(argc >= 3 && JS_IsNumber(argv[2]))
    JS_ToInt32(ctx, &count, argv[2]);

  cs = color_space(mode & 1);
  dt = dist_type((mode >> 1) & 3);

  palette = dcg.GetDomColors(src.getMat(), cs, dt, count);

  result.resize(palette.size());

  std::transform(palette.begin(), palette.end(), result.begin(), [](const cv::Scalar& entry) -> cv::Vec3i { return cv::Vec3b(entry[0], entry[1], entry[2]); });
  ret = js_array_from(ctx, result);

  return ret;
}
static JSValue
js_cv_palette_apply(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSMatData* src;
  JSOutputArray dst;
  std::array<uint32_t, 256> palette32;
  std::vector<cv::Vec3b> palette;

  src = js_mat_data2(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(src == nullptr || js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "src or dst not an array!");

  {
    cv::Mat& output = dst.getMatRef();
    int channels = output.channels();

    std::cout << "output.channels() = " << channels << std::endl;
    /*if(js_is_typedarray(ctx, argv[2])) {
      return JS_ThrowInternalError(ctx, "typed array not handled");
    } else*/

    if(channels == 1)
      channels = 3;

    if(channels == 4) {
      std::vector<JSColorData<uint8_t>> palette;
      std::vector<cv::Vec4b> palette4b;
      std::vector<cv::Scalar> palettesc;

      palette_read(ctx, argv[2], palette);

      for(auto& color : palette) {
        palette4b.push_back(cv::Vec4b(color.arr[0], color.arr[1], color.arr[2], color.arr[3]));
        palettesc.push_back(cv::Scalar(color.arr[0], color.arr[1], color.arr[2], color.arr[3]));
      }

      palette_apply<cv::Vec4b>(*src, dst, &palette4b[0]);

    } else if(channels == 3) {
      std::vector<JSColorData<uint8_t>> palette;
      std::vector<cv::Vec3b> palette3b;
      std::vector<cv::Scalar> palettesc;

      palette_read(ctx, argv[2], palette);

      for(auto& color : palette) {
        palette3b.push_back(cv::Vec3b(color.arr[2], color.arr[1], color.arr[0]));
        palettesc.push_back(cv::Scalar(color.arr[2], color.arr[1], color.arr[0]));
      }

      palette_apply<cv::Vec3b>(*src, dst, &palette3b[0]);
    } else {
      return JS_ThrowInternalError(ctx, "output mat channels = %u", output.channels());
    }
  }
  return JS_UNDEFINED;
}

static JSValue
js_cv_palette_match(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSMatData* src;
  JSOutputArray dst;
  std::vector<JSColorData<uint8_t>> palette;

  src = js_mat_data2(ctx, argv[0]);
  dst = js_umat_or_mat(ctx, argv[1]);

  if(src == nullptr)
    return JS_ThrowInternalError(ctx, "src is not an array!");

  if(js_is_noarray(dst))
    return JS_ThrowInternalError(ctx, "dst is not an array!");

  palette_read(ctx, argv[2], palette);

  palette_match(*src, dst, palette);

  return JS_UNDEFINED;
}
 

js_function_list_t js_algorithms_static_funcs{
    JS_CFUNC_DEF("skeletonization", 1, js_cv_skeletonization),
    JS_CFUNC_MAGIC_DEF("pixelNeighborhood", 2, js_cv_pixel_neighborhood, 0),
    JS_CFUNC_MAGIC_DEF("pixelNeighborhoodCross", 2, js_cv_pixel_neighborhood, 1),
    JS_CFUNC_DEF("pixelFindValue", 2, js_cv_pixel_find_value),
    JS_CFUNC_DEF("traceSkeleton", 1, js_cv_trace_skeleton),
    JS_CFUNC_DEF("paletteGenerate", 1, js_cv_palette_generate),
    JS_CFUNC_DEF("paletteApply", 2, js_cv_palette_apply),
    JS_CFUNC_DEF("paletteMatch", 3, js_cv_palette_match),
 };

extern "C" int
js_algorithms_init(JSContext* ctx, JSModuleDef* m) {
   if(m) {
    JS_SetModuleExportList(ctx, m, js_algorithms_static_funcs.data(), js_algorithms_static_funcs.size());
  } 
  return 0;
}

extern "C" void
js_algorithms_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExportList(ctx, m, js_algorithms_static_funcs.data(), js_algorithms_static_funcs.size());
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_algorithms
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  m = JS_NewCModule(ctx, module_name, &js_algorithms_init);
 
  if(!m)
    return NULL;
 
  js_algorithms_export(ctx, m);

  return m;
}
