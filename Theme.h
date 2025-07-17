#pragma once
#include <graphics.h>
#include <utility>

// 前向声明 - 不重新定义枚举
enum PlatformType;
enum ItemType;

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
    const COLORREF TEXT_COMBO = RGB(255, 165, 0);        // 连击文字 - 橙色
    const COLORREF TEXT_SCORE = RGB(50, 150, 200);       // 分数文字 - 蓝色

    // 游戏元素特定色彩
    const COLORREF PLAYER_MAIN = RGB(108, 142, 165);     // 玩家主色
    const COLORREF PLAYER_ACCENT = RGB(134, 167, 189);   // 玩家强调色
    const COLORREF PLAYER_SPEED_EFFECT = RGB(255, 200, 100); // 玩家加速效果
    const COLORREF PLAYER_SHIELD_EFFECT = RGB(100, 200, 255); // 玩家护盾效果

    // 平台颜色系统
    const COLORREF PLATFORM_NORMAL = RGB(156, 175, 183); // 普通平台 - 灰蓝
    const COLORREF PLATFORM_NORMAL_HIGHLIGHT = RGB(176, 195, 203); // 普通平台高亮

    const COLORREF PLATFORM_SPRING = RGB(120, 190, 150); // 弹簧平台 - 绿色
    const COLORREF PLATFORM_SPRING_ACTIVE = RGB(100, 220, 130); // 弹簧平台激活
    const COLORREF PLATFORM_SPRING_COMPRESSED = RGB(80, 160, 110); // 弹簧平台压缩

    const COLORREF PLATFORM_BREAKABLE = RGB(200, 160, 140); // 易碎平台 - 棕色
    const COLORREF PLATFORM_BREAKABLE_WARNING = RGB(220, 140, 120); // 易碎平台警告
    const COLORREF PLATFORM_BREAKABLE_BREAKING = RGB(240, 120, 100); // 易碎平台破裂

    const COLORREF PLATFORM_MOVING = RGB(95, 158, 160);  // 移动平台 - 青色
    const COLORREF PLATFORM_MOVING_TRAIL = RGB(115, 178, 180); // 移动平台轨迹

    // 道具颜色系统
    const COLORREF ITEM_SPEED = RGB(255, 200, 100);      // 加速道具 - 橙色
    const COLORREF ITEM_SPEED_GLOW = RGB(255, 220, 130); // 加速道具光晕
    const COLORREF ITEM_SPEED_PARTICLE = RGB(255, 180, 80); // 加速道具粒子

    const COLORREF ITEM_SHIELD = RGB(100, 200, 255);     // 护盾道具 - 蓝色
    const COLORREF ITEM_SHIELD_GLOW = RGB(130, 220, 255); // 护盾道具光晕
    const COLORREF ITEM_SHIELD_PARTICLE = RGB(80, 180, 255); // 护盾道具粒子

    const COLORREF ITEM_DOUBLE_JUMP = RGB(100, 255, 100);      // 二段跳道具 - 绿色
    const COLORREF ITEM_DOUBLE_JUMP_GLOW = RGB(130, 255, 130); // 二段跳道具光晕
    const COLORREF ITEM_DOUBLE_JUMP_PARTICLE = RGB(80, 255, 80); // 二段跳道具粒子

    const COLORREF ITEM_SLOW_TIME = RGB(100, 100, 255);        // 时间减缓道具 - 蓝色
    const COLORREF ITEM_SLOW_TIME_GLOW = RGB(130, 130, 255);   // 时间减缓道具光晕
    const COLORREF ITEM_SLOW_TIME_PARTICLE = RGB(80, 80, 255); // 时间减缓道具粒子

    const COLORREF ITEM_MAGNETIC_FIELD = RGB(255, 100, 255);   // 磁场道具 - 紫色
    const COLORREF ITEM_MAGNETIC_FIELD_GLOW = RGB(255, 130, 255); // 磁场道具光晕
    const COLORREF ITEM_MAGNETIC_FIELD_PARTICLE = RGB(255, 80, 255); // 磁场道具粒子

    const COLORREF ITEM_FREEZE_OBSTACLES = RGB(100, 255, 255); // 冻结障碍物道具 - 青色
    const COLORREF ITEM_FREEZE_OBSTACLES_GLOW = RGB(130, 255, 255); // 冻结障碍物道具光晕
    const COLORREF ITEM_FREEZE_OBSTACLES_PARTICLE = RGB(80, 255, 255); // 冻结障碍物道具粒子

    const COLORREF ITEM_HEALTH_BOOST = RGB(255, 100, 100);     // 生命值恢复道具 - 红色
    const COLORREF ITEM_HEALTH_BOOST_GLOW = RGB(255, 130, 130); // 生命值恢复道具光晕
    const COLORREF ITEM_HEALTH_BOOST_PARTICLE = RGB(255, 80, 80); // 生命值恢复道具粒子

    const COLORREF ITEM_INVINCIBILITY = RGB(255, 215, 0);      // 无敌道具 - 金色
    const COLORREF ITEM_INVINCIBILITY_GLOW = RGB(255, 235, 50); // 无敌道具光晕
    const COLORREF ITEM_INVINCIBILITY_PARTICLE = RGB(255, 200, 0); // 无敌道具粒子

    const COLORREF ITEM_COIN = RGB(255, 223, 0);               // 金币 - 金色
    const COLORREF ITEM_COIN_GLOW = RGB(255, 240, 50);         // 金币光晕
    const COLORREF ITEM_COIN_PARTICLE = RGB(255, 215, 0);      // 金币粒子

    // 粒子效果颜色
    const COLORREF PARTICLE_JUMP = RGB(134, 167, 189);   // 跳跃粒子 - 浅蓝
    const COLORREF PARTICLE_LAND = RGB(95, 158, 160);    // 着陆粒子 - 青色
    const COLORREF PARTICLE_SPEED = RGB(255, 180, 80);   // 加速粒子 - 橙色
    const COLORREF PARTICLE_BREAK = RGB(180, 140, 120);  // 破裂粒子 - 棕色
    const COLORREF PARTICLE_SPRING = RGB(120, 200, 140); // 弹簧粒子 - 绿色

    // 光晕和特效颜色
    const COLORREF SHIELD_GLOW = RGB(150, 220, 255);     // 护盾光晕
    const COLORREF SHIELD_GLOW_INNER = RGB(180, 240, 255); // 护盾内层光晕
    const COLORREF SPEED_GLOW = RGB(255, 180, 80);       // 加速光晕
    const COLORREF SPEED_GLOW_INNER = RGB(255, 200, 120); // 加速内层光晕

    // 连击系统颜色
    const COLORREF COMBO_LOW = RGB(255, 255, 255);       // 低连击 - 白色
    const COLORREF COMBO_MEDIUM = RGB(255, 200, 100);    // 中连击 - 橙色
    const COLORREF COMBO_HIGH = RGB(255, 100, 100);      // 高连击 - 红色
    const COLORREF COMBO_EXTREME = RGB(255, 50, 255);    // 极高连击 - 紫色

    // 背景滚动颜色
    const COLORREF BG_LAYER_1 = RGB(240, 245, 250);     // 背景层1 - 最浅
    const COLORREF BG_LAYER_2 = RGB(230, 240, 245);     // 背景层2 - 中等
    const COLORREF BG_LAYER_3 = RGB(220, 235, 240);     // 背景层3 - 较深
    const COLORREF BG_LAYER_4 = RGB(210, 230, 235);     // 背景层4 - 最深

    // UI元素颜色
    const COLORREF UI_BACKGROUND = RGB(250, 252, 254);   // UI背景 - 极浅
    const COLORREF UI_BORDER = RGB(220, 230, 240);       // UI边框
    const COLORREF UI_SHADOW = RGB(100, 100, 100);       // UI阴影
    const COLORREF UI_HIGHLIGHT = RGB(255, 255, 255);    // UI高光

    // 预警系统颜色
    const COLORREF PREVIEW_NORMAL = RGB(200, 220, 230);  // 普通平台预览
    const COLORREF PREVIEW_SPECIAL = RGB(180, 200, 220); // 特殊平台预览
    const COLORREF DANGER_ZONE = RGB(255, 100, 100);     // 危险区域
    const COLORREF WARNING_FLASH = RGB(255, 200, 200);   // 警告闪烁

    const COLORREF UI_TRANSPARENT_BG = RGB(240, 245, 250);   // 半透明UI背景

    // 增强的视觉效果颜色
    const COLORREF ITEM_GLOW_INNER = RGB(255, 255, 200);     // 道具内层光晕
    const COLORREF PLATFORM_GLOW = RGB(200, 220, 240);       // 平台光晕
}

