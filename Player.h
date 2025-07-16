#pragma once
#include <graphics.h>
#include <windows.h>
#include <vector>

struct Particle {
    float x, y;
    float vx, vy;
    float life;
    float maxLife;
    COLORREF color;

    Particle(float x, float y, float vx, float vy, float life, COLORREF color)
        : x(x), y(y), vx(vx), vy(vy), life(life), maxLife(life), color(color) {
    }
};

class Player {
private:
    float x, y;
    float vx, vy;
    float width, height;
    bool onGround;
    bool wasOnGround;
    int jumpCount;
    int maxJumps;

    // 风格相关
    COLORREF currentColor;
    float pulseTimer;

    // 道具效果
    float speedBoostTimer;
    float shieldTimer;
    bool hasShieldActive;
    bool shieldUsed; // 标记护盾是否已使用过

    float doubleJumpTimer;          // 额外跳跃时间
    float slowTimeTimer;            // 时间减缓时间
    float magneticFieldTimer;       // 磁场效果时间
    float freezeObstaclesTimer;     // 冻结障碍物时间

    // 新增：无敌道具效果
    float invincibilityTimer;      // 无敌时间
    bool hasInvincibility;         // 是否有无敌效果

    // 新增：玩家状态
    int health;                     // 生命值
    int maxHealth;                  // 最大生命值
    float invulnerabilityTimer;     // 无敌时间
    int coins;                      // 金币数量

    // 新增：道具效果强度
    float timeScaleFactor;          // 时间缩放因子
    float magnetRadius;             // 磁场半径
    bool hasDoubleJump;             // 是否有额外跳跃
    bool obstaclesFrozen;           // 障碍物是否被冻结

    // 连击系统
    int comboCount;
    float comboTimer;
    float lastLandingTime;
    // Combo追踪变量
    float lastPlatformY;        // 上一个平台的Y坐标
    bool hasValidLastPlatform;  // 是否有有效的上一个平台记录
    float currentPlatformY;     // 当前平台的Y坐标

    // 粒子系统
    std::vector<Particle> particles;

    // 屏幕震动
    float shakeIntensity;
    float shakeTimer;

    // 分数系统
    int bonusScore;    // 道具加分
    int itemsCollected; // 收集的道具数量

    // 物理常量
    static const float GRAVITY;
    static const float JUMP_SPEED;
    static const float MOVE_SPEED;
    static const float FRICTION;
    static const float MAX_FALL_SPEED;

public:
    Player(float x = 100, float y = 100);

    void update(float deltaTime);
    void draw();
    void drawWithOffset(float offsetX, float offsetY);
    void handleInput();

    // 位置和碰撞
    float getX() const { return x; }
    float getY() const { return y; }
    float getVX() const { return vx; }
    float getVY() const { return vy; }
    void setVY(float newVY) { vy = newVY; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }

    void setPosition(float newX, float newY) { x = newX; y = newY; }
    void setOnGround(bool grounded);
    bool isOnGround() const { return onGround; }

    // 获取屏幕震动偏移
    void getShakeOffset(float& shakeX, float& shakeY) const;

    // 移动和跳跃
    void jump();
    void moveLeft();
    void moveRight();

    // 道具效果
    void applySpeedBoost();
    void applyShield();
    void consumeShield();
    bool canTakeDamage() const { return !hasShieldActive && !isDead() && !hasInvincibilityActive(); }
    void applyDoubleJump();
    void applySlowTime();
    void applyMagneticField();
    void applyHealthBoost();
    void applyFreezeObstacles();
    void collectCoin(int value);
    void applyInvincibility();
    bool hasInvincibilityActive() const { return invincibilityTimer > 0; }

    // 新增：状态检查方法
    bool hasDoubleJumpActive() const { return doubleJumpTimer > 0; }
    bool hasSlowTimeActive() const { return slowTimeTimer > 0; }
    bool hasMagneticFieldActive() const { return magneticFieldTimer > 0; }
    bool hasObstaclesFrozen() const { return obstaclesFrozen; }
    bool isInvulnerable() const { return invulnerabilityTimer > 0; }

    // 新增：访问器
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    int getCoins() const { return coins; }
    float getTimeScaleFactor() const { return timeScaleFactor; }
    float getMagnetRadius() const { return magnetRadius; }

    // 新增：伤害系统
    void takeDamage(int damage);
    void heal(int amount);

    // 连击系统
    int getComboCount() const { return comboCount; }
    void addCombo();
    void resetCombo();
    void updateComboSystem(float platformY);
    bool shouldIncrementCombo(float newPlatformY);
    void resetComboSystem();

    // 新增：分数系统
    void addBonusScore(int points);
    int getBonusScore() const { return bonusScore; }
    int getItemsCollected() const { return itemsCollected; }
    void incrementItemsCollected() { itemsCollected++; }

    // 粒子效果
    void createJumpParticles();
    void createLandingParticles();
    void createDoubleJumpParticles();        // 新增
    void createSpeedParticles();             // 新增
    void createSpeedBoostEffect();           // 新增
    void createShieldActivateEffect();       // 新增
    void createComboEffect();                // 新增
    void createInvincibilityEffect();        // 新增：无敌激活特效
    void createRespawnEffect() { createShieldActivateEffect(); }
    void updateParticles(float deltaTime);
    void drawParticles(float offsetX, float offsetY);

    // 屏幕震动
    void addScreenShake(float intensity);

    // 死亡相关方法
    bool isDead() const;

    // 重置
    void reset();
    void checkBounds(int windowWidth, int windowHeight);

    // 道具状态查询
    bool hasSpeedBoost() const { return speedBoostTimer > 0; }
    bool hasShield() const { return hasShieldActive; }
    float getSpeedBoostTimeLeft() const { return speedBoostTimer; }
    float getShieldTimeLeft() const { return shieldTimer; }
};