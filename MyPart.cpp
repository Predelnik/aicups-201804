#include "MyPart.h"

MyPart::MyPart(const json& data)
{
    pos.x = data["X"];
    pos.y = data["Y"];
    speed.x = data["SX"];
    speed.y = data["SY"];
    radius = data["R"];
    mass = data["M"];
    ttf = data.value("TTF", -1);
}
