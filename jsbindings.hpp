#ifndef JSBINDINGS_HPP
#define JSBINDINGS_HPP

#include "util.hpp"
#include "line.hpp"
#include <cutils.h>
#include <quickjs.h>
#include <array>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <opencv2/core/cvstd_wrapper.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/videoio.hpp>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

typedef std::vector<JSCFunctionListEntry> js_function_list_t;

static inline JSValue
js_handle_exception(JSContext* ctx, const cv::Exception& e) {
  std::string msg(e.what());

  return JS_ThrowInternalError(ctx, "cv::Exception %s", msg.c_str() + msg.find_last_of("/\\") + 1);
}

namespace cv {
class CLAHE;
}

#define JS_CONSTANT(name) JS_PROP_INT32_DEF(#name, name, 0)
#define JS_CV_CONSTANT(name) JS_PROP_INT32_DEF(#name, cv::name, JS_PROP_ENUMERABLE)

typedef struct {
  BOOL done;
  JSValue value;
} IteratorValue;

typedef cv::Rect2d JSRectDataD;
typedef cv::Mat JSMatData;
typedef cv::UMat JSUMatData;
typedef cv::Size2d JSSizeDataD;
typedef cv::RotatedRect JSRotatedRectData;

template<class T> using JSPointData = cv::Point_<T>;
template<class T> using JSSizeData = cv::Size_<T>;
template<class T> using JSRectData = cv::Rect_<T>;
template<class T> using JSContourData = std::vector<JSPointData<T>>;
template<class T> using JSContoursData = std::vector<JSContourData<T>>;

/*typedef JSPointData<int> JSPointData<int>;
typedef JSPointData<float> JSPointDataF;
typedef JSPointData<double> JSPointDataD;*/

typedef cv::VideoCapture JSVideoCaptureData;
typedef cv::VideoWriter JSVideoWraiterData;
typedef cv::TickMeter JSTickMeterData;
typedef cv::Ptr<cv::CLAHE> JSCLAHEData;

typedef cv::_InputArray JSInputArray;
typedef cv::_InputOutputArray JSInputOutputArray;
typedef cv::_OutputArray JSOutputArray;

template<class T> union JSLineData {
  typedef std::array<T, 4> array_type;
  typedef std::array<JSPointData<T>, 2> points_type;
  typedef std::pair<JSPointData<T>, JSPointData<T>> pair_type;

  array_type array;
  points_type points;
  pair_type pt;

  struct {
    T x1, y1, x2, y2;
  };

  struct {
    JSPointData<T> a, b;
  };

  JSLineData(const JSLineData<T>& other) {
    x1 = other.x1;
    y1 = other.y1;
    x2 = other.x2;
    y2 = other.y2;
  }
  JSLineData(T _x1, T _y1, T _x2, T _y2) : x1(_x1), y1(_y1), x2(_x2), y2(_y2) {}

  operator array_type&() { return this->array; }
  operator array_type const&() const { return this->array; }
};

template<class T> struct JSLineTraits {
  typedef std::array<T, 4> array_type;
  typedef cv::Vec<T, 4> vector_type;
  typedef cv::Scalar_<T> scalar_type;
};

template<class T> union JSColorData {
  std::array<T, 4> arr;
  struct {
    T r, g, b, a;
  };
};

template<> union JSColorData<uint8_t> {
  std::array<uint8_t, 4> arr;
  struct {
    uint8_t r, g, b, a;
  };
  uint32_t u32;
};

#if defined(_WIN32) || defined(__MINGW32__)
#define VISIBLE __declspec(dllexport)
#define HIDDEN
#else
#define VISIBLE __attribute__((visibility("default")))
#define HIDDEN __attribute__((visibility("hidden")))
#endif

#define JS_CGETSET_ENUMERABLE_DEF(prop_name, fgetter, fsetter, magic_num) \
  { \
    .name = prop_name, .prop_flags = JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE, .def_type = JS_DEF_CGETSET_MAGIC, .magic = magic_num, .u = { \
      .getset = {.get = {.getter_magic = fgetter}, .set = {.setter_magic = fsetter}} \
    } \
  }

