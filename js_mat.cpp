#include "jsbindings.hpp"
#include "js_cv.hpp"
#include "js_mat.hpp"
#include "js_size.hpp"
#include "js_point.hpp"
#include "js_rect.hpp"
#include "js_array.hpp"
#include "js_typed_array.hpp"
#include "js_alloc.hpp"
#include "js_umat.hpp"
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
  METHOD_RESIZE,
  METHOD_STEP1,
  METHOD_LOCATE_ROI,
  METHOD_PTR
};
enum { MAT_EXPR_AND = 0, MAT_EXPR_OR, MAT_EXPR_XOR, MAT_EXPR_MUL };
enum { MAT_ITERATOR_KEYS, MAT_ITERATOR_VALUES, MAT_ITERATOR_ENTRIES };
extern "C" {
JSValue mat_proto = JS_UNDEFINED, mat_class = JS_UNDEFINED, mat_iterator_proto = JS_UNDEFINED,
        mat_iterator_class = JS_UNDEFINED;
JSClassID js_mat_class_id = 0, js_mat_iterator_class_id = 0;

JSValue umat_proto = JS_UNDEFINED, umat_class = JS_UNDEFINED;
JSClassID js_umat_class_id = 0;

typedef struct JSMatIteratorData {
  JSValue obj, buf;
  uint32_t row, col;
  int magic;
  TypedArrayType type;
} JSMatIteratorData;

static void
js_mat_free_func(JSRuntime* rt, void* opaque, void* ptr) {
  static_cast<JSMatData*>(opaque)->release();
}
}

static std::vector<JSMatData*> mat_list;
static std::list<JSMatData*> mat_freed;

static inline std::vector<int>
js_mat_sizes(const JSMatData& mat) {
  const cv::MatSize size(mat.size);
  std::vector<int> sizes;
  if(mat.dims == 2) {
    sizes.push_back(mat.rows);
    sizes.push_back(mat.cols);
  } else {
    std::copy(&size[0], &size[size.dims()], std::back_inserter(sizes));
  }
  return sizes;
}

static inline std::vector<std::string>
js_mat_dimensions(const JSMatData& mat) {
  std::vector<int> sizes = js_mat_sizes(mat);
  std::vector<std::string> dimensions;

  std::transform(sizes.cbegin(),
                 sizes.cend(),
                 std::back_inserter(dimensions),
                 static_cast<std::string (*)(int)>(&std::to_string));
  return dimensions;
}

static inline JSMatData*
js_mat_track(JSContext* ctx, JSMatData* s) {
  std::vector<cv::Mat*> deallocate;

  for(;;) {
    auto it2 = std::find(mat_freed.cbegin(), mat_freed.cend(), s);
    if(it2 != mat_freed.cend()) {
      deallocate.push_back(s);

      // std::cerr << "allocated @" << static_cast<void*>(s) << " which is in free list" <<
      // std::endl;

      // mat_freed.erase(it2);
      s = js_allocate<cv::Mat>(ctx);
      memcpy(s, deallocate[deallocate.size() - 1], sizeof(JSMatData));

    } else {
      break;
    }
  }

  mat_list.push_back(s);

  for(const auto& ptr : deallocate) js_deallocate(ctx, ptr);
  return s;
}

VISIBLE JSValue
js_mat_new(JSContext* ctx, uint32_t rows, uint32_t cols, int type) {
  JSValue ret;
  JSMatData* s;
  if(JS_IsUndefined(mat_proto))
    js_mat_init(ctx, NULL);
  ret = JS_NewObjectProtoClass(ctx, mat_proto, js_mat_class_id);
  s = js_mat_track(ctx, js_allocate<cv::Mat>(ctx));
  if(cols || rows || type) {
    new(s) cv::Mat(rows, cols, type);
  } else {
    new(s) cv::Mat();
  }
#ifdef DEBUG_MAT
  std::cerr << ((cols > 0 || rows > 0) ? "js_mat_new (h,w)" : "js_mat_new      ");
  js_mat_dump(s);
  std::cerr << std::endl;
#endif
  JS_SetOpaque(ret, s);
  return ret;
}

VISIBLE JSValue
js_mat_wrap(JSContext* ctx, const cv::Mat& mat) {
  JSValue ret;
  JSMatData* s;
  ret = JS_NewObjectProtoClass(ctx, mat_proto, js_mat_class_id);

  s = js_mat_track(ctx, js_allocate<cv::Mat>(ctx));
  new(s) cv::Mat();
  *s = mat;
#ifdef DEBUG_MAT
  std::cerr << "js_mat_wrap     ";
  auto posList = std::find(mat_list.cbegin(), mat_list.cend(), const_cast<JSMatData*>(&mat));
  bool inList;
  if((inList = posList != mat_list.cend())) {
    std::cerr << "arg[" << (posList - mat_list.cbegin()) << "]=" << static_cast<const void*>(&mat);
    std::cerr << ", inList(arg)=" << (inList ? "true" : "false");
  } else {
    std::cerr << "arg=" << static_cast<const void*>(&mat);
  }
  js_mat_dump(s);
  std::cerr << std::endl;
#endif

  JS_SetOpaque(ret, s);
  return ret;
}

#ifdef DEBUG_MAT
static inline std::map<void*, std::vector<JSMatData*>>
js_mat_data(void* data = nullptr) {
  std::map<void*, std::vector<JSMatData*>> ret;
  for(auto mat : mat_list) {
    const auto u = mat->u;
    if(u != nullptr && (data == nullptr || u == data)) {
      void* data = u;
      if(ret.find(data) == ret.end()) {
        std::vector<JSMatData*> v{mat};
        ret.insert({data, v});
      } else {
        ret[data].push_back(mat);
      }
    }
  }
  return ret;
}

static inline void
js_mat_print_data(const std::map<void*, std::vector<JSMatData*>>& data, size_t minSize = 1) {

  for(const auto& [key, value] : data) {

    if(value.size() >= minSize) {
      std::cerr << "data @" << key << " =";

      for(const auto& ptr : value) {
        size_t refcount = ptr->u ? ptr->u->refcount : 0;
        std::cerr << " mat @" << static_cast<void*>(ptr);
        if(refcount > 1)
          std::cerr << " (refcount=" << refcount << ")";
      }
      std::cerr << std::endl;
    }
  }
}

