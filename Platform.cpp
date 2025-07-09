#include "Platform.h"
#include "Theme.h"
#include <cmath>

Platform::Platform(float x, float y, float width, float height, PlatformType type)
    : x(x), y(y), width(width), height(height), type(type), animationTimer(0.0f),
    moveSpeed(50.0f), moveRange(100.0f), startX(x), moveDirection(1) {

    // 根据类型设置颜色
    switch (type) {
    case NORMAL:
        color = RGB(156, 175, 183);
        break;
    case MOVING:
        color = RGB(95, 158, 160);
        break;
    case BREAKABLE:
        color = RGB(180, 142, 120);
        break;
    }
}

void Platform::update(float deltaTime) {
    animationTimer += deltaTime;

    if (type == MOVING) {
        // 移动平台逻辑
        x += moveSpeed * moveDirection * deltaTime;

        if (x <= startX - moveRange || x >= startX + moveRange) {
            moveDirection *= -1;
        }
    }
}

void Platform::draw() const {
    drawWithOffset(0, 0);
}

void Platform::drawWithOffset(float offsetX, float offsetY) const {
    // 使用EasyX绘制极简风格的平台
    float drawX = x + offsetX;
    float drawY = y + offsetY;

    // 根据类型设置颜色
    COLORREF drawColor;

    switch (type) {
    case NORMAL:
        drawColor = RGB(200, 220, 240);
        break;
    case MOVING: {
        // 移动平台带脉动效果
        float pulse = 0.8f + 0.2f * std::sin(animationTimer * 3.0f);
        int brightness = (int)(200 * pulse);
        drawColor = RGB(brightness, brightness + 50, 255);
        break;
    }
    case BREAKABLE: {
        // 可破坏平台带闪烁效果
        float flicker = 0.6f + 0.4f * std::sin(animationTimer * 5.0f);
        int brightness = (int)(180 * flicker);
        drawColor = RGB(brightness + 40, brightness + 40, brightness);
        break;
    }
    }

    // 绘制阴影
    setfillcolor(RGB(50, 50, 50));
    solidrectangle((int)(drawX + 1), (int)(drawY + 1), (int)(drawX + width + 1), (int)(drawY + height + 1));

    // 绘制平台主体
    setfillcolor(drawColor);
    solidrectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));

    // 绘制边框高光
    setlinecolor(RGB(255, 255, 255));
    setlinestyle(PS_SOLID, 1);
    rectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));

    // 特殊类型的额外视觉提示
    if (type == MOVING) {
        // 移动方向指示器
        float arrowX = drawX + width / 2 + 10 * moveDirection;
        float arrowY = drawY - 5;

        setfillcolor(RGB(100, 150, 255));
        solidcircle((int)arrowX, (int)arrowY, 3);
    }
}