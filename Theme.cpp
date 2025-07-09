#include "Theme.h"
#include <graphics.h>
#include <cmath>
#include <algorithm>

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
        if (borderColor != -1) {
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
            float ratio = (float)i / (float)(steps - 1);
            COLORREF currentColor = interpolateColor(startColor, endColor, ratio);
            
            setfillcolor(currentColor);
            int stripY = y + (i * height) / steps;
            int stripHeight = height / steps + 1;
            
            if (i == 0 || i == steps - 1) {
                // 顶部和底部使用圆角
                drawRoundedRect(x, stripY, width, stripHeight, radius, currentColor);
            } else {
                // 中间部分使用矩形
                solidrectangle(x, stripY, x + width, stripY + stripHeight);
            }
        }
    }
    
    void drawSoftShadowRect(int x, int y, int width, int height, int radius, COLORREF fillColor, int shadowOffset) {
        // 绘制阴影
        COLORREF shadowColor = RGB(0, 0, 0);
        setfillcolor(shadowColor);
        drawRoundedRect(x + shadowOffset, y + shadowOffset, width, height, radius, shadowColor);
        
        // 绘制主体
        drawRoundedRect(x, y, width, height, radius, fillColor);
    }
    
    void drawSoftCircle(int centerX, int centerY, int radius, COLORREF fillColor, COLORREF borderColor) {
        setfillcolor(fillColor);
        solidcircle(centerX, centerY, radius);
        
        if (borderColor != -1) {
            setlinecolor(borderColor);
            circle(centerX, centerY, radius);
        }
    }
    
    void drawSoftEllipse(int x, int y, int width, int height, COLORREF fillColor, COLORREF borderColor) {
        setfillcolor(fillColor);
        solidellipse(x, y, x + width, y + height);
        
        if (borderColor != -1) {
            setlinecolor(borderColor);
            ellipse(x, y, x + width, y + height);
        }
    }
}
