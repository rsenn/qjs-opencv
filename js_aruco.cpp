#include "js_cv.hpp"
#include "js_mat.hpp"
#include "js_size.hpp"
#include "js_umat.hpp"
#include "include/jsbindings.hpp"
#include "include/js_inputoutputarray.hpp"
#include <quickjs.h>

#include <new>
#include <vector>

#if defined(HAVE_OPENCV2_CHARUCO_DETECTOR_HPP)
#include <opencv2/objdetect/charuco_detector.hpp>
#define HAVE_ARUCO_DRAW 1
#define HAVE_ARUCO_DETECT 1
#elif defined(HAVE_OPENCV2_ARUCO_CHARUCO_HPP)
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#define HAVE_ARUCO_DRAW 1
#endif

/* Corner lists and ids cross the JS boundary as plain JS arrays of Mat (one
 * 1x4 CV_32FC2 Mat per marker, like cv::aruco's own vector<Mat> output) and
 * a Mat of CV_32SC1 ids. The readers below also take arrays of point arrays
 * ([[x, y], ...] or [{x, y}, ...]) and plain number arrays, so detect and
 * draw results chain without conversion. */

static cv::Mat
js_aruco_points_mat(JSContext* ctx, JSValueConst value) {
  std::vector<cv::Vec3f> pts;
  int cn = 2;
  int64_t len = 0;
  JSValue lv = JS_GetPropertyStr(ctx, value, "length");

  JS_ToInt64(ctx, &len, lv);
  JS_FreeValue(ctx, lv);

  for(int64_t i = 0; i < len; i++) {
    JSValue pt = JS_GetPropertyUint32(ctx, value, i);
    JSValue c[3];
    double v[3] = {0, 0, 0};
    JSValue l = JS_GetPropertyStr(ctx, pt, "length");
    int32_t n = 0;

    JS_ToInt32(ctx, &n, l);
    JS_FreeValue(ctx, l);

    if(n >= 2) {
      for(int k = 0; k < 3 && k < n; k++) {
        c[k] = JS_GetPropertyUint32(ctx, pt, k);
        JS_ToFloat64(ctx, &v[k], c[k]);
        JS_FreeValue(ctx, c[k]);
      }
      if(n >= 3)
        cn = 3;
    } else {
      static const char* names[] = {"x", "y", "z"};

      for(int k = 0; k < 3; k++) {
        c[k] = JS_GetPropertyStr(ctx, pt, names[k]);
        if(!JS_IsUndefined(c[k])) {
          JS_ToFloat64(ctx, &v[k], c[k]);
          if(k == 2)
            cn = 3;
        }
        JS_FreeValue(ctx, c[k]);
      }
    }
    JS_FreeValue(ctx, pt);
    pts.push_back(cv::Vec3f(v[0], v[1], v[2]));
  }

  cv::Mat m(int(pts.size()), 1, CV_32FC(cn));

  for(size_t i = 0; i < pts.size(); i++)
    for(int k = 0; k < cn; k++)
      m.ptr<float>(int(i))[k] = pts[i][k];

  return m;
}

static bool
js_aruco_read_mats(JSContext* ctx, JSValueConst value, std::vector<cv::Mat>& out) {
  int64_t len = 0;
  JSValue lv;

  out.clear();

  if(!JS_IsObject(value))
    return false;

  lv = JS_GetPropertyStr(ctx, value, "length");
  JS_ToInt64(ctx, &len, lv);
  JS_FreeValue(ctx, lv);

  for(int64_t i = 0; i < len; i++) {
    JSValue el = JS_GetPropertyUint32(ctx, value, i);
    cv::Mat* m = js_mat_data_nothrow(el);

    out.push_back(m ? *m : js_aruco_points_mat(ctx, el));
    JS_FreeValue(ctx, el);
  }

  return true;
}

static cv::Mat
js_aruco_read_ids(JSContext* ctx, JSValueConst value) {
  cv::Mat* m = js_mat_data_nothrow(value);
  cv::Mat ret;

  if(m) {
    m->convertTo(ret, CV_32S);
    return ret.reshape(1, int(ret.total()));
  }

  std::vector<int32_t> ids;
  int64_t len = 0;
  JSValue lv = JS_GetPropertyStr(ctx, value, "length");

  JS_ToInt64(ctx, &len, lv);
  JS_FreeValue(ctx, lv);

  for(int64_t i = 0; i < len; i++) {
    JSValue el = JS_GetPropertyUint32(ctx, value, i);
    int32_t v = 0;

    JS_ToInt32(ctx, &v, el);
    JS_FreeValue(ctx, el);
    ids.push_back(v);
  }

  return cv::Mat(ids, true);
}

static bool
js_aruco_is_given(int argc, JSValueConst argv[], int idx) {
  return argc > idx && !JS_IsUndefined(argv[idx]) && !JS_IsNull(argv[idx]);
}

/* write a vector<Mat> into a JS array (cleared first) */
static void
js_aruco_write_mats(JSContext* ctx, JSValueConst array, const std::vector<cv::Mat>& mats) {
  JSValue len = JS_NewInt32(ctx, 0);

  JS_SetPropertyStr(ctx, array, "length", len);

  for(size_t i = 0; i < mats.size(); i++)
    JS_SetPropertyUint32(ctx, array, i, js_mat_wrap(ctx, mats[i]));
}

static JSValue
js_aruco_new_mats(JSContext* ctx, const std::vector<cv::Mat>& mats) {
  JSValue arr = JS_NewArray(ctx);

  for(size_t i = 0; i < mats.size(); i++)
    JS_SetPropertyUint32(ctx, arr, i, js_mat_wrap(ctx, mats[i]));

  return arr;
}

/* Result delivery: when the caller passed an output argument (opencv.js
 * style, a JS array for lists or a Mat for ids) it is filled in place;
 * otherwise the value goes into the returned object under 'name'. */
struct JSArucoResult {
  JSContext* ctx;
  JSValue obj;
  bool used_out = false;

  explicit JSArucoResult(JSContext* c) : ctx(c), obj(JS_NewObject(c)) {}
  ~JSArucoResult() { JS_FreeValue(ctx, obj); }

  void
  mats(int argc, JSValueConst argv[], int idx, const char* name, const std::vector<cv::Mat>& v) {
    if(js_aruco_is_given(argc, argv, idx) && JS_IsObject(argv[idx])) {
      js_aruco_write_mats(ctx, argv[idx], v);
      used_out = true;
    } else
      JS_SetPropertyStr(ctx, obj, name, js_aruco_new_mats(ctx, v));
  }

  void
  mat(int argc, JSValueConst argv[], int idx, const char* name, const cv::Mat& v) {
    cv::Mat* dst;

    if(js_aruco_is_given(argc, argv, idx) && (dst = js_mat_data_nothrow(argv[idx]))) {
      *dst = v;
      used_out = true;
    } else
      JS_SetPropertyStr(ctx, obj, name, js_mat_wrap(ctx, v));
  }

  JSValue
  result() {
    if(used_out)
      return JS_UNDEFINED;

    return JS_DupValue(ctx, obj);
  }
};

static cv::_InputArray
js_aruco_opt(const cv::Mat& m, bool has) {
  if(has)
    return cv::_InputArray(m);

  return cv::noArray();
}

static cv::_InputArray
js_aruco_opt_ptr(const cv::Mat* m) {
  if(m)
    return cv::_InputArray(*m);

  return cv::noArray();
}

enum {
  ARUCO_DRAW_DETECTED_CORNERS_CHARUCO = 0,
  ARUCO_DRAW_DETECTED_DIAMONDS,
  ARUCO_DRAW_DETECTED_MARKERS,
};

