#pragma once
#include <array>
#include <cmath>

#include <vector>

class Matrix;

namespace model {
class World;
class Game;
enum class WeatherType;
enum class TerrainType;
} // namespace model

class Point {
private:
  using Self = Point;

public:
  double x;
  double y;

public:
  Point() : x(0.0), y(0.0) {}

  bool is_null() const;
  Point(double x_arg, double y_arg) : x(x_arg), y(y_arg) {}

  bool is_in_rect(const std::array<Point, 2> &rect) const;
  double distance_to(const Point &other) const;
  double length() const {
    return sqrt(pow(x, 2) + pow(y, 2));
    ;
  }
  Point normalized() const {
    double len = length();
    return {x / len, y / len};
  }

  double distance_to_line(const Point &a, const Point &b) const;

  friend Self operator+(const Self &lhs, const Self &rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
  }
  friend Self operator-(const Self &lhs, const Self &rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
  }

  Self &operator+=(const Self &other) {
    *this = *this + other;
    return *this;
  }

  Self operator-() const { return {-x, -y}; }

  Self operator*(double mult) const { return {x * mult, y * mult}; }
  Self operator/(double mult) const { return {x / mult, y / mult}; }

  Self operator*(const Matrix &m) const;

  Self &operator/=(double mult) {
    *this = *this / mult;
    return *this;
  }

  double squared_distance_to(const Point &other) const;
  double squared_length() const;
  double angle() const;
};

std::array<Point, 2> rect_around(const Point &pnt, double side);
inline std::array<Point, 2> expand(const std::array<Point, 2> &rect,
                                   double amount) {
  return {Point{rect[0].x - amount, rect[0].y - amount},
          Point{rect[1].x + amount, rect[1].y + amount}};
}

Point center(const std::vector<Point> &points);
Point weighted_center(const std::vector<std::pair<Point, double>> &points);
