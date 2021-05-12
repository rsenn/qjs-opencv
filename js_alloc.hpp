#ifndef JS_ALLOC_HPP
#define JS_ALLOC_HPP

#include <unistd.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <limits.h>
#include <cstdlib>

#include <quickjs.h>

template<class T> struct js_alloc_mmap {
  static size_t page_size;
  static constexpr size_t
  round_to_page_size(size_t n) {
    size_t pages = (n + (page_size - 1)) / page_size;
    return pages * page_size;
  }
  static constexpr size_t size = round_to_page_size(sizeof(T));
  static constexpr size_t offset = size - sizeof(T);

  static T*
  allocate(JSContext* ctx) {
    return reinterpret_cast<T*>(
        static_cast<char*>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) + offset);
  }
  static void
  deallocate(JSContext* ctx, T* ptr) {
    munmap(reinterpret_cast<char*>(ptr) - offset, size);
  }
  static void
  deallocate(JSRuntime* rt, T* ptr) {
    munmap(reinterpret_cast<char*>(ptr) - offset, size);
  }
};

template<class T> size_t js_alloc_mmap<T>::page_size = ::getpagesize();

template<class T> struct js_alloc_libc {
  static constexpr size_t size = ((sizeof(T) + 7) >> 3) << 3;

  static T*
  allocate(JSContext* ctx) {
    return static_cast<T*>(malloc(size));
  }

  static void
  deallocate(JSContext* ctx, T* ptr) {
    free(ptr);
  }
  static void
  deallocate(JSRuntime* rt, T* ptr) {
    free(ptr);
  }
};

template<class T> struct js_alloc_quickjs {
  static constexpr size_t size = ((sizeof(T) + 7) >> 3) << 3;

  static T*
  allocate(JSContext* ctx) {
    return static_cast<T*>(js_mallocz(ctx, size));
  }

  static void
  deallocate(JSContext* ctx, T* ptr) {
    js_free(ctx, ptr);
  }
  static void
  deallocate(JSRuntime* rt, T* ptr) {
    js_free_rt(rt, ptr);
  }
};

template<class T> struct js_alloc_cxx {
  static constexpr size_t size = ((sizeof(T) + 7) >> 3) << 3;
  static T*
  allocate(JSContext* ctx) {
    return new T();
  }
  static void
  deallocate(JSContext* ctx, T* ptr) {
    delete ptr;
  }
  static void
  deallocate(JSRuntime* rt, T* ptr) {
    delete ptr;
  }
};

template<class T> using js_allocator = js_alloc_quickjs<T>;

template<class T>
static inline T*
js_allocate(JSContext* ctx) {
  return js_allocator<T>::allocate(ctx);
}

template<class T>
static inline void
js_deallocate(JSContext* ctx, T* ptr) {
  js_allocator<T>::deallocate(ctx, ptr);
}

template<class T>
static inline void
js_deallocate(JSRuntime* rt, T* ptr) {
  js_allocator<T>::deallocate(rt, ptr);
}

#endif /* defined(JS_ALLOC_HPP) */