static JSValue
js_aruco_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  try {
#ifdef HAVE_ARUCO_DRAW
    JSInputOutputArray image = js_cv_inputoutputarray(ctx, argv[0]);
    std::vector<cv::Mat> corners;
    cv::Mat ids;
    bool has_ids = false;

    /* charuco corners are one Mat (or a point array), the other two take a list of per-marker corners */
    if(magic == ARUCO_DRAW_DETECTED_CORNERS_CHARUCO && argc > 1) {
      cv::Mat* cm = js_mat_data_nothrow(argv[1]);

      corners.push_back(cm ? *cm : js_aruco_points_mat(ctx, argv[1]));
    } else if(argc < 2 || !js_aruco_read_mats(ctx, argv[1], corners))
      return JS_ThrowTypeError(ctx, "argument 2 must be an array of corner Mats or point arrays");

    if(js_aruco_is_given(argc, argv, 2)) {
      ids = js_aruco_read_ids(ctx, argv[2]);
      has_ids = true;
    }

    switch(magic) {
      case ARUCO_DRAW_DETECTED_CORNERS_CHARUCO: {
        JSColorData<double> cornerColor{255, 0, 0};

        if(argc > 3)
          js_color_read(ctx, argv[3], &cornerColor);

        cv::aruco::drawDetectedCornersCharuco(image, corners[0], js_aruco_opt(ids, has_ids), cornerColor);
        break;
      }

      case ARUCO_DRAW_DETECTED_DIAMONDS: {
        JSColorData<double> borderColor{0, 0, 255};

        if(argc > 3)
          js_color_read(ctx, argv[3], &borderColor);

        cv::aruco::drawDetectedDiamonds(image, corners, js_aruco_opt(ids, has_ids), borderColor);
        break;
      }

      case ARUCO_DRAW_DETECTED_MARKERS: {
        JSColorData<double> borderColor{0, 255, 0}; /* opencv.js default */

        if(argc > 3)
          js_color_read(ctx, argv[3], &borderColor);

        cv::aruco::drawDetectedMarkers(image, corners, js_aruco_opt(ids, has_ids), borderColor);
        break;
      }
    }
#else
    ret = JS_ThrowInternalError(ctx, "cv.aruco needs OpenCV's aruco module (objdetect >= 4.8 or contrib opencv_aruco)");
#endif
  } catch(const cv::Exception& e) { ret = js_cv_throw(ctx, e); }

  return ret;
}

#ifdef HAVE_ARUCO_DETECT

/* ---------------------------------------------------------------------- */
/* class plumbing: one JSClassID per C++ type, value stored in raw js_malloc memory */

template<class T> struct JSArucoClass {
  static inline JSClassID id = 0;
  static inline JSValue proto = JS_UNDEFINED, ctor = JS_UNDEFINED;

  static T*
  get(JSContext* ctx, JSValueConst v) {
    return static_cast<T*>(JS_GetOpaque2(ctx, v, id));
  }

  static T*
  peek(JSValueConst v) {
    return static_cast<T*>(JS_GetOpaque(v, id));
  }

  static void
  finalizer(JSRuntime* rt, JSValue v) {
    T* p = peek(v);

    if(p) {
      p->~T();
      js_free_rt(rt, p);
    }
  }

  /* take ownership of an already constructed *p (from alloc()) */
  static JSValue
  adopt(JSContext* ctx, JSValueConst new_target, T* p) {
    JSValue pr = JS_UNDEFINED, obj;

    if(!JS_IsUndefined(new_target))
      pr = JS_GetPropertyStr(ctx, new_target, "prototype");

    if(JS_IsException(pr) || !JS_IsObject(pr)) {
      JS_FreeValue(ctx, pr);
      pr = JS_DupValue(ctx, proto);
    }

    obj = JS_NewObjectProtoClass(ctx, pr, id);
    JS_FreeValue(ctx, pr);

    if(JS_IsException(obj)) {
      p->~T();
      js_free(ctx, p);
      return obj;
    }

    JS_SetOpaque(obj, p);
    return obj;
  }

  template<class... A>
  static JSValue
  make(JSContext* ctx, JSValueConst new_target, A&&... a) {
    void* mem = js_malloc(ctx, sizeof(T));
    T* p;

    if(!mem)
      return JS_EXCEPTION;

    try {
      p = new(mem) T(std::forward<A>(a)...);
    } catch(const cv::Exception& e) {
      js_free(ctx, mem);
      return js_cv_throw(ctx, e);
    }

    return adopt(ctx, new_target, p);
  }

  static JSValue
  wrap(JSContext* ctx, const T& v) {
    return make(ctx, JS_UNDEFINED, v);
  }

  static JSValue
  define(JSContext* ctx,
         const char* name,
         JSCFunction* fn,
         int nargs,
         const JSCFunctionListEntry* pf,
         size_t pn,
         JSValueConst parent_proto = JS_UNDEFINED,
         const JSCFunctionListEntry* sf = nullptr,
         size_t sn = 0) {
    JSClassDef def = {};

    def.class_name = name;
    def.finalizer = &finalizer;

    JS_NewClassID(&id);
    JS_NewClass(JS_GetRuntime(ctx), id, &def);

    proto = JS_NewObject(ctx);

    if(!JS_IsUndefined(parent_proto))
      JS_SetPrototype(ctx, proto, parent_proto);

    JS_SetPropertyFunctionList(ctx, proto, pf, pn);
    JS_SetClassProto(ctx, id, proto);

    ctor = JS_NewCFunction2(ctx, fn, name, nargs, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);

    if(sf)
      JS_SetPropertyFunctionList(ctx, ctor, sf, sn);

    return ctor;
  }
};

typedef JSArucoClass<cv::aruco::Dictionary> JSDictionary;
typedef JSArucoClass<cv::aruco::DetectorParameters> JSDetectorParameters;
typedef JSArucoClass<cv::aruco::RefineParameters> JSRefineParameters;
typedef JSArucoClass<cv::aruco::CharucoParameters> JSCharucoParameters;
typedef JSArucoClass<cv::aruco::ArucoDetector> JSArucoDetector;
typedef JSArucoClass<cv::aruco::CharucoDetector> JSCharucoDetector;
typedef JSArucoClass<cv::aruco::Board> JSBoard;
typedef JSArucoClass<cv::aruco::GridBoard> JSGridBoard;
typedef JSArucoClass<cv::aruco::CharucoBoard> JSCharucoBoard;

/* any board class: the pointer to the Board base, NULL (nothing thrown) if not a board */
static cv::aruco::Board*
js_aruco_any_board(JSValueConst v) {
  void* p;

  if((p = JS_GetOpaque(v, JSBoard::id)))
    return static_cast<cv::aruco::Board*>(p);
  if((p = JS_GetOpaque(v, JSGridBoard::id)))
    return static_cast<cv::aruco::GridBoard*>(p);
  if((p = JS_GetOpaque(v, JSCharucoBoard::id)))
    return static_cast<cv::aruco::CharucoBoard*>(p);

  return nullptr;
}

static cv::aruco::Board*
js_aruco_board_arg(JSContext* ctx, JSValueConst v) {
  cv::aruco::Board* b = js_aruco_any_board(v);

  if(!b)
    JS_ThrowTypeError(ctx, "not an aruco Board, GridBoard or CharucoBoard");

  return b;
}

/* a (width, height) argument, array or {width, height} */
static cv::Size
js_aruco_size(JSContext* ctx, JSValueConst v) {
  JSSizeData<int> s{0, 0};

  js_size_read(ctx, v, &s);
  return cv::Size(s.width, s.height);
}

static JSValue
js_aruco_size_new(JSContext* ctx, const cv::Size& s) {
  JSValue a = JS_NewArray(ctx);

  JS_SetPropertyUint32(ctx, a, 0, JS_NewInt32(ctx, s.width));
  JS_SetPropertyUint32(ctx, a, 1, JS_NewInt32(ctx, s.height));
  return a;
}

/* ---------------------------------------------------------------------- */
/* Dictionary */

enum {
  DICT_GENERATE_IMAGE_MARKER,
  DICT_GET_DISTANCE_TO_ID,
  DICT_GET_MARKER_BITS,
  DICT_GET_BYTE_LIST_FROM_BITS,
  DICT_GET_BITS_FROM_BYTE_LIST,
};

