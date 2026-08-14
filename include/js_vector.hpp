#ifndef JS_VECTOR_HPP
#define JS_VECTOR_HPP

#include <quickjs.h>
#include <opencv2/core.hpp>
#include <vector>
#include <string>

// Forward declarations for OpenCV type converters
#include "js_mat.hpp"
#include "js_point.hpp"
#include "js_rect.hpp"

// Forward declarations for DMatch
typedef cv::DMatch JSDMatchData;
JSDMatchData* js_dmatch_data(JSValueConst val);
JSValue js_dmatch_new(JSContext* ctx, const JSDMatchData& dm);
// Note: js_keypoint.hpp not included here to avoid js_array template conflicts
// JSConverter<cv::KeyPoint> is defined in js_keypoint.hpp instead

/**
 * @brief Common template infrastructure for OpenCV.js vector container types
 * 
 * This header provides a generic JSVector template class that factors out
 * common code for all 16 vector container types (MatVector, PointVector,
 * KeyPointVector, DMatchVector, RectVector, etc.).
 * 
 * Each vector type needs a JSConverter specialization that handles
 * JS ↔ C++ type conversion.
 */

// Forward declarations
template<typename T>
struct JSConverter;

template<typename T>
class JSVector;

// Forward declarations for template helper functions
// (needed because they're used inside template classes before their definitions)
template<typename T>
JSClassID& js_vector_get_class_id();

template<typename T>
JSClassID& js_vector_iterator_get_class_id();

/**
 * @brief JSConverter specializations for primitive types
 */
template<>
struct JSConverter<int> {
    static int fromJS(JSContext* ctx, JSValueConst val) {
        int result;
        JS_ToInt32(ctx, &result, val);
        return result;
    }
    
    static JSValue toJS(JSContext* ctx, int val) {
        return JS_NewInt32(ctx, val);
    }
};

template<>
struct JSConverter<float> {
    static float fromJS(JSContext* ctx, JSValueConst val) {
        double result;
        JS_ToFloat64(ctx, &result, val);
        return static_cast<float>(result);
    }
    
    static JSValue toJS(JSContext* ctx, float val) {
        return JS_NewFloat64(ctx, val);
    }
};

template<>
struct JSConverter<double> {
    static double fromJS(JSContext* ctx, JSValueConst val) {
        double result;
        JS_ToFloat64(ctx, &result, val);
        return result;
    }
    
    static JSValue toJS(JSContext* ctx, double val) {
        return JS_NewFloat64(ctx, val);
    }
};

template<>
struct JSConverter<char> {
    static char fromJS(JSContext* ctx, JSValueConst val) {
        int result;
        JS_ToInt32(ctx, &result, val);
        return static_cast<char>(result);
    }
    
    static JSValue toJS(JSContext* ctx, char val) {
        return JS_NewInt32(ctx, val);
    }
};

template<>
struct JSConverter<std::string> {
    static std::string fromJS(JSContext* ctx, JSValueConst val) {
        const char* str = JS_ToCString(ctx, val);
        std::string result(str ? str : "");
        if (str) JS_FreeCString(ctx, str);
        return result;
    }
    
    static JSValue toJS(JSContext* ctx, const std::string& val) {
        return JS_NewStringLen(ctx, val.c_str(), val.size());
    }
};

/**
 * @brief JSConverter specialization for cv::Mat
 * 
 * Mat objects are reference-counted, so we just wrap/unwrap the existing Mat.
 * When getting from vector, we return a new JS wrapper pointing to the same Mat data.
 * When setting into vector, we extract the Mat from the JS wrapper.
 */
template<>
struct JSConverter<cv::Mat> {
    static cv::Mat fromJS(JSContext* ctx, JSValueConst val) {
        JSMatData* mat = js_mat_data2(ctx, val);
        if (!mat) {
            return cv::Mat();
        }
        return *mat;
    }
    
    static JSValue toJS(JSContext* ctx, const cv::Mat& val) {
        return js_mat_wrap(ctx, val);
    }
};

/**
 * @brief JSConverter specialization for cv::Point
 * 
 * Point objects are value types, so we copy them.
 */
template<>
struct JSConverter<cv::Point> {
    static cv::Point fromJS(JSContext* ctx, JSValueConst val) {
        cv::Point result(0, 0);
        js_point_read(ctx, val, &result);
        return result;
    }

    static JSValue toJS(JSContext* ctx, const cv::Point& val) {
        return js_point_new(ctx, point_proto, val.x, val.y);
    }
};