static inline void
js_mat_dump(JSMatData* const s) {
  auto posList = std::find(mat_list.cbegin(), mat_list.cend(), s);
  bool inList = posList != mat_list.cend();
  bool inFreed = std::find(mat_freed.cbegin(), mat_freed.cend(), s) != mat_freed.cend();
  const auto u = s->u;
  std::cerr << " mat"
            << "[" << (posList - mat_list.cbegin()) << "]=" << static_cast<void*>(s);

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

static std::pair<JSSizeData<uint32_t>, int>
js_mat_params(JSContext* ctx, int argc, JSValueConst* argv) {
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
js_mat_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {

  const auto& [size, type] = js_mat_params(ctx, argc, argv);

  return js_mat_new(ctx, size.height, size.width, type);
}

static JSValue
js_mat_funcs(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValue ret = JS_UNDEFINED;
  int64_t i = -1, i2 = -1;
  JSPointData<double> pt;
  JSMatData* m = js_mat_data(ctx, this_val);

  if(argc > 0) {
    JS_ToInt64(ctx, &i, argv[0]);
    pt = js_point_get(ctx, argv[0]);
    if(argc > 1) {
      JS_ToInt64(ctx, &i2, argv[1]);
    }
  }

  switch(magic) {
    case METHOD_COL: {
      ret = js_mat_wrap(ctx, m->col(i));
      break;
    }
    case METHOD_ROW: {
      ret = js_mat_wrap(ctx, m->row(i));
      break;
    }
    case METHOD_COL_RANGE: {
      ret = js_mat_wrap(ctx, m->colRange(i, i2));
      break;
    }
    case METHOD_ROW_RANGE: {
      ret = js_mat_wrap(ctx, m->rowRange(i, i2));
      break;
    }
    case METHOD_CLONE: {
      ret = js_mat_wrap(ctx, m->clone());
      break;
    }
    case METHOD_ROI: {
      JSRectData<double> rect = {0, 0, 0, 0};

      if(argc > 0)
        rect = js_rect_get(ctx, argv[0]);

      ret = js_mat_wrap(ctx, (*m)(rect));
      break;
    }
    case METHOD_RELEASE: {
      m->release();
      break;
    }
    case METHOD_DUP: {
      ret = js_mat_wrap(ctx, *m);
      break;
    }
    case METHOD_CLEAR: {
      *m = cv::Mat::zeros(m->rows, m->cols, m->type());
      break;
    }
    case METHOD_RESET: {
      *m = cv::Mat();
      break;
    }
    case METHOD_RESIZE: {
      uint32_t rows = 0;
      cv::Scalar color{0, 0, 0, 0};

      JS_ToUint32(ctx, &rows, argv[0]);
      if(argc > 1) {
        js_color_read(ctx, argv[1], &color);
        m->resize(rows, color);
      } else {
        m->resize(rows);
      }
      break;
    }
    case METHOD_STEP1: {
      int32_t i = 0;

      JS_ToInt32(ctx, &i, argv[0]);
      ret = JS_NewInt64(ctx, m->step1(i));
      break;
    }
    case METHOD_LOCATE_ROI: {
      cv::Size wholeSize;
      cv::Point ofs;
      m->locateROI(wholeSize, ofs);
      js_size_write(ctx, argv[0], wholeSize);
      js_point_write(ctx, argv[0], ofs);
      break;
    }
    case METHOD_PTR: {
      uint32_t row = 0, col = 0;
      uchar* ptr;
      std::ostringstream os;
      if(argc > 0)
        JS_ToUint32(ctx, &row, argv[0]);
      if(argc > 1)
        JS_ToUint32(ctx, &col, argv[1]);
      ptr = m->ptr<uchar>(row, col);

      os << static_cast<void*>(ptr);

      ret = js_value_from(ctx, os.str());
      break;
    }
  }

  return ret;
}

static JSValue
js_mat_expr(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValue ret = JS_UNDEFINED;
  JSColorData<double> color;
  double value = 0;
  JSMatData *input = nullptr, *output = nullptr, *other = nullptr;
  double scale = 1.0;

  if((input = js_mat_data(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  if(argc < 1)
    return JS_EXCEPTION;

  if(JS_IsNumber(argv[0])) {
    JS_ToFloat64(ctx, &value, argv[0]);

  } else if((other = js_mat_data_nothrow(argv[0])) == nullptr)
    js_color_read(ctx, argv[0], &color);

  if(magic == 3 && argc > 1) {
    JS_ToFloat64(ctx, &scale, argv[1]);
    argv++;
    argc--;
  }

  if(argc > 1)
    output = js_mat_data(ctx, argv[1]);

  if(output == nullptr)
    output = input;

  {
    cv::MatExpr expr;
    cv::Mat tmp(input->rows, input->cols, input->type());

    if(other == nullptr) {
      cv::Mat& mat = *input;

      if(mat.channels() == 1) {

        if(mat.depth() == 0) {
          switch(magic) {
            case MAT_EXPR_AND: mat &= (uchar)value; break;
            case MAT_EXPR_OR: cv::bitwise_or(mat, cv::Scalar(value, value, value, value), mat); break;
            case MAT_EXPR_XOR: mat ^= (uchar)value; break;
            case MAT_EXPR_MUL: mat = mat * value; break;
          }
        } else {
          switch(magic) {
            case MAT_EXPR_AND: mat &= value; break;
            case MAT_EXPR_OR: cv::bitwise_or(mat, cv::Scalar(value, value, value, value), mat); break;
            case MAT_EXPR_XOR: mat ^= value; break;
            case MAT_EXPR_MUL: mat *= value; break;
          }
        }
      } else {
        cv::Scalar& scalar = *reinterpret_cast<cv::Scalar*>(&color);

        // std::cerr << "js_mat_expr input=" << (void*)input << " output=" << (void*)output << " scalar=" <<
        // scalar << std::endl;

        switch(magic) {
          case MAT_EXPR_AND: expr = mat & scalar; break;
          case MAT_EXPR_OR: expr = mat | scalar; break;
          case MAT_EXPR_XOR: expr = mat ^ scalar; break;
          case MAT_EXPR_MUL: expr = mat.mul(scalar, scale); break;
        }
        tmp = static_cast<cv::Mat>(expr);
      }

    } else {

      if(input->rows != other->rows || input->cols != other->cols) {
        ret = JS_ThrowInternalError(ctx, "Mat dimensions mismatch");
      } else if(input->type() != other->type()) {
        ret = JS_ThrowInternalError(ctx, "Mat type mismatch");
      } else if(input->channels() != other->channels()) {
        ret = JS_ThrowInternalError(ctx, "Mat channels mismatch");
      } else {

        if(input == output) {
          switch(magic) {
            case MAT_EXPR_AND: (*input) &= (*other); break;
            case MAT_EXPR_OR: (*input) |= (*other); break;
            case MAT_EXPR_XOR: (*input) ^= (*other); break;
            case MAT_EXPR_MUL: (*input) *= (*other); break;
          }
        } else {
          switch(magic) {
            case MAT_EXPR_AND: (*output) = (*input) & (*other); break;
            case MAT_EXPR_OR: (*output) = (*input) | (*other); break;
            case MAT_EXPR_XOR: (*output) = (*input) ^ (*other); break;
            case MAT_EXPR_MUL: (*output) = (*input) * (*other); break;
          }
        }
      }
    }
  }

  return ret;
}

static JSValue
js_mat_init(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValue ret = JS_UNDEFINED;
  JSMatData* m;

  auto [size, type] = js_mat_params(ctx, argc, argv);

  if((m = js_mat_data(ctx, this_val))) {
    if(size.width == 0 || size.height == 0) {
      if(m->rows && m->cols) {
        size.width = m->cols;
        size.height = m->rows;
        type = m->type();
      }
    }
    ret = JS_DupValue(ctx, this_val);
  } else if(size.width > 0 && size.height > 0) {
    ret = js_mat_new(ctx, size.height, size.width, type);

    m = js_mat_data(ctx, ret);
  } else {
    return JS_EXCEPTION;
  }

  cv::Mat& mat = *m;

  switch(magic) {
    case 0: {
      mat = cv::Mat::zeros(size, type);
      break;
    }
    case 1: {
      mat = cv::Mat::ones(size, type);
      break;
    }
  }

  return ret;
}

template<class T>
void
js_mat_get(JSContext* ctx, JSValueConst this_val, uint32_t row, uint32_t col, T& value) {
  cv::Mat* m = js_mat_data(ctx, this_val);

  if(m)
    value = (*m).at<T>(row, col);
  else
    value = T();
}

static JSValue
js_mat_get(JSContext* ctx, JSValueConst this_val, uint32_t row, uint32_t col) {
  JSValue ret = JS_EXCEPTION;
  cv::Mat* m = js_mat_data(ctx, this_val);

  if(m) {
    uint32_t bytes = m->elemSize();
    size_t channels = mat_channels(*m);
    size_t offset = mat_offset(*m, row, col);

    if(channels == 1) {
      switch(m->type()) {

        case CV_8UC1: {
          uint8_t value;
          js_mat_get(ctx, this_val, row, col, value);
          ret = JS_NewUint32(ctx, value);
          break;
        }

        case CV_16UC1: {
          uint16_t value;
          js_mat_get(ctx, this_val, row, col, value);
          ret = JS_NewUint32(ctx, value);
          break;
        }
        case CV_32SC1: {
          int32_t value;
          js_mat_get(ctx, this_val, row, col, value);
          ret = JS_NewInt32(ctx, value);
          break;
        }
        case CV_32FC1: {
          float value;
          js_mat_get(ctx, this_val, row, col, value);
          ret = JS_NewFloat64(ctx, value);
          break;
        }
        case CV_64FC1: {
          double value;
          js_mat_get(ctx, this_val, row, col, value);
          ret = JS_NewFloat64(ctx, value);
          break;
        }
        default: {
          ret = JS_ThrowTypeError(ctx, "Invalid Mat type %u", m->type());
          break;
        }
      }

    } else {
      JSValue buffer = js_arraybuffer_from(ctx, begin(*m), end(*m));
      TypedArrayType type(*m);

      ret = js_typedarray_new(ctx, buffer, offset, channels, type);
    }
    return ret;
  }
  return JS_UNDEFINED;
}

static int
js_mat_get_wh(JSContext* ctx, JSMatDimensions* size, JSValueConst obj) {
  cv::Mat* m = js_mat_data(ctx, obj);

  if(m) {
    size->rows = m->rows;
    size->cols = m->cols;
    return 1;
  }
  return 0;
}

static JSValue
js_mat_at(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSMatData* m = js_mat_data(ctx, this_val);
  if(!m)
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
    JSMatDimensions dim = {uint32_t(m->rows), uint32_t(m->cols)};
    uint32_t idx;

    JS_ToUint32(ctx, &idx, argv[0]);
    row = idx / dim.cols;
    col = idx % dim.cols;
  }
  return js_mat_get(ctx, this_val, row, col);
}

static JSValue
js_mat_set(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  cv::Mat* m = js_mat_data(ctx, this_val);
  uint32_t bytes;
  if(!m)
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
  bytes = m->elemSize();
  if(m->type() == CV_32FC1) {
    double data;
    if(JS_ToFloat64(ctx, &data, argv[0]))
      return JS_EXCEPTION;
    (*m).at<float>(row, col) = (float)data;
  } else if(bytes <= sizeof(uint)) {
    uint32_t mask = (1LU << (bytes * 8)) - 1;
    uint32_t data;
    if(JS_ToUint32(ctx, &data, argv[0]))
      return JS_EXCEPTION;

    if(bytes <= 1) {
      uint8_t* p = &(*m).at<uint8_t>(row, col);
      *p = (uint8_t)data & mask;
    } else if(bytes <= 2) {
      uint16_t* p = &(*m).at<uint16_t>(row, col);
      *p = (uint16_t)data & mask;

    } else if(bytes <= 4) {
      uint* p = &(*m).at<uint>(row, col);
      *p = (uint)data & mask;
    }

  } else
    return JS_UNDEFINED;
  return JS_UNDEFINED;
}
/*
template<class T>
typename std::enable_if<std::is_integral<T>::value, void>::type
js_mat_vector_get(JSContext* ctx, int argc, JSValueConst* argv, std::vector<T>& output,
std::vector<bool>& defined) { output.resize(static_cast<size_t>(argc));
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
js_mat_vector_get(JSContext* ctx, int argc, JSValueConst* argv, std::vector<T>& output,
std::vector<bool>& defined) { output.resize(static_cast<size_t>(argc));
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
js_mat_vector_get(JSContext* ctx, int argc, JSValueConst* argv, std::vector<T>& output,
std::vector<bool>& defined) { const size_t bits = (sizeof(typename T::value_type) * 8); const size_t
n = T::channels; output.resize(static_cast<size_t>(argc));
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
*/
/*
template<class T>
static std::vector<T>
js_mat_set_vector(JSContext* ctx, JSMatData* m, int argc, JSValueConst* argv) {
  JSMatDimensions dim = {static_cast<uint32_t>(m->rows), static_cast<uint32_t>(m->cols)};
  uint32_t idx;
  std::vector<bool> defined;
  std::vector<T> v;
  js_mat_vector_get(ctx, argc, argv, v, defined);

  for(idx = 0; idx < v.size(); idx++)
    if(defined[idx])
      m->at<T>(idx / dim.cols, idx % dim.cols) = v[idx];
  return v;
}
*/
static JSValue
js_mat_set_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSMatData* m = js_mat_data(ctx, this_val);
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

    // std::cerr << "Scalar [ " << s[0] << ", " << s[1] << ", " << s[2] << ", " << s[3] << " ]" <<
    // std::endl; std::cerr << "Scalar.size() = " << n << std::endl;

    if(n >= m->channels()) {
      m->setTo(s);
      return JS_UNDEFINED;
    }
  }

  bytes = m->elemSize();
  /*  if(m->depth() == CV_16U && m->channels() > 1) {
      if(m->channels() == 2)
        js_mat_set_vector<cv::Vec<uint16_t, 2>>(ctx, m, argc, argv);
      else if(m->channels() == 3)
        js_mat_set_vector<cv::Vec<uint16_t, 3>>(ctx, m, argc, argv);
      else if(m->channels() == 4)
        js_mat_set_vector<cv::Vec<uint16_t, 4>>(ctx, m, argc, argv);
    } else if(m->depth() == CV_32F) {
      if(m->channels() == 1)
        js_mat_set_vector<float>(ctx, m, argc, argv);
    } else if(bytes <= sizeof(uint)) {
      if(bytes <= 1) {
        std::vector<uint8_t> v;
        js_mat_vector_get(ctx, argc, argv, v, defined);
        m->setTo(cv::InputArray(v), defined);
      } else if(bytes <= 2) {
        std::vector<uint16_t> v;
        js_mat_vector_get(ctx, argc, argv, v, defined);
        m->setTo(cv::InputArray(v), defined);
      } else if(bytes <= 4) {

        js_mat_set_vector<uint32_t>(ctx, m, argc, argv);
      } else if(bytes <= 8) {
        js_mat_set_vector<uint64_t>(ctx, m, argc, argv);
      }
    }*/

  return JS_UNDEFINED;
}

static JSValue
js_mat_get_props(JSContext* ctx, JSValueConst this_val, int magic) {
  cv::Mat* m;
  JSValue ret = JS_UNDEFINED;

  if(!(m = js_mat_data(ctx, this_val)))
    return JS_EXCEPTION;

  if(m->empty())
    return JS_NULL;

  switch(magic) {
    case PROP_COLS: {
      ret = JS_NewUint32(ctx, m->cols);
      break;
    }
    case PROP_ROWS: {
      ret = JS_NewUint32(ctx, m->rows);
      break;
    }
    case PROP_CHANNELS: {
      ret = JS_NewUint32(ctx, m->channels());
      break;
    }
    case PROP_TYPE: {
      ret = JS_NewUint32(ctx, m->type());
      break;
    }
    case PROP_DEPTH: {
      ret = JS_NewUint32(ctx, m->depth());
      break;
    }
    case PROP_EMPTY: {
      ret = JS_NewBool(ctx, m->empty());
      break;
    }
    case PROP_TOTAL: {
      ret = JS_NewFloat64(ctx, m->total());
      break;
    }
    case PROP_SIZE: {
      ret = js_size_new(ctx, m->cols, m->rows);
      break;
    }
    case PROP_CONTINUOUS: {
      ret = JS_NewBool(ctx, m->isContinuous());
      break;
    }
    case PROP_SUBMATRIX: {
      ret = JS_NewBool(ctx, m->isSubmatrix());
      break;
    }
    case PROP_STEP: {
      ret = JS_NewUint32(ctx, m->step);
      break;
    }
    case PROP_ELEMSIZE: {
      ret = JS_NewUint32(ctx, m->elemSize());
      break;
    }
    case PROP_ELEMSIZE1: {
      ret = JS_NewUint32(ctx, m->elemSize1());
      break;
    }
  }

  return ret;
}

static JSValue
js_mat_tostring(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  cv::Mat* m = js_mat_data(ctx, this_val);
  int x, y;

  std::ostringstream os;
  std::string str;
  int i = 0;
  if(!m)
    return JS_EXCEPTION;

  os << "Mat(";

  if(false && (m->rows > 0 && m->cols > 0) && m->dims == 2) {
    os << m->rows << ", " << m->cols;
  } else {
    std::vector<std::string> sizeStrs = js_mat_dimensions(*m);
    os << "[" << join(sizeStrs.cbegin(), sizeStrs.cend(), ", ") << "]";
  }

  if(m->depth() == CV_8U || m->channels() > 1) {
    os << ", ";
    const char* tstr;
    switch(m->depth() & 7) {
      case CV_8U: tstr = "CV_8U"; break;
      case CV_8S: tstr = "CV_8S"; break;
      case CV_16U: tstr = "CV_16U"; break;
      case CV_16S: tstr = "CV_16S"; break;
      case CV_32S: tstr = "CV_32S"; break;
      case CV_32F: tstr = "CV_32F"; break;
      case CV_64F: tstr = "CV_64F"; break;
    }
    os << tstr << 'C' << m->channels() << ")" /*<< std::endl*/;
  } else {
    os << "Mat[";
    for(y = 0; y < m->rows; y++) {
      os << "\n  ";

      for(x = 0; x < m->cols; x++) {
        if(x > 0)
          os << ',';
        if(m->type() == CV_32FC1)
          os << m->at<float>(y, x);
        else
          os << std::setfill('0') << std::setbase(16) << std::setw(m->type() == CV_8UC4 ? 8 : m->type() == CV_8UC1 ? 2 : 6)
             << m->at<uint32_t>(y, x);
      }
    }

    os << ']' /*<< std::endl*/;
  }
  os << ' ';
  os << (void*)m->elemSize();
  os << "x";
  os << (void*)m->total();
  os << " @";
  os << (void*)m->ptr();

  str = os.str();

  return JS_NewStringLen(ctx, str.data(), str.size());
}

static JSValue
js_mat_inspect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSMatData* mat = js_mat_data(ctx, this_val);
  JSValue obj = JS_NewObjectProto(ctx, mat_proto);

  JS_DefinePropertyValueStr(ctx, obj, "cols", JS_NewUint32(ctx, mat->cols), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "rows", JS_NewUint32(ctx, mat->rows), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "depth", JS_NewUint32(ctx, mat->depth()), JS_PROP_ENUMERABLE);
  JS_DefinePropertyValueStr(ctx, obj, "channels", JS_NewUint32(ctx, mat->channels()), JS_PROP_ENUMERABLE);
  return obj;
}

static JSValue
js_mat_getrotationmatrix2d(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSPointData<double> s;

  double angle = 0, scale = 1;
  cv::Mat m;

  JSValue ret;
  if(argc == 0)
    return JS_EXCEPTION;
  if(argc > 0) {
    s = js_point_get(ctx, argv[0]);
    if(argc > 1) {
      JS_ToFloat64(ctx, &angle, argv[1]);
      if(argc > 2) {
        JS_ToFloat64(ctx, &scale, argv[2]);
      }
    }
  }

  m = cv::getRotationMatrix2D(s, angle, scale);

  ret = js_mat_wrap(ctx, m);
  return ret;
}

static JSValue
js_mat_convert_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSMatData *m, *output;
  int32_t rtype;
  double alpha = 1, beta = 0;

  m = js_mat_data(ctx, this_val);
  output = js_mat_data(ctx, argv[0]);

  if(m == nullptr || output == nullptr)
    return JS_EXCEPTION;

  JS_ToInt32(ctx, &rtype, argv[1]);

  if(argc >= 3)
    JS_ToFloat64(ctx, &alpha, argv[2]);

  if(argc >= 4)
    JS_ToFloat64(ctx, &beta, argv[3]);

  m->convertTo(*output, rtype, alpha, beta);

  return JS_UNDEFINED;
}