static JSValue
js_dictionary_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  if(argc >= 2) {
    cv::Mat* bytes = js_mat_data_nothrow(argv[0]);
    int32_t size = 0, maxcorr = 0;

    if(!bytes)
      return JS_ThrowTypeError(ctx, "argument 1 must be a Mat (bytesList)");

    JS_ToInt32(ctx, &size, argv[1]);
    if(argc > 2)
      JS_ToInt32(ctx, &maxcorr, argv[2]);

    return JSDictionary::make(ctx, new_target, *bytes, size, maxcorr);
  }

  return JSDictionary::make(ctx, new_target);
}

static JSValue
js_dictionary_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  cv::aruco::Dictionary* d = nullptr;

  if(magic < DICT_GET_BYTE_LIST_FROM_BITS && !(d = JSDictionary::get(ctx, this_val)))
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case DICT_GENERATE_IMAGE_MARKER: {
        int32_t id = 0, side = 0, border = 1;
        cv::Mat img;

        if(argc < 2)
          return JS_ThrowTypeError(ctx, "generateImageMarker(id, sidePixels, borderBits = 1)");

        JS_ToInt32(ctx, &id, argv[0]);
        JS_ToInt32(ctx, &side, argv[1]);
        if(argc > 2)
          JS_ToInt32(ctx, &border, argv[2]);

        d->generateImageMarker(id, side, img, border);
        return js_mat_wrap(ctx, img);
      }

      case DICT_GET_DISTANCE_TO_ID: {
        int32_t id = 0;
        cv::Mat bits = js_aruco_read_ids(ctx, argv[0]);
        cv::Mat* bm = js_mat_data_nothrow(argv[0]);

        if(bm)
          bits = *bm;
        if(argc > 1)
          JS_ToInt32(ctx, &id, argv[1]);

        return JS_NewInt32(ctx, d->getDistanceToId(bits, id, argc > 2 ? JS_ToBool(ctx, argv[2]) : true));
      }

      case DICT_GET_MARKER_BITS: {
        int32_t id = 0, rot = 0;

        JS_ToInt32(ctx, &id, argv[0]);
        if(argc > 1)
          JS_ToInt32(ctx, &rot, argv[1]);

        return js_mat_wrap(ctx, d->getMarkerBits(id, rot));
      }

      case DICT_GET_BYTE_LIST_FROM_BITS: {
        cv::Mat* bits = js_mat_data_nothrow(argv[0]);

        if(!bits)
          return JS_ThrowTypeError(ctx, "argument 1 must be a Mat");

        return js_mat_wrap(ctx, cv::aruco::Dictionary::getByteListFromBits(*bits));
      }

      case DICT_GET_BITS_FROM_BYTE_LIST: {
        cv::Mat* list = js_mat_data_nothrow(argv[0]);
        int32_t size = 0, rot = 0;

        if(!list)
          return JS_ThrowTypeError(ctx, "argument 1 must be a Mat");

        JS_ToInt32(ctx, &size, argv[1]);
        if(argc > 2)
          JS_ToInt32(ctx, &rot, argv[2]);

        return js_mat_wrap(ctx, cv::aruco::Dictionary::getBitsFromByteList(*list, size, rot));
      }
    }
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static JSValue
js_dictionary_get(JSContext* ctx, JSValueConst this_val, int magic) {
  cv::aruco::Dictionary* d = JSDictionary::get(ctx, this_val);

  if(!d)
    return JS_EXCEPTION;

  switch(magic) {
    case 0: return JS_NewInt32(ctx, d->markerSize);
    case 1: return JS_NewInt32(ctx, d->maxCorrectionBits);
    default: return js_mat_wrap(ctx, d->bytesList);
  }
}

static JSValue
js_dictionary_set(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic) {
  cv::aruco::Dictionary* d = JSDictionary::get(ctx, this_val);
  int32_t i = 0;
  cv::Mat* m;

  if(!d)
    return JS_EXCEPTION;

  switch(magic) {
    case 0:
      JS_ToInt32(ctx, &i, value);
      d->markerSize = i;
      break;
    case 1:
      JS_ToInt32(ctx, &i, value);
      d->maxCorrectionBits = i;
      break;
    default:
      if(!(m = js_mat_data_nothrow(value)))
        return JS_ThrowTypeError(ctx, "bytesList must be a Mat");
      d->bytesList = *m;
      break;
  }

  return JS_UNDEFINED;
}

static JSValue
js_aruco_get_predefined(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  int32_t name = 0;

  if(argc < 1 || JS_ToInt32(ctx, &name, argv[0]))
    return JS_ThrowTypeError(ctx, "getPredefinedDictionary(name) needs a DICT_* constant");

  try {
    return JSDictionary::wrap(ctx, cv::aruco::getPredefinedDictionary(name));
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }
}

/* generateImageMarker(dictionary, id, sidePixels, borderBits = 1) */
static JSValue
js_aruco_generate_marker(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  cv::aruco::Dictionary* d;

  if(argc < 3 || !(d = JSDictionary::get(ctx, argv[0])))
    return JS_ThrowTypeError(ctx, "generateImageMarker(dictionary, id, sidePixels, borderBits = 1)");

  return js_dictionary_method(ctx, argv[0], argc - 1, argv + 1, DICT_GENERATE_IMAGE_MARKER);
}

static const JSCFunctionListEntry js_dictionary_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("generateImageMarker", 2, js_dictionary_method, DICT_GENERATE_IMAGE_MARKER),
    JS_CFUNC_MAGIC_DEF("getDistanceToId", 2, js_dictionary_method, DICT_GET_DISTANCE_TO_ID),
    JS_CFUNC_MAGIC_DEF("getMarkerBits", 1, js_dictionary_method, DICT_GET_MARKER_BITS),
    JS_CGETSET_MAGIC_DEF("markerSize", js_dictionary_get, js_dictionary_set, 0),
    JS_CGETSET_MAGIC_DEF("maxCorrectionBits", js_dictionary_get, js_dictionary_set, 1),
    JS_CGETSET_MAGIC_DEF("bytesList", js_dictionary_get, js_dictionary_set, 2),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Dictionary", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry js_dictionary_static_funcs[] = {
    JS_CFUNC_MAGIC_DEF("getByteListFromBits", 1, js_dictionary_method, DICT_GET_BYTE_LIST_FROM_BITS),
    JS_CFUNC_MAGIC_DEF("getBitsFromByteList", 2, js_dictionary_method, DICT_GET_BITS_FROM_BYTE_LIST),
};

/* ---------------------------------------------------------------------- */
/* DetectorParameters, RefineParameters, CharucoParameters: plain fields */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

struct JSArucoField {
  const char* name;
  size_t offset;
  char type; /* i int, d double, f float, b bool */
};

static JSValue
js_aruco_field_read(JSContext* ctx, const void* base, const JSArucoField& f) {
  const char* ptr = static_cast<const char*>(base) + f.offset;

  switch(f.type) {
    case 'i': return JS_NewInt32(ctx, *reinterpret_cast<const int*>(ptr));
    case 'd': return JS_NewFloat64(ctx, *reinterpret_cast<const double*>(ptr));
    case 'f': return JS_NewFloat64(ctx, *reinterpret_cast<const float*>(ptr));
    default: return JS_NewBool(ctx, *reinterpret_cast<const bool*>(ptr));
  }
}

static JSValue
js_aruco_field_write(JSContext* ctx, void* base, const JSArucoField& f, JSValueConst value) {
  char* ptr = static_cast<char*>(base) + f.offset;
  int32_t i;
  double d;

  switch(f.type) {
    case 'i':
      if(JS_ToInt32(ctx, &i, value))
        return JS_EXCEPTION;
      *reinterpret_cast<int*>(ptr) = i;
      break;
    case 'd':
      if(JS_ToFloat64(ctx, &d, value))
        return JS_EXCEPTION;
      *reinterpret_cast<double*>(ptr) = d;
      break;
    case 'f':
      if(JS_ToFloat64(ctx, &d, value))
        return JS_EXCEPTION;
      *reinterpret_cast<float*>(ptr) = float(d);
      break;
    default: *reinterpret_cast<bool*>(ptr) = JS_ToBool(ctx, value) != 0; break;
  }

  return JS_UNDEFINED;
}

#define DP_FIELD(n, t) {#n, offsetof(cv::aruco::DetectorParameters, n), t},

