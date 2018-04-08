#include "MyPart.h"
#include "GameConfig.h"
#include "MovingPoint.h"

MyPart::MyPart(const json &data) : Player(data) {
  speed.x = data["SX"];
  speed.y = data["SY"];
  ttf = data.value("TTF", -1);
}

MovingPoint MyPart::as_moving_point() const { return {pos, speed}; }

bool is_visible(const std::vector<MyPart> &parts, const Point &pos) {
  return std::any_of(parts.begin(), parts.end(), [&](const MyPart &part) {
    return part.is_visible(static_cast<int>(parts.size()), pos);
  });
}
