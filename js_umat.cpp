#include "jsbindings.hpp"
#include "js_umat.hpp"
#include "js_size.hpp"
#include "js_point.hpp"
#include "js_rect.hpp"
#include "js_array.hpp"
#include "js_typed_array.hpp"
#include "js_alloc.hpp"
#include "geometry.hpp"
#include "util.hpp"
#include "../quickjs/cutils.h"
#include <list>
#include <map>
#include <fstream>

enum {
  PROP_COLS = 0,
  PROP_ROWS,
  PROP_CHANNELS,
  PROP_TYPE,
  PROP_DEPTH,
  PROP_EMPTY,
  PROP_TOTAL,
  PROP_SIZE,
  PROP_CONTINUOUS,
  PROP_SUBMATRIX,
  PROP_STEP,
  PROP_ELEMSIZE,
  PROP_ELEMSIZE1
};

enum {
  METHOD_COL = 0,
  METHOD_ROW,
  METHOD_COL_RANGE,
  METHOD_ROW_RANGE,
  METHOD_CLONE,
  METHOD_ROI,
  METHOD_RELEASE,
  METHOD_DUP,
  METHOD_CLEAR,
  METHOD_RESET,
  METHOD_STEP1,
  METHOD_LOCATE_ROI,
  METHOD_ZERO,
  METHOD_ONE,
  METHOD_ZEROS,
  METHOD_ONES
};

enum { UMAT_EXPR_AND = 0, UMAT_EXPR_OR, UMAT_EXPR_XOR, UMAT_EXPR_MUL };
enum { UMAT_ITERATOR_KEYS, UMAT_ITERATOR_VALUES, UMAT_ITERATOR_ENTRIES };
extern "C" {

static void
js_umat_free_func(JSRuntime* rt, void* opaque, void* ptr) {
  static_cast<JSUMatData*>(opaque)->release();
}
}

typedef struct JSUMatIteratorData {
  JSValue obj, buf;
  uint32_t row, col;
  int magic;
} JSUMatIteratorData;

VISIBLE JSUMatData*
js_umat_data(JSContext* ctx, JSValueConst val) {
  return static_cast<JSUMatData*>(JS_GetOpaque2(ctx, val, js_umat_class_id));
}

static inline std::vector<int>
js_umat_sizes(const JSUMatData& umat) {
  const cv::MatSize size(umat.size);
  std::vector<int> sizes;
  if(umat.dims == 2) {
    sizes.push_back(umat.rows);
    sizes.push_back(umat.cols);
  } else {
    std::copy(&size[0], &size[size.dims()], std::back_inserter(sizes));
  }
  return sizes;
}

static inline std::vector<std::string>
js_umat_dimensions(const JSUMatData& umat) {
  std::vector<int> sizes = js_umat_sizes(umat);
  std::vector<std::string> dimensions;

  std::transform(sizes.cbegin(),
                 sizes.cend(),
                 std::back_inserter(dimensions),
                 static_cast<std::string (*)(int)>(&std::to_string));
  return dimensions;
}

#ifdef DEBUG_UMAT
static inline std::map<void*, std::vector<JSUMatData*>>
js_umat_data(void* data = nullptr) {
  std::map<void*, std::vector<JSUMatData*>> ret;
  for(auto umat : umat_list) {
    const auto u = umat->u;
    if(u != nullptr && (data == nullptr || u == data)) {
      void* data = u;
      if(ret.find(data) == ret.end()) {
        std::vector<JSUMatData*> v{umat};
        ret.insert({data, v});
      } else {
        ret[data].push_back(umat);
      }
    }
  }
  return ret;
}

static inline void
js_umat_print_data(const std::map<void*, std::vector<JSUMatData*>>& data, size_t minSize = 1) {

  for(const auto& [key, value] : data) {

    if(value.size() >= minSize) {
      std::cerr << "data @" << key << " =";

      for(const auto& ptr : value) {
        size_t refcount = ptr->u ? ptr->u->refcount : 0;
        std::cerr << " UMat @" << static_cast<void*>(ptr);
        if(refcount > 1)
          std::cerr << " (refcount=" << refcount << ")";
      }
      std::cerr << std::endl;
    }
  }
}