#if CV_VERSION_MAJOR >= 5
#define DP_FIELDS_V5 DP_FIELD(minGroupDistance, 'f') DP_FIELD(validBitIdThreshold, 'f')
#else
#define DP_FIELDS_V5
#endif

#define DP_FIELDS \
  DP_FIELD(adaptiveThreshWinSizeMin, 'i') \
  DP_FIELD(adaptiveThreshWinSizeMax, 'i') \
  DP_FIELD(adaptiveThreshWinSizeStep, 'i') \
  DP_FIELD(adaptiveThreshConstant, 'd') \
  DP_FIELD(minMarkerPerimeterRate, 'd') \
  DP_FIELD(maxMarkerPerimeterRate, 'd') \
  DP_FIELD(polygonalApproxAccuracyRate, 'd') \
  DP_FIELD(minCornerDistanceRate, 'd') \
  DP_FIELD(minDistanceToBorder, 'i') \
  DP_FIELD(minMarkerDistanceRate, 'd') \
  DP_FIELD(cornerRefinementMethod, 'i') \
  DP_FIELD(cornerRefinementWinSize, 'i') \
  DP_FIELD(relativeCornerRefinmentWinSize, 'f') \
  DP_FIELD(cornerRefinementMaxIterations, 'i') \
  DP_FIELD(cornerRefinementMinAccuracy, 'd') \
  DP_FIELD(markerBorderBits, 'i') \
  DP_FIELD(perspectiveRemovePixelPerCell, 'i') \
  DP_FIELD(perspectiveRemoveIgnoredMarginPerCell, 'd') \
  DP_FIELD(maxErroneousBitsInBorderRate, 'd') \
  DP_FIELD(minOtsuStdDev, 'd') \
  DP_FIELD(errorCorrectionRate, 'd') \
  DP_FIELD(aprilTagQuadDecimate, 'f') \
  DP_FIELD(aprilTagQuadSigma, 'f') \
  DP_FIELD(aprilTagMinClusterPixels, 'i') \
  DP_FIELD(aprilTagMaxNmaxima, 'i') \
  DP_FIELD(aprilTagCriticalRad, 'f') \
  DP_FIELD(aprilTagMaxLineFitMse, 'f') \
  DP_FIELD(aprilTagMinWhiteBlackDiff, 'i') \
  DP_FIELD(aprilTagDeglitch, 'i') \
  DP_FIELD(detectInvertedMarker, 'b') \
  DP_FIELD(useAruco3Detection, 'b') \
  DP_FIELD(minSideLengthCanonicalImg, 'i') \
  DP_FIELD(minMarkerLengthRatioOriginalImg, 'f') \
  DP_FIELDS_V5

static const JSArucoField js_dp_fields[] = {DP_FIELDS};

#define RP_FIELD(n, t) {#n, offsetof(cv::aruco::RefineParameters, n), t},
static const JSArucoField js_rp_fields[] = {
    RP_FIELD(minRepDistance, 'f') RP_FIELD(errorCorrectionRate, 'f') RP_FIELD(checkAllOrders, 'b')};

#define CP_FIELD(n, t) {#n, offsetof(cv::aruco::CharucoParameters, n), t},
static const JSArucoField js_cp_fields[] = {
    CP_FIELD(minMarkers, 'i') CP_FIELD(tryRefineMarkers, 'b') CP_FIELD(checkMarkers, 'b')};

#pragma GCC diagnostic pop

template<class T>
static JSValue
js_aruco_params_get(JSContext* ctx, JSValueConst this_val, int magic, const JSArucoField* tab) {
  T* p = JSArucoClass<T>::get(ctx, this_val);

  return p ? js_aruco_field_read(ctx, p, tab[magic]) : JS_EXCEPTION;
}

template<class T>
static JSValue
js_aruco_params_set(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic, const JSArucoField* tab) {
  T* p = JSArucoClass<T>::get(ctx, this_val);

  return p ? js_aruco_field_write(ctx, p, tab[magic], value) : JS_EXCEPTION;
}

static JSValue
js_dp_get(JSContext* ctx, JSValueConst this_val, int magic) {
  return js_aruco_params_get<cv::aruco::DetectorParameters>(ctx, this_val, magic, js_dp_fields);
}

static JSValue
js_dp_set(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic) {
  return js_aruco_params_set<cv::aruco::DetectorParameters>(ctx, this_val, value, magic, js_dp_fields);
}

static JSValue
js_rp_get(JSContext* ctx, JSValueConst this_val, int magic) {
  return js_aruco_params_get<cv::aruco::RefineParameters>(ctx, this_val, magic, js_rp_fields);
}

static JSValue
js_rp_set(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic) {
  return js_aruco_params_set<cv::aruco::RefineParameters>(ctx, this_val, value, magic, js_rp_fields);
}

static JSValue
js_cp_get(JSContext* ctx, JSValueConst this_val, int magic) {
  cv::aruco::CharucoParameters* p;

  if(magic >= 100) {
    if(!(p = JSCharucoParameters::get(ctx, this_val)))
      return JS_EXCEPTION;

    return js_mat_wrap(ctx, magic == 100 ? p->cameraMatrix : p->distCoeffs);
  }

  return js_aruco_params_get<cv::aruco::CharucoParameters>(ctx, this_val, magic, js_cp_fields);
}

static JSValue
js_cp_set(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic) {
  cv::aruco::CharucoParameters* p;
  cv::Mat* m;

  if(magic >= 100) {
    if(!(p = JSCharucoParameters::get(ctx, this_val)))
      return JS_EXCEPTION;
    if(!(m = js_mat_data_nothrow(value)))
      return JS_ThrowTypeError(ctx, "value must be a Mat");

    (magic == 100 ? p->cameraMatrix : p->distCoeffs) = *m;
    return JS_UNDEFINED;
  }

  return js_aruco_params_set<cv::aruco::CharucoParameters>(ctx, this_val, value, magic, js_cp_fields);
}

static JSValue
js_dp_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  return JSDetectorParameters::make(ctx, new_target);
}

static JSValue
js_rp_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  double rd = 10, er = 3;
  bool all = true;

  if(argc > 0)
    JS_ToFloat64(ctx, &rd, argv[0]);
  if(argc > 1)
    JS_ToFloat64(ctx, &er, argv[1]);
  if(argc > 2)
    all = JS_ToBool(ctx, argv[2]);

  return JSRefineParameters::make(ctx, new_target, float(rd), float(er), all);
}

static JSValue
js_cp_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  return JSCharucoParameters::make(ctx, new_target);
}

/* getter/setter pairs are defined at init from the field tables: the magic of each is its index */
template<class G, class S>
static void
js_aruco_define_fields(JSContext* ctx, JSValueConst proto, const JSArucoField* tab, size_t n, G get, S set) {
  for(size_t i = 0; i < n; i++) {
    JSAtom atom = JS_NewAtom(ctx, tab[i].name);
    JSValue g = JS_NewCFunctionMagic(ctx, reinterpret_cast<JSCFunctionMagic*>(get), tab[i].name, 0, JS_CFUNC_getter_magic, int(i));
    JSValue s = JS_NewCFunctionMagic(ctx, reinterpret_cast<JSCFunctionMagic*>(set), tab[i].name, 1, JS_CFUNC_setter_magic, int(i));

    JS_DefinePropertyGetSet(ctx, proto, atom, g, s, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);
  }
}


/* ---------------------------------------------------------------------- */
/* ArucoDetector */

enum {
  AD_DETECT_MARKERS,
  AD_REFINE_DETECTED_MARKERS,
  AD_GET_DICTIONARY,
  AD_SET_DICTIONARY,
  AD_GET_DETECTOR_PARAMETERS,
  AD_SET_DETECTOR_PARAMETERS,
  AD_GET_REFINE_PARAMETERS,
  AD_SET_REFINE_PARAMETERS,
};

