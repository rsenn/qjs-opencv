#include "cutils.h"
#include "js_alloc.hpp"
#include "js_array.hpp"
#include "js_keypoint.hpp"
#include "js_umat.hpp"
#include "jsbindings.hpp"
#include <opencv2/core.hpp>
#include <opencv2/core/cvstd.hpp>
#include <opencv2/core/cvstd_wrapper.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/types.hpp>
#include <quickjs.h>
#include <cstring>
#include <cstdint>
#include <new>
#include <string>
#include <vector>
#include <opencv2/features2d.hpp>

typedef cv::Ptr<cv::Feature2D> JSFeature2DData;

#ifdef HAVE_OPENCV2_XFEATURES2D_HPP
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>

using namespace cv::xfeatures2d;
using cv::SIFT;
#else
#warning No xfeatures2d
#endif
using cv::AffineFeature;
using cv::AgastFeatureDetector;
using cv::AKAZE;
using cv::BRISK;
using cv::FastFeatureDetector;
using cv::GFTTDetector;
using cv::KAZE;
using cv::MSER;
using cv::ORB;
using cv::SimpleBlobDetector;

static SimpleBlobDetector::Params simple_blob_params;

JSValue feature2d_proto = JS_UNDEFINED, feature2d_class = JS_UNDEFINED;
JSClassID js_feature2d_class_id;

extern "C" int js_feature2d_init(JSContext*, JSModuleDef*);

