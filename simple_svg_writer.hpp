#ifndef SIMPLE_SVG_WRITER_HPP
#define SIMPLE_SVG_WRITER_HPP

#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <memory>

namespace simple_svg {

//-----------------------------------------------------------------------------
inline std::string
to_string(double value) {
  std::stringstream s;
  s << value;
  return s.str();
}

//-----------------------------------------------------------------------------
class Attribute {
  std::string name;
  std::string value;

public:
  Attribute(std::string name, std::string value) : name(name), value(value) {}
  Attribute(std::string name, double value) : name(name), value(to_string(value)) {}
  Attribute(std::string name, int32_t value) : name(name), value(std::to_string(value)) {}
  Attribute(std::string name, bool value) : name(name), value(value ? "true" : "false") {}

  std::string
  Name() const {
    return name;
  }
  std::string
  Value() const {
    return value;
  }
  void
  Value(std::string value) {
    this->value = value;
  }

  std::string
  ToText() const {
    return name + "=\"" + value + "\"";
  }

  friend std::ostream&
  operator<<(std::ostream& stream, const Attribute& attribute) {
    return stream << attribute.ToText();
  }
};

class Transform {
  // https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/transform
  std::vector<std::string> transforms;

public:
  Transform() {}

  Transform&
  matrix(double a, double b, double c, double d, double e, double f) {
    std::stringstream stream;
    stream << "matrix(" << a << " " << b << " " << c << " " << d << " " << e << " " << f << ')';
    transforms.push_back(stream.str());

    return *this;
  }

  Transform&
  Translate(double dx, double dy = 0.0) {
    std::stringstream stream;
    stream << "translate(" << dx << " " << dy << ')';
    transforms.push_back(stream.str());

    return *this;
  }

  Transform&
  Scale(double scale_x, double scale_y) {
    std::stringstream stream;
    stream << "scale("
           << " " << scale_x << " " << scale_y << ')';
    transforms.push_back(stream.str());

    return *this;
  }

  Transform&
  Scale(double scale_x) {
    return Scale(scale_x, scale_x);
  }

  Transform&
  Rotate(double angle, double about_x = 0.0, double about_y = 0.0) {
    std::stringstream stream;
    stream << "rotate(" << angle << " " << about_x << " " << about_y << ')';
    transforms.push_back(stream.str());

    return *this;
  }

  Transform&
  SkewX(double skew_x) {
    std::stringstream stream;
    stream << "skewX("
           << " " << skew_x << ')';
    transforms.push_back(stream.str());

    return *this;
  }

  Transform&
  SkewY(double skew_y) {
    std::stringstream stream;
    stream << "skewY("
           << " " << skew_y << ')';
    transforms.push_back(stream.str());

    return *this;
  }

  Attribute
  AsAttribute() const {
    std::stringstream stream;

    for(size_t i = transforms.size(); i != 0; --i)
    // for (const auto &t : transforms)
    {
      stream << transforms[i - 1] << " ";
    }

    return {"transform", stream.str()};
  }
};

//-----------------------------------------------------------------------------
class Base {
  std::string tag;
  std::vector<Attribute> attributes;

protected:
  virtual std::string
  Extras() const {
    return {};
  }

public:
  Base(const std::string& tag) : tag(tag) {}
  Base(const std::string& tag, const std::vector<Attribute>& attributes) : tag(tag), attributes(attributes) {}

  virtual ~Base() {}

  std::string
  Tag() const {
    return tag;
  }
  const auto&
  Attributes() const {
    return attributes;
  }

  Base&
  AddAttribute(const Attribute& attribute) {
    auto ii = std::find_if(attributes.begin(), attributes.end(), [attribute](const auto& a) {
      return a.Name().compare(attribute.Name()) == 0;
    });
    if(ii != attributes.end()) {
      ii->Value(attribute.Value());
    } else {
      attributes.push_back(attribute);
    }
    return *this;
  }