static JSValue
js_mat_copy_to(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSMatData* m;
  JSOutputArray output;
  JSInputArray /*output,*/ mask = cv::noArray();

  m = js_mat_data(ctx, this_val);

  output = js_umat_or_mat(ctx, argv[0]);
  // output = js_umat_or_mat(ctx, argv[0]);

  if(js_is_noarray(output))
    return JS_ThrowInternalError(ctx, "argument 1 not an array!");

  if(argc > 1)
    mask = js_umat_or_mat(ctx, argv[1]);

  try {
    if(!js_is_noarray(mask))
      m->copyTo(output, mask);
    else
      m->copyTo(output);
  } catch(const cv::Exception& e) { return JS_ThrowInternalError(ctx, "cv::Exception what='%s'", e.what()); }

  return JS_UNDEFINED;
}

static JSValue
js_mat_reshape(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSMatData *m, mat;
  int32_t cn, rows = 0;
  JSValue ret = JS_EXCEPTION;

  m = js_mat_data(ctx, this_val);

  if(m == nullptr || argc < 1)
    return ret;

  if(argc >= 1 && JS_IsNumber(argv[0])) {
    JS_ToInt32(ctx, &cn, argv[0]);
    argv++;
    argc--;
  } else {
    cn = m->channels();
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
        mat = m->reshape(cn, ndims, &newshape[0]);
      } else {
        mat = m->reshape(cn, newshape);
      }
    } else if(JS_IsNumber(argv[0])) {
      JS_ToInt32(ctx, &rows, argv[0]);
      mat = m->reshape(cn, rows);
    }
    ret = js_mat_wrap(ctx, mat);
  }

  return ret;
}