static JSValue
js_feature2d_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSFeature2DData feature2d, *s;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;

  if(!(s = js_allocate<JSFeature2DData>(ctx)))
    return JS_EXCEPTION;
  new(s) JSFeature2DData();

  /* using new_target to get the prototype is necessary when the
     class is extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_feature2d_class_id);
  JS_FreeValue(ctx, proto);
  if(JS_IsException(obj))
    goto fail;
  JS_SetOpaque(obj, s);
  return obj;
fail:
  js_deallocate(ctx, s);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

JSFeature2DData*
js_feature2d_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<JSFeature2DData*>(JS_GetOpaque2(ctx, val, js_feature2d_class_id));
}

JSFeature2DData*
js_feature2d_data(JSValueConst val) {
  return static_cast<JSFeature2DData*>(JS_GetOpaque(val, js_feature2d_class_id));
}

template<class T>
T*
js_feature2d_get(JSValueConst val) {
  JSFeature2DData* f2d;

  if((f2d = static_cast<JSFeature2DData*>(JS_GetOpaque(val, js_feature2d_class_id)))) {
    cv::Feature2D* ptr = f2d->get();

    return dynamic_cast<T*>(ptr);
  }
  return 0;
}

template<class T>
JSValue
js_feature2d_wrap(JSContext* ctx, const cv::Ptr<T>& f2d) {
  JSValue ret;
  cv::Ptr<T>* s;

  if(JS_IsUndefined(feature2d_proto))
    js_feature2d_init(ctx, NULL);

  ret = JS_NewObjectProtoClass(ctx, feature2d_proto, js_feature2d_class_id);

  s = js_allocate<cv::Ptr<T>>(ctx);

  *s = f2d;

  JS_SetOpaque(ret, s);
  return ret;
}

static JSValue
js_feature2d_affine(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<AffineFeature> affine;
  JSValue ret;
  cv::Ptr<cv::Feature2D> backend = *js_feature2d_data2(ctx, argv[0]);
  int32_t maxTilt = 5, minTilt = 0;
  double tiltStep = 1.4142135623730951, rotateStepBase = 72;

  if(argc >= 2)
    JS_ToInt32(ctx, &maxTilt, argv[1]);
  if(argc >= 3)
    JS_ToInt32(ctx, &minTilt, argv[2]);
  if(argc >= 4)
    JS_ToFloat64(ctx, &tiltStep, argv[3]);

  if(argc >= 5)
    JS_ToFloat64(ctx, &rotateStepBase, argv[4]);

  affine = AffineFeature::create(backend, maxTilt, minTilt, tiltStep, rotateStepBase);
  ret = js_feature2d_wrap(ctx, affine);
  js_set_tostringtag(ctx, ret, "AffineFeature");
  return ret;
}

static JSValue
js_feature2d_agast(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<AgastFeatureDetector> agast;
  JSValue ret;
  int32_t threshold = 10;
  BOOL nonmaxSuppression = TRUE;
  int32_t type = AgastFeatureDetector::OAST_9_16;

  if(argc >= 1)
    JS_ToInt32(ctx, &threshold, argv[0]);
  if(argc >= 2)
    nonmaxSuppression = JS_ToBool(ctx, argv[1]);
  if(argc >= 3)
    JS_ToInt32(ctx, &type, argv[2]);

  agast = AgastFeatureDetector::create(threshold, nonmaxSuppression, AgastFeatureDetector::DetectorType(type));

  ret = js_feature2d_wrap(ctx, agast);
  js_set_tostringtag(ctx, ret, "AgastFeatureDetector");
  return ret;
}

static JSValue
js_feature2d_akaze(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<AKAZE> akaze;
  JSValue ret;
  int32_t descriptor_type = AKAZE::DESCRIPTOR_MLDB, descriptor_size = 0, descriptor_channels = 3;
  double threshold = 0.001;
  int32_t nOctaves = 4, nOctaveLayers = 4;
  int32_t diffusivity = KAZE::DIFF_PM_G2;

  if(argc >= 1)
    JS_ToInt32(ctx, &descriptor_type, argv[0]);
  if(argc >= 2)
    JS_ToInt32(ctx, &descriptor_size, argv[1]);
  if(argc >= 3)
    JS_ToInt32(ctx, &descriptor_channels, argv[2]);
  if(argc >= 4)
    JS_ToFloat64(ctx, &threshold, argv[3]);
  if(argc >= 5)
    JS_ToInt32(ctx, &nOctaves, argv[4]);
  if(argc >= 6)
    JS_ToInt32(ctx, &nOctaveLayers, argv[5]);
  if(argc >= 7)
    JS_ToInt32(ctx, &diffusivity, argv[6]);

  akaze = AKAZE::create(
      AKAZE::DescriptorType(descriptor_type), descriptor_size, descriptor_channels, threshold, nOctaves, nOctaveLayers, KAZE::DiffusivityType(diffusivity));

  ret = js_feature2d_wrap(ctx, akaze);
  js_set_tostringtag(ctx, ret, "AKAZE");
  return ret;
}

static JSValue
js_feature2d_brisk(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<BRISK> brisk;
  JSValue ret;
  int32_t thresh = 30, octaves = 3;
  std::vector<float> radiusList;
  std::vector<int32_t> numberList, indexChange;
  double dMax = 5.85, dMin = 8.2;

  if(argc == 3 && JS_IsNumber(argv[2])) {
    double patternScale = 1.0f;

    if(argc >= 1)
      JS_ToInt32(ctx, &thresh, argv[0]);
    if(argc >= 2)
      JS_ToInt32(ctx, &octaves, argv[1]);
    if(argc >= 3)
      JS_ToFloat64(ctx, &patternScale, argv[2]);
    brisk = BRISK::create(thresh, octaves, patternScale);

  } else if(argc >= 2 && JS_IsArray(ctx, argv[0])) {
    js_array_to(ctx, argv[0], radiusList);
    js_array_to(ctx, argv[1], numberList);

    if(argc >= 3)
      JS_ToFloat64(ctx, &dMax, argv[2]);
    if(argc >= 4)
      JS_ToFloat64(ctx, &dMin, argv[3]);
    if(argc >= 5)
      js_array_to(ctx, argv[4], indexChange);

    brisk = BRISK::create(radiusList, numberList, dMax, dMin, indexChange);
  } else if(argc >= 4) {
    JS_ToInt32(ctx, &thresh, argv[0]);

    JS_ToInt32(ctx, &octaves, argv[1]);

    js_array_to(ctx, argv[2], radiusList);
    js_array_to(ctx, argv[3], numberList);

    if(argc >= 5)
      JS_ToFloat64(ctx, &dMax, argv[4]);
    if(argc >= 6)
      JS_ToFloat64(ctx, &dMin, argv[5]);
    if(argc >= 7)
      js_array_to(ctx, argv[6], indexChange);
    brisk = BRISK::create(thresh, octaves, radiusList, numberList, dMax, dMin, indexChange);
  }

  if(!brisk)
    return JS_ThrowInternalError(ctx, "BRISK::create([thresh, octaves,] radiusList, numberList, dMax, dMin, indexChange)");

  ret = js_feature2d_wrap(ctx, brisk);
  js_set_tostringtag(ctx, ret, "BRISK");
  return ret;
}

static JSValue
js_feature2d_fast(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<FastFeatureDetector> fast;
  JSValue ret;
  int32_t threshold = 10;
  BOOL nonmaxSuppression = TRUE;
  int32_t type = FastFeatureDetector::TYPE_9_16;

  if(argc >= 1)
    JS_ToInt32(ctx, &threshold, argv[0]);
  if(argc >= 2)
    nonmaxSuppression = JS_ToBool(ctx, argv[1]);
  if(argc >= 3)
    JS_ToInt32(ctx, &type, argv[2]);

  fast = FastFeatureDetector::create(threshold, nonmaxSuppression, FastFeatureDetector::DetectorType(type));

  ret = js_feature2d_wrap(ctx, fast);
  js_set_tostringtag(ctx, ret, "FastFeatureDetector");
  return ret;
}

static JSValue
js_feature2d_gftt(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<GFTTDetector> gftt;
  JSValue ret;

  int32_t maxCorners = 1000;
  double qualityLevel = 0.01, minDistance = 1;
  int32_t blockSize = 3;
  BOOL useHarrisDetector = FALSE;
  double k = 0.04;
  if(argc >= 1)
    JS_ToInt32(ctx, &maxCorners, argv[0]);
  if(argc >= 2)
    JS_ToFloat64(ctx, &qualityLevel, argv[1]);
  if(argc >= 3)
    JS_ToFloat64(ctx, &minDistance, argv[2]);

  if(argc >= 4)
    JS_ToInt32(ctx, &blockSize, argv[3]);
  if(argc >= 5)
    useHarrisDetector = JS_ToBool(ctx, argv[4]);

  if(argc >= 6)
    JS_ToFloat64(ctx, &k, argv[5]);

  gftt = GFTTDetector::create(maxCorners, qualityLevel, minDistance, blockSize, bool(useHarrisDetector), k);

  ret = js_feature2d_wrap(ctx, gftt);
  js_set_tostringtag(ctx, ret, "GFTTDetector");
  return ret;
}

static JSValue
js_feature2d_kaze(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<KAZE> kaze;
  JSValue ret;
  BOOL extended = FALSE, upright = FALSE;
  double threshold = 0.001;
  int32_t nOctaves = 4, nOctaveLayers = 4, diffusivity = KAZE::DIFF_PM_G2;

  if(argc >= 1)
    extended = JS_ToBool(ctx, argv[0]);
  if(argc >= 2)
    upright = JS_ToBool(ctx, argv[1]);
  if(argc >= 3)
    JS_ToFloat64(ctx, &threshold, argv[2]);
  if(argc >= 4)
    JS_ToInt32(ctx, &nOctaves, argv[3]);
  if(argc >= 5)
    JS_ToInt32(ctx, &nOctaveLayers, argv[4]);
  if(argc >= 6)
    JS_ToInt32(ctx, &diffusivity, argv[5]);

  kaze = KAZE::create(bool(extended), bool(upright), threshold, nOctaves, nOctaveLayers, KAZE::DiffusivityType(diffusivity));

  ret = js_feature2d_wrap(ctx, kaze);
  js_set_tostringtag(ctx, ret, "KAZE");
  return ret;
}

static JSValue
js_feature2d_mser(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<MSER> mser;
  JSValue ret;
  int32_t delta = 5, minArea = 60, maxArea = 14400;
  double maxVariation = 0.25, minDiversity = .2;
  int32_t maxEvolution = 200;
  double areaThreshold = 1.01, minMargin = 0.003;
  int32_t edgeBlurSize = 5;

  if(argc >= 1)
    JS_ToInt32(ctx, &delta, argv[0]);
  if(argc >= 2)
    JS_ToInt32(ctx, &minArea, argv[1]);
  if(argc >= 3)
    JS_ToInt32(ctx, &maxArea, argv[2]);
  if(argc >= 4)
    JS_ToFloat64(ctx, &maxVariation, argv[3]);
  if(argc >= 5)
    JS_ToFloat64(ctx, &minDiversity, argv[4]);
  if(argc >= 6)
    JS_ToInt32(ctx, &maxEvolution, argv[5]);
  if(argc >= 7)
    JS_ToFloat64(ctx, &areaThreshold, argv[6]);
  if(argc >= 8)
    JS_ToFloat64(ctx, &minMargin, argv[7]);
  if(argc >= 9)
    JS_ToInt32(ctx, &edgeBlurSize, argv[8]);

  mser = MSER::create(delta, minArea, maxArea, maxVariation, minDiversity, maxEvolution, areaThreshold, minMargin, edgeBlurSize);
  ret = js_feature2d_wrap(ctx, mser);
  js_set_tostringtag(ctx, ret, "MSER");
  return ret;
}

static JSValue
js_feature2d_orb(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<ORB> orb;
  JSValue ret;
  int32_t nfeatures = 500;
  double scaleFactor = 1.2f;
  int32_t nlevels = 8, edgeThreshold = 31, firstLevel = 0, WTA_K = 2;
  /*ORB::ScoreType*/ int32_t scoreType = ORB::HARRIS_SCORE;
  int32_t patchSize = 31, fastThreshold = 20;

  if(argc >= 1)
    JS_ToInt32(ctx, &nfeatures, argv[0]);
  if(argc >= 2)
    JS_ToFloat64(ctx, &scaleFactor, argv[1]);
  if(argc >= 3)
    JS_ToInt32(ctx, &nlevels, argv[2]);
  if(argc >= 4)
    JS_ToInt32(ctx, &edgeThreshold, argv[3]);
  if(argc >= 5)
    JS_ToInt32(ctx, &firstLevel, argv[4]);
  if(argc >= 6)
    JS_ToInt32(ctx, &WTA_K, argv[5]);
  if(argc >= 7)
    JS_ToInt32(ctx, &scoreType, argv[6]);
  if(argc >= 8)
    JS_ToInt32(ctx, &patchSize, argv[7]);
  if(argc >= 9)
    JS_ToInt32(ctx, &fastThreshold, argv[8]);

  orb = ORB::create(nfeatures, scaleFactor, nlevels, edgeThreshold, firstLevel, WTA_K, ORB::ScoreType(scoreType), patchSize, fastThreshold);
  ret = js_feature2d_wrap(ctx, orb);
  js_set_tostringtag(ctx, ret, "ORB");
  return ret;
}

