#ifndef JS_COMMANDLINEPARSER_HPP
#define JS_COMMANDLINEPARSER_HPP

#include <quickjs.h>
#include <opencv2/core/utility.hpp>

using JSCommandLineParserData = cv::CommandLineParser;

extern "C" int js_commandlineparser_init(JSContext*, JSModuleDef*);

extern "C" {
JSCommandLineParserData* js_commandlineparser_data(JSValueConst val);
JSCommandLineParserData* js_commandlineparser_data2(JSContext*, JSValueConst val);
int js_commandlineparser_init(JSContext*, JSModuleDef*);
JSModuleDef* js_init_module_commandlineparser(JSContext*, const char*);
}

JSValue js_commandlineparser_new(JSContext*, JSValueConst);
JSValue js_commandlineparser_new(JSContext*, JSValueConst, const JSCommandLineParserData&);
JSValue js_commandlineparser_new(JSContext*, const JSCommandLineParserData&);

static inline JSValue
js_value_from(JSContext* ctx, const JSCommandLineParserData& fn) {
  return js_commandlineparser_new(ctx, fn);
}

extern "C" int js_commandlineparser_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_COMMANDLINEPARSER_HPP) */
