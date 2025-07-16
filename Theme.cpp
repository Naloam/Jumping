#include "Theme.h"
#include "Platform.h"
#include <graphics.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>

namespace DrawUtils {

    COLORREF interpolateColor(COLORREF color1, COLORREF color2, float ratio) {
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;

        int r1 = GetRValue(color1);
        int g1 = GetGValue(color1);
        int b1 = GetBValue(color1);

        int r2 = GetRValue(color2);
        int g2 = GetGValue(color2);
        int b2 = GetBValue(color2);

        int r = (int)(r1 + (r2 - r1) * ratio);
        int g = (int)(g1 + (g2 - g1) * ratio);
        int b = (int)(b1 + (b2 - b1) * ratio);

        return RGB(r, g, b);
    }

    COLORREF blendColor(COLORREF baseColor, COLORREF blendColor, float alpha) {
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;

        int r1 = GetRValue(baseColor);
        int g1 = GetGValue(baseColor);
        int b1 = GetBValue(baseColor);

        int r2 = GetRValue(blendColor);
        int g2 = GetGValue(blendColor);
        int b2 = GetBValue(blendColor);

        int r = (int)(r1 * (1.0f - alpha) + r2 * alpha);
        int g = (int)(g1 * (1.0f - alpha) + g2 * alpha);
        int b = (int)(b1 * (1.0f - alpha) + b2 * alpha);

        return RGB(r, g, b);
    }

    COLORREF adjustBrightness(COLORREF color, float factor) {
        int r = std::min(255, (int)(GetRValue(color) * factor));
        int g = std::min(255, (int)(GetGValue(color) * factor));
        int b = std::min(255, (int)(GetBValue(color) * factor));
        return RGB(r, g, b);
    }

    COLORREF addGlow(COLORREF baseColor, COLORREF glowColor, float intensity) {
        if (intensity <= 0.0f) return baseColor;
        if (intensity > 1.0f) intensity = 1.0f;

        int r1 = GetRValue(baseColor);
        int g1 = GetGValue(baseColor);
        int b1 = GetBValue(baseColor);

        int r2 = GetRValue(glowColor);
        int g2 = GetGValue(glowColor);
        int b2 = GetBValue(glowColor);

        int r = std::min(255, (int)(r1 + r2 * intensity));
        int g = std::min(255, (int)(g1 + g2 * intensity));
        int b = std::min(255, (int)(b1 + b2 * intensity));

        return RGB(r, g, b);
    }

    void drawRoundedRect(int x, int y, int width, int height, int radius, COLORREF fillColor, COLORREF borderColor) {
        // 简化版圆角矩形绘制
        setfillcolor(fillColor);

        // 绘制主体矩形
        solidrectangle(x + radius, y, x + width - radius, y + height);
        solidrectangle(x, y + radius, x + width, y + height - radius);

        // 绘制四个角的圆
        solidcircle(x + radius, y + radius, radius);
        solidcircle(x + width - radius, y + radius, radius);
        solidcircle(x + radius, y + height - radius, radius);
        solidcircle(x + width - radius, y + height - radius, radius);

        // 绘制边框
        if (borderColor != (COLORREF)-1) {
            setlinecolor(borderColor);
            setlinestyle(PS_SOLID, 1);

            // 绘制边框线条
            line(x + radius, y, x + width - radius, y);
            line(x + radius, y + height, x + width - radius, y + height);
            line(x, y + radius, x, y + height - radius);
            line(x + width, y + radius, x + width, y + height - radius);

            // 绘制圆角边框
            circle(x + radius, y + radius, radius);
            circle(x + width - radius, y + radius, radius);
            circle(x + radius, y + height - radius, radius);
            circle(x + width - radius, y + height - radius, radius);
        }
    }

