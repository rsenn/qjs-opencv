#ifndef JS_SLICE_ITERATOR_HPP
#define JS_SLICE_ITERATOR_HPP

#include "js_typed_array.hpp"
#include "include/jsbindings.hpp"
#include <quickjs.h>
#include "include/util.hpp"
#include <stddef.h>
#include <cstdint>

extern "C" int js_slice_iterator_init(JSContext*, JSModuleDef*);

extern "C" {

extern thread_local JSValue slice_iterator_proto, slice_iterator_class;
extern thread_local JSClassID js_slice_iterator_class_id;

JSValue js_slice_iterator_new(JSContext* ctx, JSValueConst buffer, const TypedArrayType& type, int num_elems);

int js_slice_iterator_init(JSContext*, JSModuleDef* m);

JSModuleDef* js_init_module_slice_iterator(JSContext*, const char* module_name);
}

struct JSSliceIteratorData {
  uint8_t* ptr;
  range_view<uint8_t> range;
  int num_elems;
  size_t increment;
  TypedArrayType type;
  JSValue buffer, ctor;
};

#endif /* defined(JS_SLICE_ITERATOR_HPP) */
