#include "js_alloc.hpp"
#include "js_point.hpp"
#include "include/js_array.hpp"
#include "js_contour.hpp"
#include "js_rect.hpp"
#include "jsbindings.hpp"
#include <opencv2/core/matx.hpp>
#include <quickjs.h>
#include <stddef.h>
#include <cstdint>
#include <new>
#include <opencv2/imgproc.hpp>
#include <vector>

extern "C" int js_subdiv2d_init(JSContext*, JSModuleDef*);

extern "C" {
thread_local JSValue subdiv2d_proto = JS_UNDEFINED, subdiv2d_class = JS_UNDEFINED;
thread_local JSClassID js_subdiv2d_class_id = 0;
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
js_subdiv2d_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSRectData<int> rect;

  if(argc > 0)
    if(!js_rect_read(ctx, argv[0], &rect))
      return JS_EXCEPTION;

  return js_subdiv2d_new(ctx, argc > 0 ? &rect : nullptr);
}

cv::Subdiv2D*
js_subdiv2d_data2(JSContext* ctx, JSValueConst val) {
  return static_cast<cv::Subdiv2D*>(JS_GetOpaque2(ctx, val, js_subdiv2d_class_id));
}

void
js_subdiv2d_finalizer(JSRuntime* rt, JSValue val) {
  cv::Subdiv2D* s = static_cast<cv::Subdiv2D*>(JS_GetOpaque(val, js_subdiv2d_class_id));

  s->~Subdiv2D();
  js_deallocate(rt, s);
}

enum {
  SUBDIV2D_EDGE_DST = 077,
  SUBDIV2D_EDGE_ORG,
  SUBDIV2D_FIND_NEAREST,
  SUBDIV2D_GET_EDGE,
  SUBDIV2D_GET_EDGE_LIST,
  SUBDIV2D_GET_LEADING_EDGE_LIST,
  SUBDIV2D_GET_TRIANGLE_LIST,
  SUBDIV2D_GET_VERTEX,
  SUBDIV2D_GET_VORONOI_FACET_LIST,
  SUBDIV2D_INIT_DELAUNAY,
  SUBDIV2D_INSERT,
  SUBDIV2D_LOCATE,
  SUBDIV2D_NEXT_EDGE,
  SUBDIV2D_ROTATE_EDGE,
  SUBDIV2D_SYM_EDGE,
};

