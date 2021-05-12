#ifndef JS_POINT_ITERATOR_HPP
#define JS_POINT_ITERATOR_HPP

#include "jsbindings.hpp"
#include <ranges>

extern "C" VISIBLE int js_point_iterator_init(JSContext*, JSModuleDef*);

extern "C" {
enum JSPointIteratorMagic { NEXT_POINT = 0, NEXT_LINE };

extern JSValue point_iterator_proto, point_iterator_class;
extern JSClassID js_point_iterator_class_id;

VISIBLE JSValue js_point_iterator_new(JSContext* ctx, const std::ranges::subrange<JSPointData<double>*>& range, int magic);

int js_point_iterator_init(JSContext*, JSModuleDef* m);

JSModuleDef* js_init_module_point_iterator(JSContext*, const char* module_name);
}

struct JSPointIteratorData : public std::pair<JSPointData<double>*, JSPointData<double>*> {
  JSPointIteratorMagic magic;
};

#endif /* defined(JS_POINT_ITERATOR_HPP) */