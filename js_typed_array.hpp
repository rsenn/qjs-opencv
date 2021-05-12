#ifndef JS_TYPED_ARRAY_HPP
#define JS_TYPED_ARRAY_HPP

#include <type_traits>
#include <cstdint>
#include <opencv2/core/traits.hpp>
#include "util.hpp"
#include "jsbindings.hpp"

template<class T> struct number_type { static constexpr bool typed_array = false; };

template<> struct number_type<int8_t> {
  typedef int8_t value_type;
  static constexpr bool typed_array = true;
  static constexpr const char*
  constructor_name() {
    return "Int8Array";
  }
};

template<> struct number_type<uint8_t> {
  typedef uint8_t value_type;
  static constexpr bool typed_array = true;
  static constexpr const char*
  constructor_name() {
    return "Uint8Array";
  }
};

template<> struct number_type<int16_t> {
  typedef int16_t value_type;
  static constexpr bool typed_array = true;
  static constexpr const char*
  constructor_name() {
    return "Int16Array";
  }
};

template<> struct number_type<uint16_t> {
  typedef uint16_t value_type;
  static constexpr bool typed_array = true;
  static constexpr const char*
  constructor_name() {
    return "Uint16Array";
  }
};

template<> struct number_type<int32_t> {
  typedef int32_t value_type;
  static constexpr bool typed_array = true;
  static constexpr const char*
  constructor_name() {
    return "Int32Array";
  }
};

template<> struct number_type<uint32_t> {
  typedef uint32_t value_type;
  static constexpr bool typed_array = true;
  static constexpr const char*
  constructor_name() {
    return "Uint32Array";
  }
};

template<> struct number_type<int64_t> {
  typedef int64_t value_type;
  static constexpr bool typed_array = true;
  static constexpr const char*
  constructor_name() {
    return "BigInt64Array";
  }
};

template<> struct number_type<uint64_t> {
  typedef uint64_t value_type;
  static constexpr bool typed_array = true;
  static constexpr const char*
  constructor_name() {
    return "BigUint64Array";
  }
};

template<> struct number_type<float> {
  typedef float value_type;
  static constexpr bool typed_array = true;
  static constexpr const char*
  constructor_name() {
    return "Float32Array";
  }
};

template<> struct number_type<double> {
  typedef double value_type;
  static constexpr bool typed_array = true;
  static constexpr const char*
  constructor_name() {
    return "Float64Array";
  }
};

template<class T> struct pointer_type {
  typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type value_type;
  static constexpr bool typed_array = number_type<value_type>::typed_array;
};

template<class T>
inline typename std::enable_if<number_type<T>::typed_array, JSValue>::type
js_array_from(JSContext* ctx, const T* start, const T* end) {
  JSValue buf, global, ctor;
  JSValueConst args[3];
  const char* name;
  const uint8_t *s, *e;
  s = reinterpret_cast<const uint8_t*>(start);
  e = reinterpret_cast<const uint8_t*>(end);
  buf = JS_NewArrayBufferCopy(ctx, s, e - s);
  name = number_type<T>::constructor_name();
  global = JS_GetGlobalObject(ctx);
  ctor = JS_GetPropertyStr(ctx, global, name);
  args[0] = buf;
  return JS_CallConstructor(ctx, ctor, 1, args);
}

enum {
  TYPEDARRAY_FLOATING_POINT = 0x80,
  TYPEDARRAY_INTEGER = 0x00,
  TYPEDARRAY_SIGNED = 0x40,
  TYPEDARRAY_UNSIGNED = 0x00,
  TYPEDARRAY_BITS_8 = 0x01,
  TYPEDARRAY_BITS_16 = 0x02,
  TYPEDARRAY_BITS_32 = 0x04,
  TYPEDARRAY_BITS_64 = 0x08,
  TYPEDARRAY_BITS_FIELD = 0x3f
};