static JSValue
js_mat_getumat(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSMatData* m;
  JSUMatData umat;
  int32_t accessFlags, usageFlags = cv::USAGE_DEFAULT;
  JSValue ret = JS_EXCEPTION;

  m = js_mat_data(ctx, this_val);

  if(m == nullptr || argc < 1)
    return ret;

  if(js_umat_class_id == 0)
    return JS_NULL;

  JS_ToInt32(ctx, &accessFlags, argv[0]);

  if(argc > 1)
    JS_ToInt32(ctx, &usageFlags, argv[1]);

  umat = m->getUMat(cv::AccessFlag(accessFlags), cv::UMatUsageFlags(usageFlags));

  return js_umat_wrap(ctx, umat);
}

static JSValue
js_mat_class_func(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValueConst *v = argv, *e = &argv[argc];
  JSMatData result;
  JSMatData *prev = nullptr, *mat = nullptr;

  while(v < e) {
    JSValueConst arg = *v++;

    if(nullptr == (mat = js_mat_data(ctx, arg)))
      return JS_EXCEPTION;

    if(prev) {
      JSMatData const &a = *prev, &b = *mat;
      switch(magic) {
        case 0: result = a + b; break;
        case 1: result = a - b; break;
        case 2: result = a * b; break;
        case 3: result = a / b; break;
        case 4: result = a & b; break;
        case 5: result = a | b; break;
        case 6: result = a ^ b; break;
      }
      prev = &result;
    } else {
      prev = mat;
      result = cv::Mat::zeros(mat->rows, mat->cols, mat->type());
      mat->copyTo(result);
    }
  }

  return js_mat_wrap(ctx, result);
}

