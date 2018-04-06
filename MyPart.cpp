#include "MyPart.h"
#include "GameConfig.h"

MyPart::MyPart(const json &data) : Player(data) {
  speed.x = data["SX"];
  speed.y = data["SY"];
  ttf = data.value("TTF", -1);
}

MovingPoint MyPart::next_moving_point(MovingPoint speed_position,
                                      const Point &acceleration, int ticks,
                                      const GameConfig &config) const {
  auto ms = max_speed(config);
  for (int i = 0; i < ticks; ++i) {
    speed_position.speed += (acceleration.normalized() * ms - speed_position.speed) *
                            config.inertia_factor / mass;
    if (speed_position.speed.squared_length() > pow(ms, 2))
      speed_position.speed = speed_position.speed.normalized() * ms;
    speed_position.position += speed_position.speed;
  }
  return speed_position;
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
