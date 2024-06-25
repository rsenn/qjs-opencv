#ifndef JS_POINT_ITERATOR_HPP
#define JS_POINT_ITERATOR_HPP

#include "jsbindings.hpp"
#include <quickjs.h>
#include <utility>

extern "C" int js_point_iterator_init(JSContext*, JSModuleDef*);

extern "C" {
enum JSPointIteratorMagic { NEXT_POINT = 0, NEXT_LINE };

extern thread_local JSValue point_iterator_proto, point_iterator_class;
extern thread_local JSClassID js_point_iterator_class_id;

JSValue js_point_iterator_new(JSContext* ctx, JSPointData<double>* first, JSPointData<double>* last, int magic);

int js_point_iterator_init(JSContext*, JSModuleDef* m);

JSModuleDef* js_init_module_point_iterator(JSContext*, const char* module_name);
}

struct JSPointIteratorData : public std::pair<JSPointData<double>*, JSPointData<double>*> {
  JSPointIteratorMagic magic;
};

#endif /* defined(JS_POINT_ITERATOR_HPP) */