static JSValue
js_feature2d_simple_blob(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<SimpleBlobDetector> simple_blob;
  JSValue ret;
  simple_blob = SimpleBlobDetector::create();
  ret = js_feature2d_wrap(ctx, simple_blob);
  js_set_tostringtag(ctx, ret, "SimpleBlobDetector");
  return ret;
}

#ifdef HAVE_OPENCV2_XFEATURES2D_HPP
static JSValue
js_feature2d_sift(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<SIFT> sift;
  JSValue ret;
  int32_t nfeatures = 0, nOctaveLayers = 3;
  double contrastThreshold = 0.04, edgeThreshold = 10, sigma = 1.6;

  if(argc >= 1)
    JS_ToInt32(ctx, &nfeatures, argv[0]);
  if(argc >= 2)
    JS_ToInt32(ctx, &nOctaveLayers, argv[1]);
  if(argc >= 3)
    JS_ToFloat64(ctx, &contrastThreshold, argv[2]);
  if(argc >= 4)
    JS_ToFloat64(ctx, &edgeThreshold, argv[3]);
  if(argc >= 5)
    JS_ToFloat64(ctx, &sigma, argv[4]);

  if(argc >= 6) {
    int32_t descriptorType;
    if(argc >= 6)
      JS_ToInt32(ctx, &descriptorType, argv[5]);
    sift = SIFT::create(nfeatures, nOctaveLayers, contrastThreshold, edgeThreshold, sigma, descriptorType);

  } else {
    sift = SIFT::create(nfeatures, nOctaveLayers, contrastThreshold, edgeThreshold, sigma);
  }

  ret = js_feature2d_wrap(ctx, sift);
  js_set_tostringtag(ctx, ret, "SIFT");
  return ret;
}

