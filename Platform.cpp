#include "Platform.h"
#include "Theme.h"
#include "AudioManager.h"
#include <cmath>
#include <cstdlib>
#include <graphics.h>
#include <string>

// 在Platform构造函数中更新道具生成逻辑
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

    // 道具类型生成
    if (type == NORMAL && rand() % 100 < 20) {  // 20% 概率生成道具
        int itemChoice = rand() % 100;
        if (itemChoice < 35) {  // 35% 概率生成金币
            spawnItem(COIN);
        }
        else if (itemChoice < 50) {  // 15% 概率生成加速道具
            spawnItem(SPEED_BOOST);
        }
        else if (itemChoice < 65) {  // 15% 概率生成护盾道具
            spawnItem(SHIELD);
        }
        else if (itemChoice < 75) {  // 10% 概率生成生命恢复道具
            spawnItem(HEALTH_BOOST);
        }
        else if (itemChoice < 83) {  // 8% 概率生成无敌道具
            spawnItem(INVINCIBILITY);
        }
        else if (itemChoice < 89) {  // 6% 概率生成二段跳道具
            spawnItem(DOUBLE_JUMP);
        }
        else if (itemChoice < 94) {  // 5% 概率生成时间减缓道具
            spawnItem(SLOW_TIME);
        }
        else if (itemChoice < 98) {  // 4% 概率生成磁场道具
            spawnItem(MAGNETIC_FIELD);
        }
        else {  // 2% 概率生成冻结障碍物道具
            spawnItem(FREEZE_OBSTACLES);
        }
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
        // 改进弹簧触发条件
        // 1. 玩家向下移动时触发弹簧
        // 2. 玩家静止在弹簧上时也应该能够被弹起（如果按跳跃键）
        if (playerVY >= -50.0f) {  // 放宽条件，允许轻微向上速度时也能触发
            // 检查是否应该触发弹簧效果
            bool shouldTrigger = false;

            if (playerVY > 0) {
                // 向下移动时始终触发
                shouldTrigger = true;
            }
            else if (playerVY >= -50.0f && !wasTriggered) {
                // 轻微向上移动或静止时，如果弹簧未被触发过，也可以触发
                shouldTrigger = true;
            }

            if (shouldTrigger) {
                triggerSpring();
                playerVY = -500.0f;  // 弹簧力度
                springCompression = 1.0f;
                wasTriggered = true;

                // 添加弹簧粒子效果
                createSpringParticles(x + width / 2, y);
            }
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
        // 播放平台破碎音效
        AudioManager::getInstance().playSound(SoundType::PLATFORM_BREAK, false);
    }
}

