#pragma once
#include "lingal.h"

class Transform {
public:
  static Vec2 ScreenToNDC(int x, int y, int width, int height) {
    return Vec2(float(x) - float(width) / 2.0f, float(y) - float(height) / 2.0f);
  }
};
