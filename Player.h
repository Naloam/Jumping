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

    // 连击系统
    int comboCount;
    float comboTimer;
    float lastLandingTime;

    // 粒子系统
    std::vector<Particle> particles;

    // 屏幕震动
    float shakeIntensity;
    float shakeTimer;

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
    bool canTakeDamage() const { return !hasShieldActive; }

    // 连击系统
    int getComboCount() const { return comboCount; }
    void addCombo();
    void resetCombo();

    // 粒子效果
    void createJumpParticles();
    void createLandingParticles();
    void createDoubleJumpParticles();        // 新增
    void createSpeedParticles();             // 新增
    void createSpeedBoostEffect();           // 新增
    void createShieldActivateEffect();       // 新增
    void createComboEffect();                // 新增
    void updateParticles(float deltaTime);
    void drawParticles(float offsetX, float offsetY);

    // 屏幕震动
    void addScreenShake(float intensity);

    // 重置
    void reset();
    void checkBounds(int windowWidth, int windowHeight);

    // 道具状态查询
    bool hasSpeedBoost() const { return speedBoostTimer > 0; }
    bool hasShield() const { return hasShieldActive; }
    float getSpeedBoostTimeLeft() const { return speedBoostTimer; }
    float getShieldTimeLeft() const { return shieldTimer; }
};