static inline void
js_umat_dump(JSUMatData* const s) {
  auto posList = std::find(umat_list.cbegin(), umat_list.cend(), s);
  bool inList = posList != umat_list.cend();
  bool inFreed = std::find(umat_freed.cbegin(), umat_freed.cend(), s) != umat_freed.cend();
  const auto u = s->u;
  std::cerr << " UMat"
            << "[" << (posList - umat_list.cbegin()) << "]=" << static_cast<void*>(s);

  if(inList)
    std::cerr << ", inList=" << (inList ? "true" : "false");
  if(inFreed)
    std::cerr << ", inFreed=" << (inFreed ? "true" : "false");

  if(s->rows || s->cols || s->channels() > 1 || s->depth() > 0) {
    std::cerr << ", rows=" << s->rows;
    std::cerr << ", cols=" << s->cols;
    std::cerr << ", channels=" << s->channels();
    std::cerr << ", depth=" << s->depth();
  }

  if(u != nullptr) {

    // if(u->refcount)
    std::cerr << ", refcount=" << u->refcount;
    if(u->data)
      std::cerr << ", data=" << static_cast<void*>(u->data);
    if(u->size)
      std::cerr << ", size=" << u->size;
  }
}
#endif

VISIBLE JSValue
js_umat_new(JSContext* ctx, uint32_t rows, uint32_t cols, int type) {
  JSValue ret;
  JSUMatData* um;

  if(JS_IsUndefined(umat_proto))
    js_umat_init(ctx, NULL);

  ret = JS_NewObjectProtoClass(ctx, umat_proto, js_umat_class_id);

  um = js_allocate<cv::UMat>(ctx);

  if(cols || rows || type) {
    new(um) cv::UMat(rows, cols, type);
  } else {
    new(um) cv::UMat();
  }

  // um->addref();
#ifdef DEBUG_UMAT
  std::cerr << ((cols > 0 || rows > 0) ? "js_umat_new (h,w)" : "js_umat_new      ");
  js_umat_dump(um);
  std::cerr << std::endl;
#endif

  JS_SetOpaque(ret, um);
  return ret;
}

static std::pair<JSSizeData<uint32_t>, int>
js_umat_params(JSContext* ctx, int argc, JSValueConst* argv) {
  JSSizeData<uint32_t> size;
  int32_t type = 0;
  if(argc > 0) {
    if(js_size_read(ctx, argv[0], &size)) {
      argv++;
      argc--;
    } else {
      JS_ToUint32(ctx, &size.height, argv[0]);
      JS_ToUint32(ctx, &size.width, argv[1]);
      argv += 2;
      argc -= 2;
    }
    if(argc > 0) {
      if(!JS_ToInt32(ctx, &type, argv[0])) {
        argv++;
        argc--;
      }
    }
  }
  return std::make_pair(size, type);
}

static JSValue
js_umat_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {

  const auto& [size, type] = js_umat_params(ctx, argc, argv);

  return js_umat_new(ctx, size.height, size.width, type);
}

void
js_umat_finalizer(JSRuntime* rt, JSValue val) {
  JSUMatData* um;

  if((um = static_cast<JSUMatData*>(JS_GetOpaque(val, js_umat_class_id)))) {
    um->release();
    js_deallocate(rt, um);
  }
  // JS_FreeValueRT(rt, val);
}

static JSValue
js_umat_funcs(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValue ret = JS_UNDEFINED;
  int64_t i = -1, i2 = -1;
  JSPointData<double> pt;
  JSUMatData* um = js_umat_data(ctx, this_val);

  if(argc > 0) {
    JS_ToInt64(ctx, &i, argv[0]);
    pt = js_point_get(ctx, argv[0]);
    if(argc > 1) {
      JS_ToInt64(ctx, &i2, argv[1]);
    }
  }
  switch(magic) {
    case METHOD_COL: {
      ret = js_umat_wrap(ctx, um->col(i));
      break;
    }
    case METHOD_ROW: {
      ret = js_umat_wrap(ctx, um->row(i));
      break;
    }
    case METHOD_COL_RANGE: {
      ret = js_umat_wrap(ctx, um->colRange(i, i2));
      break;
    }
    case METHOD_ROW_RANGE: {
      ret = js_umat_wrap(ctx, um->rowRange(i, i2));
      break;
    }
    case METHOD_CLONE: {
      ret = js_umat_wrap(ctx, um->clone());
      break;
    }
    case METHOD_ROI: {
      JSRectData<double> rect = {0, 0, 0, 0};

      if(argc > 0)
        rect = js_rect_get(ctx, argv[0]);

      ret = js_umat_wrap(ctx, (*um)(rect));
      break;
    }
    case METHOD_RELEASE: {
      um->release();
      break;
    }
    case METHOD_DUP: {
      ret = js_umat_wrap(ctx, *um);
      break;
    }
    case METHOD_CLEAR: {
      *um = cv::UMat::zeros(um->rows, um->cols, um->type());
      break;
    }
    case METHOD_RESET: {
      *um = cv::UMat();
      break;
    }
    case METHOD_STEP1: {
      int32_t i = 0;

      JS_ToInt32(ctx, &i, argv[0]);
      ret = JS_NewInt64(ctx, um->step1(i));
      break;
    }
    case METHOD_LOCATE_ROI: {
      cv::Size wholeSize;
      cv::Point ofs;
      um->locateROI(wholeSize, ofs);
      js_size_write(ctx, argv[0], wholeSize);
      js_point_write(ctx, argv[0], ofs);
      break;
    }

    default: {
      ret = JS_EXCEPTION;
      break;
    }
  }

  return ret;
}