  Base&
  Id(const std::string& id) {
    return AddAttribute({"id", id});
  }

  Base&
  Class(const std::string& class_name) {
    return AddAttribute({"class", class_name});
  }

  Base&
  Stroke(const std::string& stroke) {
    return AddAttribute({"stroke", stroke});
  }

  Base&
  StrokeWidth(const double& stroke_width) {
    return AddAttribute({"stroke-width", stroke_width});
  }

  Base&
  Fill(const std::string& fill) {
    return AddAttribute({"fill", fill});
  }

  Base&
  Transform(const Transform& transform) {
    return AddAttribute(transform.AsAttribute());
  }

  virtual std::string
  ToText() const {
    std::ostringstream stream;
    stream << "<" << tag;

    stream << ' ' << Extras();

    for(const auto& attribute : attributes) { stream << ' ' << attribute; }
    stream << "/>";

    return stream.str();
  }

  friend std::ostream&
  operator<<(std::ostream& stream, const Base& base) {
    return stream << base.ToText();
  }
};

class Rect : public Base {
public:
  Rect() : Base("rect") {}
  Rect(double x, double y, double w, double h) : Base("rect", {{"x", x}, {"y", y}, {"width", w}, {"height", h}}) {}
  Rect(double w, double h) : Base("rect", {{"width", w}, {"height", h}}) {}
};

class Point {
  double x{0.0};
  double y{0.0};

public:
  Point() {}
  Point(double x, double y) : x(x), y(y) {}

  std::string
  ToText() const {
    std::stringstream stream;
    stream << x << ',' << y;
    return stream.str();
  }

  friend std::ostream&
  operator<<(std::ostream& stream, const Point& point) {
    return stream << point.ToText();
  }
};

class PolyBase : public Base {
  std::vector<Point> points;

protected:
  virtual std::string
  Extras() const override {
    std::ostringstream stream;

    for(const auto& p : points) { stream << p << " "; }

    return Attribute("points", stream.str()).ToText();
  }

public:
  PolyBase(std::string tag) : Base(tag) {}
  PolyBase(std::string tag, const std::vector<Point>& points) : Base(tag), points(points) {}
  virtual ~PolyBase() override {}

  PolyBase&
  Add(const Point& point) {
    points.push_back(point);
    return *this;
  }

  PolyBase&
  Add(double x, double y) {
    points.push_back({x, y});
    return *this;
  }
};

class Polyline : public PolyBase {
public:
  Polyline() : PolyBase("polyline") {}
  Polyline(const std::vector<Point>& points) : PolyBase("polyline", points) {}
  virtual ~Polyline() override {}
};

class Polygon : public PolyBase {
public:
  Polygon() : PolyBase("polygon") {}
  Polygon(const std::vector<Point>& points) : PolyBase("polygon", points) {}
  virtual ~Polygon() override {}
};

class Line : public Base {
public:
  Line() : Base("line") {}
  Line(double from_x, double from_y, double to_x, double to_y)
      : Base("line", {{"x1", from_x}, {"y1", from_y}, {"x2", to_x}, {"y2", to_y}}) {}
  virtual ~Line() override {}
};

class Circle : public Base {
public:
  Circle() : Base("circle") {}
  Circle(double center_x, double center_y, double radius)
      : Base("circle", {{"cx", center_x}, {"cy", center_y}, {"r", radius}}) {}
  virtual ~Circle() override {}
};

class Ellipse : public Base {
public:
  Ellipse() : Base("ellipse") {}
  Ellipse(double center_x, double center_y, double radius_x, double radius_y)
      : Base("ellipse", {{"cx", center_x}, {"cy", center_y}, {"rx", radius_x}, {"ry", radius_y}}) {}
  virtual ~Ellipse() override {}
};

class Use : public Base {
public:
  Use() : Base("use") {}
  Use(std::string reference_id) : Base("use", {{"xlink:href", '#' + reference_id}}) {}
  virtual ~Use() override {}
};

//-----------------------------------------------------------------------------
class GroupBase : public Base {
  std::vector<std::shared_ptr<Base>> objects;

protected:
  std::string
  StartTag() const {
    std::ostringstream stream;
    stream << "<" << Tag();

    for(const auto& attribute : Attributes()) { stream << ' ' << attribute; }
    stream << ">";

    return stream.str();
  }

