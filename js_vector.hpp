#ifndef JS_VECTOR_HPP
#define JS_VECTOR_HPP

#include <quickjs.h>
#include <opencv2/core.hpp>
#include <vector>
#include <string>

#include "js_converter.hpp"

extern "C" {

int js_vector_init(JSContext*, JSModuleDef*);
void js_vector_export(JSContext*, JSModuleDef*);
}

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
  static JSVector<T>* fromJS(JSContext* ctx, JSValueConst this_val, JSClassID class_id) {
    return static_cast<JSVector<T>*>(JS_GetOpaque2(ctx, this_val, class_id));
  }

  /**
   * @brief Create a new JS object wrapping this vector
   */
  JSValue toJS(JSContext* ctx, JSClassID class_id) {
    JSValue obj = JS_NewObjectClass(ctx, class_id);

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

    if(!(vector = fromJS(ctx, this_val, get_class_id())))
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

    if(!(vector = fromJS(ctx, this_val, get_class_id())))
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

    if(!(vector = fromJS(ctx, this_val, get_class_id())))
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

    if(!(vector = fromJS(ctx, this_val, get_class_id())))
      return JS_EXCEPTION;

    return JS_NewInt32(ctx, vector->vec->size());
  }

  /**
   * @brief delete()
   */
  static JSValue delete_(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    JSVector<T>* vector;

    if(!(vector = fromJS(ctx, this_val, get_class_id())))
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

    if(!(vector = fromJS(ctx, this_val, get_class_id())))
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

    JSVectorIterator<T>* iter = new JSVectorIterator<T>(vector);
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

    // Add methods
    JS_SetPropertyStr(ctx, proto, "push_back", JS_NewCFunction(ctx, JSVector<T>::push_back, "push_back", 1));
    JS_SetPropertyStr(ctx, proto, "get", JS_NewCFunction(ctx, JSVector<T>::get, "get", 1));
    JS_SetPropertyStr(ctx, proto, "set", JS_NewCFunction(ctx, JSVector<T>::set, "set", 2));
    JS_SetPropertyStr(ctx, proto, "size", JS_NewCFunction(ctx, JSVector<T>::size, "size", 0));
    JS_SetPropertyStr(ctx, proto, "delete", JS_NewCFunction(ctx, JSVector<T>::delete_, "delete", 0));

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

  JSVectorIterator(JSVector<T>* vec) : vector(vec), index(0) {}

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

    if((iter = static_cast<JSVectorIterator<T>*>(JS_GetOpaque(val, get_class_id()))))
      delete iter;
  }
};

/**
 * @brief JSConverter specialization for std::vector<cv::Point>
 *
 * This converts between PointVector JS objects and std::vector<cv::Point>.
 * Used for nested vectors like PointVectorVector.
 *
 * Note: This specialization is defined AFTER the JSVector class definition
 * to avoid incomplete type errors.
 */
template<> struct JSConverter<std::vector<cv::Point>> {
  static std::vector<cv::Point> fromJS(JSContext* ctx, JSValueConst val) {
    JSClassID point_vector_class_id = JSVector<cv::Point>::get_class_id();
    JSVector<cv::Point>* vector;

    if(!(vector = JSVector<cv::Point>::fromJS(ctx, val, point_vector_class_id)))
      return std::vector<cv::Point>();

    return *(vector->vec);
  }

  static JSValue toJS(JSContext* ctx, const std::vector<cv::Point>& val) {
    JSClassID point_vector_class_id = JSVector<cv::Point>::get_class_id();
    JSVector<cv::Point>* new_vector = new JSVector<cv::Point>();
    *(new_vector->vec) = val;
    return new_vector->toJS(ctx, point_vector_class_id);
  }
};

#endif // JS_VECTOR_HPP
