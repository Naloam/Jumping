#pragma once
#include <graphics.h>
#include <windows.h>
#include <vector>
#include <memory>

enum PlatformType {
    NORMAL,
    MOVING,
    BREAKABLE,
    SPRING      // 弹簧平台
};

enum ItemType {
    NONE,
    SPEED_BOOST,    // 加速道具
    SHIELD          // 护盾道具
};

struct Item {
    float x, y;
    ItemType type;
    float animationTimer;
    bool collected;

    Item(float x, float y, ItemType type)
        : x(x), y(y), type(type), animationTimer(0), collected(false) {
    }
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

    // 易碎平台相关
    bool isBroken;
    float breakTimer;
    int hitCount;

    // 弹簧平台相关
    float springCompression;
    bool wasTriggered;

    // 道具相关 - 使用智能指针
    // Platform类的Item管理，当Platform对象被复制时，多个Platform对象可能指向同一个Item，collectItem()返回原始指针但不清空item成员：导致潜在的重复删除，最终可能导致内存泄漏
    std::unique_ptr<Item> item;

    // 内部绘制方法
    void drawItem(const Item* item, float offsetX, float offsetY) const;

public:
    Platform(float x, float y, float width = 100, float height = 20, PlatformType type = NORMAL);

    // 确保正确的复制语义
    Platform(const Platform& other);
    Platform& operator=(const Platform& other);

    // 移动语义
    Platform(Platform&& other) noexcept;
    Platform& operator=(Platform&& other) noexcept;

    ~Platform() = default; // 智能指针自动管理内存

    void update(float deltaTime);
    void draw() const;
    void drawWithOffset(float offsetX, float offsetY) const;

    // 平台交互
    bool checkCollision(float playerX, float playerY, float playerWidth, float playerHeight);
    float handleCollision(float playerX, float playerY, float playerWidth, float playerHeight, float& playerVY);
    void triggerBreak();
    void triggerSpring();

    // Y轴移动方法（用于世界上升）
    void moveY(float deltaY) {
        y += deltaY;
        if (item) {
            item->y += deltaY;
        }
    }

    // 访问器
    float getX() const { return x; }
    float getY() const { return y; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }
    PlatformType getType() const { return type; }
    bool isBrokenPlatform() const { return isBroken; }
    Item* getItem() const { return item.get(); }

    // 道具管理
    void spawnItem(ItemType itemType);
    Item* collectItem(); // 返回Item但不转移所有权
    bool hasCollectedItem() const { return item && item->collected; }
};