// 绘制工具类 - 支持圆角和柔和效果
namespace DrawUtils {
    // 基础绘制函数
    void drawRoundedRect(int x, int y, int width, int height, int radius, COLORREF fillColor, COLORREF borderColor = (COLORREF)-1);
    void drawGradientRoundedRect(int x, int y, int width, int height, int radius, COLORREF startColor, COLORREF endColor);
    void drawSoftShadowRect(int x, int y, int width, int height, int radius, COLORREF fillColor, int shadowOffset = 2);
    void drawSoftCircle(int centerX, int centerY, int radius, COLORREF fillColor, COLORREF borderColor = (COLORREF)-1);
    void drawSoftEllipse(int x, int y, int width, int height, COLORREF fillColor, COLORREF borderColor = (COLORREF)-1);

    // 特效绘制函数
    void drawGlowCircle(int centerX, int centerY, int radius, COLORREF glowColor, float intensity = 1.0f);
    void drawGlowRect(int x, int y, int width, int height, COLORREF glowColor, float intensity = 1.0f);
    void drawPulsingCircle(int centerX, int centerY, int baseRadius, float pulseAmount, float time, COLORREF color);

    // 粒子绘制函数
    void drawParticle(float x, float y, float size, COLORREF color, float alpha = 1.0f);
    void drawSparkle(float x, float y, float size, COLORREF color, float rotation = 0.0f);