extern "C" {
JSModuleDef* js_init_module_draw(JSContext*, const char*);
JSModuleDef* js_init_module_video_capture(JSContext*, const char*);
JSModuleDef* js_init_module_cv(JSContext*, const char*);
}

template<class Stream>
static inline Stream&
operator<<(Stream& os, const cv::_InputOutputArray& arr) {
  os << "{ ";
  os << "type: " << arr.type();
  os << ", depth: " << arr.depth();
  os << ", channels: " << arr.channels();
  os << ", total: " << arr.total();
  os << " }";

  return os;
}

static inline std::string
dump(const cv::_InputArray& arr) {
  std::ostringstream os;
  os << "{ ";
  os << "type: " << arr.type();
  os << ", depth: " << arr.depth();
  os << ", channels: " << arr.channels();
  os << ", total: " << arr.total();
  os << " }";

  return os.str();
}

static inline std::string
dump(const cv::_InputOutputArray& arr) {
  std::ostringstream os;
  os << arr;
  return os.str();
}

static inline std::string
dump(const cv::_OutputArray& arr) {
  std::ostringstream os;
  os << "{ ";
  os << "type: " << arr.type();
  os << ", depth: " << arr.depth();
  os << ", channels: " << arr.channels();
  os << ", total: " << arr.total();
  os << " }";

  return os.str();
}

struct JSConstructor {
  JSConstructor(JSCFunction* _ctor, const char* _name) : name(_name), ctor(_ctor), proto(nullptr), nfuncs(0), class_obj(JS_UNDEFINED) {}

  template<int size>
  JSConstructor(JSCFunction* _ctor, const char* _name, const JSCFunctionListEntry (&_funcs)[size])
      : name(_name), ctor(_ctor), proto(_funcs), nfuncs(size), class_obj(JS_UNDEFINED) {}

  JSValue
  create(JSContext* ctx) {
    class_obj = JS_NewCFunction2(ctx, ctor, name, 0, JS_CFUNC_constructor, 0);
    if(proto && nfuncs)
      JS_SetPropertyFunctionList(ctx, class_obj, proto, nfuncs);
    return class_obj;
  }

  void
  set_prototype(JSContext* ctx, JSValueConst proto) const {
    assert(!JS_IsUndefined(class_obj));
    JS_SetConstructor(ctx, class_obj, proto);
  }
  void
  set_export(JSContext* ctx, JSModuleDef* m) const {
    assert(!JS_IsUndefined(class_obj));
    JS_SetModuleExport(ctx, m, name, class_obj);
  }

  void
  add_export(JSContext* ctx, JSModuleDef* m) const {
    JS_AddModuleExport(ctx, m, name);
  }

protected:
  const char* name;
  JSCFunction* ctor;
  const JSCFunctionListEntry* proto;
  size_t nfuncs;
  JSValue class_obj;
};

extern "C" {
static inline JSValue js_global_get(JSContext* ctx, const char* prop);
static inline BOOL js_is_iterable(JSContext* ctx, JSValueConst obj);
static inline JSValue js_iterator_method(JSContext* ctx, JSValueConst obj);
static inline JSValue js_iterator_new(JSContext* ctx, JSValueConst obj);
static inline IteratorValue js_iterator_next(JSContext* ctx, JSValueConst obj);
static inline JSAtom js_symbol_atom(JSContext* ctx, const char* name);
static inline JSValue js_symbol_ctor(JSContext* ctx);
static inline JSValue js_symbol_get_static(JSContext* ctx, const char* name);
}

#define countof(x) (sizeof(x) / sizeof((x)[0]))

JSValue js_vector_vec4i_to_array(JSContext*, const std::vector<cv::Vec4i>& vec);

template<class T>
static inline int
js_number_read(JSContext* ctx, JSValueConst num, T* out) {
  double d;
  int ret;
  if((ret = !JS_ToFloat64(ctx, &d, num)))
    *out = d;
  return ret;
}

template<>
inline int
js_number_read<int32_t>(JSContext* ctx, JSValueConst num, int32_t* out) {
  return !JS_ToInt32(ctx, out, num);
}