static JSValue
js_feature2d_affine2d(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<AffineFeature2D> affine2d;
  JSValue ret;
  cv::Ptr<cv::Feature2D> keypointDetector = *js_feature2d_data2(ctx, argv[0]);

  if(argc > 1) {
    cv::Ptr<cv::Feature2D> descriptorExtractor = *js_feature2d_data2(ctx, argv[1]);

    affine2d = AffineFeature2D::create(keypointDetector, descriptorExtractor);
  } else {
    affine2d = AffineFeature2D::create(keypointDetector);
  }
  ret = js_feature2d_wrap(ctx, affine2d);
  js_set_tostringtag(ctx, ret, "AffineFeature2D");
  return ret;
}

static JSValue
js_feature2d_boost(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<BoostDesc> boost;
  JSValue ret;
  int32_t desc = BoostDesc::BINBOOST_256;
  BOOL use_scale_orientation = TRUE;
  double scale_factor = 6.25;

  if(argc >= 1)
    JS_ToInt32(ctx, &desc, argv[0]);
  if(argc >= 2)
    use_scale_orientation = JS_ToBool(ctx, argv[1]);
  if(argc >= 3)
    JS_ToFloat64(ctx, &scale_factor, argv[2]);

  boost = BoostDesc::create(desc, use_scale_orientation, scale_factor);
  ret = js_feature2d_wrap(ctx, boost);
  js_set_tostringtag(ctx, ret, "BoostDesc");
  return ret;
}
#endif

static JSValue
js_feature2d_brief(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<BriefDescriptorExtractor> brief;
  JSValue ret;
  int32_t bytes = 32;
  BOOL use_orientation = FALSE;

  if(argc >= 1)
    JS_ToInt32(ctx, &bytes, argv[0]);
  if(argc >= 2)
    use_orientation = JS_ToBool(ctx, argv[1]);

  brief = BriefDescriptorExtractor::create(bytes, use_orientation);
  ret = js_feature2d_wrap(ctx, brief);
  js_set_tostringtag(ctx, ret, "BriefDescriptorExtractor");
  return ret;
}

static JSValue
js_feature2d_daisy(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<DAISY> daisy;
  JSValue ret;
  double radius = 15;
  int32_t q_radius = 3, q_theta = 8, q_hist = 8, norm = DAISY::NRM_NONE;
  JSInputArray H = cv::noArray();
  BOOL interpolation = TRUE;
  BOOL use_orientation = FALSE;

  if(argc >= 1)
    JS_ToFloat64(ctx, &radius, argv[0]);
  if(argc >= 2)
    JS_ToInt32(ctx, &q_radius, argv[1]);
  if(argc >= 3)
    JS_ToInt32(ctx, &q_theta, argv[2]);
  if(argc >= 4)
    JS_ToInt32(ctx, &q_hist, argv[3]);
  if(argc >= 5)
    JS_ToInt32(ctx, &norm, argv[4]);
  if(argc >= 6)
    H = js_input_array(ctx, argv[5]);
  if(argc >= 7)
    interpolation = JS_ToBool(ctx, argv[6]);
  if(argc >= 8)
    use_orientation = JS_ToBool(ctx, argv[7]);

  daisy = DAISY::create(radius, q_radius, q_theta, q_hist, DAISY::NormalizationType(norm), H, interpolation, use_orientation);
  ret = js_feature2d_wrap(ctx, daisy);
  js_set_tostringtag(ctx, ret, "DAISY");
  return ret;
}

static JSValue
js_feature2d_freak(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<FREAK> freak;
  JSValue ret;
  BOOL orientationNormalized = TRUE, scaleNormalized = TRUE;
  double patternScale = 22.0;
  int32_t nOctaves = 4;
  std::vector<int> selectedPairs;

  if(argc >= 1)
    orientationNormalized = JS_ToBool(ctx, argv[0]);
  if(argc >= 2)
    scaleNormalized = JS_ToBool(ctx, argv[1]);
  if(argc >= 3)
    JS_ToFloat64(ctx, &patternScale, argv[2]);
  if(argc >= 4)
    JS_ToInt32(ctx, &nOctaves, argv[3]);
  if(argc >= 5)
    js_array_to(ctx, argv[4], selectedPairs);

  freak = FREAK::create(orientationNormalized, scaleNormalized, patternScale, nOctaves, selectedPairs);

  ret = js_feature2d_wrap(ctx, freak);
  js_set_tostringtag(ctx, ret, "FREAK");
  return ret;
}