    void drawGradientRoundedRect(int x, int y, int width, int height, int radius, COLORREF startColor, COLORREF endColor) {
        // 简化版渐变，使用水平条纹模拟
        int steps = height / 2;
        if (steps < 1) steps = 1;

        for (int i = 0; i < steps; i++) {
            float ratio = (steps > 1) ? (float)i / (float)(steps - 1) : 0.0f;
            COLORREF currentColor = interpolateColor(startColor, endColor, ratio);

            setfillcolor(currentColor);
            int stripY = y + (i * height) / steps;
            int stripHeight = height / steps + 1;

            if (i == 0 || i == steps - 1) {
                // 顶部和底部使用圆角
                drawRoundedRect(x, stripY, width, stripHeight, radius, currentColor);
            }
            else {
                // 中间部分使用矩形
                solidrectangle(x, stripY, x + width, stripY + stripHeight);
            }
        }
    }

    void drawSoftShadowRect(int x, int y, int width, int height, int radius, COLORREF fillColor, int shadowOffset) {
        // 绘制阴影
        COLORREF shadowColor = blendColor(RGB(255, 255, 255), RGB(0, 0, 0), 0.3f);
        setfillcolor(shadowColor);
        drawRoundedRect(x + shadowOffset, y + shadowOffset, width, height, radius, shadowColor);

        // 绘制主体
        drawRoundedRect(x, y, width, height, radius, fillColor);
    }

    void drawSoftCircle(int centerX, int centerY, int radius, COLORREF fillColor, COLORREF borderColor) {
        setfillcolor(fillColor);
        solidcircle(centerX, centerY, radius);

        if (borderColor != (COLORREF)-1) {
            setlinecolor(borderColor);
            circle(centerX, centerY, radius);
        }
    }

    void drawSoftEllipse(int x, int y, int width, int height, COLORREF fillColor, COLORREF borderColor) {
        setfillcolor(fillColor);
        solidellipse(x, y, x + width, y + height);

        if (borderColor != (COLORREF)-1) {
            setlinecolor(borderColor);
            ellipse(x, y, x + width, y + height);
        }
    }

    void drawGlowCircle(int centerX, int centerY, int radius, COLORREF glowColor, float intensity) {
        int glowRadius = (int)(radius * (1.0f + intensity));

        // 绘制多层光晕
        for (int i = glowRadius; i >= radius; i--) {
            if (glowRadius > radius) {
                float alpha = (float)(glowRadius - i) / (float)(glowRadius - radius) * intensity;
                COLORREF currentColor = blendColor(RGB(255, 255, 255), glowColor, alpha);
                setfillcolor(currentColor);
                solidcircle(centerX, centerY, i);
            }
        }
    }

    void drawGlowRect(int x, int y, int width, int height, COLORREF glowColor, float intensity) {
        int glowSize = (int)(10 * intensity);

        for (int i = glowSize; i >= 0; i--) {
            float alpha = (glowSize > 0) ? (float)(glowSize - i) / (float)glowSize * intensity : 0.0f;
            COLORREF currentColor = blendColor(RGB(255, 255, 255), glowColor, alpha);
            setfillcolor(currentColor);
            solidrectangle(x - i, y - i, x + width + i, y + height + i);
        }
    }

    void drawPulsingCircle(int centerX, int centerY, int baseRadius, float pulseAmount, float time, COLORREF color) {
        float pulse = std::sin(time * 3.14159f * 2.0f) * 0.5f + 0.5f;
        int currentRadius = (int)(baseRadius + pulseAmount * pulse);

        float alpha = 1.0f - pulse * 0.3f;
        COLORREF currentColor = adjustBrightness(color, alpha);

        setfillcolor(currentColor);
        solidcircle(centerX, centerY, currentRadius);
    }

    void drawParticle(float x, float y, float size, COLORREF color, float alpha) {
        COLORREF particleColor = blendColor(RGB(255, 255, 255), color, alpha);
        setfillcolor(particleColor);
        solidcircle((int)x, (int)y, (int)size);
    }

