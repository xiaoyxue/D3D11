#pragma once
#include <chrono>
#include "common/core.h"
#include "../mathematics/lingal.h"
#include <windows.h>

namespace D3D11 {
  enum class DrawCommandType {
    FULLSCREEN = 0,
    BOX = 1,
    CIRCLE = 2,
    CLEAR = 3,
    CURSOR = 4,
  };

  class DrawCommand {
  public:
    explicit DrawCommand(DrawCommandType type) : type_(type) {}
    virtual ~DrawCommand() = default;

    DrawCommandType GetType() const { return type_; }

  private:
    DrawCommandType type_;
  };

  class DrawFullScreenCommand : public DrawCommand {
  public:
    DrawFullScreenCommand(int w, int h) : DrawCommand(DrawCommandType::FULLSCREEN), width_(w), height_(h) {}
    ~DrawFullScreenCommand() override = default;

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }

  private:
    int width_;
    int height_;
  };

  class DrawBoxCommand : public DrawCommand {
  public:
    DrawBoxCommand() = delete;
    DrawBoxCommand(HWND hwnd): hwnd_(hwnd), DrawCommand(DrawCommandType::BOX) {
      RECT rc;
      GetWindowRect(hwnd, &rc);
      x_ = rc.left;
      y_ = rc.top;
      width_ = rc.right - rc.left;
      height_ = rc.bottom - rc.top;
    }

    ~DrawBoxCommand() override = default;

    Vec2i GetPosition() const { return Vec2i(x_, y_); }

    Vec2i GetSize() const { return Vec2i(width_, height_); }

  private:
    HWND hwnd_;
    int width_;
    int height_;
    int x_;
    int y_;
  };

  class DrawCircleCommand : public DrawCommand {
  public:
    DrawCircleCommand(float x, float y, float radius)
        : DrawCommand(DrawCommandType::CIRCLE), x_(x), y_(y), radius_(radius), animation_start_time_(std::chrono::steady_clock::now()) {}
    ~DrawCircleCommand() override = default;

    float GetX() const { return x_; }
    float GetY() const { return y_; }
    float GetRadius() const { return radius_; }
    std::chrono::steady_clock::time_point GetAnimationStartTime() const { return animation_start_time_; }

  private:
    float x_;
    float y_;
    float radius_;
    std::chrono::steady_clock::time_point animation_start_time_;
  };

  class DrawCursorCommand : public DrawCommand {
  public:
    DrawCursorCommand(float x, float y, float rotate_ = 45.f / 180.0 * PI, float displacementMagnitude = 100.f)
      : DrawCommand(DrawCommandType::CURSOR), x_(x), y_(y), rotate_(rotate_),
      animation_start_time_(std::chrono::steady_clock::now()),
      displacementMagnitude_(displacementMagnitude)
    {

    }
    float GetX() const { return x_; }
    float GetY() const { return y_; }
    float GetRotate() const { return rotate_; }
    float GetDisplacementMagnitude() const { return displacementMagnitude_; }
    std::chrono::steady_clock::time_point GetAnimationStartTime() const { return animation_start_time_; }
  private:
    float x_;
    float y_;
    float rotate_;
    float displacementMagnitude_;
    std::chrono::steady_clock::time_point animation_start_time_;
  };

} // namespace D3D11