static JSValue
js_feature2d_harris_laplace(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<HarrisLaplaceFeatureDetector> harris_laplace;
  JSValue ret;
  int32_t numOctaves = 6;
  double corn_thresh = 0.01, DOG_thresh = 0.01;
  int32_t maxCorners = 5000, num_layers = 4;

  if(argc >= 1)
    JS_ToInt32(ctx, &numOctaves, argv[0]);
  if(argc >= 2)
    JS_ToFloat64(ctx, &corn_thresh, argv[1]);
  if(argc >= 3)
    JS_ToFloat64(ctx, &DOG_thresh, argv[2]);
  if(argc >= 4)
    JS_ToInt32(ctx, &maxCorners, argv[3]);
  if(argc >= 5)
    JS_ToInt32(ctx, &num_layers, argv[4]);

  harris_laplace = HarrisLaplaceFeatureDetector::create(numOctaves, corn_thresh, DOG_thresh, maxCorners, num_layers);
  ret = js_feature2d_wrap(ctx, harris_laplace);
  js_set_tostringtag(ctx, ret, "HarrisLaplaceFeatureDetector");
  return ret;
}

static JSValue
js_feature2d_latch(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<LATCH> latch;
  JSValue ret;
  int32_t bytes = 32;
  BOOL rotationInvariance = TRUE;
  int32_t half_ssd_size = 3;
  double sigma = 2.0;

  if(argc >= 1)
    JS_ToInt32(ctx, &bytes, argv[0]);
  if(argc >= 2)
    rotationInvariance = JS_ToBool(ctx, argv[1]);
  if(argc >= 3)
    JS_ToInt32(ctx, &half_ssd_size, argv[2]);
  if(argc >= 4)
    JS_ToFloat64(ctx, &sigma, argv[3]);

  latch = LATCH::create(bytes, rotationInvariance, half_ssd_size, sigma);
  ret = js_feature2d_wrap(ctx, latch);
  js_set_tostringtag(ctx, ret, "LATCH");
  return ret;
}

static JSValue
js_feature2d_lucid(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<LUCID> lucid;
  JSValue ret;
  int32_t lucid_kernel = 1;
  int32_t blur_kernel = 2;

  if(argc >= 1)
    JS_ToInt32(ctx, &lucid_kernel, argv[0]);
  if(argc >= 2)
    JS_ToInt32(ctx, &blur_kernel, argv[1]);

  lucid = LUCID::create(lucid_kernel, blur_kernel);

  ret = js_feature2d_wrap(ctx, lucid);
  js_set_tostringtag(ctx, ret, "LUCID");
  return ret;
}

static JSValue
js_feature2d_msd(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<MSDDetector> msd;
  JSValue ret;
  int32_t m_patch_radius = 3, m_search_area_radius = 5, m_nms_radius = 5, m_nms_scale_radius = 0;
  double m_th_saliency = 250.0f;
  int32_t m_kNN = 4;
  double m_scale_factor = 1.25f;
  int32_t m_n_scales = -1;
  BOOL m_compute_orientation = FALSE;

  if(argc >= 1)
    JS_ToInt32(ctx, &m_patch_radius, argv[0]);
  if(argc >= 2)
    JS_ToInt32(ctx, &m_search_area_radius, argv[1]);
  if(argc >= 3)
    JS_ToInt32(ctx, &m_nms_radius, argv[2]);
  if(argc >= 4)
    JS_ToInt32(ctx, &m_nms_scale_radius, argv[3]);
  if(argc >= 5)
    JS_ToFloat64(ctx, &m_th_saliency, argv[4]);
  if(argc >= 6)
    JS_ToInt32(ctx, &m_kNN, argv[5]);
  if(argc >= 7)
    JS_ToFloat64(ctx, &m_scale_factor, argv[6]);
  if(argc >= 8)
    JS_ToInt32(ctx, &m_n_scales, argv[7]);
  if(argc >= 9)
    m_compute_orientation = JS_ToBool(ctx, argv[8]);

  msd = MSDDetector::create(
      m_patch_radius, m_search_area_radius, m_nms_radius, m_nms_scale_radius, m_th_saliency, m_kNN, m_scale_factor, m_n_scales, m_compute_orientation);
  ret = js_feature2d_wrap(ctx, msd);
  js_set_tostringtag(ctx, ret, "MSDDetector");
  return ret;
}

static JSValue
js_feature2d_star(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<StarDetector> star;
  JSValue ret;
  int32_t maxSize = 45, responseThreshold = 30, lineThresholdProjected = 10, lineThresholdBinarized = 8, suppressNonmaxSize = 5;

  if(argc >= 1)
    JS_ToInt32(ctx, &maxSize, argv[0]);
  if(argc >= 2)
    JS_ToInt32(ctx, &responseThreshold, argv[1]);
  if(argc >= 3)
    JS_ToInt32(ctx, &lineThresholdProjected, argv[2]);
  if(argc >= 4)
    JS_ToInt32(ctx, &lineThresholdBinarized, argv[3]);
  if(argc >= 5)
    JS_ToInt32(ctx, &suppressNonmaxSize, argv[4]);

  star = StarDetector::create(maxSize, responseThreshold, lineThresholdProjected, lineThresholdBinarized, suppressNonmaxSize);
  ret = js_feature2d_wrap(ctx, star);
  js_set_tostringtag(ctx, ret, "StarDetector");
  return ret;
}

