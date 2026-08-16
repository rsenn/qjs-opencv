#ifndef JS_VECTOR_HPP
#define JS_VECTOR_HPP

#include <quickjs.h>
#include <opencv2/core.hpp>
#include <vector>
#include <string>

#include "include/jsbindings.hpp"
#include "include/js_converter.hpp"

extern "C" {

int js_vector_init(JSContext*, JSModuleDef*);
void js_vector_export(JSContext*, JSModuleDef*);
}

/**
 * @brief Get a JSOutputArray view of a JSVector<T>, for whichever T it holds
 *
 * Tries every registered vector element type in turn and returns an
 * _OutputArray wrapping the matching JSVector<T>'s underlying
 * std::vector<T>. Returns cv::noArray() if value isn't any registered
 * JSVector<T>.
 */
JSOutputArray js_vector_outputarray(JSValueConst value);

template<typename T> struct JSVectorRegistry;
template<typename T> class JSVectorIterator;

/**
 * @brief Generic JSVector template class
 *
 * Provides common operations for all vector container types:
 * - constructor: create empty vector
 * - push_back: append element
 * - get: retrieve element at index
 * - set: update element at index
 * - size: get number of elements
 * - delete: manual cleanup
 */
template<typename T> class JSVector {
public:
  using VectorType = std::vector<T>;
  using Converter = JSConverter<T>;

  VectorType* vec;

  JSVector() : vec(new VectorType()) {}
  ~JSVector() { delete vec; }

  /**
   * @brief Get the class ID for this vector type
   *
   * Each vector type needs its own class ID.
   */
  static JSClassID& get_class_id() {
    static JSClassID class_id = 0;
    return class_id;
  }

  /**
   * @brief Get the constructor for this vector type
   *
   * Stores the constructor so it can be exported later.
   */
  static JSValue& get_ctor() {
    static JSValue ctor = JS_UNDEFINED;
    return ctor;
  }

  /**
   * @brief Get the vector data from a JS object
   */
  static JSVector<T>* fromJS(JSValueConst this_val) { return static_cast<JSVector<T>*>(JS_GetOpaque(this_val, get_class_id())); }
  static JSVector<T>* fromJS(JSContext* ctx, JSValueConst this_val) { return static_cast<JSVector<T>*>(JS_GetOpaque2(ctx, this_val, get_class_id())); }

  operator JSInputArray() const { return JSInputArray(*vec); }
  operator JSInputOutputArray() { return JSInputOutputArray(*vec); }
  operator JSOutputArray() { return JSOutputArray(*vec); }

  /**
   * @brief Create a new JS object wrapping this vector
   */
  JSValue toJS(JSContext* ctx) {
    JSValue obj = JS_NewObjectClass(ctx, get_class_id());

    if(JS_IsException(obj)) {
      delete this;
      return JS_EXCEPTION;
    }

    JS_SetOpaque(obj, this);
    return obj;
  }

  /**
   * @brief Constructor: new VectorType()
   */
  static JSValue constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
    JSClassID class_id = get_class_id();
    JSValue obj = JS_NewObjectClass(ctx, class_id);
    if(JS_IsException(obj))
      return JS_EXCEPTION;

    JSVector<T>* vector = new JSVector<T>();
    JS_SetOpaque(obj, vector);

    return obj;
  }

  /**
   * @brief push_back(element)
   */
  static JSValue push_back(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    JSVector<T>* vector;

    if(!(vector = fromJS(ctx, this_val)))
      return JS_EXCEPTION;

    if(argc < 1)
      return JS_ThrowTypeError(ctx, "push_back requires 1 argument");

    T element = Converter::fromJS(ctx, argv[0]);
    vector->vec->push_back(element);

    return JS_UNDEFINED;
  }

  /**
   * @brief get(index)
   */
  static JSValue get(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    JSVector<T>* vector;

    if(!(vector = fromJS(ctx, this_val)))
      return JS_EXCEPTION;

    if(argc < 1)
      return JS_ThrowTypeError(ctx, "get requires 1 argument");

    int index;
    JS_ToInt32(ctx, &index, argv[0]);

    if(index < 0 || index >= (int)vector->vec->size())
      return JS_ThrowRangeError(ctx, "get: index out of range");

    return Converter::toJS(ctx, (*vector->vec)[index]);
  }

  /**
   * @brief set(index, element)
   */
  static JSValue set(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    JSVector<T>* vector;

    if(!(vector = fromJS(ctx, this_val)))
      return JS_EXCEPTION;

    if(argc < 2)
      return JS_ThrowTypeError(ctx, "set requires 2 arguments");

    int index;
    JS_ToInt32(ctx, &index, argv[0]);

    if(index < 0 || index >= (int)vector->vec->size())
      return JS_ThrowRangeError(ctx, "set: index out of range");

    (*vector->vec)[index] = Converter::fromJS(ctx, argv[1]);

    return JS_UNDEFINED;
  }

  /**
   * @brief size()
   */
  static JSValue size(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    JSVector<T>* vector;

    if(!(vector = fromJS(ctx, this_val)))
      return JS_EXCEPTION;

    return JS_NewInt32(ctx, vector->vec->size());
  }

  /**
   * @brief delete()
   */
  static JSValue delete_(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    JSVector<T>* vector;

    if(!(vector = fromJS(ctx, this_val)))
      return JS_EXCEPTION;

    delete vector;
    JS_SetOpaque(this_val, nullptr);

    return JS_UNDEFINED;
  }

  /**
   * @brief Finalizer (called by GC)
   */
  static void finalizer(JSRuntime* rt, JSValue this_val) {
    JSVector<T>* vector;

    if((vector = static_cast<JSVector<T>*>(JS_GetOpaque(this_val, get_class_id()))))
      delete vector;
  }

  /**
   * @brief Symbol.iterator implementation
   *
   * Returns an iterator object that can be used in for-of loops.
   */
  static JSValue symbol_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    JSVector<T>* vector;

    if(!(vector = fromJS(ctx, this_val)))
      return JS_EXCEPTION;

    // Create iterator object
    JSClassID& iter_class_id = JSVectorIterator<T>::get_class_id();

    // Allocate class ID if not already done
    if(iter_class_id == 0) {
      JS_NewClassID(&iter_class_id);

      // Define iterator class
      JSClassDef class_def = {0};
      class_def.class_name = "VectorIterator";
      class_def.finalizer = JSVectorIterator<T>::finalizer;

      JS_NewClass(JS_GetRuntime(ctx), iter_class_id, &class_def);

      // Create iterator prototype
      JSValue proto = JS_NewObject(ctx);
      JS_SetPropertyStr(ctx, proto, "next", JS_NewCFunction(ctx, JSVectorIterator<T>::next, "next", 0));
      JS_SetClassProto(ctx, iter_class_id, proto);
    }

    // Create iterator instance
    JSValue iter_obj = JS_NewObjectClass(ctx, iter_class_id);
    if(JS_IsException(iter_obj))
      return JS_EXCEPTION;

    JSVectorIterator<T>* iter = new JSVectorIterator<T>(vector, JS_DupValue(ctx, this_val));
    JS_SetOpaque(iter_obj, iter);

    return iter_obj;
  }

  /**
   * @brief Register this vector type with the QuickJS runtime
   *
   * Registers the class and stores the constructor for later export.
   */
  static int init(JSContext* ctx, JSModuleDef* m, const char* name) {
    JSClassID& class_id = JSVector<T>::get_class_id();

    // Allocate class ID if not already done
    if(class_id == 0)
      JS_NewClassID(&class_id);

    // Define class
    JSClassDef class_def = {0};
    class_def.class_name = name;
    class_def.finalizer = JSVector<T>::finalizer;

    JS_NewClass(JS_GetRuntime(ctx), class_id, &class_def);

    // Create prototype
    JSValue proto = JS_NewObject(ctx);

    static const JSCFunctionListEntry js_proto_funcs[] = {
        JS_PROP_STRING_DEF("[Symbol.toStringTag]", name, JS_PROP_CONFIGURABLE),
    };

    // Add methods
    JS_SetPropertyStr(ctx, proto, "push_back", JS_NewCFunction(ctx, JSVector<T>::push_back, "push_back", 1));
    JS_SetPropertyStr(ctx, proto, "get", JS_NewCFunction(ctx, JSVector<T>::get, "get", 1));
    JS_SetPropertyStr(ctx, proto, "set", JS_NewCFunction(ctx, JSVector<T>::set, "set", 2));
    JS_SetPropertyStr(ctx, proto, "size", JS_NewCFunction(ctx, JSVector<T>::size, "size", 0));
    JS_SetPropertyStr(ctx, proto, "delete", JS_NewCFunction(ctx, JSVector<T>::delete_, "delete", 0));

    JS_SetPropertyFunctionList(ctx, proto, js_proto_funcs, countof(js_proto_funcs));

    // Add Symbol.iterator for for-of loop support
    JSValue symbol_iterator = JS_GetPropertyStr(ctx, JS_GetGlobalObject(ctx), "Symbol");

    if(!JS_IsUndefined(symbol_iterator)) {
      JSValue iterator_symbol = JS_GetPropertyStr(ctx, symbol_iterator, "iterator");

      if(!JS_IsUndefined(iterator_symbol)) {
        JSAtom atom = JS_ValueToAtom(ctx, iterator_symbol);

        if(atom != JS_ATOM_NULL) {
          JS_SetProperty(ctx, proto, atom, JS_NewCFunction(ctx, JSVector<T>::symbol_iterator, "[Symbol.iterator]", 0));
          JS_FreeAtom(ctx, atom);
        }

        JS_FreeValue(ctx, iterator_symbol);
      }
      JS_FreeValue(ctx, symbol_iterator);
    }

    // Set prototype
    JS_SetClassProto(ctx, class_id, proto);

    // Create constructor and store it (don't export yet)
    JSValue ctor = JS_NewCFunction2(ctx, JSVector<T>::constructor, name, 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);

    // Store the constructor for later use
    JSVector<T>::get_ctor() = ctor;

    return 0;
  }

  /**
   * @brief Set the module export value for this vector type's constructor
   *
   * Call this from init functions to actually set the export value.
   */
  static void set_export(JSContext* ctx, JSModuleDef* m, const char* name) {
    if(m) {
      JSValue& ctor = JSVector<T>::get_ctor();

      if(!JS_IsUndefined(ctor))
        JS_SetModuleExport(ctx, m, name, JS_DupValue(ctx, ctor));
    }
  }

  /**
   * @brief Declare the module export for this vector type
   *
   * This should be called from the individual vector init functions.
   */
  static void add_export(JSContext* ctx, JSModuleDef* m, const char* name) {
    // Only declare the export here, don't set the value yet
    if(m)
      JS_AddModuleExport(ctx, m, name);
  }
};