static JSValue
js_umat_init(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValue ret = JS_UNDEFINED;
  JSUMatData* um;

  auto [size, type] = js_umat_params(ctx, argc, argv);

  if((um = js_umat_data(ctx, this_val))) {
    if(size.width == 0 || size.height == 0) {
      if(um->rows && um->cols) {
        size.width = um->cols;
        size.height = um->rows;
        type = um->type();
      }
    }
    ret = JS_DupValue(ctx, this_val);
  } else if(size.width > 0 && size.height > 0) {
    ret = js_umat_new(ctx, size.height, size.width, type);

    um = js_umat_data(ctx, ret);
  } else {
    return JS_EXCEPTION;
  }

  cv::UMat& umat = *um;

  switch(magic) {
    case 0: {
      umat = cv::UMat::zeros(size, type);
      break;
    }
    case 1: {
      umat = cv::UMat::ones(size, type);
      break;
    }
  }

  return ret;
}

template<class T>
void
js_umat_get(JSContext* ctx, JSValueConst this_val, uint32_t row, uint32_t col, T& value) {
  cv::UMat* um = js_umat_data(ctx, this_val);

  if(um)
    value = mat_at<T>(*um, row, col);
  else
    value = T();
}

static JSValue
js_umat_get(JSContext* ctx, JSValueConst this_val, uint32_t row, uint32_t col) {
  JSValue ret = JS_EXCEPTION;
  cv::UMat* um = js_umat_data(ctx, this_val);

  if(um) {
    uint32_t bytes = (1 << um->depth()) * um->channels();
    size_t channels = mat_channels(*um);

    if(channels == 1) {
      switch(um->type()) {

        case CV_8UC1: {
          uint8_t value;
          js_umat_get(ctx, this_val, row, col, value);
          ret = JS_NewUint32(ctx, value);
          break;
        }

        case CV_16UC1: {
          uint16_t value;
          js_umat_get(ctx, this_val, row, col, value);
          ret = JS_NewUint32(ctx, value);
          break;
        }
        case CV_32SC1: {
          int32_t value;
          js_umat_get(ctx, this_val, row, col, value);
          ret = JS_NewInt32(ctx, value);
          break;
        }
        case CV_32FC1: {
          float value;
          js_umat_get(ctx, this_val, row, col, value);
          ret = JS_NewFloat64(ctx, value);
          break;
        }
        case CV_64FC1: {
          double value;
          js_umat_get(ctx, this_val, row, col, value);
          ret = JS_NewFloat64(ctx, value);
          break;
        }
        default: {
          ret = JS_ThrowTypeError(ctx, "Invalid UMat type %u", um->type());
          break;
        }
      }

      return ret;
    }
  }
  return JS_UNDEFINED;
}

static int
js_umat_get_wh(JSContext* ctx, JSMatDimensions* size, JSValueConst obj) {
  cv::UMat* um = js_umat_data(ctx, obj);

  if(um) {
    size->rows = um->rows;
    size->cols = um->cols;
    return 1;
  }
  return 0;
}

static JSValue
js_umat_at(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSUMatData* um = js_umat_data(ctx, this_val);
  if(!um)
    return JS_EXCEPTION;
  JSPointData<double> pt;
  JSValue ret;
  uint32_t row = 0, col = 0;
  if(js_point_read(ctx, argv[0], &pt)) {
    col = pt.x;
    row = pt.y;
  } else if(argc >= 2 && JS_IsNumber(argv[0]) && JS_IsNumber(argv[1])) {
    JS_ToUint32(ctx, &row, argv[0]);
    JS_ToUint32(ctx, &col, argv[1]);
    argc -= 2;
    argv += 2;
  } else if(argc >= 1 && JS_IsNumber(argv[0])) {
    JSMatDimensions dim = {static_cast<uint32_t>(um->rows), static_cast<uint32_t>(um->cols)};
    uint32_t idx;

    JS_ToUint32(ctx, &idx, argv[0]);
    row = idx / dim.cols;
    col = idx % dim.cols;
  }

  return js_umat_get(ctx, this_val, row, col);
}

