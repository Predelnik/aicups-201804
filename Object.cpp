#include "Object.h"
#include "Const.h"
#include "GameConfig.h"

ObjectBase::ObjectBase(const json &data) {
  pos.x = data["X"];
  pos.y = data["Y"];
}

Virus::Virus(const json &data) : ObjectBase(data) { mass = data["M"]; }

bool Player::can_eat(double opponent_mass) const {
  return mass >= opponent_mass * constant::eating_mass_coeff;
}

double Player::max_speed(const GameConfig &config) const {
  return config.speed_factor / sqrt(mass);
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