template<>
inline int
js_number_read<uint32_t>(JSContext* ctx, JSValueConst num, uint32_t* out) {
  return !JS_ToUint32(ctx, out, num);
}

template<>
inline int
js_number_read<int64_t>(JSContext* ctx, JSValueConst num, int64_t* out) {
  return !JS_ToInt64(ctx, out, num);
}

template<class T>
static inline JSValue
js_number_new(JSContext* ctx, T num) {
  return JS_NewFloat64(ctx, num);
}

template<>
inline JSValue
js_number_new<int32_t>(JSContext* ctx, int32_t num) {
  return JS_NewInt32(ctx, num);
}
template<>
inline JSValue
js_number_new<uint32_t>(JSContext* ctx, uint32_t num) {
  return JS_NewUint32(ctx, num);
}

template<>
inline JSValue
js_number_new<int64_t>(JSContext* ctx, int64_t num) {
  return JS_NewInt64(ctx, num);
}

template<class T>
static inline T
js_property_get(JSContext* ctx, JSValue this_val, const char* prop) {
  JSValue value = JS_GetPropertyStr(ctx, this_val, prop);
  T ret;
  js_value_to(ctx, value, ret);
  JS_FreeValue(ctx, value);
  return ret;
}

int js_color_read(JSContext* ctx, JSValueConst color, JSColorData<double>* out);
int js_color_read(JSContext* ctx, JSValueConst value, JSColorData<uint8_t>* out);

static inline int
js_color_read(JSContext* ctx, JSValueConst value, cv::Scalar* out) {
  JSColorData<double> color;
  int ret;
  if((ret = js_color_read(ctx, value, &color))) {
    (*out)[0] = color.arr[0];
    (*out)[1] = color.arr[1];
    (*out)[2] = color.arr[2];
    (*out)[3] = color.arr[3];
  }

  return ret;
}

template<class T>
inline JSValue
js_color_new(JSContext* ctx, const JSColorData<T>& color) {
  JSValue ret = JS_NewArray(ctx);

  JS_SetPropertyUint32(ctx, ret, 0, js_number_new<T>(ctx, color.arr[0]));
  JS_SetPropertyUint32(ctx, ret, 1, js_number_new<T>(ctx, color.arr[1]));
  JS_SetPropertyUint32(ctx, ret, 2, js_number_new<T>(ctx, color.arr[2]));
  JS_SetPropertyUint32(ctx, ret, 3, js_number_new<T>(ctx, color.arr[3]));
  return ret;
}

static inline JSValue
js_color_new(JSContext* ctx, const cv::Scalar& scalar) {
  JSColorData<double> color;

  color.arr[0] = scalar[0];
  color.arr[1] = scalar[1];
  color.arr[2] = scalar[2];
  color.arr[3] = scalar[3];
  return js_color_new(ctx, color);
}

/*static inline const cv::Scalar&
js_to_scalar(const JSColorData<double>& color) {
  return *reinterpret_cast<const cv::Scalar*>(&color);
}

static inline cv::Scalar&
js_to_scalar(JSColorData<double>& color) {
  return *reinterpret_cast<cv::Scalar*>(&color);
}*/

template<class T>
static inline const cv::Scalar&
js_to_scalar(const JSColorData<T>& color) {
  return *reinterpret_cast<const cv::Scalar_<T>*>(&color);
}

template<class T>
static inline cv::Scalar&
js_to_scalar(JSColorData<T>& color) {
  return *reinterpret_cast<cv::Scalar_<T>*>(&color);
}

template<class T>
std::ostream&
operator<<(std::ostream& stream, const JSColorData<T>& color) {
  stream << "[ " << (int)color.arr[0] << ", " << (int)color.arr[1] << ", " << (int)color.arr[2] << ", " << (int)color.arr[3] << " ]";
  return stream;
}

int js_ref(JSContext* ctx, const char* name, JSValueConst arg, JSValue value);

static inline size_t
round_to(size_t num, size_t x) {
  num /= x;
  num *= x;
  return num;
}

