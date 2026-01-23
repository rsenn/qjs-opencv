#ifndef UTIL_HPP
#define UTIL_HPP

#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>
#include <quickjs.h>
#include <stddef.h>
#include <cstdint>
#include <sys/stat.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <numeric>
#include <string>
#include <vector>

#define countof(x) (sizeof(x) / sizeof((x)[0]))

#if defined(_WIN32) || defined(__MINGW32__)
#define VISIBLE __declspec(dllexport)
#define HIDDEN
#else
#define VISIBLE __attribute__((visibility("default")))
#define HIDDEN __attribute__((visibility("hidden")))
#endif

#ifndef thread_local
#ifdef _Thread_local
#define thread_local _Thread_local
#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
#define thread_local __thread
#elif defined(_WIN32)
#define thread_local __declspec(thread)
#else
#error No TLS implementation found.
#endif
#endif

#define JS_CGETSET_ENUMERABLE_DEF(prop_name, fgetter, fsetter, magic_num) \
  { \
    .name = prop_name, .prop_flags = JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE, .def_type = JS_DEF_CGETSET_MAGIC, .magic = magic_num, .u = { \
      .getset = {.get = {.getter_magic = fgetter}, .set = {.setter_magic = fsetter}} \
    } \
  }

#define JS_CONSTANT(name) JS_PROP_INT32_DEF(#name, name, 0)
#define JS_CV_CONSTANT(name) JS_PROP_INT32_DEF(#name, cv::name, JS_PROP_ENUMERABLE)

typedef std::vector<JSCFunctionListEntry> js_function_list_t;

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

#define ALIGN_CENTER 0
#define ALIGN_LEFT 1
#define ALIGN_RIGHT 2
#define ALIGN_HORIZONTAL (ALIGN_LEFT | ALIGN_RIGHT)
#define ALIGN_MIDDLE 0
#define ALIGN_TOP 4
#define ALIGN_BOTTOM 8
#define ALIGN_VERTICAL (ALIGN_TOP | ALIGN_BOTTOM)

bool str_end(const char* str, const char* suffix);
bool str_end(const std::string& str, const std::string& suffix);

std::string to_string(const cv::Scalar& scalar);

std::string make_filename(const std::string& name, int count, const std::string& ext, const std::string& dir = "tmp");

static inline size_t
round_to(size_t num, size_t x) {
  num /= x;
  num *= x;
  return num;
}

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
  oss << '[' << std::setfill(' ') << std::setw(pad) << scalar[0] << ',' << std::setfill(' ') << std::setw(pad) << scalar[1] << ',' << std::setfill(' ')
      << std::setw(pad) << scalar[2] << ',' << std::setfill(' ') << std::setw(pad) << scalar[3] << ']';
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

JSMatDimensions mat_dimensions(const cv::Mat& mat);
JSMatDimensions mat_dimensions(const cv::UMat& mat);

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

