#ifndef LINE_HPP
#define LINE_HPP

#include <opencv2/core/core.hpp>
#include <cmath>
#include <iostream>
#include <type_traits>
#include <exception>
#include <string>
#include <functional>
#include <array>
#include "psimpl.hpp"

template<class T> class LineEnd;

template<class T> class Line {
public:
  typedef cv::Point_<T> point_type;
  typedef Line<T> line_type;
  typedef T value_type;

  point_type a, b;

  Line(const point_type& p1, const point_type& p2) : a(p1), b(p2) {}

  Line(const std::array<T, 4>& arr) : a(arr[0], arr[1]), b(arr[2], arr[3]) {}
  Line(T x1, T y1, T x2, T y2) : a(x1, y1), b(x2, y2) {}

  T length() const;

  point_type at(double sigma) const;

  point_type center() const;
  point_type
  start() const {
    return a;
  }
  point_type
  end() const {
    return b;
  }

  point_type
  slope() const {
    point_type diff = b - a;
    return diff;
  }

  const point_type&
  pivot() const {
    return a;
  }

  const point_type&
  to() const {
    return b;
  }

  void
  swap() {
    point_type temp = a;
    a = b;
    b = temp;
  }

  point_type moment() const;

  double angle() const;

  double distance(const cv::Point_<T>& p) const;

  std::pair<T, size_t> endpoint_distances(const Line<T>& l) const;

  std::pair<T, T> endpoint_distances(const cv::Point_<T>& p) const;

  T endpoint_distance(const cv::Point_<T>& p, size_t* point_index = nullptr) const;

  template<class OtherT>
  std::pair<line_type&, line_type&>
  nearest(const Line<OtherT>& l) const {
    return std::make_pair<T, T>(distance(l.a), distance(l.b));
  }

  template<class OtherT>
  bool
  operator==(const Line<OtherT>& other) const {
    return other.a == this->a && other.b == this->b;
  }

  T min_distance(Line<T>& l2, size_t* point_index = nullptr) const;

  T nearest_end(Line<T>& l2, LineEnd<T>& end) const;

  T
  angle_diff(const Line<T>& l) const {
    return l.angle() - angle();
  }

  std::array<cv::Point_<T>, 2>
  pointsArray() const {
    std::array<cv::Point_<T>, 2> ret = {a, b};
    return ret;
  }

  std::vector<cv::Point_<T>>
  points() const {
    std::vector<cv::Point_<T>> ret{a, b};
    return ret;
  }

  /**
   * Calculates intersect of two lines if exists.
   *
   * @param line1 First line.
   * @param line2 Second line.
   * @param intersect Result intersect.
   * @return True if there is intersect of two lines, otherwise false.
   */
  bool intersect(const Line<T>& line2, cv::Point_<T>* pt = nullptr) const;

  template<class OtherValueT> bool operator<(const Line<OtherValueT>& l2) const;

  std::string str(const std::string& comma = ",", const std::string& sep = "|") const;
};

template<class T> struct line_list {
  typedef T coord_type;
  typedef Line<T> line_type;
  typedef std::vector<line_type> type;
};
typedef line_list<float> line4f_list;
typedef line_list<int> line4i_list;
typedef line_list<double> line4d_list;

float point_distance(const cv::Point2f& p1, const cv::Point2f& p2);
double point_distance(const cv::Point2d& p1, const cv::Point2d& p2);
int point_distance(const cv::Point& p1, const cv::Point& p2);

template<class T>
inline std::ostream&
operator<<(std::ostream& os, const Line<T>& line) {
  os << "Line(" << line.a << "," << line.b << ")";

  // os << ' ';
  return os;
}

template<class T>
double
angle_from_moment(const cv::Point_<T>& point) {
  return std::atan2(point.x, point.y);
}

template<class InputIterator>
inline typename std::iterator_traits<InputIterator>::value_type
segment_distance2(InputIterator s1, InputIterator s2, InputIterator p) {
  typedef typename std::iterator_traits<InputIterator>::value_type value_type;
  return psimpl::math::segment_distance2<2, InputIterator>(s1, s2, p);
}