enum TypedArrayValue {
  TYPEDARRAY_UINT8 = TYPEDARRAY_UNSIGNED | TYPEDARRAY_BITS_8,
  TYPEDARRAY_INT8 = TYPEDARRAY_SIGNED | TYPEDARRAY_BITS_8,
  TYPEDARRAY_UINT16 = TYPEDARRAY_UNSIGNED | TYPEDARRAY_BITS_16,
  TYPEDARRAY_INT16 = TYPEDARRAY_SIGNED | TYPEDARRAY_BITS_16,
  TYPEDARRAY_UINT32 = TYPEDARRAY_UNSIGNED | TYPEDARRAY_BITS_32,
  TYPEDARRAY_INT32 = TYPEDARRAY_SIGNED | TYPEDARRAY_BITS_32,
  TYPEDARRAY_BIGUINT64 = TYPEDARRAY_UNSIGNED | TYPEDARRAY_BITS_64,
  TYPEDARRAY_BIGINT64 = TYPEDARRAY_SIGNED | TYPEDARRAY_BITS_64,
  TYPEDARRAY_FLOAT32 = TYPEDARRAY_FLOATING_POINT | TYPEDARRAY_BITS_32,
  TYPEDARRAY_FLOAT64 = TYPEDARRAY_FLOATING_POINT | TYPEDARRAY_BITS_64
};

struct TypedArrayType {
  TypedArrayType(int bsize, bool sig, bool flt) : byte_size(bsize), is_signed(sig), is_floating_point(flt) {}
  TypedArrayType(const cv::Mat& mat)
      : byte_size(1 << (mat_depth(mat) >> 1)), is_signed(mat_signed(mat)), is_floating_point(mat_floating(mat)) {}
  TypedArrayType(const cv::UMat& mat)
      : byte_size(1 << (mat_depth(mat) >> 1)), is_signed(mat_signed(mat)), is_floating_point(mat_floating(mat)) {}
  TypedArrayType(int32_t cvId)
      : byte_size(1 << (mattype_depth(cvId) >> 1)), is_signed(mattype_signed(cvId)), is_floating_point(mattype_floating(cvId)) {
  }
  TypedArrayType(TypedArrayValue i)
      : byte_size(i & TYPEDARRAY_BITS_FIELD), is_signed(!!(i & TYPEDARRAY_SIGNED)),
        is_floating_point(!!(i & TYPEDARRAY_FLOATING_POINT)) {}

  template<class T> TypedArrayType(JSContext* ctx, const T& ctor_name) { *this = js_typedarray_type(ctx, ctor_name); }

  int byte_size;
  bool is_signed;
  bool is_floating_point;

  const std::string
  constructor_name() const {
    std::ostringstream os;
    if(!is_floating_point) {
      if(byte_size == 8)
        os << "Big";
      os << (is_signed ? "Int" : "Uint");
    } else {
      os << "Float";
    }
    os << (byte_size * 8);
    os << "Array";
    return os.str();
  }

  int32_t
  cv_type() const {
    if(is_floating_point)
      return int32_t(byte_size == 8 ? CV_64F : CV_32F);

    switch(byte_size) {
      case 1: return int32_t(is_signed ? CV_8S : CV_8U);
      case 2: return int32_t(is_signed ? CV_16S : CV_16U);
      case 4: return int32_t(is_signed ? CV_32S : CV_32F);
    }
    return -1;
  }

  TypedArrayValue
  flags() const {
    return TypedArrayValue(uint8_t(is_floating_point ? TYPEDARRAY_FLOATING_POINT : 0) |
                           uint8_t(is_signed ? TYPEDARRAY_SIGNED : 0) | uint8_t(byte_size) & TYPEDARRAY_BITS_FIELD);
  }
  operator TypedArrayValue() const { return flags(); }
};

struct TypedArrayProps {
  size_t byte_offset, byte_length, bytes_per_element;
  ArrayBufferProps buffer;

  template<class T>
  const T*
  ptr() const {
    return reinterpret_cast<T*>(buffer.ptr + byte_offset);
  }

  /*  template<class T>
    T*
    ptr() {
      return reinterpret_cast<T*>(buffer.ptr + byte_offset);
    }*/

  template<class T>
  int
  size() const {
    return byte_length / sizeof(T);
    ;
  }
  size_t
  size() const {
    return byte_length / bytes_per_element;
  }
};

template<class T> struct TypedArrayRange : public TypedArrayProps {
  TypedArrayRange(const TypedArrayProps& props) : TypedArrayProps(props) {}

  const T*
  begin() const {
    return ptr<T>();
  }

  const T*
  end() const {
    return begin() + size<T>();
  }
};

