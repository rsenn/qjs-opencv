#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <sys/stat.h>

#include <opencv2/core.hpp>

#define COLOR_BLACK "\x1b[30m"
#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_WHITE "\x1b[37m"

#define COLOR_GRAY "\x1b[1;30m"
#define COLOR_LIGHTRED "\x1b[1;31m"
#define COLOR_LIGHTGREEN "\x1b[1;32m"
#define COLOR_LIGHTYELLOW "\x1b[1;33m"
#define COLOR_LIGHTBLUE "\x1b[1;34m"
#define COLOR_LIGHTMAGENTA "\x1b[1;35m"
#define COLOR_LIGHTCYAN "\x1b[1;36m"
#define COLOR_LIGHTWHITE "\x1b[1;37m"

#define COLOR_NONE "\x1b[m"

bool str_end(const char* str, const char* suffix);
bool str_end(const std::string& str, const std::string& suffix);

std::string to_string(const cv::Scalar& scalar);

std::string make_filename(const std::string& name, int count, const std::string& ext, const std::string& dir = "tmp");

inline int32_t
get_mtime(const char* filename) {
#if __STDC_VERSION__ >= 201710L
  return std::filesystem::last_write_time(filename);
#else
  struct stat st;
  if(stat(filename, &st) != -1) {
    uint32_t ret = st.st_mtime;
    return ret;
  }
#endif
  return -1;
}

template<class Char, class Value>
inline std::ostream&
operator<<(std::ostream& os, const std::vector<Value>& c) {
  typedef typename std::vector<Value>::const_iterator iterator_type;
  iterator_type end = c.end();
  for(iterator_type it = c.begin(); it != end; ++it) {
    os << ' ';
    os << to_string(*it);
  }
  return os;
}

inline std::string
to_string(const cv::Scalar& scalar) {
  const int pad = 3;
  std::ostringstream oss;
  oss << '[' << std::setfill(' ') << std::setw(pad) << scalar[0] << ',' << std::setfill(' ') << std::setw(pad) << scalar[1]
      << ',' << std::setfill(' ') << std::setw(pad) << scalar[2] << ',' << std::setfill(' ') << std::setw(pad) << scalar[3]
      << ']';
  return oss.str();
}

template<class Iterator>
static inline std::string
join(const Iterator& start, const Iterator& end, const std::string& delim) {
  return std::accumulate(start, end, std::string(), [&delim](const std::string& a, const std::string& b) -> std::string {
    return a + (a.length() > 0 ? delim : "") + b;
  });
}

extern "C" void* get_heap_base();

typedef struct JSMatDimensions {
  uint32_t rows, cols;

  operator cv::Size() const { return cv::Size(cols, rows); }
} JSMatDimensions;

static inline int
mattype_depth(int type) {
  return type & 0x7;
}
static inline int
mattype_channels(int type) {
  return (type >> 3) + 1;
}
static inline bool
mattype_floating(int type) {
  switch(mattype_depth(type)) {
    case CV_32F:
    case CV_64F: return true;
    default: return false;
  }
}
static inline bool
mattype_signed(int type) {
  switch(mattype_depth(type)) {
    case CV_8S:
    case CV_16S:
    case CV_32S: return true;
    default: return false;
  }
}

template<class T>
static inline JSMatDimensions
mat_dimensions(const T& mat) {
  JSMatDimensions ret;
  ret.rows = mat.rows;
  ret.cols = mat.cols;
  return ret;
}

static inline uint8_t*
mat_ptr(cv::Mat& mat) {
  return reinterpret_cast<uint8_t*>(mat.ptr());
}

static inline uint8_t*
mat_ptr(cv::UMat& mat) {
  cv::UMatData* u;

  if((u = mat.u))
    return reinterpret_cast<uint8_t*>(u->data);

  return nullptr;
}

static inline size_t
mat_offset(const cv::Mat& mat, uint32_t row, uint32_t col) {
  const uchar *base, *ptr;

  base = mat.ptr<uchar>();
  ptr = mat.ptr<uchar>(row, col);

  return ptr - base;
}