template<class T>
void
moment_from_angle(double phi, cv::Point_<T>& point) {
  point.x = std::sin(phi);
  point.y = std::cos(phi);
}

template<class T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, T>::type* = nullptr>
inline std::string
to_string(const T& t, size_t n_pad = 3, char ch_pad = ' ') {
  std::ostringstream oss;
  oss << std::fixed << t;
  std::string ret(oss.str());

  if(ret.find('.') != std::string::npos) {
    while(ret.back() == '0') ret.pop_back();
    if(ret.back() == '.')
      ret.pop_back();
  }
  if(ret.length() < n_pad)
    ret.insert(ret.begin(), n_pad - ret.length(), ch_pad);
  else if(ret.length() > n_pad) {
    size_t i = ret.find('.');
    if(i != std::string::npos) {
      ret.resize(std::max(i, n_pad));
    }
  }

  return ret;
}

/*
template<class T>
inline std::string
to_string(const cv::Point_<T>& pt, size_t n_pad = 3, char ch_pad = '0') {
  std::ostringstream oss;
  oss << to_string(pt.x) << ',' << to_string(pt.y);
  return oss.str();
}

*/
template<class T>
double
Line<T>::distance(const cv::Point_<T>& p) const {
  return std::sqrt(segment_distance2(&a.x, &b.x, &p.x));
}

template<class T, class Char = char>
inline std::string
to_string(const Line<T>& line) {
  std::string ret;

  ret += "Line(";
  ret += to_string(line.a);
  ret += ',';
  ret += to_string(line.b);
  ret += ")";

  return ret;
}

template<class T>
inline std::string
to_string(const typename line_list<T>::type& lines) {
  typedef typename std::vector<Line<T>>::const_iterator iterator_type;
  typedef Line<T> value_type;
  std::string ret;
  iterator_type end = lines.cend();
  for(iterator_type it = lines.cbegin(); it != end; ++it) {
    if(ret.length())
      ret += " ";
    ret += to_string<T>(*it);
  }
  return ret;
}

template<class Value>
inline std::ostream&
operator<<(std::ostream& os, const typename line_list<Value>::type& c) {
  typedef typename line_list<Value>::type::const_iterator iterator_type;
  iterator_type end = c.cend();
  int i = 0;
  for(iterator_type it = c.cbegin(); it != end; ++it) {
    if(i++ > 0)
      os << " ";
    os << to_string(*it);
  }
  return os;
}

/*
template<class T>
inline std::ostream&
operator<<(std::ostream& os, const cv::Point_<T>& p) {
  os << "{x:" << p.x << ",y:" << p.y << "}";
  return os;
}*/

template<class ContainerT>

typename ContainerT::iterator
find_nearest_line(typename ContainerT::value_type& line, ContainerT& lines) {
  typedef typename ContainerT::iterator iterator_type;
  typedef typename ContainerT::value_type line_type;
  typedef typename line_type::value_type value_type;
  value_type distance = 1e10;
  iterator_type end = lines.end();
  iterator_type ret = end;

  for(iterator_type it = lines.begin(); it != end; ++it) {
    value_type d = (*it).min_distance(line);
    if(*it == line)
      continue;
    if(d < distance) {
      distance = d;
      ret = it;
    }
  }
  return ret;
}

template<class ContainerT>
typename ContainerT::iterator
find_nearest_line(typename ContainerT::iterator& line, ContainerT& lines) {
  typedef typename ContainerT::iterator iterator_type;
  typedef typename ContainerT::value_type point_type;
  typedef typename point_type::value_type value_type;
  value_type distance = 1e10;
  iterator_type index = lines.end();
  iterator_type end = lines.end();

  for(iterator_type it = lines.begin(); it != end; ++it) {
    value_type d = (*it).min_distance(*line);
    if(std::distance(line, it) == 0)
      continue;
    if(d < distance) {
      distance = d;
      index = it;
    }
  }
  return index;
}