static JSValue
js_subdiv2d_method(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  cv::Subdiv2D* s;
  JSValue ret = JS_UNDEFINED;

  if((s = js_subdiv2d_data2(ctx, this_val)) == nullptr)
    return JS_EXCEPTION;

  switch(magic) {
    case SUBDIV2D_EDGE_DST: {
      int32_t edge;
      JSPointData<float> dstpt;
      JS_ToInt32(ctx, &edge, argv[0]);

      ret = JS_NewInt32(ctx, s->edgeDst(edge, argc > 1 ? &dstpt : nullptr));

      if(argc > 1)
        js_point_write(ctx, argv[1], dstpt);

      break;
    }

    case SUBDIV2D_EDGE_ORG: {
      int32_t edge;
      JSPointData<float> orgpt;
      JS_ToInt32(ctx, &edge, argv[0]);

      ret = JS_NewInt32(ctx, s->edgeOrg(edge, argc > 1 ? &orgpt : nullptr));

      if(argc > 1)
        js_point_write(ctx, argv[1], orgpt);

      break;
    }

    case SUBDIV2D_FIND_NEAREST: {
      JSPointData<float> pt, nearestPt;
      js_point_read(ctx, argv[0], &pt);

      ret = JS_NewInt32(ctx, s->findNearest(pt, argc > 1 ? &nearestPt : nullptr));

      if(argc > 1)
        js_point_write(ctx, argv[1], nearestPt);

      break;
    }

    case SUBDIV2D_GET_EDGE: {
      int32_t edge, nextEdgeType;
      JS_ToInt32(ctx, &edge, argv[0]);
      JS_ToInt32(ctx, &nextEdgeType, argv[1]);
      ret = JS_NewInt32(ctx, s->getEdge(edge, nextEdgeType));
      break;
    }

    case SUBDIV2D_GET_EDGE_LIST: {
      std::vector<cv::Vec4f> edgeList;

      s->getEdgeList(edgeList);

      js_array_clear(ctx, argv[0]);
      js_array_copy(ctx, argv[0], edgeList);
      break;
    }

    case SUBDIV2D_GET_LEADING_EDGE_LIST: {
      std::vector<int> leadingEdgeList;

      s->getLeadingEdgeList(leadingEdgeList);

      js_array_clear(ctx, argv[0]);
      js_array_copy(ctx, argv[0], leadingEdgeList);
      break;
    }

    case SUBDIV2D_GET_TRIANGLE_LIST: {
      std::vector<cv::Vec6f> triangleList;

      s->getTriangleList(triangleList);

      js_array_clear(ctx, argv[0]);
      js_array_copy(ctx, argv[0], triangleList);
      break;
    }

    case SUBDIV2D_GET_VERTEX: {
      int32_t vertex, firstEdge = 0;
      JS_ToInt32(ctx, &vertex, argv[0]);

      ret = js_point_new(ctx, s->getVertex(vertex, &firstEdge));

      if(argc > 1) {
        JSValue val = JS_NewInt32(ctx, firstEdge);
        js_ref(ctx, "firstEdge", argv[1], val);
        JS_FreeValue(ctx, val);
      }

      break;
    }

    case SUBDIV2D_GET_VORONOI_FACET_LIST: {
      std::vector<int> idx;
      JSContoursData<float> facetList;
      JSContourData<float> facetCenters;

      js_array_to(ctx, argv[0], idx);

      s->getVoronoiFacetList(idx, facetList, facetCenters);

      js_array_clear(ctx, argv[1]);
      js_array_copy(ctx, argv[1], facetList);
      js_array_clear(ctx, argv[2]);
      js_array_copy(ctx, argv[2], facetCenters);
      break;
    }

    case SUBDIV2D_INIT_DELAUNAY: {
      JSRectData<int> rect;
      js_rect_read(ctx, argv[0], &rect);
      s->initDelaunay(rect);
      break;
    }

    case SUBDIV2D_INSERT: {
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

    case SUBDIV2D_LOCATE: {
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

        js_ref(ctx, "vertex", argv[2], val);
        JS_FreeValue(ctx, val);
      }

      break;
    }

    case SUBDIV2D_NEXT_EDGE: {
      int32_t edge;
      JS_ToInt32(ctx, &edge, argv[0]);
      ret = JS_NewInt32(ctx, s->nextEdge(edge));
      break;
    }

    case SUBDIV2D_ROTATE_EDGE: {
      int32_t edge, rotate;
      JS_ToInt32(ctx, &edge, argv[0]);
      JS_ToInt32(ctx, &rotate, argv[1]);
      ret = JS_NewInt32(ctx, s->rotateEdge(edge, rotate));
      break;
    }

    case SUBDIV2D_SYM_EDGE: {
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

const JSCFunctionListEntry js_subdiv2d_proto_funcs[] = {
    JS_CFUNC_MAGIC_DEF("edgeDst", 1, js_subdiv2d_method, SUBDIV2D_EDGE_DST),
    JS_CFUNC_MAGIC_DEF("edgeOrg", 1, js_subdiv2d_method, SUBDIV2D_EDGE_ORG),
    JS_CFUNC_MAGIC_DEF("findNearest", 1, js_subdiv2d_method, SUBDIV2D_FIND_NEAREST),
    JS_CFUNC_MAGIC_DEF("getEdge", 2, js_subdiv2d_method, SUBDIV2D_GET_EDGE),
    JS_CFUNC_MAGIC_DEF("getEdgeList", 1, js_subdiv2d_method, SUBDIV2D_GET_EDGE_LIST),
    JS_CFUNC_MAGIC_DEF("getLeadingEdgeList", 1, js_subdiv2d_method, SUBDIV2D_GET_LEADING_EDGE_LIST),
    JS_CFUNC_MAGIC_DEF("getTriangleList", 1, js_subdiv2d_method, SUBDIV2D_GET_TRIANGLE_LIST),
    JS_CFUNC_MAGIC_DEF("getVertex", 1, js_subdiv2d_method, SUBDIV2D_GET_VERTEX),
    JS_CFUNC_MAGIC_DEF("getVoronoiFacetList", 3, js_subdiv2d_method, SUBDIV2D_GET_VORONOI_FACET_LIST),
    JS_CFUNC_MAGIC_DEF("initDelaunay", 1, js_subdiv2d_method, SUBDIV2D_INIT_DELAUNAY),
    JS_CFUNC_MAGIC_DEF("insert", 1, js_subdiv2d_method, SUBDIV2D_INSERT),
    JS_CFUNC_MAGIC_DEF("locate", 3, js_subdiv2d_method, SUBDIV2D_LOCATE),
    JS_CFUNC_MAGIC_DEF("nextEdge", 1, js_subdiv2d_method, SUBDIV2D_NEXT_EDGE),
    JS_CFUNC_MAGIC_DEF("rotateEdge", 2, js_subdiv2d_method, SUBDIV2D_ROTATE_EDGE),
    JS_CFUNC_MAGIC_DEF("symEdge", 1, js_subdiv2d_method, SUBDIV2D_SYM_EDGE),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "Subdiv2D", JS_PROP_CONFIGURABLE),
};

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
    JS_PROP_INT32_DEF("PREV_AROUND_LEFT", cv::Subdiv2D::PREV_AROUND_LEFT, 0),
};

extern "C" int
js_subdiv2d_init(JSContext* ctx, JSModuleDef* m) {

  if(js_subdiv2d_class_id == 0) {
    /* create the Subdiv2D class */
    JS_NewClassID(&js_subdiv2d_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_subdiv2d_class_id, &js_subdiv2d_class);

    subdiv2d_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, subdiv2d_proto, js_subdiv2d_proto_funcs, countof(js_subdiv2d_proto_funcs));
    JS_SetClassProto(ctx, js_subdiv2d_class_id, subdiv2d_proto);

    subdiv2d_class = JS_NewCFunction2(ctx, js_subdiv2d_constructor, "Subdiv2D", 2, JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    JS_SetConstructor(ctx, subdiv2d_class, subdiv2d_proto);
    JS_SetPropertyFunctionList(ctx, subdiv2d_class, js_subdiv2d_static_funcs, countof(js_subdiv2d_static_funcs));
  }

  if(m)
    JS_SetModuleExport(ctx, m, "Subdiv2D", subdiv2d_class);

  return 0;
}

extern "C" void
js_subdiv2d_export(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Subdiv2D");
}

#if defined(JS_SUBDIV2D_MODULE)
#define JS_INIT_MODULE VISIBLE js_init_module
#else
#define JS_INIT_MODULE js_init_module_subdiv2d
#endif

extern "C" JSModuleDef*
JS_INIT_MODULE(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;
  if(!(m = JS_NewCModule(ctx, module_name, &js_subdiv2d_init)))
    return NULL;
  js_subdiv2d_export(ctx, m);
  return m;
}