template<class T> struct TypedArrayTraits {
  typedef typename std::remove_cvref<T>::type value_type;

  static_assert(std::is_arithmetic<T>::value, "TypedArray must contain arithmetic type");
  static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
                "TypedArray must contain type of size 1, 2, 4 or 8");

  static constexpr size_t byte_size = sizeof(T);

  static constexpr bool is_signed = std::is_signed<value_type>::value;
  static constexpr bool is_floating_point = std::is_floating_point<value_type>::value;

  static TypedArrayType
  getProps() {
    return TypedArrayType(byte_size, is_signed, is_floating_point);
  }
};

static inline JSValue
js_typedarray_new(JSContext* ctx, JSValueConst buffer, uint32_t byteOffset, uint32_t length, JSValueConst ctor) {
  std::array<JSValueConst, 3> args = {buffer, js_number_new(ctx, byteOffset), js_number_new(ctx, length)};

  return JS_CallConstructor(ctx, ctor, args.size(), &args[0]);
}

static inline JSValue
js_typedarray_new(JSContext* ctx, JSValueConst buffer, uint32_t byteOffset, uint32_t length, const char* ctor_name) {
  JSValue global, ctor, ret;
  std::array<JSValueConst, 3> args = {buffer, js_number_new(ctx, byteOffset), js_number_new(ctx, length)};
  global = JS_GetGlobalObject(ctx);
  ctor = JS_GetPropertyStr(ctx, global, ctor_name);
  JS_FreeValue(ctx, global);
  ret = js_typedarray_new(ctx, buffer, byteOffset, length, ctor);
  JS_FreeValue(ctx, ctor);
  return ret;
}

static inline JSValue
js_typedarray_new(JSContext* ctx, JSValueConst buffer, uint32_t byteOffset, uint32_t length, const TypedArrayType& props) {
  auto range = js_arraybuffer_range(ctx, buffer);
  assert(byteOffset + length * props.byte_size <= range.size());

  return js_typedarray_new(ctx, buffer, byteOffset, length, props.constructor_name().c_str());
}

template<class Iterator>
static inline typename std::enable_if<std::is_pointer<Iterator>::value>::type
js_typedarray_remain(Iterator& start, Iterator& end, uint32_t byteOffset, uint32_t& length) {
  typedef typename std::remove_pointer<Iterator>::type value_type;

  const uint8_t* ptr;
  size_t len;
  ptr = reinterpret_cast<const uint8_t*>(start) + byteOffset;
  len = reinterpret_cast<const uint8_t*>(end) - ptr;

  len /= sizeof(value_type);

  if(length > len)
    len = length;
}

template<class T> class js_typedarray {
public:
  template<class Container>
  static JSValue
  from(JSContext* ctx, const Container& in) {
    return from_sequence<typename Container::const_iterator>(ctx, in.begin(), in.end());
  }

  static JSValue
  from_vector(JSContext* ctx, const std::vector<T>& in) {
    return from_sequence(ctx, in.begin(), in.end());
  }

  template<class Iterator>
  static JSValue
  from_sequence(
      JSContext* ctx, const Iterator& start, const Iterator& end, uint32_t byteOffset = 0, uint32_t length = UINT32_MAX) {
    JSValue buf = js_arraybuffer_from(ctx, start, end);
    uint32_t count = std::min<uint32_t>(length, end - start);

    js_typedarray_remain(start, end, byteOffset, count);

    return js_typedarray_new(ctx, buf, 0, count, TypedArrayTraits<T>::getProps());
  }

  template<size_t N> static int64_t to_array(JSContext* ctx, JSValueConst arr, std::array<T, N>& out);
  static int64_t to_scalar(JSContext* ctx, JSValueConst arr, cv::Scalar_<T>& out);
};

template<class Iterator>
static inline typename std::enable_if<std::is_pointer<Iterator>::value, JSValue>::type
js_typedarray_from(
    JSContext* ctx, const Iterator& start, const Iterator& end, uint32_t byteOffset = 0, uint32_t length = UINT32_MAX) {
  return js_typedarray<typename std::remove_pointer<Iterator>::type>::from_sequence(ctx, start, end, byteOffset);
}