/* new ArucoDetector(dictionary = DICT_4X4_50, detectorParameters, refineParameters) */
static JSValue
js_arucodetector_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::aruco::Dictionary dict = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
  cv::aruco::DetectorParameters dp;
  cv::aruco::RefineParameters rp;

  if(js_aruco_is_given(argc, argv, 0)) {
    cv::aruco::Dictionary* d = JSDictionary::get(ctx, argv[0]);

    if(!d)
      return JS_EXCEPTION;
    dict = *d;
  }
  if(js_aruco_is_given(argc, argv, 1)) {
    cv::aruco::DetectorParameters* p = JSDetectorParameters::get(ctx, argv[1]);

    if(!p)
      return JS_EXCEPTION;
    dp = *p;
  }
  if(js_aruco_is_given(argc, argv, 2)) {
    cv::aruco::RefineParameters* p = JSRefineParameters::get(ctx, argv[2]);

    if(!p)
      return JS_EXCEPTION;
    rp = *p;
  }

  return JSArucoDetector::make(ctx, new_target, dict, dp, rp);
}

static JSValue
js_arucodetector_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  cv::aruco::ArucoDetector* det = JSArucoDetector::get(ctx, this_val);

  if(!det)
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case AD_DETECT_MARKERS: {
        JSInputArray img = js_cv_inputarray(ctx, argv[0]);
        std::vector<cv::Mat> corners, rejected;
        cv::Mat ids;
        JSArucoResult res(ctx);

        det->detectMarkers(img, corners, ids, rejected);

        res.mats(argc, argv, 1, "corners", corners);
        res.mat(argc, argv, 2, "ids", ids);
        res.mats(argc, argv, 3, "rejected", rejected);
        return res.result();
      }

      case AD_REFINE_DETECTED_MARKERS: {
        /* (image, board, corners, ids, rejected, cameraMatrix?, distCoeffs?): corners, ids and rejected are
         * read and updated in place, the recovered indices are returned */
        JSInputArray img = js_cv_inputarray(ctx, argv[0]);
        cv::aruco::Board* board;
        std::vector<cv::Mat> corners, rejected;
        cv::Mat ids, recovered, *ids_out;

        if(argc < 5 || !(board = js_aruco_board_arg(ctx, argv[1])))
          return argc < 5 ? JS_ThrowTypeError(ctx, "refineDetectedMarkers(image, board, corners, ids, rejected)") : JS_EXCEPTION;

        js_aruco_read_mats(ctx, argv[2], corners);
        ids = js_aruco_read_ids(ctx, argv[3]);
        js_aruco_read_mats(ctx, argv[4], rejected);

        cv::Mat *cam = argc > 5 ? js_mat_data_nothrow(argv[5]) : nullptr, *dist = argc > 6 ? js_mat_data_nothrow(argv[6]) : nullptr;

        det->refineDetectedMarkers(img,
                                   *board,
                                   corners,
                                   ids,
                                   rejected,
                                   js_aruco_opt_ptr(cam),
                                   js_aruco_opt_ptr(dist),
                                   recovered);

        js_aruco_write_mats(ctx, argv[2], corners);
        js_aruco_write_mats(ctx, argv[4], rejected);
        if((ids_out = js_mat_data_nothrow(argv[3])))
          *ids_out = ids;

        return js_mat_wrap(ctx, recovered);
      }

      case AD_GET_DICTIONARY: return JSDictionary::wrap(ctx, det->getDictionary());
      case AD_GET_DETECTOR_PARAMETERS: return JSDetectorParameters::wrap(ctx, det->getDetectorParameters());
      case AD_GET_REFINE_PARAMETERS: return JSRefineParameters::wrap(ctx, det->getRefineParameters());

      case AD_SET_DICTIONARY: {
        cv::aruco::Dictionary* d = JSDictionary::get(ctx, argv[0]);

        if(!d)
          return JS_EXCEPTION;
        det->setDictionary(*d);
        break;
      }
      case AD_SET_DETECTOR_PARAMETERS: {
        cv::aruco::DetectorParameters* p = JSDetectorParameters::get(ctx, argv[0]);

        if(!p)
          return JS_EXCEPTION;
        det->setDetectorParameters(*p);
        break;
      }
      case AD_SET_REFINE_PARAMETERS: {
        cv::aruco::RefineParameters* p = JSRefineParameters::get(ctx, argv[0]);

        if(!p)
          return JS_EXCEPTION;
        det->setRefineParameters(*p);
        break;
      }
    }
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_arucodetector_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("detectMarkers", 1, js_arucodetector_method, AD_DETECT_MARKERS),
    JS_CFUNC_MAGIC_DEF("refineDetectedMarkers", 5, js_arucodetector_method, AD_REFINE_DETECTED_MARKERS),
    JS_CFUNC_MAGIC_DEF("getDictionary", 0, js_arucodetector_method, AD_GET_DICTIONARY),
    JS_CFUNC_MAGIC_DEF("setDictionary", 1, js_arucodetector_method, AD_SET_DICTIONARY),
    JS_CFUNC_MAGIC_DEF("getDetectorParameters", 0, js_arucodetector_method, AD_GET_DETECTOR_PARAMETERS),
    JS_CFUNC_MAGIC_DEF("setDetectorParameters", 1, js_arucodetector_method, AD_SET_DETECTOR_PARAMETERS),
    JS_CFUNC_MAGIC_DEF("getRefineParameters", 0, js_arucodetector_method, AD_GET_REFINE_PARAMETERS),
    JS_CFUNC_MAGIC_DEF("setRefineParameters", 1, js_arucodetector_method, AD_SET_REFINE_PARAMETERS),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "ArucoDetector", JS_PROP_CONFIGURABLE),
};

/* ---------------------------------------------------------------------- */
/* Board, GridBoard, CharucoBoard: one class id each, prototypes chained to Board */

enum {
  BOARD_GET_DICTIONARY,
  BOARD_GET_OBJ_POINTS,
  BOARD_GET_IDS,
  BOARD_GENERATE_IMAGE,
  BOARD_MATCH_IMAGE_POINTS,
};

static JSValue
js_board_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  cv::aruco::Board* b = js_aruco_board_arg(ctx, this_val);

  if(!b)
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case BOARD_GET_DICTIONARY: return JSDictionary::wrap(ctx, b->getDictionary());

      case BOARD_GET_OBJ_POINTS: {
        std::vector<cv::Mat> mats;

        for(const std::vector<cv::Point3f>& pts : b->getObjPoints())
          mats.push_back(cv::Mat(pts, true));

        return js_aruco_new_mats(ctx, mats);
      }

      case BOARD_GET_IDS: return js_mat_wrap(ctx, cv::Mat(b->getIds(), true));

      case BOARD_GENERATE_IMAGE: {
        /* (size, marginSize = 0, borderBits = 1) */
        int32_t margin = 0, border = 1;
        cv::Mat img;

        if(argc < 1)
          return JS_ThrowTypeError(ctx, "generateImage(size, marginSize = 0, borderBits = 1)");
        if(argc > 1)
          JS_ToInt32(ctx, &margin, argv[1]);
        if(argc > 2)
          JS_ToInt32(ctx, &border, argv[2]);

        b->generateImage(js_aruco_size(ctx, argv[0]), img, margin, border);
        return js_mat_wrap(ctx, img);
      }

      case BOARD_MATCH_IMAGE_POINTS: {
        /* (detectedCorners, detectedIds, objPoints?, imgPoints?) -> {objPoints, imgPoints} */
        std::vector<cv::Mat> corners;
        cv::Mat ids, obj, img;
        JSArucoResult res(ctx);

        if(argc < 2 || !js_aruco_read_mats(ctx, argv[0], corners))
          return JS_ThrowTypeError(ctx, "matchImagePoints(detectedCorners, detectedIds)");

        ids = js_aruco_read_ids(ctx, argv[1]);
        b->matchImagePoints(corners, ids, obj, img);

        res.mat(argc, argv, 2, "objPoints", obj);
        res.mat(argc, argv, 3, "imgPoints", img);
        return res.result();
      }
    }
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

/* new Board(objPoints, dictionary, ids) */
static JSValue
js_board_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  std::vector<cv::Mat> obj;
  cv::aruco::Dictionary* d;

  if(argc < 3 || !js_aruco_read_mats(ctx, argv[0], obj) || !(d = JSDictionary::get(ctx, argv[1])))
    return argc < 3 ? JS_ThrowTypeError(ctx, "new Board(objPoints, dictionary, ids)") : JS_EXCEPTION;

  cv::Mat ids = js_aruco_read_ids(ctx, argv[2]);

  return JSBoard::make(ctx, new_target, obj, *d, ids);
}