    void drawSparkle(float x, float y, float size, COLORREF color, float rotation) {
        setlinecolor(color);
        setlinestyle(PS_SOLID, 2);

        // 绘制十字形星星
        float halfSize = size * 0.5f;
        line((int)(x - halfSize), (int)y, (int)(x + halfSize), (int)y);
        line((int)x, (int)(y - halfSize), (int)x, (int)(y + halfSize));

        // 绘制对角线
        float diagonalSize = size * 0.35f;
        line((int)(x - diagonalSize), (int)(y - diagonalSize),
            (int)(x + diagonalSize), (int)(y + diagonalSize));
        line((int)(x - diagonalSize), (int)(y + diagonalSize),
            (int)(x + diagonalSize), (int)(y - diagonalSize));
    }

    void drawComboText(int x, int y, int combo, COLORREF color) {
        settextcolor(color);
        int fontSize = 24 + std::min(combo * 2, 20);  // 限制最大字体大小
        settextstyle(fontSize, 0, L"Arial");

        std::wstring comboText = L"COMBO x" + std::to_wstring(combo);
        outtextxy(x, y, comboText.c_str());
    }

    void drawSpeedEffect(float x, float y, float width, float height, float intensity) {
        COLORREF speedColor = Theme::SPEED_GLOW;

        // 绘制速度光晕
        for (int i = 0; i < 3; i++) {
            float alpha = intensity * (0.5f - i * 0.1f);
            COLORREF currentColor = blendColor(RGB(255, 255, 255), speedColor, alpha);
            setfillcolor(currentColor);

            int offset = i * 3;
            solidrectangle((int)(x - offset), (int)(y - offset),
                (int)(x + width + offset), (int)(y + height + offset));
        }
    }

    void drawShieldEffect(float x, float y, float radius, float intensity) {
        COLORREF shieldColor = Theme::SHIELD_GLOW;

        // 绘制护盾光圈
        for (int i = 0; i < 5; i++) {
            float currentRadius = radius + i * 5;
            float alpha = intensity * (0.6f - i * 0.1f);
            COLORREF currentColor = blendColor(RGB(255, 255, 255), shieldColor, alpha);

            setlinecolor(currentColor);
            setlinestyle(PS_SOLID, 2);
            circle((int)x, (int)y, (int)currentRadius);
        }
    }

    void drawItemGlow(float x, float y, float size, COLORREF glowColor, float time) {
        float pulse = std::sin(time * 4.0f) * 0.3f + 0.7f;
        drawGlowCircle((int)x, (int)y, (int)(size * pulse), glowColor, pulse);
    }

    void drawSpringCompression(float x, float y, float width, float height, float compression) {
        // 绘制压缩的弹簧效果
        float compressedHeight = height * (1.0f - compression * 0.5f);
        float compressionY = y + (height - compressedHeight);

        COLORREF springColor = interpolateColor(Theme::PLATFORM_SPRING,
            Theme::PLATFORM_SPRING_COMPRESSED, compression);

        setfillcolor(springColor);
        solidrectangle((int)x, (int)compressionY, (int)(x + width), (int)(y + height));

        // 绘制弹簧线圈效果
        if (compression > 0.1f) {
            setlinecolor(Theme::PLATFORM_SPRING_ACTIVE);
            setlinestyle(PS_SOLID, 2);

            for (int i = 0; i < 3; i++) {
                float lineY = compressionY + i * (compressedHeight / 3);
                line((int)x, (int)lineY, (int)(x + width), (int)lineY);
            }
        }
    }

    void drawBreakEffect(float x, float y, float width, float height, float breakProgress) {
        COLORREF breakColor = interpolateColor(Theme::PLATFORM_BREAKABLE,
            Theme::PLATFORM_BREAKABLE_BREAKING, breakProgress);

        setfillcolor(breakColor);
        solidrectangle((int)x, (int)y, (int)(x + width), (int)(y + height));

        // 绘制裂纹效果
        if (breakProgress > 0.3f) {
            setlinecolor(Theme::PLATFORM_BREAKABLE_BREAKING);
            setlinestyle(PS_SOLID, 1);

            // 随机裂纹
            for (int i = 0; i < (int)(breakProgress * 5); i++) {
                int crackX = (int)(x + width * 0.2f * (i + 1));
                line(crackX, (int)y, crackX, (int)(y + height));
            }
        }
    }