  std::string
  EndTag() const {
    std::ostringstream stream;
    stream << "</" << Tag() << ">";

    return stream.str();
  }

public:
  GroupBase(std::string group_tag) : Base(group_tag) {}
  GroupBase(std::string group_tag, const std::vector<Attribute>& attributes) : Base(group_tag, attributes) {}
  virtual ~GroupBase() override {}

  template<typename T>
  GroupBase&
  Append(const T& object) {
    objects.push_back(std::make_shared<T>(object));
    return *this;
  }

  virtual std::string
  ToText() const override {
    std::ostringstream stream;
    stream << StartTag() << '\n';

    for(const auto& object : objects) { stream << "  " << *object << '\n'; }

    stream << EndTag();

    return stream.str();
  }

  friend std::ostream&
  operator<<(std::ostream& stream, const GroupBase& group_base) {
    return stream << group_base.ToText();
  }
};

class Text : public GroupBase {
  std::string text;

public:
  Text(double x, double y, const std::string& text) : GroupBase("text", {{"x", x}, {"y", y}}), text(text) {}
  virtual ~Text() override {}

  virtual std::string
  ToText() const override {
    std::ostringstream stream;
    stream << StartTag();
    stream << text;
    stream << EndTag();

    return stream.str();
  }
};

class Group : public GroupBase {
public:
  Group() : GroupBase("g") {}
  virtual ~Group() override {}

  friend std::ostream&
  operator<<(std::ostream& stream, const Group& group) {
    return stream << group.ToText();
  }
};

class Layer : public GroupBase {
public:
  Layer() : GroupBase("g", {{"inkscape:groupmode", std::string("layer")}}) {}
  Layer(const std::string& name) : GroupBase("g", {{"inkscape:label", name}, {"inkscape:groupmode", std::string("layer")}}) {}
  virtual ~Layer() override {}

  friend std::ostream&
  operator<<(std::ostream& stream, const Layer& group) {
    return stream << group.ToText();
  }
};

class Document : public GroupBase {
public:
  //<?xml version="1.0"?>
  //<svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
  //</svg>
  Document()
      : GroupBase("svg",
                  {{"xmlns", std::string("http://www.w3.org/2000/svg")},
                   {"xmlns:xlink", std::string("http://www.w3.org/1999/xlink")},
                   {"xmlns:inkscape", std::string("http://www.inkscape.org/namespaces/inkscape")}}) {}
  Document(double width, double height)
      : GroupBase("svg",
                  {{"width", to_string(width)},
                   {"height", to_string(height)},
                   {"xmlns", std::string("http://www.w3.org/2000/svg")},
                   {"xmlns:xlink", std::string("http://www.w3.org/1999/xlink")},
                   {"xmlns:inkscape", std::string("http://www.inkscape.org/namespaces/inkscape")}}) {}
  virtual ~Document() override {}

  Document&
  ViewBox(double x_min, double y_min, double width, double height) {
    std::ostringstream stream;
    stream << x_min << ' ' << y_min << ' ' << width << ' ' << height;
    AddAttribute({"viewBox", stream.str()});

    return *this;
  }

  virtual std::string
  ToText() const override {
    std::ostringstream stream;
    stream << "<?xml version=\"1.0\"?>" << '\n';

    stream << GroupBase::ToText();

    return stream.str();
  }

  friend std::ostream&
  operator<<(std::ostream& stream, const Document& document) {
    return stream << document.ToText();
  }
};

} // namespace simple_svg
#endif // defined SIMPLE_SVG_WRITER_HPP