    // 连击效果绘制
    void drawComboText(int x, int y, int combo, COLORREF color);

    // 道具效果绘制
    void drawSpeedEffect(float x, float y, float width, float height, float intensity = 1.0f);
    void drawShieldEffect(float x, float y, float radius, float intensity = 1.0f);
    void drawItemGlow(float x, float y, float size, COLORREF glowColor, float time);

    // 平台特效绘制
    void drawSpringCompression(float x, float y, float width, float height, float compression);
    void drawBreakEffect(float x, float y, float width, float height, float breakProgress);

    // 预警系统绘制
    void drawPlatformPreview(float x, float y, float width, float height, COLORREF previewColor, float alpha = 0.3f);
    void drawDangerZone(float y, float intensity = 1.0f);

    // 透明度绘制函数
    void drawTransparentRect(int x, int y, int width, int height, COLORREF color, float alpha);
    void drawPolygonGlow(POINT* points, int count, COLORREF glowColor, float intensity);

    // 颜色工具函数
    COLORREF interpolateColor(COLORREF color1, COLORREF color2, float ratio);
    COLORREF blendColor(COLORREF baseColor, COLORREF blendColor, float alpha);
    COLORREF adjustBrightness(COLORREF color, float factor);
    COLORREF addGlow(COLORREF baseColor, COLORREF glowColor, float intensity);

    // 获取连击颜色
    COLORREF getComboColor(int comboCount);

    // 获取平台状态颜色
    COLORREF getPlatformColor(PlatformType type, float animationTime, bool isActive = false);

    // 获取道具颜色
    COLORREF getItemColor(ItemType type, float animationTime);
}

// 动画系统
namespace AnimationUtils {
    // 缓动函数
    float easeInOut(float t);
    float easeIn(float t);
    float easeOut(float t);
    float bounce(float t);

    // 脉冲动画
    float pulse(float time, float frequency = 1.0f);
    float wave(float time, float frequency = 1.0f, float amplitude = 1.0f);

    // 震动效果
    std::pair<float, float> shake(float intensity, float time);

    // 颜色动画
    COLORREF colorPulse(COLORREF baseColor, COLORREF accentColor, float time, float frequency = 1.0f);
    COLORREF colorFlash(COLORREF baseColor, COLORREF flashColor, float intensity);
}