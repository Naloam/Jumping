#pragma once
#include <graphics.h>

// 极简冷淡风格颜色主题
namespace Theme {
    // 基础色调 - 冷淡灰蓝色系
    const COLORREF BACKGROUND = RGB(245, 248, 250);      // 浅灰蓝背景
    const COLORREF SURFACE = RGB(234, 241, 245);         // 表面色
    const COLORREF BORDER = RGB(200, 215, 225);          // 边界色
    
    // 主要元素色彩
    const COLORREF PRIMARY = RGB(108, 142, 165);         // 主色调 - 冷蓝灰
    const COLORREF PRIMARY_LIGHT = RGB(134, 167, 189);   // 主色调浅色
    const COLORREF PRIMARY_DARK = RGB(82, 117, 140);     // 主色调深色
    
    // 辅助色彩
    const COLORREF SECONDARY = RGB(156, 175, 183);       // 辅助色 - 暖灰蓝
    const COLORREF ACCENT = RGB(95, 158, 160);           // 强调色 - 青蓝色
    
    // 状态色彩
    const COLORREF SUCCESS = RGB(120, 156, 140);         // 成功色 - 冷绿
    const COLORREF WARNING = RGB(180, 142, 120);         // 警告色 - 暖棕
    const COLORREF DANGER = RGB(165, 110, 115);          // 危险色 - 冷红
    
    // 文字色彩
    const COLORREF TEXT_PRIMARY = RGB(65, 85, 95);       // 主要文字
    const COLORREF TEXT_SECONDARY = RGB(120, 135, 145);  // 次要文字
    const COLORREF TEXT_DISABLED = RGB(160, 175, 185);   // 禁用文字
    
    // 游戏元素特定色彩
    const COLORREF PLAYER_MAIN = RGB(108, 142, 165);     // 玩家主色
    const COLORREF PLAYER_ACCENT = RGB(134, 167, 189);   // 玩家强调色
    
    const COLORREF PLATFORM_NORMAL = RGB(156, 175, 183); // 普通平台
    const COLORREF PLATFORM_SPRING = RGB(120, 156, 140); // 弹簧平台
    const COLORREF PLATFORM_BREAKABLE = RGB(180, 142, 120); // 易碎平台
    const COLORREF PLATFORM_MOVING = RGB(95, 158, 160);  // 移动平台
}

// 绘制工具类 - 支持圆角和柔和效果
namespace DrawUtils {
    // 绘制圆角矩形
    void drawRoundedRect(int x, int y, int width, int height, int radius, COLORREF fillColor, COLORREF borderColor = -1);
    
    // 绘制圆角矩形（带渐变效果）
    void drawGradientRoundedRect(int x, int y, int width, int height, int radius, COLORREF startColor, COLORREF endColor);
    
    // 绘制软阴影圆角矩形
    void drawSoftShadowRect(int x, int y, int width, int height, int radius, COLORREF fillColor, int shadowOffset = 2);
    
    // 绘制圆形
    void drawSoftCircle(int centerX, int centerY, int radius, COLORREF fillColor, COLORREF borderColor = -1);
    
    // 绘制软边椭圆
    void drawSoftEllipse(int x, int y, int width, int height, COLORREF fillColor, COLORREF borderColor = -1);
    
    // 插值颜色
    COLORREF interpolateColor(COLORREF color1, COLORREF color2, float ratio);
    
    // 混合颜色（用于阴影效果）
    COLORREF blendColor(COLORREF baseColor, COLORREF blendColor, float alpha);
}
