#ifndef JS_SLICE_ITERATOR_HPP
#define JS_SLICE_ITERATOR_HPP

#include "jsbindings.hpp"
#include "js_typed_array.hpp"
#include <ranges>

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
  std::ranges::subrange<uint8_t*> range;
  int num_elems;
  size_t increment;
  TypedArrayType type;
  JSValue buffer, ctor;
};

#endif /* defined(JS_SLICE_ITERATOR_HPP) */