static JSValue
js_umat_set(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  cv::UMat* um = js_umat_data(ctx, this_val);
  uint32_t bytes;
  if(!um)
    return JS_EXCEPTION;

  JSPointData<double> pt;
  JSValue ret;
  int32_t col = -1, row = -1;

  if(js_point_read(ctx, argv[0], &pt)) {
    col = pt.x;
    row = pt.y;
    argc--;
    argv++;
  } else {
    if(argc >= 1) {
      JS_ToInt32(ctx, &row, argv[0]);
      argc--;
      argv++;
    }
    if(argc >= 1) {
      JS_ToInt32(ctx, &col, argv[0]);
      argc--;
      argv++;
    }
  }
  bytes = (1 << um->depth()) * um->channels();
  if(um->type() == CV_32FC1) {
    double data;
    if(JS_ToFloat64(ctx, &data, argv[0]))
      return JS_EXCEPTION;
    mat_at<float>(*um, row, col) = (float)data;
  } else if(bytes <= sizeof(uint)) {
    uint32_t mask = (1LU << (bytes * 8)) - 1;
    uint32_t data;
    if(JS_ToUint32(ctx, &data, argv[0]))
      return JS_EXCEPTION;

    if(bytes <= 1) {
      uint8_t* p = &mat_at<uint8_t>(*um, row, col);
      *p = (uint8_t)data & mask;
    } else if(bytes <= 2) {
      uint16_t* p = &mat_at<uint16_t>(*um, row, col);
      *p = (uint16_t)data & mask;

    } else if(bytes <= 4) {
      uint* p = &mat_at<uint>(*um, row, col);
      *p = (uint)data & mask;
    }

  } else
    return JS_UNDEFINED;
  return JS_UNDEFINED;
}

template<class T>
typename std::enable_if<std::is_integral<T>::value, void>::type
js_umat_vector_get(JSContext* ctx, int argc, JSValueConst* argv, std::vector<T>& output, std::vector<bool>& defined) {
  output.resize(static_cast<size_t>(argc));
  defined.resize(static_cast<size_t>(argc));
  for(int i = 0; i < argc; i++) {
    uint32_t val = 0;
    bool isDef = JS_IsNumber(argv[i]) && !JS_ToUint32(ctx, &val, argv[i]);

    output[i] = val;
    defined[i] = isDef;
  }
}

template<class T>
typename std::enable_if<std::is_floating_point<T>::value, void>::type
js_umat_vector_get(JSContext* ctx, int argc, JSValueConst* argv, std::vector<T>& output, std::vector<bool>& defined) {
  output.resize(static_cast<size_t>(argc));
  defined.resize(static_cast<size_t>(argc));
  for(int i = 0; i < argc; i++) {
    double val = 0;
    bool isDef = JS_IsNumber(argv[i]) && !JS_ToFloat64(ctx, &val, argv[i]);

    output[i] = val;
    defined[i] = isDef;
  }
}

template<class T>
typename std::enable_if<std::is_integral<typename T::value_type>::value, void>::type
js_umat_vector_get(JSContext* ctx, int argc, JSValueConst* argv, std::vector<T>& output, std::vector<bool>& defined) {
  const size_t bits = (sizeof(typename T::value_type) * 8);
  const size_t n = T::channels;
  output.resize(static_cast<size_t>(argc));
  defined.resize(static_cast<size_t>(argc));
  for(int i = 0; i < argc; i++) {
    double val = 0;
    bool isDef = JS_IsNumber(argv[i]) && !JS_ToFloat64(ctx, &val, argv[i]);
    if(isDef) {
      const uint64_t mask = (1U << bits) - 1;
      uint64_t ival = val;
      for(int j = 0; j < n; j++) {
        output[i][j] = ival & mask;
        ival >>= bits;
      }
    }
    defined[i] = isDef;
  }
};

template<class T>
static std::vector<T>
js_umat_set_vector(JSContext* ctx, JSUMatData* um, int argc, JSValueConst* argv) {
  JSMatDimensions dim = {static_cast<uint32_t>(um->rows), static_cast<uint32_t>(um->cols)};
  uint32_t idx;
  std::vector<bool> defined;
  std::vector<T> v;
  js_umat_vector_get(ctx, argc, argv, v, defined);

  for(idx = 0; idx < v.size(); idx++)
    if(defined[idx])
      mat_at<T>(*um, idx / dim.cols, idx % dim.cols) = v[idx];
  return v;
}

static JSValue
js_umat_set_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSUMatData* m = js_umat_data(ctx, this_val);
  uint32_t bytes;
  std::vector<bool> defined;

  if(!m)
    return JS_EXCEPTION;

  if(m->channels() == 1) {
    double value;
    js_value_to(ctx, argv[0], value);
    m->setTo(cv::Scalar(value));

  } else if(js_is_array(ctx, argv[0])) {
    cv::Scalar s;
    size_t n = js_array_to(ctx, argv[0], s);
    if(n >= m->channels()) {
      m->setTo(s);
      return JS_UNDEFINED;
    }
  }

  bytes = m->elemSize();

  return JS_UNDEFINED;
}