void Platform::triggerSpring() {
    springCompression = 1.0f;
    wasTriggered = true;
    // 播放弹簧音效
    AudioManager::getInstance().playSound(SoundType::SPRING_BOUNCE, false);
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
        // 绘制箭头
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
        fillpolygon(arrow, 3);
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

        // 危险标识 - 小三角形
        setfillcolor(Theme::WARNING);
        POINT warning[3] = {
            {(int)(drawX + width / 2), (int)(drawY - 8)},
            {(int)(drawX + width / 2 - 6), (int)(drawY - 2)},
            {(int)(drawX + width / 2 + 6), (int)(drawY - 2)}
        };
        fillpolygon(warning, 3);

        // 计算破裂进度
        float breakProgress = (float)hitCount / 1.0f; // 1次命中就破裂

        // 如果快要破裂，添加警告效果
        if (breakProgress > 0.5f) {
            // 警告光晕效果可以在这里添加
        }

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

        // 弹簧标识 - 向上箭头
        setfillcolor(Theme::PLATFORM_SPRING_ACTIVE);
        POINT springArrow[3] = {
            {(int)(drawX + width / 2), (int)(drawY - 12)},
            {(int)(drawX + width / 2 - 8), (int)(drawY - 4)},
            {(int)(drawX + width / 2 + 8), (int)(drawY - 4)}
        };
        fillpolygon(springArrow, 3);

        // 双箭头效果
        POINT springArrow2[3] = {
            {(int)(drawX + width / 2), (int)(drawY - 18)},
            {(int)(drawX + width / 2 - 6), (int)(drawY - 12)},
            {(int)(drawX + width / 2 + 6), (int)(drawY - 12)}
        };
        fillpolygon(springArrow2, 3);

        // 如果压缩了，添加弹簧激活效果
        if (springCompression > 0.1f) {
            // 可以在这里添加更多弹簧效果
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

// 增强道具绘制效果
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
    case DOUBLE_JUMP: {
        // 二段跳道具 - 双层向上箭头
        COLORREF doubleJumpColor = Theme::ITEM_DOUBLE_JUMP;

        // 绘制主体 - 圆形
        setfillcolor(doubleJumpColor);
        solidcircle((int)itemX, (int)itemY, 12);

        // 绘制内部圆形
        setfillcolor(RGB(150, 255, 150));
        solidcircle((int)itemX, (int)itemY, 8);

        // 绘制双层向上箭头
        setfillcolor(RGB(255, 255, 255));

        // 第一层箭头
        POINT arrow1[3] = {
            {(int)itemX, (int)(itemY - 6)},
            {(int)(itemX - 4), (int)(itemY - 2)},
            {(int)(itemX + 4), (int)(itemY - 2)}
        };
        fillpolygon(arrow1, 3);

        // 第二层箭头
        POINT arrow2[3] = {
            {(int)itemX, (int)(itemY + 2)},
            {(int)(itemX - 4), (int)(itemY + 6)},
            {(int)(itemX + 4), (int)(itemY + 6)}
        };
        fillpolygon(arrow2, 3);

        // 绘制连接线
        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 2);
        line((int)itemX, (int)(itemY - 2), (int)itemX, (int)(itemY + 2));

        // 绘制外边框
        setlinecolor(RGB(50, 200, 50));
        setlinestyle(PS_SOLID, 1);
        circle((int)itemX, (int)itemY, 12);
        break;
    }

    case SLOW_TIME: {
        // 时间减缓道具 - 时钟图标
        COLORREF slowTimeColor = Theme::ITEM_SLOW_TIME;

        // 绘制主体 - 圆形
        setfillcolor(slowTimeColor);
        solidcircle((int)itemX, (int)itemY, 12);

        // 绘制内部圆形
        setfillcolor(RGB(150, 150, 255));
        solidcircle((int)itemX, (int)itemY, 8);

        // 绘制时钟外圈
        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 2);
        circle((int)itemX, (int)itemY, 6);

        // 绘制时钟刻度
        for (int i = 0; i < 12; i++) {
            float angle = i * 3.14159f / 6;
            float innerRadius = 4;
            float outerRadius = 6;

            float innerX = itemX + innerRadius * cos(angle);
            float innerY = itemY + innerRadius * sin(angle);
            float outerX = itemX + outerRadius * cos(angle);
            float outerY = itemY + outerRadius * sin(angle);

            setlinecolor(RGB(255, 255, 255));
            setlinestyle(PS_SOLID, 1);
            line((int)innerX, (int)innerY, (int)outerX, (int)outerY);
        }

        // 绘制时针和分针
        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 2);
        // 时针
        line((int)itemX, (int)itemY, (int)(itemX + 3), (int)(itemY - 2));
        // 分针
        line((int)itemX, (int)itemY, (int)(itemX + 2), (int)(itemY - 4));

        // 绘制中心点
        setfillcolor(RGB(255, 255, 255));
        solidcircle((int)itemX, (int)itemY, 2);

        // 绘制外边框
        setlinecolor(RGB(50, 50, 200));
        setlinestyle(PS_SOLID, 1);
        circle((int)itemX, (int)itemY, 12);
        break;
    }

    case MAGNETIC_FIELD: {
        // 磁场道具 - 磁铁图标
        COLORREF magneticColor = Theme::ITEM_MAGNETIC_FIELD;

        // 绘制主体 - 圆形
        setfillcolor(magneticColor);
        solidcircle((int)itemX, (int)itemY, 12);

        // 绘制内部圆形
        setfillcolor(RGB(255, 150, 255));
        solidcircle((int)itemX, (int)itemY, 8);

        // 绘制磁铁形状
        setfillcolor(RGB(255, 255, 255));
        solidrectangle((int)(itemX - 6), (int)(itemY - 6), (int)(itemX + 6), (int)(itemY + 6));

        // 绘制磁铁的N和S极
        setfillcolor(RGB(255, 0, 0));
        solidrectangle((int)(itemX - 6), (int)(itemY - 6), (int)(itemX + 6), (int)itemY);

        setfillcolor(RGB(0, 0, 255));
        solidrectangle((int)(itemX - 6), (int)itemY, (int)(itemX + 6), (int)(itemY + 6));

        // 绘制N和S标记
        settextcolor(RGB(255, 255, 255));
        settextstyle(10, 0, L"Arial");
        outtextxy((int)(itemX - 3), (int)(itemY - 5), L"N");
        outtextxy((int)(itemX - 3), (int)(itemY + 1), L"S");

        // 绘制磁场线
        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 1);
        for (int i = 0; i < 4; i++) {
            float angle = i * 3.14159f / 2;
            float startX = itemX + 8 * cos(angle);
            float startY = itemY + 8 * sin(angle);
            float endX = itemX + 12 * cos(angle);
            float endY = itemY + 12 * sin(angle);

            line((int)startX, (int)startY, (int)endX, (int)endY);
        }

        // 绘制外边框
        setlinecolor(RGB(200, 50, 200));
        setlinestyle(PS_SOLID, 1);
        circle((int)itemX, (int)itemY, 12);
        break;
    }

    case FREEZE_OBSTACLES: {
        // 冻结障碍物道具 - 雪花图标
        COLORREF freezeColor = Theme::ITEM_FREEZE_OBSTACLES;

        // 绘制主体 - 圆形
        setfillcolor(freezeColor);
        solidcircle((int)itemX, (int)itemY, 12);

        // 绘制内部圆形
        setfillcolor(RGB(150, 255, 255));
        solidcircle((int)itemX, (int)itemY, 8);

        // 绘制雪花主轴
        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 2);

        // 垂直线
        line((int)itemX, (int)(itemY - 6), (int)itemX, (int)(itemY + 6));
        // 水平线
        line((int)(itemX - 6), (int)itemY, (int)(itemX + 6), (int)itemY);
        // 对角线1
        line((int)(itemX - 4), (int)(itemY - 4), (int)(itemX + 4), (int)(itemY + 4));
        // 对角线2
        line((int)(itemX - 4), (int)(itemY + 4), (int)(itemX + 4), (int)(itemY - 4));

        // 绘制雪花分支
        setlinestyle(PS_SOLID, 1);
        for (int i = 0; i < 8; i++) {
            float angle = i * 3.14159f / 4;
            float branchLength = 3;
            float mainX = itemX + 4 * cos(angle);
            float mainY = itemY + 4 * sin(angle);

            // 左分支
            float leftAngle = angle + 0.5f;
            line((int)mainX, (int)mainY,
                (int)(mainX + branchLength * cos(leftAngle)),
                (int)(mainY + branchLength * sin(leftAngle)));

            // 右分支
            float rightAngle = angle - 0.5f;
            line((int)mainX, (int)mainY,
                (int)(mainX + branchLength * cos(rightAngle)),
                (int)(mainY + branchLength * sin(rightAngle)));
        }

        // 绘制外边框
        setlinecolor(RGB(50, 200, 200));
        setlinestyle(PS_SOLID, 1);
        circle((int)itemX, (int)itemY, 12);
        break;
    }

    case HEALTH_BOOST: {
        // 生命值恢复道具 - 红十字
        COLORREF healthColor = RGB(255, 100, 100);

        // 绘制主体 - 圆形
        setfillcolor(healthColor);
        solidcircle((int)itemX, (int)itemY, 12);

        // 绘制内部圆形
        setfillcolor(RGB(255, 150, 150));
        solidcircle((int)itemX, (int)itemY, 8);

        // 绘制十字
        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 3);
        line((int)(itemX - 6), (int)itemY, (int)(itemX + 6), (int)itemY);
        line((int)itemX, (int)(itemY - 6), (int)itemX, (int)(itemY + 6));

        // 绘制外边框
        setlinecolor(RGB(200, 50, 50));
        setlinestyle(PS_SOLID, 1);
        circle((int)itemX, (int)itemY, 12);
        break;
    }

    case INVINCIBILITY: {
        // 无敌道具 - 金色星星
        COLORREF invincibilityColor = RGB(255, 215, 0);

        // 绘制主体 - 八角星
        setfillcolor(invincibilityColor);
        POINT star[8];
        for (int i = 0; i < 8; i++) {
            float angle = i * 3.14159f / 4 + rotation;
            float radius = (i % 2 == 0) ? 12 : 6;  // 交替长短
            star[i].x = (int)(itemX + radius * cos(angle));
            star[i].y = (int)(itemY + radius * sin(angle));
        }
        fillpolygon(star, 8);

        // 绘制内部圆形
        setfillcolor(RGB(255, 255, 150));
        solidcircle((int)itemX, (int)itemY, 6);

        // 绘制中心点
        setfillcolor(RGB(255, 255, 255));
        solidcircle((int)itemX, (int)itemY, 3);

        // 绘制光晕效果（简化版，不调用可能不存在的函数）
        setfillcolor(RGB(255, 240, 150));
        solidcircle((int)itemX, (int)itemY, 18);
        setfillcolor(invincibilityColor);
        solidcircle((int)itemX, (int)itemY, 12);
        break;
    }

    case COIN: {
        // 金币绘制
        // 绘制金币外层光晕
        setfillcolor(RGB(255, 215, 0));
        solidcircle((int)itemX, (int)itemY, 16);

        // 绘制金币主体
        setfillcolor(RGB(255, 223, 0));
        solidcircle((int)itemX, (int)itemY, 12);

        // 绘制金币内层
        setfillcolor(RGB(255, 255, 100));
        solidcircle((int)itemX, (int)itemY, 8);

        // 绘制金币中心图案
        setfillcolor(RGB(255, 215, 0));
        solidrectangle((int)itemX - 4, (int)itemY - 4, (int)itemX + 4, (int)itemY + 4);

        // 绘制十字纹理
        setlinecolor(RGB(255, 255, 150));
        setlinestyle(PS_SOLID, 2);
        line((int)itemX - 6, (int)itemY, (int)itemX + 6, (int)itemY);
        line((int)itemX, (int)itemY - 6, (int)itemX, (int)itemY + 6);

        // 绘制边缘装饰
        setlinecolor(RGB(200, 170, 0));
        setlinestyle(PS_SOLID, 1);
        circle((int)itemX, (int)itemY, 12);
        circle((int)itemX, (int)itemY, 8);

        break;
    }

    case SPEED_BOOST: {
        COLORREF itemColor = Theme::ITEM_SPEED;

        // 绘制主体 - 菱形
        setfillcolor(itemColor);
        POINT diamond[4] = {
            {(int)itemX, (int)(itemY - 12)},      // 上
            {(int)(itemX + 12), (int)itemY},      // 右
            {(int)itemX, (int)(itemY + 12)},      // 下
            {(int)(itemX - 12), (int)itemY}       // 左
        };
        fillpolygon(diamond, 4);

        // 内部闪电符号
        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 2);
        line((int)(itemX - 4), (int)(itemY - 6), (int)(itemX + 2), (int)(itemY - 2));
        line((int)(itemX + 2), (int)(itemY - 2), (int)(itemX - 2), (int)(itemY + 2));
        line((int)(itemX - 2), (int)(itemY + 2), (int)(itemX + 4), (int)(itemY + 6));
        break;
    }

    case SHIELD: {
        COLORREF itemColor = Theme::ITEM_SHIELD;

        // 绘制主体 - 六边形盾牌
        setfillcolor(itemColor);
        POINT shield[6];
        for (int i = 0; i < 6; i++) {
            float angle = i * 3.14159f / 3 + rotation;
            shield[i].x = (int)(itemX + 12 * cos(angle));
            shield[i].y = (int)(itemY + 12 * sin(angle));
        }
        fillpolygon(shield, 6);

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

    // 移除闪烁效果代码
    /* 原来的闪烁效果代码已被注释掉：
    if (((int)(itemPtr->animationTimer * 2.0f)) % 2 == 0) {
        setfillcolor(RGB(255, 255, 255));
        solidcircle((int)itemX, (int)itemY, 16);
    }
    */

    // 绘制边框
    setlinecolor(RGB(255, 255, 255));
    setlinestyle(PS_SOLID, 1);
    circle((int)itemX, (int)itemY, 14);
}