static inline range_view<uint8_t>
js_arraybuffer_range(JSContext* ctx, JSValueConst buffer) {
  size_t size;
  uint8_t* ptr;
  ptr = JS_GetArrayBuffer(ctx, &size, buffer);
  return range_view<uint8_t>(ptr, size);
}

template<class T>
static inline range_view<T>
js_arraybuffer_range(JSContext* ctx, JSValueConst buffer) {
  typedef typename std::remove_pointer<T>::type value_type;
  size_t size;
  uint8_t* byte_ptr;
  byte_ptr = JS_GetArrayBuffer(ctx, &size, buffer);
  size = round_to(size, sizeof(value_type));
  return range_view<T>(reinterpret_cast<T>(byte_ptr), size);
}

template<class Ptr>
static inline JSValue
js_arraybuffer_from(JSContext* ctx, const Ptr& begin, const Ptr& end) {
  const uint8_t* ptr;
  size_t len;
  ptr = reinterpret_cast<const uint8_t*>(begin);
  len = reinterpret_cast<const uint8_t*>(end) - ptr;
  return JS_NewArrayBufferCopy(ctx, ptr, len);
}

template<class Ptr>
static inline JSValue
js_arraybuffer_from(JSContext* ctx, const Ptr& begin, const Ptr& end, JSFreeArrayBufferDataFunc& free_func, void* opaque = nullptr, bool is_shared = false) {
  const uint8_t* ptr;
  size_t len;
  ptr = reinterpret_cast<const uint8_t*>(begin);
  len = reinterpret_cast<const uint8_t*>(end) - ptr;
  return JS_NewArrayBuffer(ctx, const_cast<uint8_t*>(ptr), len, &free_func, opaque, is_shared);
}

static void
js_arraybuffer_free(JSRuntime* rt, void* opaque, void* ptr) {
  JS_FreeValueRT(rt, JS_MKPTR(JS_TAG_OBJECT, opaque));
}

template<class Ptr>
static inline JSValue
js_arraybuffer_from(JSContext* ctx, const Ptr& begin, const Ptr& end, JSValueConst value) {
  JSObject* obj;

  assert(JS_VALUE_GET_TAG(value) == JS_TAG_OBJECT);

  obj = JS_VALUE_GET_OBJ(value);
  JS_DupValue(ctx, value);

  return js_arraybuffer_from(ctx, begin, end, js_arraybuffer_free, obj);
}

static inline JSValue
js_arraybuffer_slice(JSContext* ctx, JSValueConst value, size_t from, size_t to) {
  uint8_t* buf;
  size_t len;

  if(!(buf = JS_GetArrayBuffer(ctx, &len, value)))
    return JS_ThrowTypeError(ctx, "value must be an ArrayBuffer");

  return js_arraybuffer_from(ctx, buf + std::min(from, len), buf + std::min(to, len), value);
}

template<class T, size_t N>
static inline JSValue
js_arraybuffer_from(JSContext* ctx, std::array<T, N>& arr, JSValueConst value) {
  return js_arraybuffer_from(ctx, arr.begin(), arr.end(), value);
}

static inline JSValue
js_global_get(JSContext* ctx, const char* prop) {
  JSValue global_obj, ret;

  global_obj = JS_GetGlobalObject(ctx);
  ret = JS_GetPropertyStr(ctx, global_obj, prop);
  JS_FreeValue(ctx, global_obj);
  return ret;
}

static inline JSValue
js_global_prototype(JSContext* ctx, const char* class_name) {
  JSValue ctor, ret;
  ctor = js_global_get(ctx, class_name);
  ret = JS_GetPropertyStr(ctx, ctor, "prototype");
  JS_FreeValue(ctx, ctor);
  return ret;
}

static inline JSValue
js_global_prototype_func(JSContext* ctx, const char* class_name, const char* func_name) {
  JSValue proto, func;
  proto = js_global_prototype(ctx, class_name);
  func = JS_GetPropertyStr(ctx, proto, func_name);
  JS_FreeValue(ctx, proto);
  return func;
}

static inline BOOL
js_global_instanceof(JSContext* ctx, JSValueConst obj, const char* prop) {
  JSValue ctor = js_global_get(ctx, prop);
  BOOL ret = JS_IsInstanceOf(ctx, obj, ctor);
  JS_FreeValue(ctx, ctor);
  return ret;
}

