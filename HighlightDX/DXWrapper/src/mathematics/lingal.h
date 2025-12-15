#pragma once

template<typename T>
struct Vector2{
    T x;
    T y;

    Vector2() : x(0), y(0) {}
    Vector2(T x_, T y_) : x(x_), y(y_) {}

    T operator[](size_t index) const {
        return index == 0 ? x : y;
    }
};

using Vec2f = Vector2<float>;
using Vec2i = Vector2<int>;
using Vec2 = Vec2f;