void drawRotatedRect(float centerX, float centerY, float width, float height, float angle, COLORREF color) {
    // 将角度转换为弧度
    float rad = angle * 3.14159f / 180.0f;

    // 计算旋转后的四个顶点
    float halfWidth = width / 2;
    float halfHeight = height / 2;

    POINT points[4];

    // 原始四个顶点相对于中心点的位置
    float vertices[4][2] = {
        {-halfWidth, -halfHeight},  // 左上
        {halfWidth, -halfHeight},   // 右上
        {halfWidth, halfHeight},    // 右下
        {-halfWidth, halfHeight}    // 左下
    };

    // 应用旋转变换
    for (int i = 0; i < 4; i++) {
        float x = vertices[i][0];
        float y = vertices[i][1];

        points[i].x = (LONG)(centerX + x * cos(rad) - y * sin(rad));
        points[i].y = (LONG)(centerY + x * sin(rad) + y * cos(rad));
    }

    // 绘制旋转后的矩形
    setfillcolor(color);
    setlinecolor(color);
    fillpolygon(points, 4);
}

// 障碍物实现
Obstacle::Obstacle(float x, float y, ObstacleType type)
    : x(x), y(y), type(type), vx(0), vy(0), animationTimer(0),
    rotationAngle(0), active(true), lifetime(30.0f), damage(1),
    amplitude(0), frequency(0), startY(y) {

    switch (type) {
    case SPIKE:
        width = 20;
        height = 40;
        damage = 1;
        break;
    case FIREBALL:
        width = 30;
        height = 30;
        vy = 200.0f; // 下降速度
        damage = 2;
        lifetime = 10.0f;
        break;
    case LASER:
        width = 100;
        height = 8;
        vx = 150.0f; // 水平移动速度
        damage = 3;
        break;
    case ROTATING_SAW:
        width = 40;
        height = 40;
        damage = 2;
        break;
    case FALLING_ROCK:
        width = 25;
        height = 25;
        vy = 100.0f + rand() % 100; // 随机下降速度
        damage = 1;
        break;
    case MOVING_WALL:
        width = 20;
        height = 100;
        vx = 80.0f;
        damage = 1;
        break;
    }
}

