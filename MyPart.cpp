#include "MyPart.h"

MyPart::MyPart(const json& data)
{
    center.x = data["X"];
    center.y = data["Y"];
    speed.x = data["SX"];
    speed.y = data["SY"];
    radius = data["R"];
    mass = data["M"];
    ttf = data.value("TTF", -1);
}