    void drawPlatformPreview(float x, float y, float width, float height, COLORREF previewColor, float alpha) {
        COLORREF previewDrawColor = blendColor(RGB(255, 255, 255), previewColor, alpha);

        setlinecolor(previewDrawColor);
        setlinestyle(PS_DOT, 1);
        rectangle((int)x, (int)y, (int)(x + width), (int)(y + height));
    }

    void drawDangerZone(float y, float intensity) {
        COLORREF dangerColor = blendColor(RGB(255, 255, 255), Theme::DANGER_ZONE, intensity);

        setlinecolor(dangerColor);
        setlinestyle(PS_SOLID, 3);
        line(0, (int)y, 800, (int)y); // 假设窗口宽度为800

        // 添加危险区域文字效果
        if (intensity > 0.5f) {
            settextcolor(dangerColor);
            settextstyle(16, 0, L"Arial");
            outtextxy(350, (int)y - 25, L"DANGER ZONE");
        }
    }

    // 二段跳特效
    void drawDoubleJumpEffect(float x, float y, float intensity) {
        COLORREF doubleJumpColor = Theme::ITEM_DOUBLE_JUMP;

        // 绘制双层跳跃轨迹
        for (int i = 0; i < 2; i++) {
            float offset = i * 8.0f;
            float alpha = intensity * (1.0f - i * 0.3f);
            COLORREF currentColor = blendColor(RGB(255, 255, 255), doubleJumpColor, alpha);

            setlinecolor(currentColor);
            setlinestyle(PS_SOLID, 3);

            // 绘制向上的弧形轨迹
            for (int j = 0; j < 10; j++) {
                float startX = x - 15 + j * 3;
                float startY = y + 10 - j * 2;
                float endX = x - 15 + (j + 1) * 3;
                float endY = y + 10 - (j + 1) * 2 - offset;

                line((int)startX, (int)startY, (int)endX, (int)endY);
            }
        }
    }