void Obstacle::update(float deltaTime, float worldSpeed) {
    if (!active) return;

    animationTimer += deltaTime;
    lifetime -= deltaTime;

    // 世界移动
    y += worldSpeed * deltaTime;

    switch (type) {
    case FIREBALL:
        y += vy * deltaTime;
        break;

    case LASER:
        x += vx * deltaTime;
        // 激光边界检查
        if (x < -width || x > 1200 + width) {
            vx = -vx;
        }
        break;

    case ROTATING_SAW:
        rotationAngle += 360.0f * deltaTime; // 每秒旋转360度
        if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;
        break;

    case FALLING_ROCK:
        y += vy * deltaTime;
        // 添加重力效果
        vy += 200.0f * deltaTime;
        break;

    case MOVING_WALL:
        x += vx * deltaTime;
        // 正弦波动
        y = startY + amplitude * sin(frequency * animationTimer);
        break;

    case SPIKE:
        // 尖刺不移动
        break;
    }

    // 检查是否需要移除
    if (lifetime <= 0 || y > 1000) {
        active = false;
    }
}

bool Obstacle::checkCollision(float playerX, float playerY, float playerWidth, float playerHeight) const {
    if (!active) return false;

    return (playerX < x + width && playerX + playerWidth > x &&
        playerY < y + height && playerY + playerHeight > y);
}