static inline size_t
mat_offset(const cv::UMat& mat, uint32_t row, uint32_t col) {
  return (size_t(mat.cols) * row + col) * mat.elemSize();
}

template<class T>
static inline T&
mat_at(cv::Mat& mat, uint32_t row, uint32_t col) {
  return *mat.ptr<T>(row, col);
}

template<class T>
static inline T&
mat_at(cv::UMat& mat, uint32_t row, uint32_t col) {
  size_t offs = mat_offset(mat, row, col);
  return *reinterpret_cast<T*>(mat_ptr(mat) + offs);
}

template<class T>
static inline size_t
mat_bytesize(const T& mat) {
  return mat.elemSize() * mat.total();
}

template<class T>
static inline int
mat_depth(const T& mat) {
  return mattype_depth(mat.type());
}

template<class T>
static inline int
mat_channels(const T& mat) {
  return mattype_channels(mat.type());
}

template<class T>
static inline bool
mat_signed(const T& mat) {
  return mattype_signed(mat.type());
}

template<class T>
static inline bool
mat_floating(const T& mat) {
  return mattype_floating(mat.type());
}

template<class T, int N>
static inline T*
begin(cv::Vec<T, N>& v) {
  return &v[0];
}
template<class T, int N>
static inline T*
end(cv::Vec<T, N>& v) {
  return &v[N];
}

template<class T, int N>
static inline T const*
begin(cv::Vec<T, N> const& v) {
  return &v[0];
}
template<class T, int N>
static inline T const*
end(cv::Vec<T, N> const& v) {
  return &v[N];
}
/*
template<class T>
static inline typename T::value_type const*
begin(const T& obj) {
  return obj.begin();
}
template<class T>
static inline typename T::value_type*
begin(T& obj) {
  return obj.begin();
}

template<class T>
static inline typename T::value_type const*
end(const T& obj) {
  return obj.end();
}
template<class T>
static inline typename T::value_type*
end(T& obj) {
  return obj.end();
}
*/
template<class T>
static inline T*
begin(std::vector<T>& v) {
  return &v[0];
}
template<class T>
static inline T*
end(std::vector<T>& v) {
  return &v[v.size()];
}
template<class T>
static inline T const*
begin(std::vector<T> const& v) {
  return &v[0];
}
template<class T>
static inline T const*
end(std::vector<T> const& v) {
  return &v[v.size()];
}

static inline uint8_t*
begin(cv::Mat& mat) {
  return mat.ptr<uint8_t>();
}
static inline uint8_t*
end(cv::Mat& mat) {
  return mat.ptr<uint8_t>() + (mat.total() * mat.elemSize());
}

static inline uint8_t const*
begin(cv::Mat const& mat) {
  return mat.ptr<uint8_t const>();
}
static inline uint8_t const*
end(cv::Mat const& mat) {
  return mat.ptr<uint8_t const>() + (mat.total() * mat.elemSize());
}

#if CXX_STANDARD >= 20
#include <ranges>

template<class T>
static inline std::ranges::subrange<T>
sized_range(T ptr, size_t len) {
  return std::ranges::subrange<T>(ptr, ptr + len);
}

template<class T>
static inline std::ranges::subrange<T*>
argument_range(int argc, T* argv) {
  return std::ranges::subrange<T*>(argv, argv + argc);
}

template<class T>
static inline std::ranges::subrange<T*>
range(T* begin, T* end) {
  return std::ranges::subrange<T*>(begin, end);
}

template<class Container>
static inline std::ranges::subrange<typename Container::value_type*>
range(Container& c) {
  return std::ranges::subrange<typename Container::value_type*>(begin(c), end(c));
}

template<class T, class Container>
static inline std::ranges::subrange<T>
range(Container& c) {
  return std::ranges::subrange<T>(reinterpret_cast<T>(begin(c)), reinterpret_cast<T>(end(c)));
}
#endif

#endif // defined(UTIL_HPP)