template<class Iterator>
static inline typename std::enable_if<Iterator::value_type, JSValue>::type
js_typedarray_from(
    JSContext* ctx, const Iterator& start, const Iterator& end, uint32_t byteOffset = 0, uint32_t length = UINT32_MAX) {
  return js_typedarray<typename Iterator::value_type>::from_sequence(ctx, start, end, byteOffset);
}

template<class Container>
static inline JSValue
js_typedarray_from(JSContext* ctx, const Container& v, uint32_t byteOffset = 0, uint32_t length = UINT32_MAX) {
  return js_typedarray<typename Container::value_type>::from_sequence(ctx, v.begin(), v.end(), byteOffset);
}

static inline TypedArrayType
js_typedarray_type(const std::string& class_name) {
  char* start = const_cast<char*>(class_name.data());
  char* end = start + class_name.size();
  bool is_signed = true, is_floating_point = false;

  if(start < end && *start == 'U') {
    start++;
    is_signed = false;
    *start = toupper(*start);
  }

  char* num_start = std::find_if(start, end, &::isdigit);
  char* num_end = std::find_if_not(num_start, end, &::isdigit);
  char* next;
  const auto bits = strtoul(num_start, &next, 10);

  assert(next == num_end);
  assert(bits == 8 || bits == 16 || bits == 32 || bits == 64);

  is_floating_point = !strncmp(start, "Float", 5);

  return TypedArrayType(bits / 8, is_signed, is_floating_point);
}

static inline TypedArrayType
js_typedarray_type(JSContext* ctx, JSValueConst obj) {
  std::string class_name;

  if(JS_IsFunction(ctx, obj))
    class_name = js_function_name(ctx, obj);
  else if(JS_IsString(obj))
    js_value_to(ctx, obj, class_name);
  else
    class_name = js_class_name(ctx, obj);

  return js_typedarray_type(class_name);
}

static inline TypedArrayProps
js_typedarray_props(JSContext* ctx, JSValueConst obj) {
  JSValue buffer;
  size_t byte_offset, byte_length, bytes_per_element;

  buffer = JS_GetTypedArrayBuffer(ctx, obj, &byte_offset, &byte_length, &bytes_per_element);

  return TypedArrayProps(byte_offset, byte_length, bytes_per_element, js_arraybuffer_props(ctx, buffer));
}

static inline JSInputOutputArray
js_typedarray_inputoutputarray(JSContext* ctx, JSValueConst obj) {

  TypedArrayType type = js_typedarray_type(ctx, obj);
  TypedArrayProps props = js_typedarray_props(ctx, obj);

  switch(type.flags()) {
    case TYPEDARRAY_UINT8: return JSInputOutputArray(props.ptr<uint8_t>(), props.size<uint8_t>());
    case TYPEDARRAY_INT8: return JSInputOutputArray(props.ptr<int8_t>(), props.size<int8_t>());
    case TYPEDARRAY_UINT16: return JSInputOutputArray(props.ptr<uint16_t>(), props.size<uint16_t>());
    case TYPEDARRAY_INT16: return JSInputOutputArray(props.ptr<int16_t>(), props.size<int16_t>());
    /*case TYPEDARRAY_UINT32: {
      TypedArrayRange<uint32_t> range(props);
      return std::vector<uint32_t>(range.begin(), range.end());
    }*/
    case TYPEDARRAY_INT32: return JSInputOutputArray(props.ptr<int>(), props.size<int>());
    /*case TYPEDARRAY_BIGUINT64: {
      TypedArrayRange<uint64_t> range(props);
      return std::vector<uint64_t>(range.begin(), range.end());
    }
    case TYPEDARRAY_BIGINT64: {
      TypedArrayRange<int64_t> range(props);
      return std::vector<int64_t>(range.begin(), range.end());
    }*/
    case TYPEDARRAY_FLOAT32: return JSInputOutputArray(props.ptr<float>(), props.size<float>());
    case TYPEDARRAY_FLOAT64: return JSInputOutputArray(props.ptr<double>(), props.size<double>());
    default: {
      std::string name = type.constructor_name();

      JS_ThrowTypeError(ctx, "Expected TypedArray %s", name.c_str());
      break;
    }
  }

  return cv::noArray();
}
#endif /* defined(JS_TYPED_ARRAY_HPP) */