    // 新增：时间减缓特效
    void drawSlowTimeEffect(float x, float y, float intensity) {
        COLORREF slowTimeColor = Theme::ITEM_SLOW_TIME;

        // 绘制时间波纹
        for (int i = 0; i < 3; i++) {
            float radius = 20 + i * 10;
            float alpha = intensity * (0.6f - i * 0.2f);
            COLORREF currentColor = blendColor(RGB(255, 255, 255), slowTimeColor, alpha);

            setlinecolor(currentColor);
            setlinestyle(PS_SOLID, 2);
            circle((int)x, (int)y, (int)radius);
        }

        // 绘制中心时钟图标
        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 2);
        circle((int)x, (int)y, 8);
        line((int)x, (int)y, (int)(x + 6), (int)(y - 3)); // 时针
        line((int)x, (int)y, (int)(x + 3), (int)(y - 6)); // 分针
    }

    // 新增：磁场特效
    void drawMagneticFieldEffect(float x, float y, float radius, float intensity) {
        COLORREF magneticColor = Theme::ITEM_MAGNETIC_FIELD;

        // 绘制磁场线
        for (int i = 0; i < 8; i++) {
            float angle = i * 3.14159f / 4;
            float startRadius = radius * 0.3f;
            float endRadius = radius;

            float startX = x + startRadius * cos(angle);
            float startY = y + startRadius * sin(angle);
            float endX = x + endRadius * cos(angle);
            float endY = y + endRadius * sin(angle);

            COLORREF currentColor = blendColor(RGB(255, 255, 255), magneticColor, intensity);
            setlinecolor(currentColor);
            setlinestyle(PS_SOLID, 2);
            line((int)startX, (int)startY, (int)endX, (int)endY);
        }

        // 绘制中心磁铁图标
        setfillcolor(magneticColor);
        solidrectangle((int)(x - 6), (int)(y - 8), (int)(x + 6), (int)(y + 8));

        // 绘制N和S标记
        settextcolor(RGB(255, 255, 255));
        settextstyle(10, 0, L"Arial");
        outtextxy((int)(x - 3), (int)(y - 6), L"N");
        outtextxy((int)(x - 3), (int)(y + 2), L"S");
    }

    // 新增：冻结障碍物特效
    void drawFreezeObstaclesEffect(float x, float y, float intensity) {
        COLORREF freezeColor = Theme::ITEM_FREEZE_OBSTACLES;

        // 绘制冰晶效果
        for (int i = 0; i < 6; i++) {
            float angle = i * 3.14159f / 3;
            float length = 15 * intensity;

            float endX = x + length * cos(angle);
            float endY = y + length * sin(angle);

            setlinecolor(freezeColor);
            setlinestyle(PS_SOLID, 3);
            line((int)x, (int)y, (int)endX, (int)endY);

            // 绘制分支
            float branchLength = length * 0.5f;
            float branchAngle1 = angle + 0.5f;
            float branchAngle2 = angle - 0.5f;

            line((int)endX, (int)endY,
                (int)(endX + branchLength * cos(branchAngle1)),
                (int)(endY + branchLength * sin(branchAngle1)));
            line((int)endX, (int)endY,
                (int)(endX + branchLength * cos(branchAngle2)),
                (int)(endY + branchLength * sin(branchAngle2)));
        }
    }

    // 新增：生命值恢复特效
    void drawHealthBoostEffect(float x, float y, float intensity) {
        COLORREF healthColor = Theme::ITEM_HEALTH_BOOST;

        // 绘制生命十字光晕
        for (int i = 0; i < 3; i++) {
            float size = 8 + i * 4;
            float alpha = intensity * (0.8f - i * 0.2f);
            COLORREF currentColor = blendColor(RGB(255, 255, 255), healthColor, alpha);

            setlinecolor(currentColor);
            setlinestyle(PS_SOLID, 4 - i);

            // 绘制十字
            line((int)(x - size), (int)y, (int)(x + size), (int)y);
            line((int)x, (int)(y - size), (int)x, (int)(y + size));
        }
    }

    // 新增：无敌特效
    void drawInvincibilityEffect(float x, float y, float intensity) {
        COLORREF invincibilityColor = Theme::ITEM_INVINCIBILITY;

        // 绘制星星光环
        for (int ring = 0; ring < 3; ring++) {
            float radius = 20 + ring * 10;
            float alpha = intensity * (0.7f - ring * 0.2f);
            COLORREF currentColor = blendColor(RGB(255, 255, 255), invincibilityColor, alpha);

            setlinecolor(currentColor);
            setlinestyle(PS_SOLID, 2);

            // 绘制星星形状
            for (int i = 0; i < 8; i++) {
                float angle = i * 3.14159f / 4;
                float starRadius = (i % 2 == 0) ? radius : radius * 0.6f;

                float pointX = x + starRadius * cos(angle);
                float pointY = y + starRadius * sin(angle);

                if (i == 0) {
                    line((int)x, (int)y, (int)pointX, (int)pointY);
                }
                else {
                    float prevAngle = (i - 1) * 3.14159f / 4;
                    float prevRadius = ((i - 1) % 2 == 0) ? radius : radius * 0.6f;
                    float prevX = x + prevRadius * cos(prevAngle);
                    float prevY = y + prevRadius * sin(prevAngle);

                    line((int)prevX, (int)prevY, (int)pointX, (int)pointY);
                }
            }
        }
    }

    // 更新getItemColor函数以支持新道具
    COLORREF getItemColor(ItemType type, float animationTime) {
        float pulse = std::sin(animationTime * 3.0f) * 0.2f + 0.8f;

        switch (type) {
        case SPEED_BOOST:
            return adjustBrightness(Theme::ITEM_SPEED, pulse);
        case SHIELD:
            return adjustBrightness(Theme::ITEM_SHIELD, pulse);
        case DOUBLE_JUMP:
            return adjustBrightness(Theme::ITEM_DOUBLE_JUMP, pulse);
        case SLOW_TIME:
            return adjustBrightness(Theme::ITEM_SLOW_TIME, pulse);
        case MAGNETIC_FIELD:
            return adjustBrightness(Theme::ITEM_MAGNETIC_FIELD, pulse);
        case FREEZE_OBSTACLES:
            return adjustBrightness(Theme::ITEM_FREEZE_OBSTACLES, pulse);
        case HEALTH_BOOST:
            return adjustBrightness(Theme::ITEM_HEALTH_BOOST, pulse);
        case INVINCIBILITY:
            return adjustBrightness(Theme::ITEM_INVINCIBILITY, pulse);
        case COIN:
            return adjustBrightness(Theme::ITEM_COIN, pulse);
        case NONE:
        default:
            return RGB(255, 255, 255);
        }
    }

    COLORREF getComboColor(int comboCount) {
        if (comboCount < 5) return Theme::COMBO_LOW;
        else if (comboCount < 10) return Theme::COMBO_MEDIUM;
        else if (comboCount < 20) return Theme::COMBO_HIGH;
        else return Theme::COMBO_EXTREME;
    }

    COLORREF getPlatformColor(PlatformType type, float animationTime, bool isActive) {
        switch (type) {
        case NORMAL:
            return isActive ? Theme::PLATFORM_NORMAL_HIGHLIGHT : Theme::PLATFORM_NORMAL;
        case MOVING:
            return isActive ? Theme::PLATFORM_MOVING_TRAIL : Theme::PLATFORM_MOVING;
        case BREAKABLE:
            return isActive ? Theme::PLATFORM_BREAKABLE_WARNING : Theme::PLATFORM_BREAKABLE;
        case SPRING:
            return isActive ? Theme::PLATFORM_SPRING_ACTIVE : Theme::PLATFORM_SPRING;
        default:
            return Theme::PLATFORM_NORMAL;
        }
    }

    // 绘制透明矩形
    void drawTransparentRect(int x, int y, int width, int height, COLORREF color, float alpha) {
        COLORREF transparentColor = blendColor(RGB(255, 255, 255), color, alpha);
        setfillcolor(transparentColor);
        solidrectangle(x, y, x + width, y + height);
    }

    // 绘制多边形光晕
    void drawPolygonGlow(POINT* points, int count, COLORREF glowColor, float intensity) {
        for (int i = 0; i < 3; i++) {
            float alpha = intensity * (0.4f - i * 0.1f);
            COLORREF currentColor = blendColor(RGB(255, 255, 255), glowColor, alpha);
            setlinecolor(currentColor);
            setlinestyle(PS_SOLID, 2 + i);
            polygon(points, count);
        }
    }
}