/* new GridBoard(size, markerLength, markerSeparation, dictionary, ids?) */
static JSValue
js_gridboard_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::aruco::Dictionary* d;
  double len = 0, sep = 0;
  cv::Mat ids;

  if(argc < 4 || !(d = JSDictionary::get(ctx, argv[3])))
    return argc < 4 ? JS_ThrowTypeError(ctx, "new GridBoard(size, markerLength, markerSeparation, dictionary, ids)") : JS_EXCEPTION;

  JS_ToFloat64(ctx, &len, argv[1]);
  JS_ToFloat64(ctx, &sep, argv[2]);
  if(js_aruco_is_given(argc, argv, 4))
    ids = js_aruco_read_ids(ctx, argv[4]);

  return JSGridBoard::make(ctx, new_target, js_aruco_size(ctx, argv[0]), float(len), float(sep), *d, ids);
}

/* new CharucoBoard(size, squareLength, markerLength, dictionary, ids?) */
static JSValue
js_charucoboard_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::aruco::Dictionary* d;
  double sq = 0, len = 0;
  cv::Mat ids;

  if(argc < 4 || !(d = JSDictionary::get(ctx, argv[3])))
    return argc < 4 ? JS_ThrowTypeError(ctx, "new CharucoBoard(size, squareLength, markerLength, dictionary, ids)") : JS_EXCEPTION;

  JS_ToFloat64(ctx, &sq, argv[1]);
  JS_ToFloat64(ctx, &len, argv[2]);
  if(js_aruco_is_given(argc, argv, 4))
    ids = js_aruco_read_ids(ctx, argv[4]);

  return JSCharucoBoard::make(ctx, new_target, js_aruco_size(ctx, argv[0]), float(sq), float(len), *d, ids);
}

enum {
  GB_GET_GRID_SIZE,
  GB_GET_MARKER_LENGTH,
  GB_GET_MARKER_SEPARATION,
};

static JSValue
js_gridboard_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  cv::aruco::GridBoard* b = JSGridBoard::get(ctx, this_val);

  if(!b)
    return JS_EXCEPTION;

  switch(magic) {
    case GB_GET_GRID_SIZE: return js_aruco_size_new(ctx, b->getGridSize());
    case GB_GET_MARKER_LENGTH: return JS_NewFloat64(ctx, b->getMarkerLength());
    default: return JS_NewFloat64(ctx, b->getMarkerSeparation());
  }
}

enum {
  CB_GET_CHESSBOARD_SIZE,
  CB_GET_SQUARE_LENGTH,
  CB_GET_MARKER_LENGTH,
  CB_GET_CHESSBOARD_CORNERS,
  CB_GET_NEAREST_MARKER_IDX,
  CB_GET_NEAREST_MARKER_CORNERS,
  CB_CHECK_COLLINEAR,
  CB_GET_LEGACY_PATTERN,
  CB_SET_LEGACY_PATTERN,
};

static JSValue
js_charucoboard_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  cv::aruco::CharucoBoard* b = JSCharucoBoard::get(ctx, this_val);

  if(!b)
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case CB_GET_CHESSBOARD_SIZE: return js_aruco_size_new(ctx, b->getChessboardSize());
      case CB_GET_SQUARE_LENGTH: return JS_NewFloat64(ctx, b->getSquareLength());
      case CB_GET_MARKER_LENGTH: return JS_NewFloat64(ctx, b->getMarkerLength());
      case CB_GET_CHESSBOARD_CORNERS: return js_mat_wrap(ctx, cv::Mat(b->getChessboardCorners(), true));

      case CB_GET_NEAREST_MARKER_IDX:
      case CB_GET_NEAREST_MARKER_CORNERS: {
        std::vector<std::vector<int>> v = magic == CB_GET_NEAREST_MARKER_IDX ? b->getNearestMarkerIdx() : b->getNearestMarkerCorners();
        JSValue outer = JS_NewArray(ctx);

        for(size_t i = 0; i < v.size(); i++) {
          JSValue inner = JS_NewArray(ctx);

          for(size_t j = 0; j < v[i].size(); j++)
            JS_SetPropertyUint32(ctx, inner, j, JS_NewInt32(ctx, v[i][j]));
          JS_SetPropertyUint32(ctx, outer, i, inner);
        }
        return outer;
      }

      case CB_CHECK_COLLINEAR: return JS_NewBool(ctx, b->checkCharucoCornersCollinear(js_aruco_read_ids(ctx, argv[0])));
      case CB_GET_LEGACY_PATTERN: return JS_NewBool(ctx, b->getLegacyPattern());
      case CB_SET_LEGACY_PATTERN: b->setLegacyPattern(JS_ToBool(ctx, argv[0])); break;
    }
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_board_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("getDictionary", 0, js_board_method, BOARD_GET_DICTIONARY),
    JS_CFUNC_MAGIC_DEF("getObjPoints", 0, js_board_method, BOARD_GET_OBJ_POINTS),
    JS_CFUNC_MAGIC_DEF("getIds", 0, js_board_method, BOARD_GET_IDS),
    JS_CFUNC_MAGIC_DEF("generateImage", 1, js_board_method, BOARD_GENERATE_IMAGE),
    JS_CFUNC_MAGIC_DEF("matchImagePoints", 2, js_board_method, BOARD_MATCH_IMAGE_POINTS),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Board", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry js_gridboard_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("getGridSize", 0, js_gridboard_method, GB_GET_GRID_SIZE),
    JS_CFUNC_MAGIC_DEF("getMarkerLength", 0, js_gridboard_method, GB_GET_MARKER_LENGTH),
    JS_CFUNC_MAGIC_DEF("getMarkerSeparation", 0, js_gridboard_method, GB_GET_MARKER_SEPARATION),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "GridBoard", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry js_charucoboard_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("getChessboardSize", 0, js_charucoboard_method, CB_GET_CHESSBOARD_SIZE),
    JS_CFUNC_MAGIC_DEF("getSquareLength", 0, js_charucoboard_method, CB_GET_SQUARE_LENGTH),
    JS_CFUNC_MAGIC_DEF("getMarkerLength", 0, js_charucoboard_method, CB_GET_MARKER_LENGTH),
    JS_CFUNC_MAGIC_DEF("getChessboardCorners", 0, js_charucoboard_method, CB_GET_CHESSBOARD_CORNERS),
    JS_CFUNC_MAGIC_DEF("getNearestMarkerIdx", 0, js_charucoboard_method, CB_GET_NEAREST_MARKER_IDX),
    JS_CFUNC_MAGIC_DEF("getNearestMarkerCorners", 0, js_charucoboard_method, CB_GET_NEAREST_MARKER_CORNERS),
    JS_CFUNC_MAGIC_DEF("checkCharucoCornersCollinear", 1, js_charucoboard_method, CB_CHECK_COLLINEAR),
    JS_CFUNC_MAGIC_DEF("getLegacyPattern", 0, js_charucoboard_method, CB_GET_LEGACY_PATTERN),
    JS_CFUNC_MAGIC_DEF("setLegacyPattern", 1, js_charucoboard_method, CB_SET_LEGACY_PATTERN),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "CharucoBoard", JS_PROP_CONFIGURABLE),
};

/* ---------------------------------------------------------------------- */
/* CharucoDetector */

enum {
  CD_DETECT_BOARD,
  CD_DETECT_DIAMONDS,
  CD_GET_BOARD,
  CD_SET_BOARD,
  CD_GET_CHARUCO_PARAMETERS,
  CD_SET_CHARUCO_PARAMETERS,
  CD_GET_DETECTOR_PARAMETERS,
  CD_SET_DETECTOR_PARAMETERS,
  CD_GET_REFINE_PARAMETERS,
  CD_SET_REFINE_PARAMETERS,
};