template<class T, int rows, int cols>
static inline T*
mat_ptr(cv::Matx<T, rows, cols>& mat) {
  return &static_cast<T&>(mat(0, 0));
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
mat_at(const cv::Mat& mat, uint32_t row, uint32_t col) {
  return *const_cast<cv::Mat*>(&mat)->ptr<T>(row, col);
}

template<class T>
static inline T&
mat_at(cv::UMat& mat, uint32_t row, uint32_t col) {
  size_t offs = mat_offset(mat, row, col);
  return *reinterpret_cast<T*>(mat_ptr(mat) + offs);
}

template<class T, int rows, int cols>
static inline T&
mat_at(const cv::Matx<T, rows, cols>& mat, uint32_t row, uint32_t col) {
  return mat(row, col);
}

static inline cv::Size
mat_size(const cv::Mat& mat) {
  return cv::Size(mat.cols, mat.rows);
}

template<class T, int rows, int cols>
static inline cv::Size
mat_size(const cv::Matx<T, rows, cols>& mat) {
  return cv::Size(cols, rows);
}

template<class T, int rows, int cols>
static inline std::array<T, rows * cols>&
mat_array(cv::Matx<T, rows, cols>& mat) {
  return *reinterpret_cast<std::array<T, rows * cols>*>(mat_ptr(mat));
}
/*
template<class T, int rows, int cols>
static inline std::array<T, rows * cols> const&
mat_array(cv::Matx<T, rows, cols> const& mat) {
  return *reinterpret_cast<std::array<T, rows * cols> const*>(mat_ptr(mat));
}
*/
static inline size_t
mat_bytesize(const cv::Mat& mat) {
  return mat.ptr<uchar>(mat.rows - 1, mat.cols - 1) - mat.ptr<uchar>() + mat.elemSize();
  // return mat.elemSize() * mat.total();
}

int mat_depth(const cv::Mat& mat);
int mat_channels(const cv::Mat& mat);
bool mat_signed(const cv::Mat& mat);
bool mat_floating(const cv::Mat& mat);
int mat_depth(const cv::UMat& mat);
int mat_channels(const cv::UMat& mat);
bool mat_signed(const cv::UMat& mat);
bool mat_floating(const cv::UMat& mat);

template<typename T = int>
static inline T
mat_col(const cv::Mat& mat, const T col) {
  T x = col;
  if(x < 0 && -x >= T(mat.cols))
    x %= T(mat.cols);
  if(x < 0)
    x += mat.cols;
  return x;
}

template<typename T = int>
static inline T
mat_row(const cv::Mat& mat, const T row) {
  T y = row;
  if(y < 0 && -y >= T(mat.rows))
    y %= T(mat.rows);
  if(y < 0)
    y += mat.rows;
  return y;
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

template<class T> class range_view {
public:
  // range_view(range_view<T> const& range) : p(range.begin()), n(range.size()) {}
  range_view(T* const base, size_t size) : p(base), n(size) {}

  range_view<T>& operator=(range_view<T> const& range) {
    p = range.begin();
    n = range.size();
    return *this;
  }

  // clang-format off
  T* const begin() const {return p; }
  T* const end() const {return p + n; }
  size_t size() const {return n; }
  // clang-format on

private:
  T* p;
  size_t n;
};

template<class T>
static inline range_view<T>
argument_range(int argc, T argv[]) {
  return range_view<T>(argv, argc);
}

template<class T>
static inline range_view<T>
sized_range(T ptr, size_t len) {
  return range_view<T>(ptr, ptr + len);
}

template<class T>
static inline range_view<T>
range(T* begin, T* end) {
  return range_view<T>(begin, end - begin);
}

template<class Container>
static inline range_view<typename Container::value_type>
range(Container& c) {
  return range_view<typename Container::value_type>(begin(c), end(c) - begin(c));
}

template<class T, class Container>
static inline range_view<T>
range(Container& c) {
  return range_view<T>(reinterpret_cast<T>(begin(c)), reinterpret_cast<T>(end(c)));
}

std::string js_prop_flags(int flags);

std::ostream& operator<<(std::ostream& s, const JSCFunctionListEntry& entry);

template<class Stream, class Item>
Stream&
operator<<(Stream& s, const std::vector<Item>& vector) {
  size_t i = 0;
  for(auto entry : vector) {

    s << "#" << i << " ";
    s << entry;
    i++;
  }

  return s;
}

template<class T>
static inline cv::Point_<T>
add(const cv::Point_<T>& a, const cv::Point_<T>& b) {
  return cv::Point_<T>(a.x + b.x, a.y + b.y);
}

template<class T, class U>
static inline cv::Point_<double>
add(const cv::Point_<T>& a, const cv::Point_<U>& b) {
  return cv::Point_<double>(a.x + b.x, a.y + b.y);
}

template<class T>
static inline cv::Point_<T>
sub(const cv::Point_<T>& a, const cv::Point_<T>& b) {
  return cv::Point_<T>(a.x - b.x, a.y - b.y);
}

template<class T, class U>
static inline cv::Point_<double>
sub(const cv::Point_<T>& a, const cv::Point_<U>& b) {
  return cv::Point_<double>(a.x - b.x, a.y - b.y);
}

template<class T>
static inline cv::Point_<T>
div(const cv::Point_<T>& p, T d) {
  return cv::Point_<T>(p.x / d, p.y / d);
}

template<class T>
static inline cv::Point_<T>
div(const cv::Point_<T>& p, const cv::Size_<T>& s) {
  return cv::Point_<T>(p.x / s.width, p.y / s.height);
}

template<class T>
static inline cv::Point_<T>
mul(const cv::Point_<T>& p, T f) {
  return cv::Point_<T>(p.x * f, p.y * f);
}

template<class T>
static inline cv::Point_<T>
mul(const cv::Point_<T>& p, const cv::Size_<T>& s) {
  return cv::Point_<T>(p.x * s.width, p.y * s.height);
}

static inline std::string
dump(const cv::_InputArray& arr) {
  std::ostringstream os;
  os << "{ ";
  os << "type: " << arr.type();
  os << ", depth: " << arr.depth();
  os << ", channels: " << arr.channels();
  os << ", total: " << arr.total();
  os << " }";

  return os.str();
}

static inline std::string
dump(const cv::_InputOutputArray& arr) {
  std::ostringstream os;
  os << "{ ";
  os << "type: " << arr.type();
  os << ", depth: " << arr.depth();
  os << ", channels: " << arr.channels();
  os << ", total: " << arr.total();
  os << " }";
  return os.str();
}

static inline std::string
dump(const cv::_OutputArray& arr) {
  std::ostringstream os;
  os << "{ ";
  os << "type: " << arr.type();
  os << ", depth: " << arr.depth();
  os << ", channels: " << arr.channels();
  os << ", total: " << arr.total();
  os << " }";

  return os.str();
}

#endif // defined(UTIL_HPP)
