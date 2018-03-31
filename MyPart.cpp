#include "MyPart.h"

MyPart::MyPart(const json &data) {
  center.x = data["X"];
  center.y = data["Y"];
  speed.x = data["SX"];
  speed.y = data["SY"];
  radius = data["R"];
  mass = data["M"];
  ttf = data.value("TTF", -1);
}

double MyPart::visibility_radius(int fragment_cnt) const {
  if (fragment_cnt == 1)
    return radius * 4.0;

  return 2.5 * radius * sqrt(fragment_cnt);
}

bool is_visible(const std::vector<MyPart> &parts, const Point &pos) {
  return std::any_of(parts.begin(), parts.end(), [&](const MyPart &part) {
    return part.is_visible(static_cast<int>(parts.size()), pos);
  });
}
