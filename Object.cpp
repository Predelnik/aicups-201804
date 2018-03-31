#include "Object.h"
#include "Const.h"

ObjectBase::ObjectBase(const json &data) {
  pos.x = data["X"];
  pos.y = data["Y"];
}

Virus::Virus(const json &data) : ObjectBase(data) { mass = data["M"]; }

bool Player::is_dangerous(double my_mass) const {
  return mass >= my_mass * constant::eating_mass_coeff;
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