/**
 * @brief JSConverter specialization for cv::Point2f
 */
template<>
struct JSConverter<cv::Point2f> {
    static cv::Point2f fromJS(JSContext* ctx, JSValueConst val) {
        cv::Point2f result(0.0f, 0.0f);
        js_point_read(ctx, val, &result);
        return result;
    }

    static JSValue toJS(JSContext* ctx, const cv::Point2f& val) {
        return js_point_new(ctx, point_proto, val.x, val.y);
    }
};

/**
 * @brief JSConverter specialization for cv::Rect
 */
template<>
struct JSConverter<cv::Rect> {
    static cv::Rect fromJS(JSContext* ctx, JSValueConst val) {
        JSRectData<double>* rect = js_rect_data2(ctx, val);
        if (!rect) {
            return cv::Rect(0, 0, 0, 0);
        }
        return cv::Rect(
            static_cast<int>(rect->x),
            static_cast<int>(rect->y),
            static_cast<int>(rect->width),
            static_cast<int>(rect->height)
        );
    }
    
    static JSValue toJS(JSContext* ctx, const cv::Rect& val) {
        return js_rect_new(ctx, rect_proto, val.x, val.y, val.width, val.height);
    }
};

/**
 * @brief JSConverter specialization for cv::DMatch
 *
 * DMatch doesn't have JS bindings yet, so we use a simple object representation.
 * TODO: Create proper DMatch bindings when needed.
 */
template<>
struct JSConverter<cv::DMatch> {
    static cv::DMatch fromJS(JSContext* ctx, JSValueConst val) {
        // Try to extract from DMatch instance first
        JSDMatchData* dm = js_dmatch_data(val);
        if (dm) {
            return *dm;
        }
        
        // Fall back to plain object
        cv::DMatch match;

        JSValue queryIdx = JS_GetPropertyStr(ctx, val, "queryIdx");
        JSValue trainIdx = JS_GetPropertyStr(ctx, val, "trainIdx");
        JSValue imgIdx = JS_GetPropertyStr(ctx, val, "imgIdx");
        JSValue distance = JS_GetPropertyStr(ctx, val, "distance");

        if (!JS_IsUndefined(queryIdx)) {
            int idx;
            JS_ToInt32(ctx, &idx, queryIdx);
            match.queryIdx = idx;
        }
        if (!JS_IsUndefined(trainIdx)) {
            int idx;
            JS_ToInt32(ctx, &idx, trainIdx);
            match.trainIdx = idx;
        }
        if (!JS_IsUndefined(imgIdx)) {
            int idx;
            JS_ToInt32(ctx, &idx, imgIdx);
            match.imgIdx = idx;
        }
        if (!JS_IsUndefined(distance)) {
            double dist;
            JS_ToFloat64(ctx, &dist, distance);
            match.distance = static_cast<float>(dist);
        }

        JS_FreeValue(ctx, queryIdx);
        JS_FreeValue(ctx, trainIdx);
        JS_FreeValue(ctx, imgIdx);
        JS_FreeValue(ctx, distance);

        return match;
    }

    static JSValue toJS(JSContext* ctx, const cv::DMatch& val) {
        // Return a DMatch instance instead of a plain object
        return js_dmatch_new(ctx, val);
    }
};

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
template<typename T>
class JSVector {
public:
    using VectorType = std::vector<T>;
    using Converter = JSConverter<T>;
    
    VectorType* vec;
    
    JSVector() : vec(new VectorType()) {}
    