static inline const char*
js_object_tostring2(JSContext* ctx, JSValueConst method, JSValueConst value) {
  JSValue str = JS_Call(ctx, method, value, 0, 0);
  const char* s = JS_ToCString(ctx, str);
  JS_FreeValue(ctx, str);
  return s;
}

static inline const char*
js_object_tostring(JSContext* ctx, JSValueConst value) {
  static thread_local JSValue method;

  if(JS_VALUE_GET_TAG(method) == 0)
    method = js_global_prototype_func(ctx, "Object", "toString");

  return js_object_tostring2(ctx, method, value);
}

static inline int
js_object_is(JSContext* ctx, JSValueConst value, const char* cmp) {
  BOOL ret = FALSE;
  const char* str;
  if((str = js_object_tostring(ctx, value))) {
    ret = strcmp(str, cmp) == 0;
    JS_FreeCString(ctx, str);
  }

  return ret;
}

static inline JSValue
js_symbol_ctor(JSContext* ctx) {
  return js_global_get(ctx, "Symbol");
}

static inline JSValue
js_symbol_invoke_static(JSContext* ctx, const char* name, JSValueConst arg) {
  JSValue ret;
  JSAtom method_name = JS_NewAtom(ctx, name);
  ret = JS_Invoke(ctx, js_symbol_ctor(ctx), method_name, 1, &arg);
  JS_FreeAtom(ctx, method_name);
  return ret;
}

static inline JSValue
js_symbol_for(JSContext* ctx, const char* sym_for) {
  JSValue key, sym;
  JSAtom atom;
  key = JS_NewString(ctx, sym_for);
  sym = js_symbol_invoke_static(ctx, "for", key);
  JS_FreeValue(ctx, key);
  return sym;
}

static inline JSAtom
js_symbol_for_atom(JSContext* ctx, const char* sym_for) {
  JSValue sym = js_symbol_for(ctx, sym_for);
  JSAtom atom = JS_ValueToAtom(ctx, sym);
  JS_FreeValue(ctx, sym);
  return atom;
}

static inline void
js_set_inspect_method(JSContext* ctx, JSValueConst obj, JSCFunction* func) {
  JSAtom inspect_symbol = js_symbol_for_atom(ctx, "quickjs.inspect.custom");
  JS_DefinePropertyValue(ctx, obj, inspect_symbol, JS_NewCFunction(ctx, func, "inspect", 1), JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE);
  JS_FreeAtom(ctx, inspect_symbol);
}

static inline JSValue
js_get_tostringtag(JSContext* ctx, JSValueConst obj) {
  JSAtom tostringtag_symbol = js_symbol_atom(ctx, "toStringTag");
  JSValue ret = JS_GetProperty(ctx, obj, tostringtag_symbol);
  JS_FreeAtom(ctx, tostringtag_symbol);
  return ret;
}

static inline void
js_set_tostringtag(JSContext* ctx, JSValueConst obj, JSValue value) {
  JSAtom tostringtag_symbol = js_symbol_atom(ctx, "toStringTag");
  JS_DefinePropertyValue(ctx, obj, tostringtag_symbol, value, JS_PROP_CONFIGURABLE);
  JS_FreeAtom(ctx, tostringtag_symbol);
}

static inline void
js_set_tostringtag(JSContext* ctx, JSValueConst obj, const char* str) {
  return js_set_tostringtag(ctx, obj, JS_NewString(ctx, str));
}

static inline JSValue
js_symbol_get_static(JSContext* ctx, const char* name) {
  JSValue symbol_ctor, ret;
  symbol_ctor = js_symbol_ctor(ctx);
  ret = JS_GetPropertyStr(ctx, symbol_ctor, name);
  JS_FreeValue(ctx, symbol_ctor);
  return ret;
}

static inline JSAtom
js_symbol_atom(JSContext* ctx, const char* name) {
  JSValue sym = js_symbol_get_static(ctx, name);
  JSAtom ret = JS_ValueToAtom(ctx, sym);
  JS_FreeValue(ctx, sym);
  return ret;
}

