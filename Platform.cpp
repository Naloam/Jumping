#include "Platform.h"
#include "Theme.h"
#include <cmath>
#include <cstdlib>

Platform::Platform(float x, float y, float width, float height, PlatformType type)
    : x(x), y(y), width(width), height(height), type(type), animationTimer(0.0f),
    moveSpeed(50.0f), moveRange(100.0f), startX(x), moveDirection(1),
    isBroken(false), breakTimer(0.0f), hitCount(0),
    springCompression(0.0f), wasTriggered(false), item(nullptr) {

    // 根据类型设置颜色
    switch (type) {
    case NORMAL:
        color = Theme::PLATFORM_NORMAL;
        break;
    case MOVING:
        color = Theme::PLATFORM_MOVING;
        break;
    case BREAKABLE:
        color = Theme::PLATFORM_BREAKABLE;
        break;
    case SPRING:
        color = Theme::PLATFORM_SPRING;
        break;
    }

    // 有概率生成道具
    if (type == NORMAL && rand() % 10 == 0) {  // 10% 概率
        ItemType itemType = (rand() % 2 == 0) ? SPEED_BOOST : SHIELD;
        spawnItem(itemType);
    }
}

// 复制构造函数
Platform::Platform(const Platform& other)
    : x(other.x), y(other.y), width(other.width), height(other.height),
    type(other.type), color(other.color), animationTimer(other.animationTimer),
    moveSpeed(other.moveSpeed), moveRange(other.moveRange), startX(other.startX),
    moveDirection(other.moveDirection), isBroken(other.isBroken),
    breakTimer(other.breakTimer), hitCount(other.hitCount),
    springCompression(other.springCompression), wasTriggered(other.wasTriggered) {

    // 深拷贝道具
    if (other.item) {
        item = std::make_unique<Item>(*other.item);
    }
}

// 赋值操作符
Platform& Platform::operator=(const Platform& other) {
    if (this != &other) {
        x = other.x;
        y = other.y;
        width = other.width;
        height = other.height;
        type = other.type;
        color = other.color;
        animationTimer = other.animationTimer;
        moveSpeed = other.moveSpeed;
        moveRange = other.moveRange;
        startX = other.startX;
        moveDirection = other.moveDirection;
        isBroken = other.isBroken;
        breakTimer = other.breakTimer;
        hitCount = other.hitCount;
        springCompression = other.springCompression;
        wasTriggered = other.wasTriggered;

        // 深拷贝道具
        if (other.item) {
            item = std::make_unique<Item>(*other.item);
        }
        else {
            item.reset();
        }
    }
    return *this;
}

// 移动构造函数
Platform::Platform(Platform&& other) noexcept
    : x(other.x), y(other.y), width(other.width), height(other.height),
    type(other.type), color(other.color), animationTimer(other.animationTimer),
    moveSpeed(other.moveSpeed), moveRange(other.moveRange), startX(other.startX),
    moveDirection(other.moveDirection), isBroken(other.isBroken),
    breakTimer(other.breakTimer), hitCount(other.hitCount),
    springCompression(other.springCompression), wasTriggered(other.wasTriggered),
    item(std::move(other.item)) {
}

// 移动赋值操作符
Platform& Platform::operator=(Platform&& other) noexcept {
    if (this != &other) {
        x = other.x;
        y = other.y;
        width = other.width;
        height = other.height;
        type = other.type;
        color = other.color;
        animationTimer = other.animationTimer;
        moveSpeed = other.moveSpeed;
        moveRange = other.moveRange;
        startX = other.startX;
        moveDirection = other.moveDirection;
        isBroken = other.isBroken;
        breakTimer = other.breakTimer;
        hitCount = other.hitCount;
        springCompression = other.springCompression;
        wasTriggered = other.wasTriggered;
        item = std::move(other.item);
    }
    return *this;
}

void Platform::update(float deltaTime) {
    animationTimer += deltaTime;

    if (type == MOVING && !isBroken) {
        // 移动平台逻辑
        x += moveSpeed * moveDirection * deltaTime;

        if (x <= startX - moveRange || x >= startX + moveRange) {
            moveDirection *= -1;
        }
    }

    if (type == BREAKABLE && isBroken) {
        breakTimer += deltaTime;
        // 破碎平台3秒后重生
        if (breakTimer > 3.0f) {
            isBroken = false;
            breakTimer = 0.0f;
            hitCount = 0;
        }
    }

    if (type == SPRING) {
        // 弹簧恢复
        if (springCompression > 0) {
            springCompression -= deltaTime * 5.0f;
            if (springCompression < 0) springCompression = 0;
        }
        wasTriggered = false;
    }

    // 更新道具动画
    if (item && !item->collected) {
        item->animationTimer += deltaTime;
    }
}

