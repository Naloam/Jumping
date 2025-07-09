#pragma once
#include <graphics.h>
#include <windows.h>

class Player {
private:
    float x, y;
    float vx, vy;
    float width, height;
    bool onGround;
    bool wasOnGround;
    int jumpCount;
    int maxJumps;

    // 风格相关
    COLORREF currentColor;
    float pulseTimer;

    // 物理常量
    static const float GRAVITY;
    static const float JUMP_SPEED;
    static const float MOVE_SPEED;
    static const float FRICTION;
    static const float MAX_FALL_SPEED;

public:
    Player(float x = 100, float y = 100);

    void update(float deltaTime);
    void draw();
    void drawWithOffset(float offsetX, float offsetY);
    void handleInput();

    // 位置和碰撞
    float getX() const { return x; }
    float getY() const { return y; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }

    void setPosition(float newX, float newY) { x = newX; y = newY; }
    void setOnGround(bool grounded) {
        wasOnGround = onGround;
        onGround = grounded;
        if (onGround && !wasOnGround) {
            jumpCount = 0;
        }
    }

    bool isOnGround() const { return onGround; }

    // 移动和跳跃
    void jump();
    void moveLeft() { vx -= MOVE_SPEED; }
    void moveRight() { vx += MOVE_SPEED; }

    // 重置
    void reset();
    void checkBounds(int windowWidth, int windowHeight);
};