static inline JSValue
js_iterator_method(JSContext* ctx, JSValueConst obj) {
  JSAtom atom;
  JSValue ret = JS_UNDEFINED;
  atom = js_symbol_atom(ctx, "iterator");
  if(JS_HasProperty(ctx, obj, atom))
    ret = JS_GetProperty(ctx, obj, atom);
  JS_FreeAtom(ctx, atom);
  if(!JS_IsFunction(ctx, ret)) {
    atom = js_symbol_atom(ctx, "asyncIterator");
    if(JS_HasProperty(ctx, obj, atom))
      ret = JS_GetProperty(ctx, obj, atom);
    JS_FreeAtom(ctx, atom);
  }

  return ret;
}

static inline BOOL
js_is_iterable(JSContext* ctx, JSValueConst obj) {
  JSAtom atom;
  BOOL ret = FALSE;
  atom = js_symbol_atom(ctx, "iterator");
  if(JS_HasProperty(ctx, obj, atom))
    ret = TRUE;
  JS_FreeAtom(ctx, atom);
  if(!ret) {
    atom = js_symbol_atom(ctx, "asyncIterator");
    if(JS_HasProperty(ctx, obj, atom))
      ret = TRUE;
    JS_FreeAtom(ctx, atom);
  }

  return ret;
}

static inline BOOL
js_is_array_like(JSContext* ctx, JSValueConst obj) {
  JSValue len;
  bool ret;
  len = JS_GetPropertyStr(ctx, obj, "length");
  ret = JS_IsNumber(len);
  JS_FreeValue(ctx, len);
  return ret;
}

static inline BOOL
js_is_scalar(JSContext* ctx, JSValueConst obj) {
  JSValue length;
  size_t len = 0;
  bool ret = FALSE;
  length = JS_GetPropertyStr(ctx, obj, "length");
  JS_ToIndex(ctx, &len, length);
  JS_FreeValue(ctx, length);
  if(len >= 2 && len <= 4) {
    size_t i;
    ret = TRUE;
    for(i = 0; i < len; i++) {
      JSValue item = JS_GetPropertyUint32(ctx, obj, i);
      BOOL is_num = JS_IsNumber(item);
      JS_FreeValue(ctx, item);
      if(!is_num)
        return FALSE;
    }
  }

  return ret;
}

struct ArrayBufferProps {
  uint8_t* ptr;
  size_t len;

  ArrayBufferProps(uint8_t* _ptr, size_t _len) : ptr(_ptr), len(_len) {}
};

template<class Stream>
static inline Stream&
operator<<(Stream& os, const ArrayBufferProps& abp) {
  os << "{ ";
  os << "ptr: " << static_cast<void*>(abp.ptr);
  os << ", len: " << abp.len;
  os << " }";
  return os;
}

static inline std::string
dump(const ArrayBufferProps& abp) {
  std::ostringstream os;
  os << abp;
  return os.str();
}

static inline BOOL
js_is_arraybuffer(JSContext* ctx, JSValueConst value) {
  return JS_IsObject(value) && (js_global_instanceof(ctx, value, "ArrayBuffer") || js_object_is(ctx, value, "[object ArrayBuffer]"));
}

static inline ArrayBufferProps
js_arraybuffer_props(JSContext* ctx, JSValueConst obj) {
  uint8_t* ptr;
  size_t len;
  ptr = JS_GetArrayBuffer(ctx, &len, obj);
  return ArrayBufferProps(ptr, len);
}

static inline std::string
js_function_name(JSContext* ctx, JSValueConst value) {
  const char* str;
  const char* name = 0;
  std::string ret;
  int namelen;
  if((str = JS_ToCString(ctx, value))) {
    if(!strncmp(str, "function ", 9)) {
      name = str + 9;
      namelen = strchr(str + 9, '(') - name;
      ret = std::string(name, namelen);
    }
  }
  if(!name) {
    if(str)
      JS_FreeCString(ctx, str);
    if((str = JS_ToCString(ctx, JS_GetPropertyStr(ctx, value, "name"))))
      ret = std::string(str);
  }
  if(str)
    JS_FreeCString(ctx, str);
  return ret;
}

