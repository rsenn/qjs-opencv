#include "jsbindings.hpp"
#include "js_rect.hpp"
#include "js_point.hpp"
#include "js_alloc.hpp"
#include "js_array.hpp"

#include <opencv2/imgproc.hpp>

extern "C" {
JSValue subdiv2d_proto = JS_UNDEFINED, subdiv2d_class = JS_UNDEFINED;
JSClassID js_subdiv2d_class_id = 0;
}

JSValue
js_subdiv2d_new(JSContext* ctx, JSRectData<int>* rect = nullptr) {
  JSValue ret;
  cv::Subdiv2D* s;
  ret = JS_NewObjectProtoClass(ctx, subdiv2d_proto, js_subdiv2d_class_id);
  s = js_allocate<cv::Subdiv2D>(ctx);

  if(rect)
    new(s) cv::Subdiv2D(*rect);
  else
    new(s) cv::Subdiv2D();

  JS_SetOpaque(ret, s);

  return ret;
}

static JSValue
js_subdiv2d_ctor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) {
  JSRectData<int> rect;

  if(argc > 0)
    if(!js_rect_read(ctx, argv[0], &rect))
      return JS_EXCEPTION;

  return js_subdiv2d_new(ctx, argc > 0 ? &rect : nullptr);
}

cv::Subdiv2D*
js_subdiv2d_data(JSContext* ctx, JSValueConst val) {
  return static_cast<cv::Subdiv2D*>(JS_GetOpaque2(ctx, val, js_subdiv2d_class_id));
}

void
js_subdiv2d_finalizer(JSRuntime* rt, JSValue val) {
  cv::Subdiv2D* s = static_cast<cv::Subdiv2D*>(JS_GetOpaque(val, js_subdiv2d_class_id));

  s->~Subdiv2D();
  js_deallocate(rt, s);
}