bool Platform::checkCollision(float playerX, float playerY, float playerWidth, float playerHeight) {
    if (isBroken) return false;

    return playerX < x + width &&
        playerX + playerWidth > x &&
        playerY < y + height &&
        playerY + playerHeight > y;
}

float Platform::handleCollision(float playerX, float playerY, float playerWidth, float playerHeight, float& playerVY) {
    if (isBroken) return y;

    float resultY = y - playerHeight;

    // 处理不同类型平台的特殊效果
    switch (type) {
    case BREAKABLE:
        hitCount++;
        if (hitCount >= 1) {  // 踩一次就破
            triggerBreak();
        }
        break;

    case SPRING:
        if (playerVY >= 0) {  // 只要是向下或静止就可以弹起
            triggerSpring();
            playerVY = -500.0f;  // 增强弹簧力度，从-450改为-500
            springCompression = 1.0f;
            wasTriggered = true;

            // 添加弹簧粒子效果
            createSpringParticles(x + width / 2, y);
        }
        break;
    }

    return resultY;
}

void Platform::createSpringParticles(float centerX, float centerY) {
    // 这里创建一些向上的粒子来表示弹簧效果
    // 注意：这个函数目前不会真正创建粒子，因为Platform类没有粒子系统
    // 但我们可以通过视觉效果来表示弹簧激活

    // 触发弹簧压缩动画
    springCompression = 1.0f;

    // 增加动画计时器来创建视觉反馈
    animationTimer += 0.5f; // 让弹簧动画更明显
}

void Platform::triggerBreak() {
    if (type == BREAKABLE) {
        isBroken = true;
        breakTimer = 0.0f;
    }
}

void Platform::triggerSpring() {
    springCompression = 1.0f;
    wasTriggered = true;
}

void Platform::spawnItem(ItemType itemType) {
    if (!item) {
        item = std::make_unique<Item>(x + width / 2 - 10, y - 25, itemType);
    }
}

Item* Platform::collectItem() {
    if (item && !item->collected) {
        item->collected = true;
        return item.get(); // 返回原始指针但保持所有权
    }
    return nullptr;
}

void Platform::draw() const {
    drawWithOffset(0, 0);
}