static JSValue
js_feature2d_surf(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<SURF> surf;
  JSValue ret;
  double hessianThreshold = 100;
  int32_t nOctaves = 4, nOctaveLayers = 3;
  BOOL extended = FALSE, upright = FALSE;

  if(argc >= 1)
    JS_ToFloat64(ctx, &hessianThreshold, argv[0]);
  if(argc >= 2)
    JS_ToInt32(ctx, &nOctaves, argv[1]);
  if(argc >= 3)
    JS_ToInt32(ctx, &nOctaveLayers, argv[2]);
  if(argc >= 4)
    extended = JS_ToBool(ctx, argv[3]);
  if(argc >= 5)
    upright = JS_ToBool(ctx, argv[4]);

  surf = SURF::create(hessianThreshold, nOctaves, nOctaveLayers, extended, upright);

  ret = js_feature2d_wrap(ctx, surf);
  js_set_tostringtag(ctx, ret, "SURF");
  return ret;
}

static JSValue
js_feature2d_vgg(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::Ptr<VGG> vgg;
  JSValue ret;
  int32_t desc = VGG::VGG_120;
  double isigma = 1.4, scale_factor = 6.25;
  BOOL img_normalize = TRUE, use_scale_orientation = TRUE, dsc_normalize = FALSE;

  if(argc >= 1)
    JS_ToInt32(ctx, &desc, argv[0]);
  if(argc >= 2)
    JS_ToFloat64(ctx, &isigma, argv[1]);
  if(argc >= 3)
    img_normalize = JS_ToBool(ctx, argv[2]);
  if(argc >= 4)
    use_scale_orientation = JS_ToBool(ctx, argv[3]);
  if(argc >= 5)
    JS_ToFloat64(ctx, &scale_factor, argv[4]);
  if(argc >= 6)
    dsc_normalize = JS_ToBool(ctx, argv[5]);

  vgg = VGG::create(desc, isigma, img_normalize, use_scale_orientation, scale_factor, dsc_normalize);
  ret = js_feature2d_wrap(ctx, vgg);
  js_set_tostringtag(ctx, ret, "VGG");
  return ret;
}

void
js_feature2d_finalizer(JSRuntime* rt, JSValue val) {
  JSFeature2DData* s = static_cast<JSFeature2DData*>(JS_GetOpaque(val, js_feature2d_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  s->~JSFeature2DData();
  js_deallocate(rt, s);
}

static JSValue
js_feature2d_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSFeature2DData* s = js_feature2d_data2(ctx, this_val);
  JSValue obj = JS_NewObject(ctx);
  cv::Feature2D* f2d = s->get();
  FREAK* freak = js_feature2d_get<FREAK>(this_val);
  KAZE* kaze = js_feature2d_get<KAZE>(this_val);

  JS_DefinePropertyValueStr(ctx, obj, "empty", JS_NewBool(ctx, (*s)->empty()), JS_PROP_ENUMERABLE);
  if(freak)
    JS_DefinePropertyValueStr(ctx, obj, "defaultName", js_get_tostringtag(ctx, this_val), JS_PROP_ENUMERABLE);
  else // if(kaze)
    JS_DefinePropertyValueStr(ctx, obj, "defaultName", JS_NewString(ctx, f2d->getDefaultName().c_str()), JS_PROP_ENUMERABLE);
  js_set_tostringtag(ctx, obj, js_get_tostringtag(ctx, this_val));
  return obj;
}

enum { METHOD_CLEAR = 0, METHOD_COMPUTE, METHOD_DETECT, METHOD_WRITE, METHOD_READ };

static JSValue
js_feature2d_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSFeature2DData* s = static_cast<JSFeature2DData*>(JS_GetOpaque2(ctx, this_val, js_feature2d_class_id));
  JSValue ret = JS_UNDEFINED;
  cv::Feature2D* ptr = s->get();
  try {

    switch(magic) {
      case METHOD_CLEAR: {
        (*s)->clear();
        break;
      }

      case METHOD_COMPUTE: {
        JSInputOutputArray image = js_umat_or_mat(ctx, argv[0]);
        JSOutputArray descriptors = js_cv_outputarray(ctx, argv[2]);
        std::vector<JSKeyPointData> keypoints;

        ptr->compute(image, keypoints, descriptors);

        js_array_copy(ctx, argv[1], keypoints.data(), keypoints.data() + keypoints.size());
        break;
      }

      case METHOD_DETECT: {
        JSInputOutputArray image = js_umat_or_mat(ctx, argv[0]);
        JSInputArray mask = cv::noArray();
        std::vector<JSKeyPointData> keypoints;

        if(argc >= 3)
          mask = js_input_array(ctx, argv[2]);

        ptr->detect(image, keypoints, mask);

        js_array_copy(ctx, argv[1], keypoints.data(), keypoints.data() + keypoints.size());
        break;
      }

      case METHOD_WRITE: {
        std::string str;
        js_value_to(ctx, argv[0], str);
        ptr->write(str);
      }

      case METHOD_READ: {
        std::string str;
        js_value_to(ctx, argv[0], str);
        ptr->read(str);
      }
    }
  } catch(const cv::Exception& e) {
    const char *msg, *what = e.what();
    if((msg = strstr(what, ") ")))
      what = msg + 2;
    ret = JS_ThrowInternalError(ctx, "cv::Exception %s", what);
  }

  return ret;
}