static JSValue
js_subdiv2d_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic) {
  cv::Subdiv2D* s;
  JSValue ret = JS_UNDEFINED;

  if((s = js_subdiv2d_data(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  switch(magic) {
    // int Subdiv2D::edgeDst(int edge, Point2f *dstpt = 0) const
    case 0: {
      int32_t edge;
      JSPointData<float> dstpt;
      JS_ToInt32(ctx, &edge, argv[0]);
      if(argc > 1)
        js_point_read(ctx, argv[1], &dstpt);
      ret = JS_NewInt32(ctx, s->edgeDst(edge, argc > 1 ? &dstpt : nullptr));
      break;
    }
    // int Subdiv2D::edgeOrg(int edge, Point2f *orgpt = 0) const
    case 1: {
      int32_t edge;
      JSPointData<float> orgpt;
      if(argc > 1)
        js_point_read(ctx, argv[1], &orgpt);
      ret = JS_NewInt32(ctx, s->edgeOrg(edge, argc > 1 ? &orgpt : nullptr));
      break;
    }
    // int Subdiv2D::findNearest(Point2f pt, Point2f *nearestPt = 0)
    case 2: {
      JSPointData<float> pt, nearestPt;
      js_point_read(ctx, argv[0], &pt);
      if(argc > 1)
        js_point_read(ctx, argv[1], &nearestPt);
      ret = JS_NewInt32(ctx, s->findNearest(pt, argc > 1 ? &nearestPt : nullptr));
      break;
    }
    // int Subdiv2D::getEdge(int edge, int nextEdgeType) const
    case 3: {
      int32_t edge, nextEdgeType;
      JS_ToInt32(ctx, &edge, argv[0]);
      JS_ToInt32(ctx, &nextEdgeType, argv[1]);
      ret = JS_NewInt32(ctx, s->getEdge(edge, nextEdgeType));
      break;
    }
    // void Subdiv2D::getEdgeList(std::vector<Vec4f> &edgeList) const
    case 4: {
      std::vector<cv::Vec4f> edgeList;
      js_array_to(ctx, argv[0], edgeList);
      s->getEdgeList(edgeList);
      break;
    }
    // void Subdiv2D::getLeadingEdgeList(std::vector<int> &leadingEdgeList) const
    case 5: {
      std::vector<int> leadingEdgeList;
      js_array_to(ctx, argv[0], leadingEdgeList);
      s->getLeadingEdgeList(leadingEdgeList);
      break;
    }
    // void Subdiv2D::getTriangleList(std::vector<Vec6f> &triangleList) const
    case 6: {
      std::vector<cv::Vec6f> triangleList;
      js_array_to(ctx, argv[0], triangleList);
      s->getTriangleList(triangleList);
      break;
      break;
    }
    // Point2f Subdiv2D::getVertex(int vertex, int *firstEdge = 0) const
    case 7: {
      int32_t vertex, firstEdge = 0;
      JS_ToInt32(ctx, &vertex, argv[0]);

      ret = js_point_wrap(ctx, s->getVertex(vertex, &firstEdge));

      if(argc > 1) {
        JSValue val = JS_NewInt32(ctx, firstEdge);
        js_ref(ctx, "firstEdge", argv[1], val);
        JS_FreeValue(ctx, val);
      }
      break;
    }
    // void Subdiv2D::getVoronoiFacetList(const std::vector<int> &idx,
    // std::vector<std::vector<Point2f>> &facetList, std::vector<Point2f> &facetCenters)
    case 8: {
      std::vector<int> idx;
      JSContoursData<float> facetList;
      std::vector<JSPointData<float>> facetCenters;
      js_array_to(ctx, argv[0], idx);
      js_array_to(ctx, argv[1], facetList);
      js_array_to(ctx, argv[2], facetCenters);
      s->getVoronoiFacetList(idx, facetList, facetCenters);
      break;
    }
    // void Subdiv2D::initDelaunay(Rect rect)
    case 9: {
      JSRectData<int> rect;
      js_rect_read(ctx, argv[0], &rect);
      s->initDelaunay(rect);
      break;
    }
      // int Subdiv2D::insert(Point2f pt)
      // void Subdiv2D::insert(const std::vector<Point2f> &ptvec)
    case 10: {
      JSPointData<float> pt;
      if(!js_point_read(ctx, argv[0], &pt)) {
        std::vector<JSPointData<float>> ptvec;
        js_array_to(ctx, argv[0], ptvec);
        s->insert(ptvec);
      } else {
        s->insert(pt);
      }
      break;
    }

    // int Subdiv2D::locate(Point2f pt, int &edge, int &vertex)
    case 11: {
      JSPointData<float> pt;
      js_point_read(ctx, argv[0], &pt);
      int32_t edge = 0, vertex = 0;
      ret = JS_NewInt32(ctx, s->locate(pt, edge, vertex));
      if(argc > 1) {
        JSValue val = JS_NewInt32(ctx, edge);
        js_ref(ctx, "edge", argv[1], val);
        JS_FreeValue(ctx, val);
      }
      if(argc > 2) {
        JSValue val = JS_NewInt32(ctx, vertex);
        js_ref(ctx, "vertex", argv[1], val);
        JS_FreeValue(ctx, val);
      }
      break;
    }
    // int Subdiv2D::nextEdge(int edge) const
    case 12: {
      int32_t edge;
      JS_ToInt32(ctx, &edge, argv[0]);
      ret = JS_NewInt32(ctx, s->nextEdge(edge));
      break;
    }
    // int Subdiv2D::rotateEdge(int edge, int rotate) const
    case 13: {
      int32_t edge, rotate;
      JS_ToInt32(ctx, &edge, argv[0]);
      JS_ToInt32(ctx, &rotate, argv[1]);
      ret = JS_NewInt32(ctx, s->rotateEdge(edge, rotate));
      break;
    }
    // int Subdiv2D::symEdge(int edge) const
    case 14: {
      int32_t edge;
      JS_ToInt32(ctx, &edge, argv[0]);
      ret = JS_NewInt32(ctx, s->symEdge(edge));
      break;
    }
  }
  return ret;
}

JSClassDef js_subdiv2d_class = {
    .class_name = "Subdiv2D",
    .finalizer = js_subdiv2d_finalizer,
};

const JSCFunctionListEntry js_subdiv2d_proto_funcs[] = {JS_CFUNC_MAGIC_DEF("edgeDst", 1, js_subdiv2d_method, 0),
                                                        JS_CFUNC_MAGIC_DEF("edgeOrg", 1, js_subdiv2d_method, 1),
                                                        JS_CFUNC_MAGIC_DEF("findNearest", 1, js_subdiv2d_method, 2),
                                                        JS_CFUNC_MAGIC_DEF("getEdge", 2, js_subdiv2d_method, 3),
                                                        JS_CFUNC_MAGIC_DEF("getEdgeList", 1, js_subdiv2d_method, 4),
                                                        JS_CFUNC_MAGIC_DEF("getLeadingEdgeList", 1, js_subdiv2d_method, 5),
                                                        JS_CFUNC_MAGIC_DEF("getTriangleList", 1, js_subdiv2d_method, 6),
                                                        JS_CFUNC_MAGIC_DEF("getVertex", 1, js_subdiv2d_method, 7),
                                                        JS_CFUNC_MAGIC_DEF("getVoronoiFacetList", 3, js_subdiv2d_method, 8),
                                                        JS_CFUNC_MAGIC_DEF("initDelaunay", 1, js_subdiv2d_method, 9),
                                                        JS_CFUNC_MAGIC_DEF("insert", 1, js_subdiv2d_method, 10),
                                                        JS_CFUNC_MAGIC_DEF("locate", 3, js_subdiv2d_method, 11),
                                                        JS_CFUNC_MAGIC_DEF("nextEdge", 1, js_subdiv2d_method, 12),
                                                        JS_CFUNC_MAGIC_DEF("rotateEdge", 2, js_subdiv2d_method, 13),
                                                        JS_CFUNC_MAGIC_DEF("symEdge", 1, js_subdiv2d_method, 14),
                                                        JS_PROP_STRING_DEF("[Symbol.toStringTag]",
                                                                           "Subdiv2D",
                                                                           JS_PROP_CONFIGURABLE)};

const JSCFunctionListEntry js_subdiv2d_static_funcs[] = {
    JS_PROP_INT32_DEF("PTLOC_ERROR", cv::Subdiv2D::PTLOC_ERROR, 0),
    JS_PROP_INT32_DEF("PTLOC_OUTSIDE_RECT", cv::Subdiv2D::PTLOC_OUTSIDE_RECT, 0),
    JS_PROP_INT32_DEF("PTLOC_INSIDE", cv::Subdiv2D::PTLOC_INSIDE, 0),
    JS_PROP_INT32_DEF("PTLOC_VERTEX", cv::Subdiv2D::PTLOC_VERTEX, 0),

    JS_PROP_INT32_DEF("NEXT_AROUND_ORG", cv::Subdiv2D::NEXT_AROUND_ORG, 0),
    JS_PROP_INT32_DEF("NEXT_AROUND_DST", cv::Subdiv2D::NEXT_AROUND_DST, 0),
    JS_PROP_INT32_DEF("PREV_AROUND_ORG", cv::Subdiv2D::PREV_AROUND_ORG, 0),
    JS_PROP_INT32_DEF("PREV_AROUND_DST", cv::Subdiv2D::PREV_AROUND_DST, 0),
    JS_PROP_INT32_DEF("NEXT_AROUND_LEFT", cv::Subdiv2D::NEXT_AROUND_LEFT, 0),
    JS_PROP_INT32_DEF("NEXT_AROUND_RIGHT", cv::Subdiv2D::NEXT_AROUND_RIGHT, 0),
    JS_PROP_INT32_DEF("PREV_AROUND_LEFT", cv::Subdiv2D::PREV_AROUND_LEFT, 0)};

extern "C" int
js_subdiv2d_init(JSContext* ctx, JSModuleDef* m) {

  if(js_subdiv2d_class_id == 0) {
    /* create the Subdiv2D class */
    JS_NewClassID(&js_subdiv2d_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_subdiv2d_class_id, &js_subdiv2d_class);

    subdiv2d_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, subdiv2d_proto, js_subdiv2d_proto_funcs, countof(js_subdiv2d_proto_funcs));
    JS_SetClassProto(ctx, js_subdiv2d_class_id, subdiv2d_proto);

    subdiv2d_class = JS_NewCFunction2(ctx, js_subdiv2d_ctor, "Subdiv2D", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, subdiv2d_class, subdiv2d_proto);
    JS_SetPropertyFunctionList(ctx, subdiv2d_class, js_subdiv2d_static_funcs, countof(js_subdiv2d_static_funcs));
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Subdiv2D", subdiv2d_class);

  return 0;
}

void
js_subdiv2d_constructor(JSContext* ctx, JSValue parent, const char* name) {
  if(JS_IsUndefined(subdiv2d_class))
    js_subdiv2d_init(ctx, 0);

  JS_SetPropertyStr(ctx, parent, name ? name : "Subdiv2D", subdiv2d_class);
}

#if defined(JS_SUBDIV2D_MODULE)
#define JS_INIT_MODULE /*VISIBLE*/ js_init_module
#else
#define JS_INIT_MODULE /*VISIBLE*/ js_init_module_subdiv2d
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, module_name, &js_subdiv2d_init);
  if(!m)
    return NULL;
  JS_AddModuleExport(ctx, m, "Subdiv2D");
  return m;
}