/*
template <class InputIterator>
InputIterator
find_nearest_line(const InputIterator& line, InputIterator from, InputIterator
to) { typedef InputIterator iterator_type; typedef typename
std::iterator_traits<InputIterator>::value_type point_type; typedef typename
point_type::value_type value_type; value_type distance = 1e10; iterator_type
index = to;

  for(iterator_type it = from; it != to; ++it) {
    value_type d = (*it).min_distance(*line);
    if(std::distance(line, it) == 0)
      continue;
    if(d < distance) {
      distance = d;
      index = it;
    }
  }
  return index;
}

*/

template<class T> class LineEnd {
  Line<T>* line;
  size_t point_index;

protected:
  cv::Point_<T>*
  ptr() {
    return line == nullptr ? nullptr : point_index > 0 ? &line->b : &line->a;
  }
  const cv::Point_<T>*
  const_ptr() const {
    return line == nullptr ? nullptr : point_index > 0 ? &line->b : &line->a;
  }

public:
  LineEnd() {}
  LineEnd(Line<T>& l, size_t pt_i) : line(&l), point_index(pt_i) {}
  ~LineEnd() {}

  cv::Point_<T>&
  point() {
    return *ptr();
  }
  cv::Point_<T> const&
  point() const {
    return *const_ptr();
  }

  operator cv::Point_<T>() const { return point(); }
};

struct LineHierarchy {
  int prevSibling;
  int nextSibling;
  int prevParallel;
  int nextParallel;
};

template<class T>
inline T
Line<T>::length() const {
  point_type diff = a - b;
  return std::sqrt(diff.x * diff.x + diff.y * diff.y);
}

template<class T>
inline typename Line<T>::point_type
Line<T>::at(double sigma) const {
  point_type ret;
  ret.x = (a.x + b.x) / 2;
  ret.y = (a.y + b.y) / 2;
  return ret;
}

template<class T>
inline typename Line<T>::point_type
Line<T>::center() const {
  return point_type((a.x + b.x) / 2, (a.y + b.y) / 2);
}

template<class T>
inline typename Line<T>::point_type
Line<T>::moment() const {
  point_type diff(slope());
  double len = length();
  return point_type(diff.x / len, diff.y / len);
}

template<class T>
inline double
Line<T>::angle() const {
  point_type diff(slope());
  double phi = angle_from_moment(diff);
  // double len = length();
  // point_type mom, norm(moment());
  //  point_type mom;
  // moment_from_angle(phi, mom);
  // std::cout << "angle " << (phi *180/M_PI) << " x=" << norm.x << ",y=" <<
  // norm.y << " x=" << mom.x << ",y=" << mom.y << std::endl;
  return phi;
}

template<class T>
inline std::pair<T, T>
Line<T>::endpoint_distances(const cv::Point_<T>& p) const {
  return std::make_pair<T, T>(point_distance(a, p), point_distance(b, p));
}

#if SIZEOF_SIZE_T == SIZEOF_LONG
template<class T>
inline std::pair<T, unsigned long int>
Line<T>::endpoint_distances(const Line<T>& l) const {
  size_t offs1, offs2;
  std::pair<T, T> dist(endpoint_distance(l.a, &offs1), endpoint_distance(l.b, &offs2));
  size_t offs = dist.first < dist.second ? offs1 : offs2;
  return std::make_pair(dist.first < dist.second ? dist.first : dist.second, offs);
}

#else
template<class T>
inline std::pair<T, unsigned long long int>
Line<T>::endpoint_distances(const Line<T>& l) const {
  size_t offs1, offs2;
  std::pair<T, T> dist(endpoint_distance(l.a, &offs1), endpoint_distance(l.b, &offs2));
  size_t offs = dist.first < dist.second ? offs1 : offs2;
  return std::make_pair(dist.first < dist.second ? dist.first : dist.second, offs);
}

#endif

template<class T>
inline T
Line<T>::endpoint_distance(const cv::Point_<T>& p, size_t* point_index) const {
  T dist1 = point_distance(a, p), dist2 = point_distance(b, p);
  T ret = std::min(dist1, dist2);
  if(point_index)
    *point_index = ret;

  return ret;
}

