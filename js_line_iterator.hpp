#ifndef JS_LINE_ITERATOR_HPP
#define JS_LINE_ITERATOR_HPP

#include "jsbindings.hpp"
#include <opencv2/imgproc.hpp>

typedef cv::LineIterator JSLineIteratorData;

extern "C" {

extern JSValue line_iterator_proto, line_iterator_class;
extern JSClassID js_line_iterator_class_id;

VISIBLE JSLineIteratorData* js_line_iterator_data2(JSContext*, JSValueConst val);
VISIBLE JSLineIteratorData* js_line_iterator_data(JSValueConst val);
}

VISIBLE JSValue js_line_iterator_wrap(JSContext* ctx, const cv::LineIterator& line_iterator);

extern "C" VISIBLE int js_line_iterator_init(JSContext*, JSModuleDef*);

VISIBLE JSValue js_line_iterator_new(JSContext* ctx, const JSLineIteratorData& line_iterator);

extern "C" int js_line_iterator_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_LINE_ITERATOR_HPP) */
