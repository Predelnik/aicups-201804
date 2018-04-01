#include "Matrix.h"

auto Matrix::rotation(double angle) -> Self
{
    return {std::cos(angle), -std::sin(angle), std::sin(angle), std::cos(angle)};
}