static JSValue
js_feature2d_hasinstance(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  return static_cast<JSFeature2DData*>(JS_GetOpaque2(ctx, argv[0], js_feature2d_class_id)) ? JS_TRUE : JS_FALSE;
}

enum {
  PROP_EMPTY = 0,
  PROP_DEFAULT_NAME,
  PROP_DEFAULT_NORM,
  PROP_DESCRIPTOR_SIZE,
  PROP_DESCRIPTOR_TYPE,
};

static JSValue
js_feature2d_getter(JSContext* ctx, JSValueConst this_val, int magic) {
  JSFeature2DData* s;
  JSValue ret = JS_UNDEFINED;

  if(!(s = js_feature2d_data(this_val)))
    return ret;
  try {

    switch(magic) {
      case PROP_DEFAULT_NAME: {
        std::string name = (*s)->getDefaultName();

        ret = JS_NewStringLen(ctx, name.data(), name.size());
        break;
      }

      case PROP_DEFAULT_NORM: {
        ret = JS_NewInt32(ctx, (*s)->defaultNorm());
        break;
      }

      case PROP_DESCRIPTOR_SIZE: {
        ret = JS_NewInt32(ctx, (*s)->descriptorSize());
        break;
      }

      case PROP_DESCRIPTOR_TYPE: {
        ret = JS_NewInt32(ctx, (*s)->descriptorType());
        break;
      }
    }
  } catch(const cv::Exception& e) {
    const char *msg, *what = e.what();
    if((msg = strstr(what, ") ")))
      what = msg + 2;
    ret = JS_ThrowInternalError(ctx, "cv::Exception %s", what);
  }

  return ret;
}

JSClassDef js_feature2d_class = {
    .class_name = "Feature2D",
    .finalizer = js_feature2d_finalizer,
};

