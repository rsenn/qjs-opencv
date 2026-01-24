#ifndef JS_FILENODE_HPP
#define JS_FILENODE_HPP

#include <quickjs.h>
#include <opencv2/core/persistence.hpp>

using JSFileNodeData = cv::FileNode;
using JSFileNodeIteratorData = cv::FileNodeIterator;

extern "C" int js_filenode_init(JSContext*, JSModuleDef*);

extern "C" {
JSFileNodeData* js_filenode_data(JSValueConst val);
JSFileNodeData* js_filenode_data2(JSContext*, JSValueConst val);
int js_filenode_init(JSContext*, JSModuleDef*);
JSModuleDef* js_init_module_filenode(JSContext*, const char*);
}

JSValue js_filenode_new(JSContext*, JSValueConst);
JSValue js_filenode_new(JSContext*, JSValueConst, const JSFileNodeData&);
JSValue js_filenode_new(JSContext*, const JSFileNodeData&);

static inline JSValue
js_value_from(JSContext* ctx, const JSFileNodeData& fn) {
  return js_filenode_new(ctx, fn);
}

extern "C" int js_filenode_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_FILENODE_HPP) */
