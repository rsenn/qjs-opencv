#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/types_c.h>

#include <vector>
#include <sstream>
#include <iomanip>

#include "psimpl.hpp"

template<class T> struct point {
  typedef T value_type;
  typedef cv::Point_<T> type;
  typedef std::vector<type> vector_type;
};

template<class T> struct point_list { typedef std::vector<typename point<T>::type> type; };
template<class T> struct contour {
  typedef T coord_type;
  typedef typename point<T>::type point_type;
  typedef std::vector<point_type> type;
};

template<class T> struct contour_list {
  typedef T coord_type;
  typedef typename contour<T>::point_type point_type;
  typedef typename contour<T>::type contour_type;
  typedef std::vector<contour_type> type;
};

template<class T> using point_type = typename point<T>::type;

template<class T> using point_vector = typename point_list<T>::type;

template<class T> using contour_type = typename contour<T>::type;

template<class T> using contour_vector = typename contour_list<T>::type;

typedef std::vector<cv::Vec4i> vec4i_vector;

// Function that calculates the area given a
// std::vector of vertices in the XY plane.
template<class P>
inline double
polygon_area(std::vector<P> list) {

  if(list.size() < 3)
    return 0;
  double area = 0;                     // Total Area
  double diff = 0;                     // Difference Of Y{i + 1} - Y{i - 1}
  unsigned int last = list.size() - 1; // Size Of Vector - 1
  /* Given vertices from 1 to n, we first loop through
  the vertices 2 to n - 1. We will take into account
  vertex 1 and vertex n sepereately */
  for(size_t i = 1; i < last; i++) {
    diff = list[i + 1].y - list[i - 1].y;
    area += list[i].x * diff;
  }
  /* Now We Consider The Vertex 1 And The Vertex N */
  diff = list[1].y - list[last].y;
  area += list[0].x * diff; // Vertex 1
  diff = list[0].y - list[last - 1].y;
  area += list[last].x * diff; // Vertex N
  /* Calculate The Final Answer */
  area = 0.5 * fabs(area);
  return area; // Return The Area
}

point_vector<float> get_mass_centers(std::vector<point_vector<int>> contours);

template<class T, class Char = char>
inline std::basic_string<Char>
to_string(const cv::Point_<T>& point) {
  const int pad = 3;
  std::basic_ostringstream<Char> os;
  os << "{x:" << std::setfill(' ') << std::setw(pad) << point.x << ",y:" << std::setfill(' ') << std::setw(pad) << point.y
     << "}";
  return os.str();
}

template<class ValueT, template<typename> class Container = std::vector, class Char = char>
inline std::basic_string<Char>
to_string(const Container<cv::Point_<ValueT>>& points) {
  typedef typename Container<cv::Point_<ValueT>>::const_iterator iterator_type;
  typedef cv::Point_<ValueT> value_type;
  std::basic_string<Char> ret;
  iterator_type end = points.cend();
  for(iterator_type it = points.cbegin(); it != end; ++it) {
    if(ret.length())
      ret += ",";
    ret += to_string<ValueT, Char>(*it);
  }
  return "[" + ret + "]";
}

template<class T>
inline T*
coord_pointer(cv::Point_<T>* point_ptr) {
  return reinterpret_cast<T*>(point_ptr);
}

template<class T>
inline const T*
coord_pointer(const cv::Point_<T>* point_ptr) {
  return reinterpret_cast<const T*>(point_ptr);
}

template<class T>
inline std::vector<cv::Point_<T>>
simplify_polyline(const std::vector<cv::Point_<T>>& points) {
  typedef T coord_type;
  typedef cv::Point_<T> point_type;
  typedef std::vector<point_type> vector_type;
  vector_type ret;
  ret.resize(points.size());

  psimpl::PolylineSimplification<2, const coord_type*, coord_type*> psimpl;
  auto output = coord_pointer(ret.data());

  // auto end = psimpl.nth_point(coord_pointer(points.data()),
  // coord_pointer(&points.data()[points.size()]), 20, output); auto end =
  // psimpl.radial_distance(coord_pointer(points.data()),
  // coord_pointer(&points.data()[points.size()]), 10, output);
  auto end = psimpl.Opheim(coord_pointer(points.data()), coord_pointer(&points.data()[points.size()]), 4, 30, output);
  size_t outn = std::distance(output, end) / 2;

  // logfile << "simplification 1:" << ((double)points.size() / outn) <<
  // std::endl;
  ret.resize(outn);
  return ret;
}

// helper function:
// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2
template<class T>
inline double
angle(cv::Point_<T> pt1, cv::Point_<T> pt2, cv::Point_<T> pt0) {
  T dx1 = pt1.x - pt0.x;
  T dy1 = pt1.y - pt0.y;
  T dx2 = pt2.x - pt0.x;
  T dy2 = pt2.y - pt0.y;
  return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
}

template<class To, class From>
inline void
convert_points(const typename point_list<From>::type& from, typename point_list<To>::type& to) {
  std::transform(from.cbegin(), from.cend(), std::back_inserter(to), [](cv::Point_<From> p) -> cv::Point_<To> {
    return cv::Point_<To>(p.x, p.y);
  });
}

template<class To, class From>
inline typename point_list<To>::type
transform_points(const typename point_list<From>::type& from) {
  typename point_list<To>::type ret;
  convert_points<To, From>(from, ret);
  return ret;
}

template<class InputIterator, class OutputIterator>
inline OutputIterator
transform_points(InputIterator s, InputIterator e, OutputIterator o) {
  typedef typename std::iterator_traits<InputIterator>::value_type input_type;
  typedef typename std::iterator_traits<OutputIterator>::value_type output_type;
  o = std::transform(s, e, o, [](const input_type& p) -> output_type { return output_type(p.x, p.y); });
  return o;
}

template<class InputIterator, class OutputIterator>
inline OutputIterator
transform_contours(InputIterator s, InputIterator e, OutputIterator o) {
  typedef typename std::iterator_traits<InputIterator>::value_type input_type;
  typedef typename std::iterator_traits<OutputIterator>::value_type output_type;
  o = std::transform(s, e, o, [](const input_type& p) -> output_type {
    output_type ret;
    ret.resize(p.size());

    transform_points(p.cbegin(), p.cend(), ret.begin());

    return ret;
  });
  return o;
}

#endif // defined GEOMETRY_HPP