template<class T>
inline T
Line<T>::min_distance(Line<T>& l2, size_t* point_index) const {
  std::pair<T, size_t> dist = endpoint_distances(l2);
  /* if(intersect(l2))
   return 0;*/
  if(point_index)
    *point_index = dist.second;

  return dist.first;
}

template<class T>
inline T
Line<T>::nearest_end(Line<T>& l2, LineEnd<T>& end) const {
  size_t point_index;
  T dist = min_distance(l2, &point_index);
  end = LineEnd<T>(l2, point_index);
  return dist;
}

template<class T>
inline bool
Line<T>::intersect(const Line<T>& line2, cv::Point_<T>* pt) const {
  point_type x = line2.pivot() - pivot();
  point_type d1 = slope();
  point_type d2 = line2.slope();
  float inter = d1.x * d2.y - d1.y * d2.x;
  if(fabs(inter) < 1e-8) {
    return false;
  }
  double t1 = (x.x * d2.y - x.y * d2.x) / inter;
  if(pt)
    *pt = pivot() + d1 * t1;

  return true;
}

template<class T, class Pred>
inline std::vector<int>
filter_lines(const std::vector<T>& c, bool (&pred)(const Line<T>&, size_t)) {
  return filter_lines<typename line_list<T>::type::iterator, bool(Line<T>&, size_t)>(c.begin(), c.end(), pred);
}

template<class ValueT, class InputIterator>
inline std::vector<typename std::iterator_traits<InputIterator>::value_type::value_type>
angle_diffs(Line<ValueT>& line, InputIterator from, InputIterator to) {
  typedef InputIterator iterator_type;
  typedef typename std::iterator_traits<InputIterator>::value_type point_type;
  typedef typename point_type::value_type value_type;
  typedef std::vector<value_type> ret_type;

  ret_type ret;
  value_type distance = 1e10;
  iterator_type index = to;

  for(iterator_type it = from; it != to; ++it) {
    value_type d;

    ret.push_back((*it).angle_diff(line));
  }
  return ret;
}

template<class InputIterator>
inline std::vector<float>
line_distances(typename std::iterator_traits<InputIterator>::value_type& line, InputIterator from, InputIterator to) {
  typedef InputIterator iterator_type;
  typedef typename std::iterator_traits<InputIterator>::value_type line_type;
  typedef typename line_type::value_type value_type;
  typedef std::vector<float> ret_type;

  ret_type ret;
  value_type distance = 1e10;
  iterator_type index = to;

  for(InputIterator it = from; it != to; ++it) {
    /* if(line == *it)
       continue;*/
    ret.push_back(it->min_distance(line));
  }
  return ret;
}

template<class InputIterator, class Pred>
inline std::vector<int>
filter_lines(InputIterator from, InputIterator to, Pred predicate) {
  typedef InputIterator iterator_type;
  typedef typename std::iterator_traits<InputIterator>::value_type value_type;
  std::vector<int> ret;
  size_t index = 0;
  for(iterator_type it = from; it != to; ++it) {

    if(predicate(*it, index++)) {

      std::size_t index = std::distance(from, it);
      ret.push_back(index);
    }
  }
  return ret;
}

template<class T> class PredicateTraits {
public:
  typedef bool type(const Line<T>&, size_t);
  typedef std::function<bool(const Line<T>&, size_t)> function;
};

template<class T>
template<class OtherValueT>
inline bool
Line<T>::operator<(const Line<OtherValueT>& l2) const {
  cv::Point2f a, b;
  a = center();
  b = l2.center();
  return a.y < b.y ? true : a.x < b.x;
}

template<class T>
inline std::string
Line<T>::str(const std::string& comma, const std::string& sep) const {
  point_type p = pivot(), s = slope();
  std::ostringstream os;
  // os << '[';
  os << to_string(a.x) << comma << to_string(a.y);
  os << sep << to_string(b.x);
  os << comma << to_string(b.y);
  os << '=' << to_string(length());
  // os << '@' << to_string(floor(angle()*180/ M_PI));
  // os << ']';
  return os.str();
}

#endif // defined LINE_HPP