static inline std::string
js_class_name(JSContext* ctx, JSValueConst value) {
  JSValue proto, ctor;

  std::string ret;
  proto = JS_GetPrototype(ctx, value);
  ctor = JS_GetPropertyStr(ctx, proto, "constructor");

  ret = js_function_name(ctx, ctor);
  JS_FreeValue(ctx, ctor);
  JS_FreeValue(ctx, proto);

  return ret;
}

static inline JSValue
js_typedarray_prototype(JSContext* ctx) {
  JSValue u8arr_proto = js_global_prototype(ctx, "Uint8Array");
  JSValue typedarr_proto = JS_GetPrototype(ctx, u8arr_proto);
  JS_FreeValue(ctx, u8arr_proto);
  return typedarr_proto;
}

static inline JSValue
js_typedarray_constructor(JSContext* ctx) {
  JSValue typedarr_proto = js_typedarray_prototype(ctx);
  JSValue typedarr_ctor = JS_GetPropertyStr(ctx, typedarr_proto, "constructor");
  JS_FreeValue(ctx, typedarr_proto);
  return typedarr_ctor;
}

static inline BOOL
js_is_typedarray(JSContext* ctx, JSValueConst value) {
  JSValue ctor = js_typedarray_constructor(ctx);
  BOOL ret = JS_IsInstanceOf(ctx, value, js_typedarray_constructor(ctx));
  JS_FreeValue(ctx, ctor);
  return ret;
}

static inline BOOL
js_is_array(JSContext* ctx, JSValueConst obj) {
  return JS_IsArray(ctx, obj) || js_is_typedarray(ctx, obj);
}

static inline JSValue
js_iterator_new(JSContext* ctx, JSValueConst obj) {
  JSValue fn, ret;
  fn = js_iterator_method(ctx, obj);

  ret = JS_Call(ctx, fn, obj, 0, 0);
  JS_FreeValue(ctx, fn);
  return ret;
}

static inline IteratorValue
js_iterator_next(JSContext* ctx, JSValueConst obj) {
  JSValue fn, result, done;
  IteratorValue ret;

  fn = JS_GetPropertyStr(ctx, obj, "next");

  result = JS_Call(ctx, fn, obj, 0, 0);
  JS_FreeValue(ctx, fn);

  done = JS_GetPropertyStr(ctx, result, "done");
  ret.value = JS_GetPropertyStr(ctx, result, "value");
  JS_FreeValue(ctx, result);

  ret.done = JS_ToBool(ctx, done);
  JS_FreeValue(ctx, done);

  return ret;
}

template<class T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, T>::type* = nullptr>
static inline int
js_value_to(JSContext* ctx, JSValueConst value, T& out) {
  return js_number_read(ctx, value, &out);
}

static inline int
js_value_to(JSContext* ctx, JSValueConst value, bool& out) {
  out = JS_ToBool(ctx, value);
  return 1;
}

static inline int
js_value_to(JSContext* ctx, JSValueConst value, std::string& out) {
  const char* str;
  size_t len;
  str = JS_ToCStringLen(ctx, &len, value);
  out.clear();
  out.assign(str, len);
  JS_FreeCString(ctx, str);
  return 1;
}

template<class T, int N>
static inline int
js_value_to(JSContext* ctx, JSValueConst value, cv::Vec<T, N>& in) {
  return js_array_to(ctx, value, in);
}

template<class T>
static inline int
js_value_to(JSContext* ctx, JSValueConst value, std::vector<T>& in) {
  return js_array_to(ctx, value, in);
}

template<class T>
static inline int
js_value_to(JSContext* ctx, JSValueConst value, Line<T>& in) {
  return js_array_to(ctx, value, reinterpret_cast<std::array<T, 4>*>(&in));
}

template<class T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, T>::type* = nullptr>
static inline JSValue
js_value_from(JSContext* ctx, const T& in) {
  return js_number_new<T>(ctx, in);
}

static inline JSValue
js_value_from(JSContext* ctx, bool in) {
  return JS_NewBool(ctx, in);
}

