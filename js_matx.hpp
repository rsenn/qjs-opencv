#ifndef JS_MATX_HPP
#define JS_MATX_HPP

template<class T,size_t rows,size_t cols> using JSMatxData = cv::Matx<T, rows,cols>;

extern "C" int js_matx_init(JSContext*, JSModuleDef*);

extern "C" { 
int js_matx_init(JSContext*, JSModuleDef*);
 } 
template<class T,size_t rows,size_t cols> 
static inline JSValue
js_matx_new(JSContext* ctx, const JSMatxData<T,rows,cols>& matx) {
  
}


template<class T,size_t rows,size_t cols> 
static inline int
js_matx_read(JSContext* ctx, JSValueConst matx, JSMatxData<T,rows,cols>* out) {
  int ret = 1;
 
  return ret;
}
 
template<class T,size_t rows,size_t cols> 
static inline int
js_value_to(JSContext* ctx, JSValueConst value, JSMatxData<T,rows,cols>& matx) {
  return js_matx_read(ctx, value, &matx);
}

template<class T,size_t rows,size_t cols> 
static inline JSValue
js_value_from(JSContext* ctx, const JSMatxData<T,rows,cols>& matx) {
  return js_matx_new(ctx, matx);
}

extern "C" int js_matx_init(JSContext*, JSModuleDef*);

#endif /* defined(JS_MATX_HPP) */
