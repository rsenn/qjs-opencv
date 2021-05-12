//#include "plot-cv.hpp"
#include "geometry.hpp"
#include "jsbindings.hpp"
#include "js_point.hpp"
#include "js_size.hpp"
#include "js_rect.hpp"
#include "js_contour.hpp"
#include "js_mat.hpp"
#include "js_point_iterator.hpp"
#include "quickjs/cutils.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iomanip>

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

/*extern "C" {}

JSValue int32array_proto, int32array_ctor;
JSClassID int32array_class_id;*/

extern "C++" template<class Type> JSValue js_contour_new(JSContext* ctx, const std::vector<Type>& points);

/*JSValue
js_vector_vec4i_to_array(JSContext* ctx, const std::vector<cv::Vec4i>& vec) {
  JSValue ret = JS_NewArray(ctx);
  uint32_t i, j, n = vec.size();
  for(i = 0; i < n; i++) {
    JSValue item = JS_NewObjectProto(ctx, int32array_proto);
    for(j = 0; j < 4; j++) { JS_SetPropertyUint32(ctx, item, j, JS_NewInt32(ctx, vec[i][j])); }
    JS_SetPropertyUint32(ctx, ret, i, item);
  }
  return ret;
}*/

int
js_color_read(JSContext* ctx, JSValueConst color, JSColorData<double>* out) {
  int ret = 1;
  std::array<double, 4> c;
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
    JS_ToFloat64(ctx, &c[0], v[0]);
    JS_ToFloat64(ctx, &c[1], v[1]);
    JS_ToFloat64(ctx, &c[2], v[2]);
    JS_ToFloat64(ctx, &c[3], v[3]);

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

#ifdef JS_BINDINGS_INIT_MODULE
static int
js_bindings_init(JSContext* ctx, JSModuleDef* m) {
  js_point_init(ctx, m);
  js_point_iterator_init(ctx, m);
  js_size_init(ctx, m);
  js_rect_init(ctx, m);
  js_mat_init(ctx, m);
  js_contour_init(ctx, m);
  js_draw_init(ctx, m);
  js_video_capture_init(ctx, m);
  return 0;
}

extern "C" JSModuleDef*

js_init_module(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  m = JS_NewCModule(ctx, module_name, &js_bindings_init);

  return m;
}

#endif
