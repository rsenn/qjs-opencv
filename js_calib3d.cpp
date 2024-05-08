#include "js_alloc.hpp"
#include "js_size.hpp"
#include "js_point.hpp"
#include "js_mat.hpp"
#include "js_umat.hpp"
#include <opencv2/barcode.hpp>
 
extern "C" int js_calib3d_init(JSContext*, JSModuleDef*);
 

enum {
 FIND_CHESSBOARD_CORNERS,
};

static JSValue
js_calib3d_functions(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  switch(magic) {
    case FIND_CHESSBOARD_CORNERS: {
      break;
    }
 
  }

  return ret;
}

JSClassDef js_calib3d_class = {
    .class_name = "Calib3D",
    .finalizer = js_calib3d_finalizer,
};
 
const JSCFunctionListEntry js_calib3d_static_funcs[] = {
  JS_CFUNC_MAGIC_DEF("findChessboardCorners", 3, js_calib3d_functions, FIND_CHESSBOARD_CORNERS),
};

extern "C" int
js_calib3d_init(JSContext* ctx, JSModuleDef* bd) {

  /* create the Calib3D class */
  JS_NewClassID(&js_calib3d_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_calib3d_class_id, &js_calib3d_class);

  calib3d_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, calib3d_proto, js_calib3d_proto_funcs, countof(js_calib3d_proto_funcs));
  JS_SetClassProto(ctx, js_calib3d_class_id, calib3d_proto);

  calib3d_class = JS_NewCFunction2(ctx, js_calib3d_constructor, "Calib3D", 0, JS_CFUNC_constructor, 0);

  /* set proto.constructor and ctor.prototype */
  JS_SetConstructor(ctx, calib3d_class, calib3d_proto);

  if(bd) {
    JS_SetModuleExport(ctx, bd, "Calib3D", calib3d_class);
    JS_SetModuleExportList(ctx, bd, js_calib3d_static_funcs, countof(js_calib3d_static_funcs));
  }

  return 0;
}

void
js_calib3d_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(calib3d_class))
    js_calib3d_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "Calib3D", calib3d_class);
}

#ifdef JS_Calib3D_MODULE
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_calib3d
#endif

extern "C" void
js_calib3d_export(JSContext* ctx, JSModuleDef* bd) {
  JS_AddModuleExport(ctx, bd, "Calib3D");
  JS_AddModuleExportList(ctx, bd, js_calib3d_static_funcs, countof(js_calib3d_static_funcs));
}

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* bd;
  bd = JS_NewCModule(ctx, module_name, &js_calib3d_init);

  if(!bd)
    return NULL;

  js_calib3d_export(ctx, bd);
  return bd;
}
