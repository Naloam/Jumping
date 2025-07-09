#pragma once
#include <graphics.h>
#include <windows.h>

enum PlatformType {
    NORMAL,
    MOVING,
    BREAKABLE
};

class Platform {
private:
    float x, y;
    float width, height;
    PlatformType type;
    COLORREF color;
    float animationTimer;

    // 移动平台相关
    float moveSpeed;
    float moveRange;
    float startX;
    int moveDirection;

public:
    Platform(float x, float y, float width = 100, float height = 20, PlatformType type = NORMAL);

    void update(float deltaTime);
    void draw() const;
    void drawWithOffset(float offsetX, float offsetY) const;

    // Y轴移动方法（用于世界上升）
    void moveY(float deltaY) { y += deltaY; }

    // 访问器
    float getX() const { return x; }
    float getY() const { return y; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }
    PlatformType getType() const { return type; }
};