bool Obstacle::shouldRemove() const {
    return !active || lifetime <= 0;
}

void Obstacle::drawWithOffset(float offsetX, float offsetY) const {
    if (!active) return;

    float drawX = x + offsetX;
    float drawY = y + offsetY;

    switch (type) {
    case SPIKE: {
        // 绘制尖刺
        setfillcolor(RGB(150, 150, 150));
        solidrectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));
        // 绘制尖刺顶部
        setfillcolor(RGB(200, 50, 50));
        POINT spikes[3] = {
            {(int)(drawX + width / 2), (int)drawY},
            {(int)drawX, (int)(drawY + height / 3)},
            {(int)(drawX + width), (int)(drawY + height / 3)}
        };
        fillpolygon(spikes, 3);
        break;
    }
    case FIREBALL:
        // 绘制火球
        setfillcolor(RGB(255, 100, 0));
        solidcircle((int)(drawX + width / 2), (int)(drawY + height / 2), (int)(width / 2));
        setfillcolor(RGB(255, 150, 0));
        solidcircle((int)(drawX + width / 2), (int)(drawY + height / 2), (int)(width / 3));
        break;

    case LASER:
        // 绘制激光
        setfillcolor(RGB(255, 0, 0));
        solidrectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));
        break;

    case ROTATING_SAW:
        // 绘制旋转锯
        drawRotatedRect(drawX + width / 2, drawY + height / 2, width, height,
            rotationAngle, RGB(180, 180, 180));
        break;

    case FALLING_ROCK:
        // 绘制落石
        setfillcolor(RGB(100, 80, 60));
        solidcircle((int)(drawX + width / 2), (int)(drawY + height / 2), (int)(width / 2));
        break;

    case MOVING_WALL:
        // 绘制移动墙壁
        setfillcolor(RGB(120, 120, 120));
        solidrectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));
        break;
    }
}