static JSValue
js_umat_get_props(JSContext* ctx, JSValueConst this_val, int magic) {
  cv::UMat* um = js_umat_data(ctx, this_val);
  if(!um)
    return JS_EXCEPTION;

  switch(magic) {
    case PROP_COLS: return JS_NewUint32(ctx, um->cols);
    case PROP_ROWS: return JS_NewUint32(ctx, um->rows);
    case PROP_CHANNELS: return JS_NewUint32(ctx, um->channels());
    case PROP_TYPE: return JS_NewUint32(ctx, um->type());
    case PROP_DEPTH: return JS_NewUint32(ctx, um->depth());
    case PROP_EMPTY: return JS_NewBool(ctx, um->empty());
    case PROP_TOTAL: return JS_NewFloat64(ctx, um->total());
    case PROP_SIZE: return js_size_new(ctx, um->cols, um->rows);
    case PROP_CONTINUOUS: return JS_NewBool(ctx, um->isContinuous());
    case PROP_SUBMATRIX: return JS_NewBool(ctx, um->isSubmatrix());
    case PROP_STEP: return JS_NewUint32(ctx, um->step);
    case PROP_ELEMSIZE: return JS_NewUint32(ctx, um->elemSize());
    case PROP_ELEMSIZE1: return JS_NewUint32(ctx, um->elemSize1());
  }

  return JS_UNDEFINED;
}

static JSValue
js_umat_tostring(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  cv::UMat* um = js_umat_data(ctx, this_val);
  int x, y;

  std::ostringstream os;
  std::string str;
  int i = 0;
  if(!um)
    return JS_EXCEPTION;

  os << "UMat(";

  if(false && (um->rows > 0 && um->cols > 0) && um->dims == 2) {
    os << um->rows << ", " << um->cols;
  } else {
    std::vector<std::string> sizeStrs = js_umat_dimensions(*um);
    os << "[" << join(sizeStrs.cbegin(), sizeStrs.cend(), ", ") << "]";
  }

  if(um->depth() == CV_8U || um->channels() > 1) {
    os << ", ";
    const char* tstr;
    switch(um->depth() & 7) {
      case CV_8U: tstr = "CV_8U"; break;
      case CV_8S: tstr = "CV_8S"; break;
      case CV_16U: tstr = "CV_16U"; break;
      case CV_16S: tstr = "CV_16S"; break;
      case CV_32S: tstr = "CV_32S"; break;
      case CV_32F: tstr = "CV_32F"; break;
      case CV_64F: tstr = "CV_64F"; break;
    }
    os << tstr << 'C' << um->channels() << ")" /*<< std::endl*/;
  } else {
    os << "UMat[";
    for(y = 0; y < um->rows; y++) {
      os << "\n  ";

      for(x = 0; x < um->cols; x++) {
        if(x > 0)
          os << ',';
        if(um->type() == CV_32FC1)
          os << mat_at<float>(*um, y, x);
        else
          os << std::setfill('0') << std::setbase(16) << std::setw(um->type() == CV_8UC4 ? 8 : um->type() == CV_8UC1 ? 2 : 6)
             << mat_at<uint32_t>(*um, y, x);
      }
    }

    os << ']' /*<< std::endl*/;
  }
  os << ' ';
  os << (void*)um->elemSize();
  os << "x";
  os << (void*)um->total();
  os << " @";
  os << (void*)mat_ptr(*um);

  str = os.str();

  return JS_NewStringLen(ctx, str.data(), str.size());
}

static JSValue
js_umat_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  cv::UMat* um = js_umat_data(ctx, this_val);
  int x, y;

  std::ostringstream os;
  std::string str;
  int i = 0;
  if(!um)
    return JS_EXCEPTION;

  int bytes = 1 << ((um->type() & 0x7) >> 1);
  char sign = (um->type() & 0x7) >= 5 ? 'F' : (um->type() & 1) ? 'S' : 'U';

  std::vector<std::string> sizeStrs = js_umat_dimensions(*um);
  ;

  os << "UMat "
     /*     << "@ "
          << reinterpret_cast<void*>(reinterpret_cast<char*>(um)  )*/
     << " [ ";
  if(sizeStrs.size() || um->type()) {
    os << "size: " COLOR_YELLOW "" << join(sizeStrs.cbegin(), sizeStrs.cend(), "" COLOR_NONE "*" COLOR_YELLOW "")
       << "" COLOR_NONE ", ";
    os << "type: " COLOR_YELLOW "CV_" << (bytes * 8) << sign << 'C' << um->channels() << "" COLOR_NONE ", ";
    os << "elemSize: " COLOR_YELLOW "" << um->elemSize() << "" COLOR_NONE ", ";
    os << "elemSize1: " COLOR_YELLOW "" << um->elemSize1() << "" COLOR_NONE ", ";
    os << "total: " COLOR_YELLOW "" << um->total() << "" COLOR_NONE ", ";
    os << "dims: " COLOR_YELLOW "" << um->dims << "" COLOR_NONE "";
  } else {
    os << "empty";
  }
  if(um->u)
    os << ", refcount: " COLOR_YELLOW "" << um->u->refcount;
  os << "" COLOR_NONE " ]";
  str = os.str();
  return JS_NewStringLen(ctx, str.data(), str.size());
}

