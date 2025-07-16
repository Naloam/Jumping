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

// 扩展道具类型
enum ItemType {
    NONE,
    SPEED_BOOST,        // 加速道具
    SHIELD,             // 护盾道具
    DOUBLE_JUMP,        // 额外跳跃道具
    SLOW_TIME,          // 时间减缓道具
    MAGNETIC_FIELD,     // 磁场道具（吸引金币）
    HEALTH_BOOST,       // 生命值恢复道具
    FREEZE_OBSTACLES,   // 冻结障碍物道具
    COIN,               // 金币
    INVINCIBILITY    // 无敌道具
};

// 新增：障碍物类型
enum ObstacleType {
    SPIKE,              // 尖刺
    FIREBALL,           // 火球（从天而降）
    LASER,              // 激光（水平移动）
    ROTATING_SAW,       // 旋转锯（固定位置）
    FALLING_ROCK,       // 落石
    MOVING_WALL         // 移动墙壁
};

// 新增：障碍物类
class Obstacle {
private:
    float x, y;
    float width, height;
    ObstacleType type;
    float vx, vy;           // 速度
    float animationTimer;
    float rotationAngle;    // 旋转角度
    bool active;            // 是否活跃
    float lifetime;         // 生命周期
    float damage;           // 伤害值

    // 特殊属性
    float amplitude;        // 振幅（用于正弦波动）
    float frequency;        // 频率
    float startY;           // 起始Y位置

public:
    Obstacle(float x, float y, ObstacleType type);

    void update(float deltaTime, float worldSpeed);
    void draw(float offsetX, float offsetY) const;
    void drawWithOffset(float offsetX, float offsetY) const;

    // 碰撞检测
    bool checkCollision(float playerX, float playerY, float playerWidth, float playerHeight) const;

    // 访问器
    float getX() const { return x; }
    float getY() const { return y; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }
    ObstacleType getType() const { return type; }
    bool isActive() const { return active; }
    float getDamage() const { return damage; }

    // 世界移动
    void moveY(float deltaY) { y += deltaY; }

    // 生命周期管理
    bool shouldRemove() const;
};

// 新增：金币类
class Coin {
private:
    float x, y;
    float animationTimer;
    float bobOffset;        // 浮动偏移
    float rotationAngle;    // 旋转角度
    bool collected;
    int value;              // 金币价值
    float magnetRadius;     // 磁场半径
    bool beingMagnetized;   // 是否被磁化
    float magnetSpeed;      // 磁化速度

public:
    Coin(float x, float y, int value = 10);

    void update(float deltaTime, float worldSpeed);
    void draw(float offsetX, float offsetY) const;
    void drawWithOffset(float offsetX, float offsetY) const;

    // 磁场效果
    void applyMagnetism(float playerX, float playerY, float magnetRadius, float deltaTime);

    // 碰撞检测
    bool checkCollision(float playerX, float playerY, float playerWidth, float playerHeight);

    // 访问器
    float getX() const { return x; }
    float getY() const { return y; }
    int getValue() const { return value; }
    bool isCollected() const { return collected; }

    // 世界移动
    void moveY(float deltaY) { y += deltaY; }

    // 收集
    void collect() { collected = true; }
};

// 修改Item结构体，增加更多属性
struct Item {
    float x, y;
    ItemType type;
    float animationTimer;
    bool collected;
    float effectDuration;   // 效果持续时间
    float effectStrength;   // 效果强度
    int value;              // 道具价值（对于金币）

    Item(float x, float y, ItemType type, float duration = 5.0f, int value = 0)
        : x(x), y(y), type(type), animationTimer(0), collected(false),
        effectDuration(duration), effectStrength(1.0f), value(value) {
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
    void createSpringParticles(float centerX, float centerY);

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