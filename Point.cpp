#include "point.h"
#include <cmath>

bool Point::is_null() const { return fabs(x) < 1e-5 && fabs(y) < 1e-5; }

bool Point::is_in_rect(const std::array<Point, 2> &rect) const {
  return x > rect[0].x - 1e-6 && y > rect[0].y - 1e-6 && x < rect[1].x + 1e-6 &&
         y < rect[1].y + 1e-6;
}

double Point::distance_to(const Point &other) const {
  return std::sqrt(squared_distance_to(other));
}

double Point::squared_distance_to(const Point &other) const {
  auto x_d = x - other.x;
  auto y_d = y - other.y;
  return x_d * x_d + y_d * y_d;
}

std::array<Point, 2> rect_around(const Point &pnt, double side) {
  return {Point{pnt.x - side / 2., pnt.y - side / 2.},
          Point{pnt.x + side / 2., pnt.y + side / 2.}};
}