void Obstacle::draw(float offsetX, float offsetY) const {
    drawWithOffset(offsetX, offsetY);
}

// 金币实现
Coin::Coin(float x, float y, int value)
    : x(x), y(y), animationTimer(0), bobOffset(0), rotationAngle(0),
    collected(false), value(value), magnetRadius(100.0f),
    beingMagnetized(false), magnetSpeed(200.0f) {
}

void Coin::update(float deltaTime, float worldSpeed) {
    if (collected) return;

    animationTimer += deltaTime;

    // 世界移动
    y += worldSpeed * deltaTime;

    // 浮动效果
    bobOffset = sin(animationTimer * 3.0f) * 3.0f;

    // 旋转效果
    rotationAngle += 180.0f * deltaTime;
    if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;
}

void Coin::applyMagnetism(float playerX, float playerY, float magnetRadius, float deltaTime) {
    if (collected) return;

    float dx = playerX - x;
    float dy = playerY - y;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance < magnetRadius) {
        beingMagnetized = true;

        // 计算磁化方向
        float dirX = dx / distance;
        float dirY = dy / distance;

        // 应用磁化移动
        x += dirX * magnetSpeed * deltaTime;
        y += dirY * magnetSpeed * deltaTime;
    }
}

void Coin::drawWithOffset(float offsetX, float offsetY) const {
    if (collected) return;

    float drawX = x + offsetX;
    float drawY = y + offsetY + bobOffset;

    // 绘制金币外层光晕
    setfillcolor(RGB(255, 215, 0));
    solidcircle((int)drawX, (int)drawY, 16);

    // 绘制金币主体
    setfillcolor(RGB(255, 223, 0));
    solidcircle((int)drawX, (int)drawY, 12);

    // 绘制金币内层
    setfillcolor(RGB(255, 255, 100));
    solidcircle((int)drawX, (int)drawY, 8);

    // 绘制金币中心图案
    setfillcolor(RGB(255, 215, 0));
    solidrectangle((int)drawX - 4, (int)drawY - 4, (int)drawX + 4, (int)drawY + 4);

    // 绘制十字纹理
    setlinecolor(RGB(255, 255, 150));
    setlinestyle(PS_SOLID, 2);
    line((int)drawX - 6, (int)drawY, (int)drawX + 6, (int)drawY);
    line((int)drawX, (int)drawY - 6, (int)drawX, (int)drawY + 6);

    // 绘制边缘装饰
    setlinecolor(RGB(200, 170, 0));
    setlinestyle(PS_SOLID, 1);
    circle((int)drawX, (int)drawY, 12);
    circle((int)drawX, (int)drawY, 8);
}

void Coin::draw(float offsetX, float offsetY) const {
    drawWithOffset(offsetX, offsetY);
}

bool Coin::checkCollision(float playerX, float playerY, float playerWidth, float playerHeight) {
    // 如果已经被收集，直接返回false
    if (collected) return false;

    float coinCenterX = x;
    float coinCenterY = y + bobOffset;
    float coinRadius = 12.0f;

    // 检查圆形与矩形的碰撞
    float closestX = std::max(playerX, std::min(coinCenterX, playerX + playerWidth));
    float closestY = std::max(playerY, std::min(coinCenterY, playerY + playerHeight));

    float distance = sqrt((coinCenterX - closestX) * (coinCenterX - closestX) +
        (coinCenterY - closestY) * (coinCenterY - closestY));

    return distance < coinRadius;
}