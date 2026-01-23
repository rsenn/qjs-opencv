#ifndef JS_FILENODE_HPP
#define JS_FILENODE_HPP

#include <quickjs.h>
#include <opencv2/core/persistence.hpp>

using JSFileNodeData = cv::FileNode;
using JSFileNodeIteratorData = cv::FileNodeIterator;

extern "C" int js_filenode_init(JSContext*, JSModuleDef*);

extern "C" {
extern thread_local JSValue filenode_proto, filenode_class;
extern thread_local JSClassID js_filenode_class_id;

extern thread_local JSValue filenode_iterator_proto, filenode_iterator_class;
extern thread_local JSClassID js_filenode_iterator_class_id;

JSFileNodeData* js_filenode_data(JSValueConst val);
JSFileNodeData* js_filenode_data2(JSContext*, JSValueConst val);
int js_filenode_init(JSContext*, JSModuleDef*);
JSModuleDef* js_init_module_filenode(JSContext*, const char*);
}

JSValue js_filenode_new(JSContext*, JSValueConst);
JSValue js_filenode_new(JSContext*, JSValueConst, const JSFileNodeData&);
static inline JSValue
js_filenode_new(JSContext* ctx, const JSFileNodeData& fn) {
  return js_filenode_new(ctx, filenode_proto, fn);
}

extern "C" int js_filenode_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_FILENODE_HPP) */