static JSValue
js_mat_fill(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSMatData* m;

  m = js_mat_data(ctx, this_val);

  if(m == nullptr)
    return JS_EXCEPTION;

  if(m->empty() || m->rows == 0 || m->cols == 0)
    return JS_EXCEPTION;

  cv::Mat& mat = *m;

  switch(magic) {
    case 0: {
      mat = cv::Mat::zeros(mat.rows, mat.cols, mat.type());
      break;
    }
    case 1: {
      mat = cv::Mat::ones(mat.rows, mat.cols, mat.type());
      break;
    }
  }

  return JS_DupValue(ctx, this_val);
}

static JSValue
js_mat_class_create(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValue ret = JS_UNDEFINED;

  const auto& [size, type] = js_mat_params(ctx, argc, argv);

  if(size.width == 0 || size.height == 0)
    return JS_EXCEPTION;

  ret = js_mat_new(ctx, uint32_t(0), uint32_t(0), int(0));
  JSMatData& mat = *js_mat_data(ctx, ret);

  switch(magic) {
    case 0: {
      mat = cv::Scalar::all(0);
      break;
    }
    case 1: {
      mat = cv::Scalar::all(1);
      break;
    }
  }

  return ret;
}

/*static JSValue
js_mat_create_vec(JSContext* ctx, int len, JSValue* vec) {
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
js_mat_buffer(JSContext* ctx, JSValueConst this_val) {
  JSMatData* m;
  JSValue buf;
  /*  buf = JS_GetPropertyStr(ctx, this_val, "arrayBuffer");

    if(JS_IsObject(buf))
      return buf;*/

  if((m = js_mat_data(ctx, this_val))) {
    JSValue buf;
    size_t byte_size;
    m->addref();
    // m->addref();
    byte_size = mat_bytesize(*m);

    if(byte_size == 0)
      byte_size = end(*m) - begin(*m);
    if(byte_size == 0)
      byte_size = m->elemSize() * m->total();
    assert(byte_size);
    buf = js_arraybuffer_from(
        ctx, mat_ptr(*m), mat_ptr(*m) + byte_size, *(JSFreeArrayBufferDataFunc*)&js_mat_free_func, (void*)m);

    //   JS_SetPropertyStr(ctx, this_val, "arrayBuffer", JS_DupValue(ctx, buf));
    return buf;
  }
  return JS_EXCEPTION;
}