const JSCFunctionListEntry js_feature2d_agast_static_funcs[] = {
    JS_PROP_INT32_DEF("AGAST_5_8", AgastFeatureDetector::AGAST_5_8, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("AGAST_7_12d", AgastFeatureDetector::AGAST_7_12d, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("AGAST_7_12s", AgastFeatureDetector::AGAST_7_12s, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("OAST_9_16", AgastFeatureDetector::OAST_9_16, JS_PROP_ENUMERABLE),
};

const JSCFunctionListEntry js_feature2d_akaze_static_funcs[] = {
    JS_PROP_INT32_DEF("DESCRIPTOR_KAZE_UPRIGHT", AKAZE::DESCRIPTOR_KAZE_UPRIGHT, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DESCRIPTOR_KAZE", AKAZE::DESCRIPTOR_KAZE, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DESCRIPTOR_MLDB_UPRIGHT", AKAZE::DESCRIPTOR_MLDB_UPRIGHT, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DESCRIPTOR_MLDB", AKAZE::DESCRIPTOR_MLDB, JS_PROP_ENUMERABLE),
};
const JSCFunctionListEntry js_feature2d_orb_static_funcs[] = {
    JS_PROP_INT32_DEF("HARRIS_SCORE", ORB::HARRIS_SCORE, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("FAST_SCORE", ORB::FAST_SCORE, JS_PROP_ENUMERABLE),
};
const JSCFunctionListEntry js_feature2d_boost_static_funcs[] = {
    JS_PROP_INT32_DEF("BGM", BoostDesc::BGM, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("BGM_HARD", BoostDesc::BGM_HARD, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("BGM_BILINEAR", BoostDesc::BGM_BILINEAR, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("LBGM", BoostDesc::LBGM, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("BINBOOST_64", BoostDesc::BINBOOST_64, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("BINBOOST_128", BoostDesc::BINBOOST_128, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("BINBOOST_256", BoostDesc::BINBOOST_256, JS_PROP_ENUMERABLE),
};
const JSCFunctionListEntry js_feature2d_daisy_static_funcs[] = {
    JS_PROP_INT32_DEF("NRM_NONE", DAISY::NRM_NONE, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("NRM_PARTIAL", DAISY::NRM_PARTIAL, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("NRM_FULL", DAISY::NRM_FULL, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("NRM_SIFT", DAISY::NRM_SIFT, JS_PROP_ENUMERABLE),
};
const JSCFunctionListEntry js_feature2d_fast_static_funcs[] = {
    JS_PROP_INT32_DEF("TYPE_5_8", cv::FastFeatureDetector::TYPE_5_8, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("TYPE_7_12", cv::FastFeatureDetector::TYPE_7_12, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("TYPE_9_16", cv::FastFeatureDetector::TYPE_9_16, JS_PROP_ENUMERABLE),
};

const JSCFunctionListEntry js_feature2d_kaze_static_funcs[] = {
    JS_PROP_INT32_DEF("DIFF_PM_G1 ", KAZE::DIFF_PM_G1, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DIFF_PM_G2 ", KAZE::DIFF_PM_G2, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DIFF_WEICKERT ", KAZE::DIFF_WEICKERT, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("DIFF_CHARBONNIER ", KAZE::DIFF_CHARBONNIER, JS_PROP_ENUMERABLE),
};
const JSCFunctionListEntry js_feature2d_vgg_static_funcs[] = {
    JS_PROP_INT32_DEF("VGG_120", VGG::VGG_120, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("VGG_80", VGG::VGG_80, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("VGG_64", VGG::VGG_64, JS_PROP_ENUMERABLE),
    JS_PROP_INT32_DEF("VGG_48", VGG::VGG_48, JS_PROP_ENUMERABLE),
};

const JSCFunctionListEntry js_feature2d_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("clear", 0, js_feature2d_method, METHOD_CLEAR),
    JS_CFUNC_MAGIC_DEF("compute", 2, js_feature2d_method, METHOD_COMPUTE),
    JS_CFUNC_MAGIC_DEF("detect", 2, js_feature2d_method, METHOD_DETECT),
    JS_CFUNC_MAGIC_DEF("write", 1, js_feature2d_method, METHOD_WRITE),
    JS_CFUNC_MAGIC_DEF("read", 1, js_feature2d_method, METHOD_READ),
    JS_CGETSET_MAGIC_DEF("empty", js_feature2d_getter, 0, PROP_EMPTY),
    JS_CGETSET_MAGIC_DEF("defaultName", js_feature2d_getter, 0, PROP_DEFAULT_NAME),
    JS_CGETSET_MAGIC_DEF("defaultNorm", js_feature2d_getter, 0, PROP_DEFAULT_NORM),
    JS_CGETSET_MAGIC_DEF("descriptorSize", js_feature2d_getter, 0, PROP_DESCRIPTOR_SIZE),
    JS_CGETSET_MAGIC_DEF("descriptorType", js_feature2d_getter, 0, PROP_DESCRIPTOR_TYPE),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Feature2D", JS_PROP_CONFIGURABLE),
};

const JSCFunctionListEntry js_feature2d_static_funcs[] = {
    JS_CFUNC_DEF("[Symbol.hasInstance]", 1, js_feature2d_hasinstance),
};

static JSConstructor js_feature2d_classes[] = {
    JSConstructor(js_feature2d_affine, "AffineFeature"),
    JSConstructor(js_feature2d_agast, "AgastFeatureDetector", js_feature2d_agast_static_funcs),
    JSConstructor(js_feature2d_akaze, "AKAZE", js_feature2d_akaze_static_funcs),
    JSConstructor(js_feature2d_brisk, "BRISK"),
    JSConstructor(js_feature2d_fast, "FastFeatureDetector", js_feature2d_fast_static_funcs),
    JSConstructor(js_feature2d_gftt, "GFTTDetector"),
    JSConstructor(js_feature2d_kaze, "KAZE", js_feature2d_kaze_static_funcs),
    JSConstructor(js_feature2d_mser, "MSER"),
    JSConstructor(js_feature2d_orb, "ORB", js_feature2d_orb_static_funcs),
    JSConstructor(js_feature2d_sift, "SIFT"),
    JSConstructor(js_feature2d_simple_blob, "SimpleBlobDetector"),
    JSConstructor(js_feature2d_affine, "AffineFeature2D"),
    JSConstructor(js_feature2d_boost, "BoostDesc", js_feature2d_boost_static_funcs),
    JSConstructor(js_feature2d_brief, "BriefDescriptorExtractor"),
    JSConstructor(js_feature2d_daisy, "DAISY", js_feature2d_daisy_static_funcs),
    JSConstructor(js_feature2d_freak, "FREAK"),
    JSConstructor(js_feature2d_harris_laplace, "HarrisLaplaceFeatureDetector"),
    JSConstructor(js_feature2d_latch, "LATCH"),
    JSConstructor(js_feature2d_lucid, "LUCID"),
    JSConstructor(js_feature2d_msd, "MSDDetector"),
    JSConstructor(js_feature2d_star, "StarDetector"),
    JSConstructor(js_feature2d_surf, "SURF"),
    JSConstructor(js_feature2d_vgg, "VGG", js_feature2d_vgg_static_funcs),
};

extern "C" int
js_feature2d_init(JSContext* ctx, JSModuleDef* m) {

  /* create the Feature2D class */
  JS_NewClassID(&js_feature2d_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_feature2d_class_id, &js_feature2d_class);

  feature2d_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, feature2d_proto, js_feature2d_proto_funcs, countof(js_feature2d_proto_funcs));
  JS_SetClassProto(ctx, js_feature2d_class_id, feature2d_proto);

  feature2d_class = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, feature2d_class, js_feature2d_static_funcs, countof(js_feature2d_static_funcs));
  JS_SetConstructor(ctx, feature2d_class, feature2d_proto);

  for(auto& cl : js_feature2d_classes)
    cl.create(ctx);

  if(m) {
    JS_SetModuleExport(ctx, m, "Feature2D", feature2d_class);

    for(const auto& cl : js_feature2d_classes)
      cl.set_export(ctx, m);
  }

  return 0;
}

#ifdef JS_FEATURE2D_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_feature2d
#endif

extern "C" void
js_feature2d_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Feature2D");

  for(const auto& cl : js_feature2d_classes)
    cl.add_export(ctx, m);
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_feature2d_init)))
    return NULL;

  js_feature2d_export(ctx, m);
  return m;
}