/**
 * @brief Iterator class for JSVector
 *
 * Implements the iterator protocol for use with for-of loops.
 */
template<typename T> class JSVectorIterator {
public:
  JSVector<T>* vector;
  size_t index;
  JSValue owner;

  /* Holds a strong reference to the JSVector<T> JS object owning `vec`, so a
   * temporary iterable (e.g. `for(const p of pvv.get(i))`) stays alive for
   * the duration of iteration instead of being finalized (and `vector` freed
   * with it) as soon as the for-of loop drops its only other reference. */
  JSVectorIterator(JSVector<T>* vec, JSValue owner) : vector(vec), index(0), owner(owner) {}

  /**
   * @brief Get the class ID for this vector iterator type
   */
  static JSClassID& get_class_id() {
    static JSClassID class_id = 0;
    return class_id;
  }

  static JSValue next(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    JSVectorIterator<T>* iter = static_cast<JSVectorIterator<T>*>(JS_GetOpaque(this_val, get_class_id()));

    if(!iter)
      return JS_EXCEPTION;

    JSValue result = JS_NewObject(ctx);

    if(iter->index >= iter->vector->vec->size()) {
      // Iterator is done
      JS_SetPropertyStr(ctx, result, "done", JS_TRUE);
      JS_SetPropertyStr(ctx, result, "value", JS_UNDEFINED);
    } else {
      // Return current element
      JS_SetPropertyStr(ctx, result, "done", JS_FALSE);
      JS_SetPropertyStr(ctx, result, "value", JSConverter<T>::toJS(ctx, (*iter->vector->vec)[iter->index]));
      iter->index++;
    }

    return result;
  }

  static void finalizer(JSRuntime* rt, JSValue val) {
    JSVectorIterator<T>* iter;

    if((iter = static_cast<JSVectorIterator<T>*>(JS_GetOpaque(val, get_class_id())))) {
      JS_FreeValueRT(rt, iter->owner);
      delete iter;
    }
  }
};