    ~JSVector() {
        delete vec;
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
        if (JS_IsException(obj)) {
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
        JSClassID class_id = js_vector_get_class_id<T>();
        JSValue obj = JS_NewObjectClass(ctx, class_id);
        if (JS_IsException(obj)) {
            return JS_EXCEPTION;
        }
        
        JSVector<T>* vector = new JSVector<T>();
        JS_SetOpaque(obj, vector);
        
        return obj;
    }
    
    /**
     * @brief push_back(element)
     */
    static JSValue push_back(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        JSVector<T>* vector = fromJS(ctx, this_val, js_vector_get_class_id<T>());
        if (!vector) {
            return JS_EXCEPTION;
        }
        
        if (argc < 1) {
            return JS_ThrowTypeError(ctx, "push_back requires 1 argument");
        }
        
        T element = Converter::fromJS(ctx, argv[0]);
        vector->vec->push_back(element);
        
        return JS_UNDEFINED;
    }
    
    /**
     * @brief get(index)
     */
    static JSValue get(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        JSVector<T>* vector = fromJS(ctx, this_val, js_vector_get_class_id<T>());
        if (!vector) {
            return JS_EXCEPTION;
        }
        
        if (argc < 1) {
            return JS_ThrowTypeError(ctx, "get requires 1 argument");
        }
        
        int index;
        JS_ToInt32(ctx, &index, argv[0]);
        
        if (index < 0 || index >= (int)vector->vec->size()) {
            return JS_ThrowRangeError(ctx, "get: index out of range");
        }
        
        return Converter::toJS(ctx, (*vector->vec)[index]);
    }
    
    /**
     * @brief set(index, element)
     */
    static JSValue set(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        JSVector<T>* vector = fromJS(ctx, this_val, js_vector_get_class_id<T>());
        if (!vector) {
            return JS_EXCEPTION;
        }
        
        if (argc < 2) {
            return JS_ThrowTypeError(ctx, "set requires 2 arguments");
        }
        
        int index;
        JS_ToInt32(ctx, &index, argv[0]);
        
        if (index < 0 || index >= (int)vector->vec->size()) {
            return JS_ThrowRangeError(ctx, "set: index out of range");
        }
        
        (*vector->vec)[index] = Converter::fromJS(ctx, argv[1]);
        
        return JS_UNDEFINED;
    }
    
    /**
     * @brief size()
     */
    static JSValue size(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        JSVector<T>* vector = fromJS(ctx, this_val, js_vector_get_class_id<T>());
        if (!vector) {
            return JS_EXCEPTION;
        }
        
        return JS_NewInt32(ctx, vector->vec->size());
    }
    
    /**
     * @brief delete()
     */
    static JSValue delete_(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        JSVector<T>* vector = fromJS(ctx, this_val, js_vector_get_class_id<T>());
        if (!vector) {
            return JS_EXCEPTION;
        }
        
        delete vector;
        JS_SetOpaque(this_val, nullptr);
        
        return JS_UNDEFINED;
    }
    
    /**
     * @brief Finalizer (called by GC)
     */
    static void finalizer(JSRuntime* rt, JSValue this_val) {
        JSVector<T>* vector = static_cast<JSVector<T>*>(JS_GetOpaque(this_val, js_vector_get_class_id<T>()));
        if (vector) {
            delete vector;
        }
    }
};

/**
 * @brief Helper to get class ID for a vector type
 * 
 * Each vector type needs its own class ID. This template function
 * returns a reference to the class ID for type T.
 */
template<typename T>
JSClassID& js_vector_get_class_id() {
    static JSClassID class_id = 0;
    return class_id;
}

/**
 * @brief Iterator class for JSVector
 * 
 * Implements the iterator protocol for use with for-of loops.
 */
template<typename T>
class JSVectorIterator {
public:
    JSVector<T>* vector;
    size_t index;
    
    JSVectorIterator(JSVector<T>* vec) : vector(vec), index(0) {}
    
    static JSValue next(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        JSVectorIterator<T>* iter = static_cast<JSVectorIterator<T>*>(
            JS_GetOpaque(this_val, js_vector_iterator_get_class_id<T>())
        );
        
        if (!iter) {
            return JS_EXCEPTION;
        }
        
        JSValue result = JS_NewObject(ctx);
        
        if (iter->index >= iter->vector->vec->size()) {
            // Iterator is done
            JS_SetPropertyStr(ctx, result, "done", JS_TRUE);
            JS_SetPropertyStr(ctx, result, "value", JS_UNDEFINED);
        } else {
            // Return current element
            JS_SetPropertyStr(ctx, result, "done", JS_FALSE);
            JS_SetPropertyStr(ctx, result, "value", 
                JSConverter<T>::toJS(ctx, (*iter->vector->vec)[iter->index]));
            iter->index++;
        }
        
        return result;
    }
    
    static void finalizer(JSRuntime* rt, JSValue val) {
        JSVectorIterator<T>* iter = static_cast<JSVectorIterator<T>*>(
            JS_GetOpaque(val, js_vector_iterator_get_class_id<T>())
        );
        if (iter) {
            delete iter;
        }
    }
};

template<typename T>
JSClassID& js_vector_iterator_get_class_id() {
    static JSClassID class_id = 0;
    return class_id;
}

/**
 * @brief Symbol.iterator implementation for JSVector
 * 
 * Returns an iterator object that can be used in for-of loops.
 */
template<typename T>
static JSValue js_vector_symbol_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    JSVector<T>* vector = JSVector<T>::fromJS(ctx, this_val, js_vector_get_class_id<T>());
    if (!vector) {
        return JS_EXCEPTION;
    }
    
