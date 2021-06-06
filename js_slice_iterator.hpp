#ifndef JS_SLICE_ITERATOR_HPP
#define JS_SLICE_ITERATOR_HPP

#include "js_typed_array.hpp"
#include "jsbindings.hpp"
#include <quickjs.h>
#include "util.hpp"
#include <stddef.h>
#include <cstdint>

extern "C" VISIBLE int js_slice_iterator_init(JSContext*, JSModuleDef*);

extern "C" {

extern JSValue slice_iterator_proto, slice_iterator_class;
extern JSClassID js_slice_iterator_class_id;

VISIBLE JSValue js_slice_iterator_new(JSContext* ctx, JSValueConst buffer, const TypedArrayType& type, int num_elems);

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