namespace AnimationUtils {

    float easeInOut(float t) {
        return t * t * (3.0f - 2.0f * t);
    }

    float easeIn(float t) {
        return t * t;
    }

    float easeOut(float t) {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    float bounce(float t) {
        if (t < 0.5f) {
            return 2.0f * t * t;
        }
        else {
            return 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
        }
    }

    float pulse(float time, float frequency) {
        return std::sin(time * frequency * 3.14159f * 2.0f) * 0.5f + 0.5f;
    }

    float wave(float time, float frequency, float amplitude) {
        return std::sin(time * frequency * 3.14159f * 2.0f) * amplitude;
    }

    std::pair<float, float> shake(float intensity, float time) {
        float shakeX = (std::sin(time * 50.0f) + std::sin(time * 73.0f)) * intensity;
        float shakeY = (std::cos(time * 47.0f) + std::cos(time * 69.0f)) * intensity;
        return std::make_pair(shakeX, shakeY);
    }

    COLORREF colorPulse(COLORREF baseColor, COLORREF accentColor, float time, float frequency) {
        float pulse = AnimationUtils::pulse(time, frequency);
        return DrawUtils::interpolateColor(baseColor, accentColor, pulse);
    }

    COLORREF colorFlash(COLORREF baseColor, COLORREF flashColor, float intensity) {
        return DrawUtils::blendColor(baseColor, flashColor, intensity);
    }
}