    // Create iterator object
    JSClassID& iter_class_id = js_vector_iterator_get_class_id<T>();
    
    // Allocate class ID if not already done
    if (iter_class_id == 0) {
        JS_NewClassID(&iter_class_id);
        
        // Define iterator class
        JSClassDef class_def = {0};
        class_def.class_name = "VectorIterator";
        class_def.finalizer = JSVectorIterator<T>::finalizer;
        
        JS_NewClass(JS_GetRuntime(ctx), iter_class_id, &class_def);
        
        // Create iterator prototype
        JSValue proto = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, proto, "next", 
            JS_NewCFunction(ctx, JSVectorIterator<T>::next, "next", 0));
        JS_SetClassProto(ctx, iter_class_id, proto);
    }
    
    // Create iterator instance
    JSValue iter_obj = JS_NewObjectClass(ctx, iter_class_id);
    if (JS_IsException(iter_obj)) {
        return JS_EXCEPTION;
    }
    
    JSVectorIterator<T>* iter = new JSVectorIterator<T>(vector);
    JS_SetOpaque(iter_obj, iter);
    
    return iter_obj;
}

/**
 * @brief Helper to get the constructor for a vector type
 * 
 * Stores the constructor so it can be exported later.
 */
template<typename T>
JSValue& js_vector_get_ctor() {
    static JSValue ctor = JS_UNDEFINED;
    return ctor;
}

/**
 * @brief Helper to register a vector type
 * 
 * This function registers a new vector type with the QuickJS runtime
 * and stores the constructor for later export.
 */
template<typename T>
int js_register_vector(JSContext* ctx, JSModuleDef* m, const char* name) {
    JSClassID& class_id = js_vector_get_class_id<T>();

    // Allocate class ID if not already done
    if (class_id == 0) {
        JS_NewClassID(&class_id);
    }

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
    if (!JS_IsUndefined(symbol_iterator)) {
        JSValue iterator_symbol = JS_GetPropertyStr(ctx, symbol_iterator, "iterator");
        if (!JS_IsUndefined(iterator_symbol)) {
            JSAtom atom = JS_ValueToAtom(ctx, iterator_symbol);
            if (atom != JS_ATOM_NULL) {
                JS_SetProperty(ctx, proto, atom,
                    JS_NewCFunction(ctx, js_vector_symbol_iterator<T>, "[Symbol.iterator]", 0));
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
    js_vector_get_ctor<T>() = ctor;

    return 0;
}

/**
 * @brief Helper to export a vector type
 *
 * This function should be called from the individual vector init functions.
 */
template<typename T>
void js_export_vector(JSContext* ctx, JSModuleDef* m, const char* name) {
    if (m) {
        // Only declare the export here, don't set the value yet
        JS_AddModuleExport(ctx, m, name);
    }
}

// Call this from init functions to actually set the export value
template<typename T>
void js_set_vector_export(JSContext* ctx, JSModuleDef* m, const char* name) {
    if (m) {
        JSValue& ctor = js_vector_get_ctor<T>();
        if (!JS_IsUndefined(ctor)) {
            JS_SetModuleExport(ctx, m, name, JS_DupValue(ctx, ctor));
        }
    }
}

/**
 * @brief JSConverter specialization for std::vector<cv::Point>
 *
 * This converts between PointVector JS objects and std::vector<cv::Point>.
 * Used for nested vectors like PointVectorVector.
 * 
 * Note: This specialization is defined AFTER the JSVector class definition
 * to avoid incomplete type errors.
 */
template<>
struct JSConverter<std::vector<cv::Point>> {
    static std::vector<cv::Point> fromJS(JSContext* ctx, JSValueConst val) {
        JSClassID point_vector_class_id = js_vector_get_class_id<cv::Point>();
        JSVector<cv::Point>* vector = JSVector<cv::Point>::fromJS(ctx, val, point_vector_class_id);
        if (!vector) {
            return std::vector<cv::Point>();
        }
        return *(vector->vec);
    }

    static JSValue toJS(JSContext* ctx, const std::vector<cv::Point>& val) {
        JSClassID point_vector_class_id = js_vector_get_class_id<cv::Point>();
        JSVector<cv::Point>* new_vector = new JSVector<cv::Point>();
        *(new_vector->vec) = val;
        return new_vector->toJS(ctx, point_vector_class_id);
    }
};

#endif // JS_VECTOR_HPP
