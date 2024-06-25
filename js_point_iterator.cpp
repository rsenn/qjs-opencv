#include "js_point_iterator.hpp"
#include "cutils.h"
#include "js_alloc.hpp"
#include "js_line.hpp"
#include "js_point.hpp"
#include "jsbindings.hpp"
#include <opencv2/core/types.hpp>
#include <quickjs.h>
#include <cassert>
#include <stddef.h>
#include <iosfwd>
#include <new>
#include <string>

extern "C" {

JSValue point_iterator_proto = JS_UNDEFINED, point_iterator_class = JS_UNDEFINED;
thread_local JSClassID js_point_iterator_class_id = 0;

JSValue
js_point_iterator_new(JSContext* ctx, JSPointData<double>* first, JSPointData<double>* last, int magic) {
  JSPointIteratorData* it;
  JSValue iterator;

  if(js_point_iterator_class_id == 0)
    js_point_iterator_init(ctx, 0);

  assert(js_point_iterator_class_id);

  iterator = JS_NewObjectProtoClass(ctx, point_iterator_proto, js_point_iterator_class_id);
  
  if(JS_IsException(iterator))
    goto fail;
  
  if(!(it = js_allocate<JSPointIteratorData>(ctx)))
    goto fail1;

  new(it) JSPointIteratorData();

  it->magic = JSPointIteratorMagic(magic);
  it->first = first;
  it->second = last;

  JS_SetOpaque(iterator, it);
  return iterator;

fail1:
  JS_FreeValue(ctx, iterator);

fail:
  return JS_EXCEPTION;
}
}

static JSValue
js_point_iterator_next(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], BOOL* pdone, int magic) {
  JSPointIteratorData* it;
  JSValue result;

  if((it = static_cast<JSPointIteratorData*>(JS_GetOpaque(this_val, js_point_iterator_class_id))) == nullptr)
    return JS_EXCEPTION;

  switch(it->magic) {
    case NEXT_POINT: {
      *pdone = it->first == nullptr || it->second == nullptr || (it->first == it->second);
      result = *pdone ? JS_NULL : js_point_new(ctx, it->first->x, it->first->y);
      break;
    }

    case NEXT_LINE: {
      *pdone = it->first == nullptr || it->second == nullptr || (it->first + 1 >= it->second);
      result = *pdone ? JS_NULL : js_line_new(ctx, it->first[0], it->first[1]);
      break;
    }
  }

  it->first++;
  
  return result;
}

static JSValue
js_point_iterator_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSPointIteratorData* s;
  JSContourData<double>* v;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;
  assert(0);

  if(js_point_iterator_class_id == 0)
    js_point_iterator_init(ctx, 0);

  assert(js_point_iterator_class_id);

  s = js_allocate<JSPointIteratorData>(ctx);
  if(!s)
    return JS_EXCEPTION;

  new(s) JSPointIteratorData();

  v = static_cast<JSContourData<double>*>(JS_GetOpaque(argv[0], 0 /*js_contour_class_id*/));

  s->first = &(*v)[0];
  s->second = s->first + v->size();

  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  
  if(JS_IsException(proto))
    goto fail;
  
  obj = JS_NewObjectProtoClass(ctx, proto, js_point_iterator_class_id);
  JS_FreeValue(ctx, proto);
  
  if(JS_IsException(obj))
    goto fail;
  
  JS_SetOpaque(obj, s);
  return obj;

fail:
  js_deallocate(ctx, s);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

static JSValue
js_point_iterator_dup(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  return JS_DupValue(ctx, this_val);
}
static void
js_point_iterator_finalizer(JSRuntime* rt, JSValue val) {
  JSPointIteratorData* s = static_cast<JSPointIteratorData*>(JS_GetOpaque(val, js_point_iterator_class_id));
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */

  if(s != nullptr)
    js_deallocate(rt, s);

  // JS_FreeValueRT(rt, val);
}

extern "C" {
JSClassDef js_point_iterator_class = {
    .class_name = "PointIterator",
    .finalizer = js_point_iterator_finalizer,
};

const JSCFunctionListEntry js_point_iterator_proto_funcs[] = {
    JS_ITERATOR_NEXT_DEF("next", 0, js_point_iterator_next, 0),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, js_point_iterator_dup),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "PointIterator", JS_PROP_CONFIGURABLE),
};

int
js_point_iterator_init(JSContext* ctx, JSModuleDef* m) {

  if(js_point_iterator_class_id == 0) {
    /* create the PointIterator class */
    JS_NewClassID(&js_point_iterator_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_point_iterator_class_id, &js_point_iterator_class);

    point_iterator_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, point_iterator_proto, js_point_iterator_proto_funcs, countof(js_point_iterator_proto_funcs));
    JS_SetClassProto(ctx, js_point_iterator_class_id, point_iterator_proto);

    point_iterator_class = JS_NewCFunction2(ctx, js_point_iterator_constructor, "PointIterator", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */

    JS_SetConstructor(ctx, point_iterator_class, point_iterator_proto);
    // JS_SetPropertyFunctionList(ctx, point_iterator_class, js_point_iterator_static_funcs,
    // countof(js_point_iterator_static_funcs));
  }

  if(m)
    JS_SetModuleExport(ctx, m, "PointIterator", point_iterator_class);

  return 0;
}

extern "C" void
js_point_iterator_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "PointIterator");
}

#if defined(JS_POINT_ITERATOR_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_point_iterator
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if(!(m = JS_NewCModule(ctx, module_name, &js_point_iterator_init)))
    return NULL;
  
  js_point_iterator_export(ctx, m);
  return m;
}

static JSValue
js_point_iterator_to_string(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[]) {
  JSPointIteratorData* s;
  std::ostringstream os;
 
  if(!(s = static_cast<JSPointIteratorData*>(JS_GetOpaque2(ctx, this_val, js_point_iterator_class_id))))
    return JS_EXCEPTION;

  // os << "{x:" << s->x << ",y:" << s->y << "}" << std::endl;

  return JS_NewString(ctx, os.str().c_str());
}
}