void Platform::drawWithOffset(float offsetX, float offsetY) const {
    if (isBroken) return;  // 不绘制已破碎的平台

    float drawX = x + offsetX;
    float drawY = y + offsetY;

    // 根据类型设置颜色和效果
    COLORREF drawColor;

    switch (type) {
    case NORMAL:
        // 普通平台 - 简洁的矩形
        drawColor = Theme::PLATFORM_NORMAL;
        setfillcolor(drawColor);
        solidrectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));

        // 简单的顶部高光
        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 2);
        line((int)drawX, (int)drawY, (int)(drawX + width), (int)drawY);
        break;

    case MOVING: {
        // 移动平台 - 带有动态箭头指示器
        float pulse = 0.8f + 0.2f * std::sin(animationTimer * 3.0f);
        int r = GetRValue(Theme::PLATFORM_MOVING);
        int g = GetGValue(Theme::PLATFORM_MOVING);
        int b = GetBValue(Theme::PLATFORM_MOVING);
        drawColor = RGB((int)(r * pulse), (int)(g * pulse), (int)(b * pulse));

        // 绘制主体
        setfillcolor(drawColor);
        solidrectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));

        // 绘制移动轨迹线
        setlinecolor(Theme::PLATFORM_MOVING_TRAIL);
        setlinestyle(PS_SOLID, 1);
        line((int)(startX - moveRange + offsetX), (int)(drawY + height / 2),
            (int)(startX + moveRange + offsetX), (int)(drawY + height / 2));

        // 动态移动方向箭头
        float arrowX = drawX + width / 2 + 15 * moveDirection;
        float arrowY = drawY - 8;

        setfillcolor(Theme::ACCENT);
        // 绘制箭头 - 修复：用大括号包装变量定义
        {
            POINT arrow[3];
            if (moveDirection > 0) {
                arrow[0] = { (int)arrowX, (int)arrowY };
                arrow[1] = { (int)(arrowX - 8), (int)(arrowY - 4) };
                arrow[2] = { (int)(arrowX - 8), (int)(arrowY + 4) };
            }
            else {
                arrow[0] = { (int)arrowX, (int)arrowY };
                arrow[1] = { (int)(arrowX + 8), (int)(arrowY - 4) };
                arrow[2] = { (int)(arrowX + 8), (int)(arrowY + 4) };
            }
            solidpolygon(arrow, 3);
        }
        break;
    }

    case BREAKABLE: {
        // 易碎平台 - 带有裂纹效果
        float flicker = 0.7f + 0.3f * std::sin(animationTimer * 6.0f);
        int r = GetRValue(Theme::PLATFORM_BREAKABLE);
        int g = GetGValue(Theme::PLATFORM_BREAKABLE);
        int b = GetBValue(Theme::PLATFORM_BREAKABLE);
        drawColor = RGB((int)(r * flicker), (int)(g * flicker), (int)(b * flicker));

        setfillcolor(drawColor);
        solidrectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));

        // 绘制裂纹图案
        setlinecolor(Theme::PLATFORM_BREAKABLE_WARNING);
        setlinestyle(PS_SOLID, 1);

        // 绘制几条裂纹线
        for (int i = 1; i <= 3; i++) {
            float crackX = drawX + width * i / 4;
            line((int)crackX, (int)drawY, (int)crackX, (int)(drawY + height));
        }

        // 危险标识 - 小三角形 - 修复：用大括号包装变量定义
        {
            setfillcolor(Theme::WARNING);
            POINT warning[3] = {
                {(int)(drawX + width / 2), (int)(drawY - 8)},
                {(int)(drawX + width / 2 - 6), (int)(drawY - 2)},
                {(int)(drawX + width / 2 + 6), (int)(drawY - 2)}
            };
            solidpolygon(warning, 3);
        }

        // 计算破裂进度
        float breakProgress = (float)hitCount / 1.0f; // 1次命中就破裂

        // 使用Theme.cpp中的drawBreakEffect
        DrawUtils::drawBreakEffect(drawX, drawY, width, height, breakProgress);

        // 如果快要破裂，添加警告效果
        if (breakProgress > 0.5f) {
            float warningPulse = AnimationUtils::pulse(animationTimer, 6.0f);
            DrawUtils::drawGlowRect((int)drawX, (int)drawY, (int)width, (int)height,
                Theme::PLATFORM_BREAKABLE_WARNING, warningPulse * 0.5f);
        }

        // 危险标识增强
        setfillcolor(Theme::WARNING);
        POINT warning[3] = {
            {(int)(drawX + width / 2), (int)(drawY - 8)},
            {(int)(drawX + width / 2 - 6), (int)(drawY - 2)},
            {(int)(drawX + width / 2 + 6), (int)(drawY - 2)}
        };
        solidpolygon(warning, 3);

        // 警告光晕
        DrawUtils::drawGlowCircle((int)(drawX + width / 2), (int)(drawY - 5), 8,
            Theme::WARNING, AnimationUtils::pulse(animationTimer, 4.0f));

        // 感叹号
        settextcolor(RGB(255, 255, 255));
        settextstyle(12, 0, L"Arial");
        outtextxy((int)(drawX + width / 2 - 3), (int)(drawY - 7), L"!");
        break;
    }

    case SPRING: {
        // 弹簧平台 - 根据压缩状态调整高度，添加弹簧视觉效果
        float actualHeight = height * (1.0f - springCompression * 0.3f);
        float actualY = drawY + (height - actualHeight);

        drawColor = Theme::PLATFORM_SPRING;

        // 绘制弹簧平台主体
        setfillcolor(drawColor);
        solidrectangle((int)drawX, (int)actualY, (int)(drawX + width), (int)(actualY + actualHeight));

        // 绘制弹簧螺旋线纹理
        setlinecolor(RGB(80, 120, 100));
        setlinestyle(PS_SOLID, 2);

        for (int i = 0; i < 4; i++) {
            float lineY = actualY + actualHeight * (i + 1) / 5;
            // 波浪线效果
            for (int j = 0; j < width - 10; j += 5) {
                float waveY = lineY + 2 * std::sin((j + animationTimer * 100) * 0.3f);
                line((int)(drawX + j), (int)lineY, (int)(drawX + j + 5), (int)waveY);
            }
        }

        // 弹簧标识 - 向上箭头 - 修复：用大括号包装变量定义
        {
            setfillcolor(Theme::PLATFORM_SPRING_ACTIVE);
            POINT springArrow[3] = {
                {(int)(drawX + width / 2), (int)(drawY - 12)},
                {(int)(drawX + width / 2 - 8), (int)(drawY - 4)},
                {(int)(drawX + width / 2 + 8), (int)(drawY - 4)}
            };
            solidpolygon(springArrow, 3);

            // 双箭头效果
            POINT springArrow2[3] = {
                {(int)(drawX + width / 2), (int)(drawY - 18)},
                {(int)(drawX + width / 2 - 6), (int)(drawY - 12)},
                {(int)(drawX + width / 2 + 6), (int)(drawY - 12)}
            };
            solidpolygon(springArrow2, 3);
        }

        // 弹簧平台 - 使用Theme.cpp中的drawSpringCompression
        DrawUtils::drawSpringCompression(drawX, drawY, width, height, springCompression);

        // 如果压缩了，添加弹簧粒子效果
        if (springCompression > 0.1f) {
            // 绘制弹簧激活的光晕效果
            DrawUtils::drawGlowRect((int)drawX, (int)drawY, (int)width, (int)height,
                Theme::PLATFORM_SPRING_ACTIVE, springCompression);

            // 弹簧能量脉冲
            DrawUtils::drawPulsingCircle((int)(drawX + width / 2), (int)(drawY + height / 2),
                (int)(15 + springCompression * 10), 5.0f,
                animationTimer, Theme::PLATFORM_SPRING_ACTIVE);
        }

        // 绘制道具
        if (item && !item->collected) {
            drawItem(item.get(), offsetX, offsetY);
        }
        return;
    }
    }

    // 绘制边框高光（对于普通平台）
    if (type == NORMAL) {
        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 1);
        rectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));
    }

    // 绘制道具
    if (item && !item->collected) {
        drawItem(item.get(), offsetX, offsetY);
    }
}