/**
 * @brief JSConverter specialization for std::vector<T>
 *
 * This converts between JSVector<T> JS objects and std::vector<T>.
 *
 * Note: This specialization is defined AFTER the JSVector class definition
 * to avoid incomplete type errors.
 */
template<class T> struct JSConverter<std::vector<T>> {
  static std::vector<T> fromJS(JSContext* ctx, JSValueConst val) {
    JSVector<T>* vector;

    if((vector = JSVector<T>::fromJS(ctx, val)))
      return *(vector->vec);

    std::vector<T> vec;
    BOOL done;
    JSValue iter = js_iterator_new(ctx, val);

    for(uint32_t i = 0;; ++i) {
      JSValue item = js_iterator_next(ctx, iter, done);

      if(done)
        break;

      vec.push_back(JSConverter<T>::fromJS(ctx, item));
      JS_FreeValue(ctx, item);
    }

    JS_FreeValue(ctx, iter);
    return vec;
  }

  static JSValue toJS(JSContext* ctx, const std::vector<T>& val) {
    // If a JSVector<T> class is registered (e.g. T is cv::Point and
    // PointVector was registered by js_vector_init()), wrap `val` in one and
    // return a genuine JSVector<T> instance instead of a plain JS array -
    // mirrors the short-circuit fromJS() does above, for the JS-bound
    // direction. get_class_id() is always a well-formed call for any T (the
    // template compiles regardless), so this is necessarily a runtime check:
    // registration only happens once js_vector_init() runs, which isn't
    // something a compile-time (SFINAE) check on T could observe.
    if(JSVector<T>::get_class_id() != 0) {
      JSVector<T>* vector = new JSVector<T>();
      *vector->vec = val;
      return vector->toJS(ctx);
    }

    JSValue ret = JS_NewArray(ctx);
    uint32_t i = 0;

    for(const auto& item : val)
      JS_SetPropertyUint32(ctx, ret, i++, JSConverter<T>::toJS(ctx, item));

    return ret;
  }
};

JSInputArray js_vector_inputarray(JSValueConst value);
JSInputOutputArray js_vector_inputoutputarray(JSValueConst value);

#endif // JS_VECTOR_HPP
