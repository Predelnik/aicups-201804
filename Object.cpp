#include "Object.h"
#include "Const.h"
#include "GameConfig.h"
#include "GameHelpers.h"

ObjectBase::ObjectBase(const json &data) {
  pos.x = data["X"];
  pos.y = data["Y"];
}

Virus::Virus(const json &data) : ObjectBase(data) { mass = data["M"]; }

double Player::max_speed(const GameConfig &config) const {
  return ::max_speed(mass, config);
}

Player::Player(const json &data) : ObjectBase(data) {
  id = data["Id"];
  mass = data["M"];
  radius = data["R"];
}

Object to_object(const json &data) {
  switch (data.value("T", "").front()) {
  case 'P':
    return Player{data};
  case 'V':
    return Virus{data};
  case 'E':
    return Ejection{data};
  case 'F':
    return Food{data};
  default:
    break;
  }
  return {};
}

double Player::visibility_radius(int fragment_cnt) const {
  if (fragment_cnt == 1)
    return radius * constant::visibiliy_coeff;

  return constant::fragments_visibiliy_coeff * radius * sqrt(fragment_cnt);
}
