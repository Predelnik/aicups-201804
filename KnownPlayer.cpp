#include "KnownPlayer.h"
#include "GameConfig.h"
#include "MovingPoint.h"

KnownPlayer::KnownPlayer(const json &data) : Player(data) {
  speed.x = data["SX"];
  speed.y = data["SY"];
  ttf = data.value("TTF", -1);
}

KnownPlayer::KnownPlayer(const Player &player) {
  static_cast<Player &>(*this) = player;
}

MovingPoint KnownPlayer::as_moving_point() const { return {pos, speed}; }

bool is_visible(const std::vector<KnownPlayer> &parts, const Point &pos) {
  return std::any_of(parts.begin(), parts.end(), [&](const KnownPlayer &part) {
    return part.is_visible(static_cast<int>(parts.size()), pos);
  });
}
