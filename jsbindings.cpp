#include "jsbindings.hpp"
#include "js_array.hpp"
#include <quickjs.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>

template<class T> class jsallocator {
public:
  typedef T value_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  template<class U> struct rebind { typedef jsallocator<U> other; };
  pointer
  address(reference value) const {
    return &value;
  }
  const_pointer
  address(const_reference value) const {
    return &value;
  }
  jsallocator() throw() {}
  jsallocator(const jsallocator&) throw() {}
  template<class U> jsallocator(const jsallocator<U>&) throw() {}
  ~jsallocator() throw() {}
  size_type
  max_size() const throw() {
    return std::numeric_limits<std::size_t>::max() / sizeof(T);
  }
  pointer
  allocate(size_type num, const void* = 0) {
    pointer ret;
    std::cerr << "allocate " << num << " element(s)"
              << " of size " << sizeof(T) << std::endl;

    std::cerr << " allocated at: " << (void*)ret << std::endl;
    return ret;
  }
  void
  construct(pointer p, const T& value) {
    p->T(value);
  }
  void
  destroy(pointer p) {
    p->~T();
  }
  void
  deallocate(pointer p, size_type num) {
    std::cerr << "deallocate " << num << " element(s)"
              << " of size " << sizeof(T) << " at: " << (void*)p << std::endl;
    js_free(p);
  }
};

int
js_color_read(JSContext* ctx, JSValueConst color, JSColorData<double>* out) {
  int ret = 1;
  std::array<double, 4> c = {0, 0, 0, 255};
  if(JS_IsObject(color)) {
    JSValue v[4];
    if(js_is_array(ctx, color)) {
      v[0] = JS_GetPropertyUint32(ctx, color, 0);
      v[1] = JS_GetPropertyUint32(ctx, color, 1);
      v[2] = JS_GetPropertyUint32(ctx, color, 2);
      v[3] = JS_GetPropertyUint32(ctx, color, 3);
    } else {
      v[0] = JS_GetPropertyStr(ctx, color, "r");
      v[1] = JS_GetPropertyStr(ctx, color, "g");
      v[2] = JS_GetPropertyStr(ctx, color, "b");
      v[3] = JS_GetPropertyStr(ctx, color, "a");
    }

    if(!(JS_IsNumber(v[0]) && JS_IsNumber(v[1]) && JS_IsNumber(v[2]) /* && JS_IsNumber(v[3])*/)) {
      ret = 0;
    } else {
      JS_ToFloat64(ctx, &c[0], v[0]);
      JS_ToFloat64(ctx, &c[1], v[1]);
      JS_ToFloat64(ctx, &c[2], v[2]);
      if(JS_IsNumber(v[3]))
        JS_ToFloat64(ctx, &c[3], v[3]);
    }

    JS_FreeValue(ctx, v[0]);
    JS_FreeValue(ctx, v[1]);
    JS_FreeValue(ctx, v[2]);
    JS_FreeValue(ctx, v[3]);
  } else if(JS_IsNumber(color)) {
    uint32_t value;
    JS_ToUint32(ctx, &value, color);
    c[0] = value & 0xff;
    c[1] = (value >> 8) & 0xff;
    c[2] = (value >> 16) & 0xff;
    c[3] = (value >> 24) & 0xff;
  } else {
    ret = 0;
  }

  std::copy(c.cbegin(), c.cend(), out->arr.begin());

  return ret;
}

int
js_color_read(JSContext* ctx, JSValueConst value, JSColorData<uint8_t>* out) {
  JSColorData<double> color;

  if(js_is_array(ctx, value)) {
    std::array<uint8_t, 4> a;
    if(js_array_to(ctx, value, a) >= 3) {
      out->arr[0] = a[0];
      out->arr[1] = a[1];
      out->arr[2] = a[2];
      out->arr[3] = a[3];
      return 1;
    }
  }

  if(js_color_read(ctx, value, &color)) {
    out->arr[0] = color.arr[0];
    out->arr[1] = color.arr[1];
    out->arr[2] = color.arr[2];
    out->arr[3] = color.arr[3];
    return 1;
  }

  return 0;
}

int
js_ref(JSContext* ctx, const char* name, JSValueConst arg, JSValue value) {
  if(JS_IsFunction(ctx, arg)) {
    JSValueConst v = value;
    JS_Call(ctx, arg, JS_UNDEFINED, 1, &v);
  } else if(js_is_array(ctx, arg)) {
    JS_SetPropertyUint32(ctx, arg, 0, value);
  } else if(JS_IsObject(arg)) {
    JS_SetPropertyStr(ctx, arg, name, value);
  } else {
    return 0;
  }

  return 1;
}
 