static JSValue
js_umat_convert_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSUMatData *um, *output;
  int32_t rtype;
  double alpha = 1, beta = 0;

  um = js_umat_data(ctx, this_val);
  output = js_umat_data(ctx, argv[0]);

  if(um == nullptr || output == nullptr)
    return JS_EXCEPTION;

  JS_ToInt32(ctx, &rtype, argv[1]);

  if(argc >= 3)
    JS_ToFloat64(ctx, &alpha, argv[2]);

  if(argc >= 4)
    JS_ToFloat64(ctx, &beta, argv[3]);

  um->convertTo(*output, rtype, alpha, beta);

  return JS_UNDEFINED;
}

static JSValue
js_umat_copy_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSUMatData *um = nullptr, *output = nullptr, *mask = nullptr;

  um = js_umat_data(ctx, this_val);
  output = js_umat_data(ctx, argv[0]);

  if(argc > 1)
    mask = js_umat_data(ctx, argv[1]);

  if(um == nullptr || output == nullptr)
    return JS_EXCEPTION;

  if(mask)
    um->copyTo(*output, *mask);
  else
    um->copyTo(*output);
  /*if(mask)
    um->copyTo(*output, *mask);
  else
    *output = *um;*/

  return JS_UNDEFINED;
}

static JSValue
js_umat_reshape(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSUMatData *um, umat;
  int32_t cn, rows = 0;
  JSValue ret = JS_EXCEPTION;

  um = js_umat_data(ctx, this_val);

  if(um == nullptr || argc < 1)
    return ret;

  if(argc >= 1 && JS_IsNumber(argv[0])) {
    JS_ToInt32(ctx, &cn, argv[0]);
    argv++;
    argc--;
  } else {
    cn = um->channels();
  }

  if(argc >= 1) {
    std::vector<int> newshape;
    if(js_is_array(ctx, argv[0])) {
      js_array_to(ctx, argv[0], newshape);
      if(argc >= 2 && JS_IsNumber(argv[1])) {
        uint32_t ndims;
        JS_ToUint32(ctx, &ndims, argv[1]);
        if(ndims > newshape.size())
          return JS_EXCEPTION;
        umat = um->reshape(cn, ndims, &newshape[0]);
      } else {
        umat = um->reshape(cn, newshape.size(), &newshape[0]);
      }
    } else if(JS_IsNumber(argv[0])) {
      JS_ToInt32(ctx, &rows, argv[0]);
      umat = um->reshape(cn, rows);
    }
    ret = js_umat_wrap(ctx, umat);
  }

  return ret;
}

static JSValue
js_umat_getmat(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSUMatData* um;
  JSMatData mat;
  int32_t accessFlags;
  JSValue ret = JS_EXCEPTION;

  um = js_umat_data(ctx, this_val);

  if(um == nullptr || argc < 1)
    return ret;

  if(js_umat_class_id == 0)
    return JS_NULL;

  JS_ToInt32(ctx, &accessFlags, argv[0]);

  mat = um->getMat(cv::AccessFlag(accessFlags));

  return js_mat_wrap(ctx, mat);
}

static JSValue
js_umat_fill(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSUMatData* um;

  um = js_umat_data(ctx, this_val);

  if(um == nullptr)
    return JS_EXCEPTION;

  if(um->empty() || um->rows == 0 || um->cols == 0)
    return JS_EXCEPTION;

  cv::UMat& umat = *um;

  switch(magic) {
    case 0: {
      umat = cv::UMat::zeros(umat.rows, umat.cols, umat.type());
      break;
    }
    case 1: {
      umat = cv::UMat::ones(umat.rows, umat.cols, umat.type());
      break;
    }
  }

  return JS_DupValue(ctx, this_val);
}

