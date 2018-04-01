#include "Matrix.h"
#include <cmath>

auto Matrix::rotation(double angle) -> Self
{
    return {std::cos(angle), -std::sin(angle), std::sin(angle), std::cos(angle)};
}