/* new CharucoDetector(board, charucoParameters?, detectorParameters?, refineParameters?) */
static JSValue
js_charucodetector_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  cv::aruco::CharucoBoard* board;
  cv::aruco::CharucoParameters cp;
  cv::aruco::DetectorParameters dp;
  cv::aruco::RefineParameters rp;

  if(argc < 1 || !(board = JSCharucoBoard::get(ctx, argv[0])))
    return argc < 1 ? JS_ThrowTypeError(ctx, "new CharucoDetector(board, ...)") : JS_EXCEPTION;

  if(js_aruco_is_given(argc, argv, 1)) {
    cv::aruco::CharucoParameters* p = JSCharucoParameters::get(ctx, argv[1]);

    if(!p)
      return JS_EXCEPTION;
    cp = *p;
  }
  if(js_aruco_is_given(argc, argv, 2)) {
    cv::aruco::DetectorParameters* p = JSDetectorParameters::get(ctx, argv[2]);

    if(!p)
      return JS_EXCEPTION;
    dp = *p;
  }
  if(js_aruco_is_given(argc, argv, 3)) {
    cv::aruco::RefineParameters* p = JSRefineParameters::get(ctx, argv[3]);

    if(!p)
      return JS_EXCEPTION;
    rp = *p;
  }

  return JSCharucoDetector::make(ctx, new_target, *board, cp, dp, rp);
}

static JSValue
js_charucodetector_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  cv::aruco::CharucoDetector* det = JSCharucoDetector::get(ctx, this_val);

  if(!det)
    return JS_EXCEPTION;

  try {
    switch(magic) {
      case CD_DETECT_BOARD:
      case CD_DETECT_DIAMONDS: {
        /* detectBoard(image, charucoCorners?, charucoIds?, markerCorners?, markerIds?)
         * detectDiamonds(image, diamondCorners?, diamondIds?, markerCorners?, markerIds?)
         * markerCorners/markerIds are optional detected markers to reuse and are written back */
        JSInputArray img = js_cv_inputarray(ctx, argv[0]);
        std::vector<cv::Mat> mcorners, dcorners;
        cv::Mat mids, out_corners, out_ids;
        JSArucoResult res(ctx);

        if(js_aruco_is_given(argc, argv, 3) && JS_IsObject(argv[3]))
          js_aruco_read_mats(ctx, argv[3], mcorners);
        if(js_aruco_is_given(argc, argv, 4) && js_mat_data_nothrow(argv[4]))
          mids = *js_mat_data_nothrow(argv[4]);

        if(magic == CD_DETECT_BOARD) {
          det->detectBoard(img, out_corners, out_ids, mcorners, mids);
          res.mat(argc, argv, 1, "charucoCorners", out_corners);
          res.mat(argc, argv, 2, "charucoIds", out_ids);
        } else {
          det->detectDiamonds(img, dcorners, out_ids, mcorners, mids);
          res.mats(argc, argv, 1, "diamondCorners", dcorners);
          res.mat(argc, argv, 2, "diamondIds", out_ids);
        }

        res.mats(argc, argv, 3, "markerCorners", mcorners);
        res.mat(argc, argv, 4, "markerIds", mids);
        return res.result();
      }

      case CD_GET_BOARD: return JSCharucoBoard::wrap(ctx, det->getBoard());
      case CD_GET_CHARUCO_PARAMETERS: return JSCharucoParameters::wrap(ctx, det->getCharucoParameters());
      case CD_GET_DETECTOR_PARAMETERS: return JSDetectorParameters::wrap(ctx, det->getDetectorParameters());
      case CD_GET_REFINE_PARAMETERS: return JSRefineParameters::wrap(ctx, det->getRefineParameters());

      case CD_SET_BOARD: {
        cv::aruco::CharucoBoard* b = JSCharucoBoard::get(ctx, argv[0]);

        if(!b)
          return JS_EXCEPTION;
        det->setBoard(*b);
        break;
      }
      case CD_SET_CHARUCO_PARAMETERS: {
        cv::aruco::CharucoParameters* p = JSCharucoParameters::get(ctx, argv[0]);

        if(!p)
          return JS_EXCEPTION;
        det->setCharucoParameters(*p);
        break;
      }
      case CD_SET_DETECTOR_PARAMETERS: {
        cv::aruco::DetectorParameters* p = JSDetectorParameters::get(ctx, argv[0]);

        if(!p)
          return JS_EXCEPTION;
        det->setDetectorParameters(*p);
        break;
      }
      case CD_SET_REFINE_PARAMETERS: {
        cv::aruco::RefineParameters* p = JSRefineParameters::get(ctx, argv[0]);

        if(!p)
          return JS_EXCEPTION;
        det->setRefineParameters(*p);
        break;
      }
    }
  } catch(const cv::Exception& e) { return js_cv_throw(ctx, e); }

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_charucodetector_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("detectBoard", 1, js_charucodetector_method, CD_DETECT_BOARD),
    JS_CFUNC_MAGIC_DEF("detectDiamonds", 1, js_charucodetector_method, CD_DETECT_DIAMONDS),
    JS_CFUNC_MAGIC_DEF("getBoard", 0, js_charucodetector_method, CD_GET_BOARD),
    JS_CFUNC_MAGIC_DEF("setBoard", 1, js_charucodetector_method, CD_SET_BOARD),
    JS_CFUNC_MAGIC_DEF("getCharucoParameters", 0, js_charucodetector_method, CD_GET_CHARUCO_PARAMETERS),
    JS_CFUNC_MAGIC_DEF("setCharucoParameters", 1, js_charucodetector_method, CD_SET_CHARUCO_PARAMETERS),
    JS_CFUNC_MAGIC_DEF("getDetectorParameters", 0, js_charucodetector_method, CD_GET_DETECTOR_PARAMETERS),
    JS_CFUNC_MAGIC_DEF("setDetectorParameters", 1, js_charucodetector_method, CD_SET_DETECTOR_PARAMETERS),
    JS_CFUNC_MAGIC_DEF("getRefineParameters", 0, js_charucodetector_method, CD_GET_REFINE_PARAMETERS),
    JS_CFUNC_MAGIC_DEF("setRefineParameters", 1, js_charucodetector_method, CD_SET_REFINE_PARAMETERS),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "CharucoDetector", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry js_charucoparameters_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("cameraMatrix", js_cp_get, js_cp_set, 100),
    JS_CGETSET_MAGIC_DEF("distCoeffs", js_cp_get, js_cp_set, 101),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "CharucoParameters", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry js_params_tag_funcs[][1] = {
    {JS_PROP_STRING_DEF("[Symbol.toStringTag]", "DetectorParameters", JS_PROP_CONFIGURABLE)},
    {JS_PROP_STRING_DEF("[Symbol.toStringTag]", "RefineParameters", JS_PROP_CONFIGURABLE)},
};

#endif /* HAVE_ARUCO_DETECT */

/* ---------------------------------------------------------------------- */
/* exports */

#define ARUCO_DICT(name) JS_PROP_INT32_DEF(#name, int(cv::aruco::name), JS_PROP_ENUMERABLE)

#ifdef HAVE_ARUCO_DETECT
#define ARUCO_DICT_LIST \
  ARUCO_DICT(DICT_4X4_50), ARUCO_DICT(DICT_4X4_100), ARUCO_DICT(DICT_4X4_250), ARUCO_DICT(DICT_4X4_1000), ARUCO_DICT(DICT_5X5_50), \
      ARUCO_DICT(DICT_5X5_100), ARUCO_DICT(DICT_5X5_250), ARUCO_DICT(DICT_5X5_1000), ARUCO_DICT(DICT_6X6_50), ARUCO_DICT(DICT_6X6_100), \
      ARUCO_DICT(DICT_6X6_250), ARUCO_DICT(DICT_6X6_1000), ARUCO_DICT(DICT_7X7_50), ARUCO_DICT(DICT_7X7_100), ARUCO_DICT(DICT_7X7_250), \
      ARUCO_DICT(DICT_7X7_1000), ARUCO_DICT(DICT_ARUCO_ORIGINAL), ARUCO_DICT(DICT_APRILTAG_16h5), ARUCO_DICT(DICT_APRILTAG_25h9), \
      ARUCO_DICT(DICT_APRILTAG_36h10), ARUCO_DICT(DICT_APRILTAG_36h11)