static JSValue
js_umat_class_create(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValue ret = JS_UNDEFINED;

  const auto& [size, type] = js_umat_params(ctx, argc, argv);

  if(size.width == 0 || size.height == 0)
    return JS_EXCEPTION;

  ret = js_umat_new(ctx, uint32_t(0), uint32_t(0), int(0));
  JSUMatData& umat = *js_umat_data(ctx, ret);

  switch(magic) {
    case 0: {
      umat = cv::Scalar::all(0);
      break;
    }
    case 1: {
      umat = cv::Scalar::all(1);
      break;
    }
  }

  return ret;
}

/*static JSValue
js_umat_create_vec(JSContext* ctx, int len, JSValue* vec) {
  JSValue obj = JS_EXCEPTION;
  int i;

  obj = JS_NewArray(ctx);
  if(!JS_IsException(obj)) {

    for(i = 0; i < len; i++) {

      if(JS_SetPropertyUint32(ctx, obj, i, vec[i]) < 0) {
        JS_FreeValue(ctx, obj);
        return JS_EXCEPTION;
      }
    }
  }
  return obj;
}*/

static JSValue
js_umat_buffer(JSContext* ctx, JSValueConst this_val) {
  JSUMatData* um;
  cv::UMatData* data;
  size_t size;
  uint8_t* ptr;

  if((um = js_umat_data(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  data = um->u;
  ptr = data ? data->data : nullptr;
  size = data ? data->size : 0;

  if(ptr == nullptr)
    return JS_NULL;

  um->addref();
  // um->addref();

  return JS_NewArrayBuffer(ctx, ptr, size, &js_umat_free_func, um, FALSE /*TRUE*/);
}

static JSValue
js_umat_array(JSContext* ctx, JSValueConst this_val) {
  JSUMatData* um;
  JSValueConst global, typed_array, buffer;
  int elem_size;
  const char* ctor;

  if((um = js_umat_data(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  elem_size = um->elemSize();

  global = JS_GetGlobalObject(ctx);

  printf("um->type()=%x um->channels()=%x\n", um->type(), um->channels());
  switch(um->type()) {
    case CV_8U:
    case CV_8S: ctor = "Uint8Array"; break;
    case CV_16U:
    case CV_16S: ctor = "Uint16Array"; break;
    case CV_32S: ctor = "Int32Array"; break;
    case CV_32F: ctor = "Float32Array"; break;
    case CV_64F: ctor = "Float64Array"; break;
    default: JS_ThrowTypeError(ctx, "cv:UMat type=%02x channels=%02x", um->type(), um->channels()); return JS_EXCEPTION;
  }
  typed_array = JS_GetPropertyStr(ctx, global, ctor);
  buffer = js_umat_get_props(ctx, this_val, 9);

  return JS_CallConstructor(ctx, typed_array, 1, &buffer);
}

JSValue
js_umat_call(JSContext* ctx, JSValueConst func_obj, JSValueConst this_val, int argc, JSValueConst* argv, int flags) {
  cv::Rect rect = {0, 0, 0, 0};
  JSUMatData* src;

  if((src = js_umat_data(ctx, func_obj)) == nullptr)
    return JS_EXCEPTION;

  if(!js_rect_read(ctx, argv[0], &rect))
    return JS_ThrowTypeError(ctx, "argument 1 expecting Rect");
  // printf("js_mat_call %u,%u %ux%u\n", rect.x, rect.y, rect.width, rect.height);

  cv::UMat umat = src->operator()(rect);
  return js_umat_wrap(ctx, umat);
}

JSClassDef js_umat_class = {
    .class_name = "UMat",
    .finalizer = js_umat_finalizer,
    .call = js_umat_call,
};

const JSCFunctionListEntry js_umat_proto_funcs[] = {
    JS_CGETSET_MAGIC_DEF("cols", js_umat_get_props, NULL, PROP_COLS),
    JS_CGETSET_MAGIC_DEF("rows", js_umat_get_props, NULL, PROP_ROWS),
    JS_CGETSET_MAGIC_DEF("channels", js_umat_get_props, NULL, PROP_CHANNELS),
    JS_CGETSET_MAGIC_DEF("type", js_umat_get_props, NULL, PROP_TYPE),
    JS_CGETSET_MAGIC_DEF("depth", js_umat_get_props, NULL, PROP_DEPTH),
    JS_CGETSET_MAGIC_DEF("empty", js_umat_get_props, NULL, PROP_EMPTY),
    JS_CGETSET_MAGIC_DEF("total", js_umat_get_props, NULL, PROP_TOTAL),
    JS_CGETSET_MAGIC_DEF("size", js_umat_get_props, NULL, PROP_SIZE),
    JS_CGETSET_MAGIC_DEF("continuous", js_umat_get_props, NULL, PROP_CONTINUOUS),
    JS_CGETSET_MAGIC_DEF("submatrix", js_umat_get_props, NULL, PROP_SUBMATRIX),
    JS_CGETSET_MAGIC_DEF("step", js_umat_get_props, NULL, PROP_STEP),
    JS_CGETSET_MAGIC_DEF("elemSize", js_umat_get_props, NULL, PROP_ELEMSIZE),
    JS_CGETSET_MAGIC_DEF("elemSize1", js_umat_get_props, NULL, PROP_ELEMSIZE1),
    JS_CGETSET_DEF("buffer", js_umat_buffer, NULL),
    JS_CGETSET_DEF("array", js_umat_array, NULL),
    JS_CFUNC_MAGIC_DEF("col", 1, js_umat_funcs, METHOD_COL),
    JS_CFUNC_MAGIC_DEF("row", 1, js_umat_funcs, METHOD_ROW),
    JS_CFUNC_MAGIC_DEF("colRange", 2, js_umat_funcs, METHOD_COL_RANGE),
    JS_CFUNC_MAGIC_DEF("rowRange", 2, js_umat_funcs, METHOD_ROW_RANGE),
    JS_CFUNC_MAGIC_DEF("clone", 0, js_umat_funcs, METHOD_CLONE),
    JS_CFUNC_MAGIC_DEF("roi", 0, js_umat_funcs, METHOD_ROI),
    JS_CFUNC_MAGIC_DEF("release", 0, js_umat_funcs, METHOD_RELEASE),
    JS_CFUNC_MAGIC_DEF("dup", 0, js_umat_funcs, METHOD_DUP),
    JS_CFUNC_MAGIC_DEF("clear", 0, js_umat_funcs, METHOD_CLEAR),
    JS_CFUNC_MAGIC_DEF("reset", 0, js_umat_funcs, METHOD_RESET),
    JS_CFUNC_MAGIC_DEF("step1", 0, js_umat_funcs, METHOD_STEP1),
    JS_CFUNC_MAGIC_DEF("locateROI", 0, js_umat_funcs, METHOD_LOCATE_ROI),

    JS_CFUNC_MAGIC_DEF("zero", 2, js_umat_fill, 0),
    JS_CFUNC_MAGIC_DEF("one", 2, js_umat_fill, 1),

    JS_CFUNC_DEF("toString", 0, js_umat_tostring),
    JS_CFUNC_DEF("inspect", 0, js_umat_inspect),
    JS_CFUNC_DEF("at", 1, js_umat_at),
    JS_CFUNC_DEF("set", 2, js_umat_set),
    JS_CFUNC_DEF("setTo", 0, js_umat_set_to),
    JS_CFUNC_DEF("convertTo", 2, js_umat_convert_to),
    JS_CFUNC_DEF("copyTo", 1, js_umat_copy_to),
    JS_CFUNC_DEF("reshape", 1, js_umat_reshape),
    JS_CFUNC_DEF("getMat", 1, js_umat_getmat),
    JS_ALIAS_DEF("[Symbol.toPrimitive]", "toString"),

    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "UMat", JS_PROP_CONFIGURABLE)

};

const JSCFunctionListEntry js_umat_static_funcs[] = {
    JS_CFUNC_MAGIC_DEF("zeros", 1, js_umat_class_create, 0),
    JS_CFUNC_MAGIC_DEF("ones", 1, js_umat_class_create, 1),
    JS_PROP_INT32_DEF("CV_8U", CV_MAKETYPE(CV_8U, 1), JS_PROP_ENUMERABLE),
};

int
js_umat_init(JSContext* ctx, JSModuleDef* m) {
  if(js_umat_class_id == 0) {
    /* create the UMat class */
    JS_NewClassID(&js_umat_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_umat_class_id, &js_umat_class);

    umat_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, umat_proto, js_umat_proto_funcs, countof(js_umat_proto_funcs));
    JS_SetClassProto(ctx, js_umat_class_id, umat_proto);

    umat_class = JS_NewCFunction2(ctx, js_umat_ctor, "UMat", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, umat_class, umat_proto);

    JS_SetPropertyFunctionList(ctx, umat_class, js_umat_static_funcs, countof(js_umat_static_funcs));

    /*  JSValue g = JS_GetGlobalObject(ctx);
      int32array_ctor = JS_GetProperty(ctx, g, JS_ATOM_Int32Array);
      int32array_proto = JS_GetPrototype(ctx, int32array_ctor);

      JS_FreeValue(ctx, g);*/
  }

  if(m)
    JS_SetModuleExport(ctx, m, "UMat", umat_class);
  return 0;
}

extern "C" VISIBLE void
js_umat_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "UMat");
}

#if defined(JS_UMAT_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_umat
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_umat_init);
  if(!m)
    return NULL;
  js_umat_export(ctx, m);
  return m;
}