static JSValue
js_mat_array(JSContext* ctx, JSValueConst this_val) {
  JSMatData* m;
  const char* ctor;

  if((m = js_mat_data(ctx, this_val))) {
    JSValue buffer, typed_array;
    TypedArrayType type(*m);
    buffer = js_mat_buffer(ctx, this_val);

    std::ranges::subrange<uint8_t*> range(begin(*m), end(*m));

    /*printf("m->rows=%i m->cols=%i m->step=%zu m->total()=%zu range.size()=%zu type.byte_size=%u
       size=%zu\n", m->rows, m->cols, size_t(m->step), range.size() / type.byte_size, range.size(),
           type.byte_size,
           range.size() / type.byte_size);*/

    return js_typedarray_new(ctx, buffer, 0, range.size() / type.byte_size, type);
  }
  return JS_EXCEPTION;
}

JSValue
js_mat_call(JSContext* ctx, JSValueConst func_obj, JSValueConst this_val, int argc, JSValueConst* argv, int flags) {
  cv::Rect rect = {0, 0, 0, 0};
  JSMatData* src;

  if((src = js_mat_data(ctx, func_obj)) == nullptr)
    return JS_EXCEPTION;

  if(!js_rect_read(ctx, argv[0], &rect))
    return JS_ThrowTypeError(ctx, "argument 1 expecting Rect");
  // printf("js_mat_call %u,%u %ux%u\n", rect.x, rect.y, rect.width, rect.height);

  cv::Mat mat = src->operator()(rect);
  return js_mat_wrap(ctx, mat);
}

void
js_mat_finalizer(JSRuntime* rt, JSValue val) {
  JSMatData* s;

  if((s = static_cast<JSMatData*>(JS_GetOpaque(val, js_mat_class_id)))) {
    js_deallocate(rt, s);
  }
  return;

  /*auto it2 = std::find(mat_freed.cbegin(), mat_freed.cend(), s);
  auto it = std::find(mat_list.cbegin(), mat_list.cend(), s);

  if(it2 != mat_freed.cend()) {
#ifdef DEBUG_MAT
    std::cerr << "js_mat_finalizer";
    js_mat_dump(s);
#endif

    std::cerr << " ERROR: already freed" << std::endl;
    return;
  }
  JS_FreeValueRT(rt, val);
  if(it != mat_list.cend()) {
    size_t refcount = s->u != nullptr ? s->u->refcount : 0;
    if(s->u) {
      auto data = s->u;
#ifdef DEBUG_MAT
      std::cerr << "cv::Mat::release";
      std::cerr << " mat=" << static_cast<void*>(s);
      std::cerr << ", refcount=" << refcount;
      std::cerr << std::endl;
#endif
      if(refcount > 1)
        s->release();
      if(s->u)
        refcount = s->u->refcount;
      else
        refcount = 0;
    }
    if(s->u == nullptr) {
      mat_list.erase(it);
      mat_freed.push_front(s);
    }
#ifdef DEBUG_MAT
    std::cerr << "js_mat_finalizer";
    js_mat_dump(s);
    std::cerr << ", refcount=" << refcount;
    std::cerr << ", mat_list.size()=" << mat_list.size();
    std::cerr << ", mat_freed.size()=" << mat_freed.size() << std::endl;
#endif
  } else {
#ifdef DEBUG_MAT
    std::cerr << "js_mat_finalizer";
    js_mat_dump(s);
#endif
    std::cerr << " ERROR: not found" << std::endl;
  }*/
}

JSValue
js_mat_iterator_new(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  JSValue enum_obj, mat;
  JSMatData* m;
  JSMatIteratorData* it;

  if(!(m = static_cast<JSMatData*>(JS_GetOpaque(this_val, js_mat_class_id))))
    return JS_EXCEPTION;

  mat = JS_DupValue(ctx, this_val);
  if(!JS_IsException(mat)) {
    enum_obj = JS_NewObjectProtoClass(ctx, mat_iterator_proto, js_mat_iterator_class_id);
    if(!JS_IsException(enum_obj)) {
      it = js_allocate<JSMatIteratorData>(ctx);

      it->obj = mat;
      it->buf = m->empty() ? JS_UNDEFINED : js_mat_buffer(ctx, this_val);
      it->row = 0;
      it->col = 0;
      it->magic = magic;
      it->type = TypedArrayType(*m);

      // printf("js_mat_iterator_new type=%s\n", it->type.constructor_name().c_str());

      JS_SetOpaque(/*ctx, */ enum_obj, it);
      return enum_obj;
    }
    JS_FreeValue(ctx, enum_obj);
  }
  JS_FreeValue(ctx, mat);
  return JS_EXCEPTION;
}

