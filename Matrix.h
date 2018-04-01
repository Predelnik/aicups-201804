#pragma once

#include <array>

class Point;

class Matrix
{
private:
    using Self = Matrix;
    Matrix (double m11, double m12, double m21, double m22) : m {m11, m12, m21, m22} {}

public:
    static Self rotation(double angle);

private:
    std::array<double, 4> m;
    friend class Point;
};