// 增强道具绘制效果 - 修复diamond初始化问题
void Platform::drawItem(const Item* itemPtr, float offsetX, float offsetY) const {
    if (!itemPtr) return;

    float itemX = itemPtr->x + offsetX;
    float itemY = itemPtr->y + offsetY;

    // 道具浮动动画
    float bounce = std::sin(itemPtr->animationTimer * 4.0f) * 5.0f;
    itemY += bounce;

    // 旋转效果
    float rotation = itemPtr->animationTimer * 2.0f;

    switch (itemPtr->type) {
    case SPEED_BOOST: {
        // 修复：用大括号包装变量定义，避免跳转初始化问题
        COLORREF itemColor = Theme::ITEM_SPEED;
        COLORREF glowColor = Theme::ITEM_SPEED_GLOW;

        // 绘制外层光晕
        DrawUtils::drawGlowCircle((int)itemX, (int)itemY, 18, glowColor, 0.6f);

        // 绘制主体 - 菱形
        setfillcolor(itemColor);
        POINT diamond[4] = {
            {(int)itemX, (int)(itemY - 12)},      // 上
            {(int)(itemX + 12), (int)itemY},      // 右
            {(int)itemX, (int)(itemY + 12)},      // 下
            {(int)(itemX - 12), (int)itemY}       // 左
        };
        solidpolygon(diamond, 4);

        // 内部闪电符号
        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 2);
        line((int)(itemX - 4), (int)(itemY - 6), (int)(itemX + 2), (int)(itemY - 2));
        line((int)(itemX + 2), (int)(itemY - 2), (int)(itemX - 2), (int)(itemY + 2));
        line((int)(itemX - 2), (int)(itemY + 2), (int)(itemX + 4), (int)(itemY + 6));
        break;
    }

    case SHIELD: {
        // 修复：用大括号包装变量定义
        COLORREF itemColor = Theme::ITEM_SHIELD;
        COLORREF glowColor = Theme::ITEM_SHIELD_GLOW;

        // 绘制外层光晕
        DrawUtils::drawGlowCircle((int)itemX, (int)itemY, 18, glowColor, 0.6f);

        // 绘制主体 - 六边形盾牌
        setfillcolor(itemColor);
        POINT shield[6];
        for (int i = 0; i < 6; i++) {
            float angle = i * 3.14159f / 3 + rotation;
            shield[i].x = (int)(itemX + 12 * cos(angle));
            shield[i].y = (int)(itemY + 12 * sin(angle));
        }
        solidpolygon(shield, 6);

        // 内部十字
        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 2);
        line((int)(itemX - 6), (int)itemY, (int)(itemX + 6), (int)itemY);
        line((int)itemX, (int)(itemY - 6), (int)itemX, (int)(itemY + 6));
        break;
    }

    default:
        return;
    }

    // 绘制边框
    setlinecolor(RGB(255, 255, 255));
    setlinestyle(PS_SOLID, 1);
    circle((int)itemX, (int)itemY, 14);
}