/*typedef struct JSMatIteratorData {
  JSValue obj, buf;
  uint32_t row, col;
  int magic;
  TypedArrayType type;
} JSMatIteratorData;
*/

void
js_mat_iterator_dump(JSMatIteratorData* it) {
  std::cout << "MatIterator { row: " << it->row << ", col: " << it->col << ", magic: " << it->magic << ", type: " << it->type
            << " }" << std::endl;
}

JSValue
js_mat_iterator_next(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, BOOL* pdone, int magic) {
  JSMatIteratorData* it;
  JSValue ret = JS_UNDEFINED;
  *pdone = FALSE;
  it = static_cast<JSMatIteratorData*>(JS_GetOpaque(this_val, js_mat_iterator_class_id));
  if(it) {
    JSMatData* m;
    uint32_t row, col;
    size_t offset, channels;
    JSMatDimensions dim;
    if((m = js_mat_data(ctx, it->obj)) == nullptr)
      return JS_EXCEPTION;
    dim = mat_dimensions(*m);

    /*std::cout << "mat_dimensions(*m) = " << dim.cols << "x" << dim.rows << std::endl;
    js_mat_iterator_dump(it);*/

    row = it->row;
    col = it->col;
    if(row >= m->rows) {
      JS_FreeValue(ctx, it->obj);
      it->obj = JS_UNDEFINED;
    done:
      *pdone = TRUE;
      return JS_UNDEFINED;
    }
    if(col + 1 < dim.cols) {
      it->col = col + 1;
    } else {
      it->col = 0;
      it->row = row + 1;
    }
    *pdone = FALSE;
    channels = mat_channels(*m);
    offset = mat_offset(*m, row, col);
    switch(it->magic) {
      case MAT_ITERATOR_KEYS: {
        std::array<uint32_t, 2> pos = {row, col};
        ret = js_array_from(ctx, pos);
        break;
      }
      case MAT_ITERATOR_VALUES: {
        TypedArrayType type(*m);

        /*   printf("js_mat_iterator_next is_signed=%d\n", type.is_signed);
           printf("js_mat_iterator_next is_floating_point=%d\n", type.is_floating_point);
           printf("js_mat_iterator_next byte_size=%d\n", type.byte_size);
           printf("js_mat_iterator_next channels=%d type=%d\n", m->channels(), m->type());
           printf("js_mat_iterator_next %s\n", type.constructor_name().c_str());*/

        if(channels == 1)
          return js_mat_get(ctx, it->obj, row, col);

        ret = js_typedarray_new(ctx, it->buf, offset, channels, type);
        break;
      }
      case MAT_ITERATOR_ENTRIES: {
        JSValue value = channels == 1 ? js_mat_get(ctx, it->obj, row, col)
                                      : js_typedarray_new(ctx, it->buf, offset, channels, TypedArrayType(*m));
        std::array<uint32_t, 2> pos = {row, col};
        std::array<JSValue, 2> entry = {js_array_from(ctx, pos), value};
        ret = js_array_from(ctx, entry);
        break;
      }
    }
  }
  return ret;
}

void
js_mat_iterator_finalizer(JSRuntime* rt, JSValue val) {
  JSMatIteratorData* it = static_cast<JSMatIteratorData*>(JS_GetOpaque(val, js_mat_iterator_class_id));
  js_deallocate(rt, it);
}

static JSValue
js_mat_iterator_dup(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  return JS_DupValue(ctx, this_val);
}

JSClassDef js_mat_class = {
    .class_name = "Mat",
    .finalizer = js_mat_finalizer,
    .call = js_mat_call,
};

JSClassDef js_mat_iterator_class = {
    .class_name = "MatIterator",
    .finalizer = js_mat_iterator_finalizer,
};