static inline JSValue
js_value_from(JSContext* ctx, const std::string& in) {
  return JS_NewStringLen(ctx, in.data(), in.size());
}

template<class T, int N>
static inline JSValue
js_value_from(JSContext* ctx, const cv::Vec<T, N>& in) {
  return js_array_from(ctx, begin(in), end(in));
}

template<class T>
static inline JSValue
js_value_from(JSContext* ctx, const std::vector<T>& in) {
  return js_array_from(ctx, begin(in), end(in));
}

template<class T> class js_iterable {
public:
  static int64_t
  to_vector(JSContext* ctx, JSValueConst arg, std::vector<T>& out) {
    IteratorValue item;
    JSValue iter = js_iterator_new(ctx, arg);
    out.clear();
    for(;;) {
      T value;
      item = js_iterator_next(ctx, iter);
      if(item.done)
        break;
      js_value_to(ctx, item.value, value);
      out.push_back(value);
      JS_FreeValue(ctx, item.value);
    }
    JS_FreeValue(ctx, iter);
    return out.size();
  }

  template<size_t N>
  static int64_t
  to_array(JSContext* ctx, JSValueConst arg, std::array<T, N>& out) {
    int64_t i = 0;
    IteratorValue item;
    JSValue iter = js_iterator_new(ctx, arg);
    for(i = 0; i < N; i++) {
      T value;
      item = js_iterator_next(ctx, iter);
      if(item.done)
        break;
      js_value_to(ctx, item.value, value);
      out[i] = value;
      JS_FreeValue(ctx, item.value);
    }
    JS_FreeValue(ctx, iter);
    return i;
  }

  static int64_t
  to_scalar(JSContext* ctx, JSValueConst arg, cv::Scalar_<T>& out) {
    return to_array(ctx, arg, *reinterpret_cast<std::array<T, 4>*>(&out));
  }
};

template<class T>
static inline int64_t
js_iterable_to(JSContext* ctx, JSValueConst arr, std::vector<T>& out) {
  return js_iterable<T>::to_vector(ctx, arr, out);
}

template<class T, size_t N>
static inline int64_t
js_iterable_to(JSContext* ctx, JSValueConst arr, std::array<T, N>& out) {
  typedef js_iterable<T> array_type;
  return array_type::to_array(ctx, arr, out);
}

template<class T>
static inline int64_t
js_iterable_to(JSContext* ctx, JSValueConst arr, cv::Scalar_<T>& out) {
  return js_iterable<T>::to_scalar(ctx, arr, out);
}

template<class T>
static inline bool
js_is_noarray(const T& array) {
  return &array == &cv::noArray();
}

static inline BOOL
js_atom_is_index(JSContext* ctx, uint32_t* pval, JSAtom atom) {
  JSValue value;
  BOOL ret = FALSE;
  int64_t index;

  if(atom & (1U << 31)) {
    *pval = atom & (~(1U << 31));
    return TRUE;
  }

  value = JS_AtomToValue(ctx, atom);

  if(JS_IsNumber(value)) {
    JS_ToInt64(ctx, &index, value);
    ret = TRUE;
  } else if(JS_IsString(value)) {
    const char* s = JS_ToCString(ctx, value);
    if(isdigit(s[0])) {
      index = atoi(s);
      ret = TRUE;
    }
    JS_FreeCString(ctx, s);
  }

  if(ret == TRUE && index >= 0 && index <= UINT32_MAX)
    *pval = index;

  return ret;
}

static BOOL
js_atom_is_length(JSContext* ctx, JSAtom atom) {
  const char* str = JS_AtomToCString(ctx, atom);
  BOOL ret = !strcmp(str, "length");
  JS_FreeCString(ctx, str);
  return ret;
}

static JSValue
js_invoke(JSContext* ctx, JSValueConst this_obj, const char* method, int argc, JSValueConst argv[]) {
  JSAtom atom;
  JSValue ret;
  atom = JS_NewAtom(ctx, method);
  ret = JS_Invoke(ctx, this_obj, atom, argc, argv);
  JS_FreeAtom(ctx, atom);
  return ret;
}

#endif