static const JSCFunctionListEntry js_aruco_dict_consts[] = {ARUCO_DICT_LIST};

#define ARUCO_DETECT_FUNCS \
  JS_CFUNC_DEF("getPredefinedDictionary", 1, js_aruco_get_predefined), JS_CFUNC_DEF("generateImageMarker", 3, js_aruco_generate_marker),
#define ARUCO_DETECT_TOP_ENTRIES \
  JS_OBJECT_DEF("aruco_PredefinedDictionaryType", js_aruco_dict_consts, countof(js_aruco_dict_consts), JS_PROP_ENUMERABLE), ARUCO_DICT_LIST,
#else
#define ARUCO_DETECT_FUNCS
#define ARUCO_DETECT_TOP_ENTRIES
#endif

static const JSCFunctionListEntry js_aruco_aruco_funcs[] = {
    JS_CFUNC_MAGIC_DEF("drawDetectedCornersCharuco", 2, js_aruco_func, ARUCO_DRAW_DETECTED_CORNERS_CHARUCO),
    JS_CFUNC_MAGIC_DEF("drawDetectedDiamonds", 2, js_aruco_func, ARUCO_DRAW_DETECTED_DIAMONDS),
    JS_CFUNC_MAGIC_DEF("drawDetectedMarkers", 2, js_aruco_func, ARUCO_DRAW_DETECTED_MARKERS),
    ARUCO_DETECT_FUNCS
};

/* opencv.js keeps the aruco functions and DICT_* constants at the top level */
static const JSCFunctionListEntry js_aruco_top_funcs[] = {
    JS_CFUNC_MAGIC_DEF("drawDetectedCornersCharuco", 2, js_aruco_func, ARUCO_DRAW_DETECTED_CORNERS_CHARUCO),
    JS_CFUNC_MAGIC_DEF("drawDetectedDiamonds", 2, js_aruco_func, ARUCO_DRAW_DETECTED_DIAMONDS),
    JS_CFUNC_MAGIC_DEF("drawDetectedMarkers", 2, js_aruco_func, ARUCO_DRAW_DETECTED_MARKERS),
#ifdef HAVE_ARUCO_DETECT
    JS_CFUNC_DEF("getPredefinedDictionary", 1, js_aruco_get_predefined),
    JS_OBJECT_DEF("aruco_PredefinedDictionaryType", js_aruco_dict_consts, countof(js_aruco_dict_consts), JS_PROP_ENUMERABLE),
    ARUCO_DICT_LIST,
#endif
};

#ifdef HAVE_ARUCO_DETECT
static const char* const js_aruco_class_names[] = {
    "Dictionary", "DetectorParameters", "RefineParameters", "CharucoParameters", "ArucoDetector",
    "CharucoDetector", "Board", "GridBoard", "CharucoBoard",
};

static JSValue js_aruco_class_ctors[countof(js_aruco_class_names)];
#endif

static JSValue js_aruco_object = JS_UNDEFINED;

extern "C" int
js_aruco_init(JSContext* ctx, JSModuleDef* m) {
  JSValue aruco = JS_NewObject(ctx);

  JS_SetPropertyFunctionList(ctx, aruco, js_aruco_aruco_funcs, countof(js_aruco_aruco_funcs));

#ifdef HAVE_ARUCO_DETECT
  if(JS_IsUndefined(JSDictionary::proto)) {
    js_aruco_class_ctors[0] = JSDictionary::define(ctx,
                                                   "Dictionary",
                                                   js_dictionary_ctor,
                                                   2,
                                                   js_dictionary_proto_funcs,
                                                   countof(js_dictionary_proto_funcs),
                                                   JS_UNDEFINED,
                                                   js_dictionary_static_funcs,
                                                   countof(js_dictionary_static_funcs));
    js_aruco_class_ctors[1] = JSDetectorParameters::define(ctx, "DetectorParameters", js_dp_ctor, 0, js_params_tag_funcs[0], 1);
    js_aruco_define_fields(ctx, JSDetectorParameters::proto, js_dp_fields, countof(js_dp_fields), js_dp_get, js_dp_set);
    js_aruco_class_ctors[2] = JSRefineParameters::define(ctx, "RefineParameters", js_rp_ctor, 3, js_params_tag_funcs[1], 1);
    js_aruco_define_fields(ctx, JSRefineParameters::proto, js_rp_fields, countof(js_rp_fields), js_rp_get, js_rp_set);
    js_aruco_class_ctors[3] = JSCharucoParameters::define(
        ctx, "CharucoParameters", js_cp_ctor, 0, js_charucoparameters_proto_funcs, countof(js_charucoparameters_proto_funcs));
    js_aruco_define_fields(ctx, JSCharucoParameters::proto, js_cp_fields, countof(js_cp_fields), js_cp_get, js_cp_set);
    js_aruco_class_ctors[4] =
        JSArucoDetector::define(ctx, "ArucoDetector", js_arucodetector_ctor, 0, js_arucodetector_proto_funcs, countof(js_arucodetector_proto_funcs));
    js_aruco_class_ctors[5] = JSCharucoDetector::define(
        ctx, "CharucoDetector", js_charucodetector_ctor, 1, js_charucodetector_proto_funcs, countof(js_charucodetector_proto_funcs));
    js_aruco_class_ctors[6] = JSBoard::define(ctx, "Board", js_board_ctor, 3, js_board_proto_funcs, countof(js_board_proto_funcs));
    js_aruco_class_ctors[7] = JSGridBoard::define(
        ctx, "GridBoard", js_gridboard_ctor, 4, js_gridboard_proto_funcs, countof(js_gridboard_proto_funcs), JSBoard::proto);
    js_aruco_class_ctors[8] = JSCharucoBoard::define(
        ctx, "CharucoBoard", js_charucoboard_ctor, 4, js_charucoboard_proto_funcs, countof(js_charucoboard_proto_funcs), JSBoard::proto);
  }

  for(size_t i = 0; i < countof(js_aruco_class_names); i++)
    JS_SetPropertyStr(ctx, aruco, js_aruco_class_names[i], JS_DupValue(ctx, js_aruco_class_ctors[i]));

  JS_SetPropertyStr(ctx, aruco, "PredefinedDictionaryType", JS_NewObject(ctx));
  {
    JSValue t = JS_GetPropertyStr(ctx, aruco, "PredefinedDictionaryType");

    JS_SetPropertyFunctionList(ctx, t, js_aruco_dict_consts, countof(js_aruco_dict_consts));
    JS_FreeValue(ctx, t);
  }
  JS_SetPropertyFunctionList(ctx, aruco, js_aruco_dict_consts, countof(js_aruco_dict_consts));
#endif

  if(m) {
#ifdef HAVE_ARUCO_DETECT
    for(size_t i = 0; i < countof(js_aruco_class_names); i++) {
      std::string name = std::string("aruco_") + js_aruco_class_names[i];

      JS_SetModuleExport(ctx, m, name.c_str(), JS_DupValue(ctx, js_aruco_class_ctors[i]));
    }
#endif
    JS_SetModuleExportList(ctx, m, js_aruco_top_funcs, countof(js_aruco_top_funcs));
    JS_SetModuleExport(ctx, m, "aruco", aruco);
  } else
    JS_FreeValue(ctx, aruco);

  return 0;
}

extern "C" void
js_aruco_export(JSContext* ctx, JSModuleDef* m) {
#ifdef HAVE_ARUCO_DETECT
  for(size_t i = 0; i < countof(js_aruco_class_names); i++) {
    std::string name = std::string("aruco_") + js_aruco_class_names[i];

    JS_AddModuleExport(ctx, m, name.c_str());
  }
#endif
  JS_AddModuleExportList(ctx, m, js_aruco_top_funcs, countof(js_aruco_top_funcs));
  JS_AddModuleExport(ctx, m, "aruco");
}

#if defined(JS_CV_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_aruco
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_aruco_init)))
    return NULL;

  js_aruco_export(ctx, m);
  return m;
}