const JSCFunctionListEntry js_mat_proto_funcs[] = {JS_CGETSET_MAGIC_DEF("cols", js_mat_get_props, NULL, PROP_COLS),
                                                   JS_CGETSET_MAGIC_DEF("rows", js_mat_get_props, NULL, PROP_ROWS),
                                                   JS_CGETSET_MAGIC_DEF("channels", js_mat_get_props, NULL, PROP_CHANNELS),
                                                   JS_CGETSET_MAGIC_DEF("type", js_mat_get_props, NULL, PROP_TYPE),
                                                   JS_CGETSET_MAGIC_DEF("depth", js_mat_get_props, NULL, PROP_DEPTH),
                                                   JS_CGETSET_MAGIC_DEF("empty", js_mat_get_props, NULL, PROP_EMPTY),
                                                   JS_CGETSET_MAGIC_DEF("total", js_mat_get_props, NULL, PROP_TOTAL),
                                                   JS_CGETSET_MAGIC_DEF("size", js_mat_get_props, NULL, PROP_SIZE),
                                                   JS_CGETSET_MAGIC_DEF("continuous", js_mat_get_props, NULL, PROP_CONTINUOUS),
                                                   JS_CGETSET_MAGIC_DEF("submatrix", js_mat_get_props, NULL, PROP_SUBMATRIX),
                                                   JS_CGETSET_MAGIC_DEF("step", js_mat_get_props, NULL, PROP_STEP),
                                                   JS_CGETSET_MAGIC_DEF("elemSize", js_mat_get_props, NULL, PROP_ELEMSIZE),
                                                   JS_CGETSET_MAGIC_DEF("elemSize1", js_mat_get_props, NULL, PROP_ELEMSIZE1),
                                                   JS_CGETSET_DEF("buffer", js_mat_buffer, NULL),
                                                   JS_CGETSET_DEF("array", js_mat_array, NULL),
                                                   JS_CFUNC_MAGIC_DEF("col", 1, js_mat_funcs, METHOD_COL),
                                                   JS_CFUNC_MAGIC_DEF("row", 1, js_mat_funcs, METHOD_ROW),
                                                   JS_CFUNC_MAGIC_DEF("colRange", 2, js_mat_funcs, METHOD_COL_RANGE),
                                                   JS_CFUNC_MAGIC_DEF("rowRange", 2, js_mat_funcs, METHOD_ROW_RANGE),
                                                   JS_CFUNC_MAGIC_DEF("clone", 0, js_mat_funcs, METHOD_CLONE),
                                                   JS_CFUNC_MAGIC_DEF("roi", 0, js_mat_funcs, METHOD_ROI),
                                                   JS_CFUNC_MAGIC_DEF("release", 0, js_mat_funcs, METHOD_RELEASE),
                                                   JS_CFUNC_MAGIC_DEF("dup", 0, js_mat_funcs, METHOD_DUP),
                                                   JS_CFUNC_MAGIC_DEF("clear", 0, js_mat_funcs, METHOD_CLEAR),
                                                   JS_CFUNC_MAGIC_DEF("reset", 0, js_mat_funcs, METHOD_RESET),
                                                   JS_CFUNC_MAGIC_DEF("resize", 1, js_mat_funcs, METHOD_RESIZE),
                                                   JS_CFUNC_MAGIC_DEF("step1", 0, js_mat_funcs, METHOD_STEP1),
                                                   JS_CFUNC_MAGIC_DEF("locateROI", 0, js_mat_funcs, METHOD_LOCATE_ROI),
                                                   JS_CFUNC_MAGIC_DEF("ptr", 0, js_mat_funcs, METHOD_PTR),

                                                   JS_CFUNC_MAGIC_DEF("and", 2, js_mat_expr, MAT_EXPR_AND),
                                                   JS_CFUNC_MAGIC_DEF("or", 2, js_mat_expr, MAT_EXPR_OR),
                                                   JS_CFUNC_MAGIC_DEF("xor", 3, js_mat_expr, MAT_EXPR_XOR),
                                                   JS_CFUNC_MAGIC_DEF("mul", 3, js_mat_expr, MAT_EXPR_MUL),

                                                   JS_CFUNC_MAGIC_DEF("zero", 2, js_mat_fill, 0),
                                                   JS_CFUNC_MAGIC_DEF("one", 2, js_mat_fill, 1),

                                                   JS_CFUNC_DEF("toString", 0, js_mat_tostring),
                                                   JS_CFUNC_DEF("at", 1, js_mat_at),
                                                   JS_CFUNC_DEF("set", 2, js_mat_set),
                                                   JS_CFUNC_DEF("setTo", 1, js_mat_set_to),
                                                   JS_CFUNC_DEF("convertTo", 2, js_mat_convert_to),
                                                   JS_CFUNC_DEF("copyTo", 1, js_mat_copy_to),
                                                   JS_CFUNC_DEF("reshape", 1, js_mat_reshape),
                                                   JS_CFUNC_DEF("getUMat", 1, js_mat_getumat),
                                                   JS_CFUNC_MAGIC_DEF("keys", 0, js_mat_iterator_new, MAT_ITERATOR_KEYS),
                                                   JS_CFUNC_MAGIC_DEF("values", 0, js_mat_iterator_new, MAT_ITERATOR_VALUES),
                                                   JS_CFUNC_MAGIC_DEF("entries", 0, js_mat_iterator_new, MAT_ITERATOR_ENTRIES),
                                                   JS_ALIAS_DEF("[Symbol.iterator]", "values"),
                                                   JS_ALIAS_DEF("[Symbol.toPrimitive]", "toString"),

                                                   JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Mat", JS_PROP_CONFIGURABLE)

};

const JSCFunctionListEntry js_mat_iterator_proto_funcs[] = {
    JS_ITERATOR_NEXT_DEF("next", 0, js_mat_iterator_next, 0),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "MatIterator", JS_PROP_CONFIGURABLE),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, js_mat_iterator_dup),

};

const JSCFunctionListEntry js_mat_static_funcs[] = {
    JS_CFUNC_DEF("getRotationMatrix2D", 3, js_mat_getrotationmatrix2d),
    JS_CFUNC_MAGIC_DEF("add", 2, js_mat_class_func, 0),
    JS_CFUNC_MAGIC_DEF("sub", 2, js_mat_class_func, 1),
    JS_CFUNC_MAGIC_DEF("mul", 2, js_mat_class_func, 2),
    JS_CFUNC_MAGIC_DEF("div", 2, js_mat_class_func, 3),
    JS_CFUNC_MAGIC_DEF("and", 2, js_mat_class_func, 4),
    JS_CFUNC_MAGIC_DEF("or", 2, js_mat_class_func, 5),
    JS_CFUNC_MAGIC_DEF("xor", 3, js_mat_class_func, 6),
    JS_CFUNC_MAGIC_DEF("zeros", 1, js_mat_class_create, 0),
    JS_CFUNC_MAGIC_DEF("ones", 1, js_mat_class_create, 1),
    JS_PROP_INT32_DEF("CV_8U", CV_MAKETYPE(CV_8U, 1), JS_PROP_ENUMERABLE),
};

int
js_mat_init(JSContext* ctx, JSModuleDef* m) {
  if(js_mat_class_id == 0) {
    /* create the Mat class */
    JS_NewClassID(&js_mat_class_id);
    JS_NewClassID(&js_mat_iterator_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_mat_class_id, &js_mat_class);
    JS_NewClass(JS_GetRuntime(ctx), js_mat_iterator_class_id, &js_mat_iterator_class);

    mat_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, mat_proto, js_mat_proto_funcs, countof(js_mat_proto_funcs));
    JS_SetClassProto(ctx, js_mat_class_id, mat_proto);

    mat_iterator_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, mat_iterator_proto, js_mat_iterator_proto_funcs, countof(js_mat_iterator_proto_funcs));
    JS_SetClassProto(ctx, js_mat_iterator_class_id, mat_iterator_proto);

    mat_class = JS_NewCFunction2(ctx, js_mat_ctor, "Mat", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, mat_class, mat_proto);

    JS_SetPropertyFunctionList(ctx, mat_class, js_mat_static_funcs, countof(js_mat_static_funcs));

    js_set_inspect_method(ctx, mat_proto, js_mat_inspect);
    /*   JSValue g = JS_GetGlobalObject(ctx);
       int32array_ctor = JS_GetProperty(ctx, g, JS_ATOM_Int32Array);
       int32array_proto = JS_GetPrototype(ctx, int32array_ctor);

       JS_FreeValue(ctx, g);*/
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Mat", mat_class);
  return 0;
}

extern "C" VISIBLE void
js_mat_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Mat");
}

#if defined(JS_MAT_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_mat
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_mat_init);
  if(!m)
    return NULL;
  js_mat_export(ctx, m);
  return m;
}
