#include "jsbindings.hpp"
#include "js_alloc.hpp"
#include "js_slice_iterator.hpp"

extern "C" {

JSValue slice_iterator_proto = JS_UNDEFINED, slice_iterator_class = JS_UNDEFINED;
VISIBLE JSClassID js_slice_iterator_class_id = 0;

VISIBLE JSSliceIteratorData*
js_slice_iterator_create(JSContext* ctx, JSValueConst buffer, const TypedArrayType& type, int num_elems) {
  JSSliceIteratorData* it;

  if((it = js_allocate<JSSliceIteratorData>(ctx))) {

    it->buffer = JS_DupValue(ctx, buffer);
    it->type = type;
    it->range = js_arraybuffer_range(ctx, buffer);
    it->ptr = it->range.begin();
    it->ctor = js_global_get(ctx, type.constructor_name().c_str());
    it->num_elems = num_elems;
    it->increment = it->type.byte_size * num_elems;
  }
  return it;
}

VISIBLE JSValue
js_slice_iterator_new(JSContext* ctx, JSValueConst buffer, const TypedArrayType& type, int num_elems) {
  JSSliceIteratorData* it;
  JSValue iterator;

  if(js_slice_iterator_class_id == 0)
    js_slice_iterator_init(ctx, 0);

  assert(js_slice_iterator_class_id);

  iterator = JS_NewObjectProtoClass(ctx, slice_iterator_proto, js_slice_iterator_class_id);
  if(JS_IsException(iterator))
    goto fail;
  if(!(it = js_slice_iterator_create(ctx, buffer, type, num_elems)))
    goto fail1;

  JS_SetOpaque(iterator, it);
  return iterator;
fail1:
  JS_FreeValue(ctx, iterator);
fail:
  return JS_EXCEPTION;
}
}

static JSValue
js_slice_iterator_next(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, BOOL* pdone, int magic) {
  JSSliceIteratorData* it;
  JSValue result = JS_UNDEFINED;

  if((it = static_cast<JSSliceIteratorData*>(JS_GetOpaque(this_val, js_slice_iterator_class_id))) == nullptr)
    return JS_EXCEPTION;

  if(!(*pdone = it->ptr >= it->range.end())) {

    // printf("byte_pos = %zu, byte_size = %zu, num_elems = %i, increment = %zu\n", size_t(it->ptr -
    // it->range.begin()), it->range.size(), it->num_elems, it->increment);

    result = js_typedarray_new(ctx, it->buffer, it->ptr - it->range.begin(), it->num_elems, it->ctor);
    it->ptr += it->increment;
  }

  return result;
}

static JSValue
js_slice_iterator_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  JSSliceIteratorData* s;
  JSContourData<double>* v;
  JSValue obj = JS_UNDEFINED;
  JSValue proto;
  int32_t num_elems = 1;
  assert(0);

  if(js_slice_iterator_class_id == 0)
    js_slice_iterator_init(ctx, 0);

  assert(js_slice_iterator_class_id);

  if(!(s = js_allocate<JSSliceIteratorData>(ctx)))
    return JS_EXCEPTION;

  s->buffer = JS_DupValue(ctx, argv[0]);
  s->type = js_typedarray_type(ctx, argv[1]);
  s->range = js_arraybuffer_range(ctx, s->buffer);
  s->ptr = s->range.begin();
  s->ctor = JS_IsFunction(ctx, argv[1]) ? JS_DupValue(ctx, argv[1]) : js_global_get(ctx, s->type.constructor_name().c_str());

  if(argc > 2)
    js_value_to(ctx, argv[2], num_elems);

  s->num_elems = num_elems;
  s->increment = s->type.byte_size * num_elems;

  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;
  obj = JS_NewObjectProtoClass(ctx, proto, js_slice_iterator_class_id);
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
js_slice_iterator_dup(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  return JS_DupValue(ctx, this_val);
}

static void
js_slice_iterator_finalizer(JSRuntime* rt, JSValue val) {
  JSSliceIteratorData* s;
  /* Note: 's' can be NULL in case JS_SetOpaque() was not called */
  if((s = static_cast<JSSliceIteratorData*>(JS_GetOpaque(val, js_slice_iterator_class_id)))) {
    JS_FreeValueRT(rt, s->buffer);
    JS_FreeValueRT(rt, s->ctor);
    js_deallocate(rt, s);
  }
  // JS_FreeValueRT(rt, val);
}

extern "C" {
JSClassDef js_slice_iterator_class = {
    .class_name = "SliceIterator",
    .finalizer = js_slice_iterator_finalizer,
};

const JSCFunctionListEntry js_slice_iterator_proto_funcs[] = {
    JS_ITERATOR_NEXT_DEF("next", 0, js_slice_iterator_next, 0),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, js_slice_iterator_dup),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "SliceIterator", JS_PROP_CONFIGURABLE),
};

int
js_slice_iterator_init(JSContext* ctx, JSModuleDef* m) {

  if(js_slice_iterator_class_id == 0) {
    /* create the SliceIterator class */
    JS_NewClassID(&js_slice_iterator_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_slice_iterator_class_id, &js_slice_iterator_class);

    slice_iterator_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx,
                               slice_iterator_proto,
                               js_slice_iterator_proto_funcs,
                               countof(js_slice_iterator_proto_funcs));
    JS_SetClassProto(ctx, js_slice_iterator_class_id, slice_iterator_proto);

    slice_iterator_class = JS_NewCFunction2(ctx, js_slice_iterator_constructor, "SliceIterator", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */

    JS_SetConstructor(ctx, slice_iterator_class, slice_iterator_proto);
    // JS_SetPropertyFunctionList(ctx, slice_iterator_class, js_slice_iterator_static_funcs,
    // countof(js_slice_iterator_static_funcs));
  }

  if(m)
    JS_SetModuleExport(ctx, m, "SliceIterator", slice_iterator_class);
  /* else
     JS_SetPropertyStr(ctx, *static_cast<JSValue*>(m), name, slice_iterator_class);*/
  return 0;
}

extern "C" VISIBLE void
js_slice_iterator_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "SliceIterator");
}

#if defined(JS_SLICE_ITERATOR_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_slice_iterator
#endif

JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_slice_iterator_init);
  if(!m)
    return NULL;
  js_slice_iterator_export(ctx, m);
  return m;
}

static JSValue
js_slice_iterator_to_string(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  JSSliceIteratorData* s = static_cast<JSSliceIteratorData*>(JS_GetOpaque2(ctx, this_val, js_slice_iterator_class_id));
  std::ostringstream os;
  if(!s)
    return JS_EXCEPTION;

  // os << "{x:" << s->x << ",y:" << s->y << "}" << std::endl;

  return JS_NewString(ctx, os.str().c_str());
}
}