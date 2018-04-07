#include "Point.h"
#include <cmath>
#include <numeric>
#include "Matrix.h"

bool Point::is_null() const { return fabs(x) < 1e-5 && fabs(y) < 1e-5; }

bool Point::is_in_rect(const std::array<Point, 2> &rect) const {
  return x > rect[0].x - 1e-6 && y > rect[0].y - 1e-6 && x < rect[1].x + 1e-6 &&
         y < rect[1].y + 1e-6;
}

double Point::distance_to(const Point &other) const {
  return std::sqrt(squared_distance_to(other));
}

double Point::distance_to_line(const Point &a, const Point &b) const {
  return ((b.y - a.y) * x + (b.x - a.x) * y + b.x * a.y - b.y * a.x) /
         a.distance_to(b);
}

auto Point::operator*(const Matrix& m) const -> Self
{
    return {x * m.m[0] + y * m.m[2], x * m.m[1] + y * m.m[3]};
}

double Point::squared_distance_to(const Point &other) const {
  auto x_d = x - other.x;
  auto y_d = y - other.y;
  return x_d * x_d + y_d * y_d;
}

double Point::squared_length() const
{
    return pow (x, 2) + pow (y, 2);
}

double Point::angle()
{
    return std::atan2 (y, x);
}

std::array<Point, 2> rect_around(const Point &pnt, double side) {
  return {Point{pnt.x - side / 2., pnt.y - side / 2.},
          Point{pnt.x + side / 2., pnt.y + side / 2.}};
}

Point center(const std::vector<Point> &points) {
  return std::accumulate(points.begin(), points.end(), Point{}) /
         static_cast<double>(points.size());
}

Point weighted_center(const std::vector<std::pair<Point, double>> &points) {
  Point result;
  for (auto &p : points)
    result += std::get<Point>(p) * std::get<double>(p);
  result /= std::accumulate(points.begin(), points.end(), 0.0,
                            [](double sum, const auto &point) {
                              return sum += std::get<double>(point);
                            });
  return result;
}
