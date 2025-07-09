#include "Player.h"
#include "Theme.h"
#include <graphics.h>
#include <windows.h>
#include <cmath>

// 静态常量定义
const float Player::GRAVITY = 500.0f;
const float Player::JUMP_SPEED = -300.0f;
const float Player::MOVE_SPEED = 200.0f;
const float Player::FRICTION = 0.8f;
const float Player::MAX_FALL_SPEED = 400.0f;

Player::Player(float x, float y)
    : x(x), y(y), vx(0), vy(0), width(30), height(30),
    onGround(false), wasOnGround(false), jumpCount(0), maxJumps(2),
    currentColor(RGB(108, 142, 165)), pulseTimer(0.0f) {
}

void Player::update(float deltaTime) {
    pulseTimer += deltaTime;

    // 重力
    if (!onGround) {
        vy += GRAVITY * deltaTime;
        if (vy > MAX_FALL_SPEED) {
            vy = MAX_FALL_SPEED;
        }
    }

    // 摩擦力
    vx *= FRICTION;

    // 更新位置
    x += vx * deltaTime;
    y += vy * deltaTime;

    // 重置地面状态
    onGround = false;
}

void Player::draw() {
    // 脉动效果
    float pulse = 0.9f + 0.1f * std::sin(pulseTimer * 4.0f);
    int brightness = (int)(108 * pulse);
    COLORREF drawColor = RGB(brightness, (int)(142 * pulse), (int)(165 * pulse));

    // 绘制阴影
    setfillcolor(RGB(50, 50, 50));
    solidrectangle((int)(x + 2), (int)(y + 2), (int)(x + width + 2), (int)(y + height + 2));

    // 绘制玩家主体
    setfillcolor(drawColor);
    solidrectangle((int)x, (int)y, (int)(x + width), (int)(y + height));

    // 绘制高光
    setfillcolor(RGB(255, 255, 255));
    solidrectangle((int)(x + 2), (int)(y + 2), (int)(x + width - 2), (int)(y + 8));

    // 绘制边框
    setlinecolor(RGB(255, 255, 255));
    setlinestyle(PS_SOLID, 2);
    rectangle((int)x, (int)y, (int)(x + width), (int)(y + height));
}

void Player::handleInput() {
    // 移动输入
    if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) {
        moveLeft();
    }
    if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000) {
        moveRight();
    }

    // 跳跃输入
    static bool spacePressed = false;
    bool spaceDown = GetAsyncKeyState(VK_SPACE) & 0x8000;

    if (spaceDown && !spacePressed) {
        jump();
    }
    spacePressed = spaceDown;
}

void Player::jump() {
    if (onGround || jumpCount < maxJumps) {
        vy = JUMP_SPEED;
        onGround = false;
        jumpCount++;
    }
}

void Player::reset() {
    x = 100;
    y = 400;
    vx = 0;
    vy = 0;
    onGround = false;
    wasOnGround = false;
    jumpCount = 0;
    pulseTimer = 0.0f;
}

void Player::checkBounds(int windowWidth, int windowHeight) {
    // 左右边界
    if (x < 0) {
        x = 0;
        vx = 0;
    }
    if (x + width > windowWidth) {
        x = windowWidth - width;
        vx = 0;
    }

    // 下边界（游戏结束检查在 Game 类中处理）
    // 上边界
    if (y < -height) {
        y = -height;
        vy = 0;
    }
}

void Player::drawWithOffset(float offsetX, float offsetY) {
    // 脉动效果
    float pulse = 0.9f + 0.1f * std::sin(pulseTimer * 4.0f);
    int brightness = (int)(108 * pulse);
    COLORREF drawColor = RGB(brightness, (int)(142 * pulse), (int)(165 * pulse));

    float drawX = x + offsetX;
    float drawY = y + offsetY;

    // 绘制阴影
    setfillcolor(RGB(50, 50, 50));
    solidrectangle((int)(drawX + 2), (int)(drawY + 2), (int)(drawX + width + 2), (int)(drawY + height + 2));

    // 绘制玩家主体
    setfillcolor(drawColor);
    solidrectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));

    // 绘制高光
    setfillcolor(RGB(255, 255, 255));
    solidrectangle((int)(drawX + 2), (int)(drawY + 2), (int)(drawX + width - 2), (int)(drawY + 8));

    // 绘制边框
    setlinecolor(RGB(255, 255, 255));
    setlinestyle(PS_SOLID, 2);
    rectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));
}