#include "Player.h"
#include "Theme.h"
#include <graphics.h>
#include <windows.h>
#include <cmath>
#include <cstdlib>

// 静态常量定义
const float Player::GRAVITY = 500.0f;
const float Player::JUMP_SPEED = -300.0f;
const float Player::MOVE_SPEED = 200.0f;
const float Player::FRICTION = 0.8f;
const float Player::MAX_FALL_SPEED = 400.0f;

Player::Player(float x, float y)
    : x(x), y(y), vx(0), vy(0), width(30), height(30),
    onGround(false), wasOnGround(false), jumpCount(0), maxJumps(2),
    currentColor(Theme::PLAYER_MAIN), pulseTimer(0.0f),
    speedBoostTimer(0.0f), shieldTimer(0.0f), hasShieldActive(false),
    comboCount(0), comboTimer(0.0f), lastLandingTime(0.0f),
    shakeIntensity(0.0f), shakeTimer(0.0f) {
}

void Player::update(float deltaTime) {
    pulseTimer += deltaTime;

    // 更新道具效果计时器
    if (speedBoostTimer > 0) {
        speedBoostTimer -= deltaTime;
    }

    if (shieldTimer > 0) {
        shieldTimer -= deltaTime;
        if (shieldTimer <= 0) {
            hasShieldActive = false;
        }
    }

    // 更新连击计时器
    if (comboTimer > 0) {
        comboTimer -= deltaTime;
        if (comboTimer <= 0) {
            resetCombo();
        }
    }

    // 更新屏幕震动
    if (shakeTimer > 0) {
        shakeTimer -= deltaTime;
        shakeIntensity *= 0.95f;  // 震动衰减
        if (shakeTimer <= 0) {
            shakeIntensity = 0;
        }
    }

    // 重力
    if (!onGround) {
        vy += GRAVITY * deltaTime;
        if (vy > MAX_FALL_SPEED) {
            vy = MAX_FALL_SPEED;
        }
    }

    // 摩擦力
    vx *= FRICTION;

    // 更新位置
    x += vx * deltaTime;
    y += vy * deltaTime;

    // 更新粒子
    updateParticles(deltaTime);

    // 重置地面状态
    onGround = false;
}

void Player::setOnGround(bool grounded) {
    wasOnGround = onGround;
    onGround = grounded;

    if (onGround && !wasOnGround) {
        jumpCount = 0;
        createLandingParticles();
        addCombo();

        // 着陆震动
        addScreenShake(2.0f);

        lastLandingTime = pulseTimer;
    }
}

void Player::getShakeOffset(float& shakeX, float& shakeY) const {
    if (shakeIntensity > 0) {
        auto shakeVec = AnimationUtils::shake(shakeIntensity, pulseTimer);
        shakeX = shakeVec.first;
        shakeY = shakeVec.second;
    }
    else {
        shakeX = shakeY = 0;
    }
}

void Player::moveLeft() {
    float speed = MOVE_SPEED;
    if (speedBoostTimer > 0) speed *= 1.5f;  // 加速效果
    vx -= speed;

    // 添加移动粒子效果
    if (speedBoostTimer > 0 && rand() % 3 == 0) {
        createSpeedParticles();
    }
}

void Player::moveRight() {
    float speed = MOVE_SPEED;
    if (speedBoostTimer > 0) speed *= 1.5f;  // 加速效果
    vx += speed;

    // 添加移动粒子效果
    if (speedBoostTimer > 0 && rand() % 3 == 0) {
        createSpeedParticles();
    }
}

void Player::jump() {
    if (onGround || jumpCount < maxJumps) {
        vy = JUMP_SPEED;
        onGround = false;
        jumpCount++;

        createJumpParticles();
        addScreenShake(1.5f);

        // 二段跳的特殊效果
        if (jumpCount > 1) {
            createDoubleJumpParticles();
        }
    }
}

void Player::applySpeedBoost() {
    speedBoostTimer = 5.0f;  // 5秒加速效果
    // 添加特效爆发
    createSpeedBoostEffect();
}

void Player::applyShield() {
    hasShieldActive = true;
    shieldTimer = 10.0f;  // 10秒护盾效果
    // 添加护盾激活特效
    createShieldActivateEffect();
}

void Player::addCombo() {
    comboCount++;
    comboTimer = 3.0f;  // 3秒内必须继续跳跃才能保持连击

    // 连击特效
    if (comboCount > 3) {
        createComboEffect();
    }
}

void Player::resetCombo() {
    comboCount = 0;
    comboTimer = 0.0f;
}

void Player::createJumpParticles() {
    for (int i = 0; i < 8; i++) {
        float angle = (float)i / 8.0f * 6.28f;  // 2π
        float speed = 50.0f + rand() % 50;
        float px = x + width / 2;
        float py = y + height;

        particles.push_back(Particle(
            px, py,
            cos(angle) * speed, sin(angle) * speed - 20,
            0.8f + (rand() % 40) * 0.01f,
            Theme::PARTICLE_JUMP
        ));
    }
}

void Player::createLandingParticles() {
    for (int i = 0; i < 5; i++) {
        float px = x + (rand() % (int)width);
        float py = y + height;
        float vx = (rand() % 100 - 50) * 0.5f;

        particles.push_back(Particle(
            px, py,
            vx, -30.0f - rand() % 20,
            1.0f + (rand() % 30) * 0.01f,
            Theme::PARTICLE_LAND
        ));
    }
}

// 新增：二段跳粒子效果
void Player::createDoubleJumpParticles() {
    for (int i = 0; i < 12; i++) {
        float angle = (float)i / 12.0f * 6.28f;
        float speed = 80.0f + rand() % 40;
        float px = x + width / 2;
        float py = y + height / 2;

        particles.push_back(Particle(
            px, py,
            cos(angle) * speed, sin(angle) * speed,
            1.0f + (rand() % 50) * 0.01f,
            Theme::ACCENT
        ));
    }
}

// 新增：速度粒子效果
void Player::createSpeedParticles() {
    for (int i = 0; i < 3; i++) {
        float px = x + rand() % (int)width;
        float py = y + rand() % (int)height;

        // 根据玩家移动方向创建相反方向的粒子效果
        float direction = (this->vx > 0) ? -1.0f : 1.0f;
        float particleSpeed = direction * (50 + rand() % 30);

        particles.push_back(Particle(
            px, py,
            particleSpeed, (rand() % 20 - 10) * 0.5f,
            0.5f + (rand() % 30) * 0.01f,
            Theme::PARTICLE_SPEED
        ));
    }
}

// 新增：速度提升激活特效
void Player::createSpeedBoostEffect() {
    for (int i = 0; i < 16; i++) {
        float angle = (float)i / 16.0f * 6.28f;
        float speed = 100.0f + rand() % 50;
        float px = x + width / 2;
        float py = y + height / 2;

        particles.push_back(Particle(
            px, py,
            cos(angle) * speed, sin(angle) * speed,
            1.5f,
            Theme::ITEM_SPEED_PARTICLE
        ));
    }
}

// 新增：护盾激活特效
void Player::createShieldActivateEffect() {
    for (int i = 0; i < 20; i++) {
        float angle = (float)i / 20.0f * 6.28f;
        float radius = 25 + rand() % 10;
        float px = x + width / 2 + cos(angle) * radius;
        float py = y + height / 2 + sin(angle) * radius;

        particles.push_back(Particle(
            px, py,
            cos(angle) * 20, sin(angle) * 20,
            2.0f,
            Theme::ITEM_SHIELD_PARTICLE
        ));
    }
}

// 新增：连击特效
void Player::createComboEffect() {
    COLORREF comboColor = DrawUtils::getComboColor(comboCount);

    for (int i = 0; i < 6; i++) {
        float angle = (float)i / 6.0f * 6.28f;
        float px = x + width / 2 + cos(angle) * 15;
        float py = y + height / 2 + sin(angle) * 15;

        particles.push_back(Particle(
            px, py,
            cos(angle) * 30, sin(angle) * 30 - 20,
            1.0f,
            comboColor
        ));
    }
}

void Player::updateParticles(float deltaTime) {
    for (auto it = particles.begin(); it != particles.end();) {
        it->x += it->vx * deltaTime;
        it->y += it->vy * deltaTime;
        it->vy += 200.0f * deltaTime;  // 重力
        it->life -= deltaTime;

        if (it->life <= 0) {
            it = particles.erase(it);
        }
        else {
            ++it;
        }
    }
}

void Player::drawParticles(float offsetX, float offsetY) {
    for (const auto& particle : particles) {
        float alpha = particle.life / particle.maxLife;
        if (alpha > 0) {
            // 使用Theme.cpp中的drawParticle函数
            DrawUtils::drawParticle(particle.x + offsetX, particle.y + offsetY,
                3.0f * alpha, particle.color, alpha);

            // 为特殊粒子添加光晕效果
            if (particle.color == Theme::ITEM_SPEED_PARTICLE ||
                particle.color == Theme::ITEM_SHIELD_PARTICLE) {
                DrawUtils::drawSparkle(particle.x + offsetX, particle.y + offsetY,
                    6.0f * alpha, particle.color, pulseTimer);
            }
        }
    }
}

void Player::addScreenShake(float intensity) {
    shakeIntensity = std::max(shakeIntensity, intensity);
    shakeTimer = 0.3f;
}

void Player::draw() {
    drawWithOffset(0, 0);
}

void Player::drawWithOffset(float offsetX, float offsetY) {
    float drawX = x + offsetX;
    float drawY = y + offsetY;

    // 使用AnimationUtils进行脉动效果
    float pulse = AnimationUtils::pulse(pulseTimer, 2.0f);

    // 道具效果增强
    if (speedBoostTimer > 0) {
        // 使用Theme.cpp中的drawSpeedEffect
        DrawUtils::drawSpeedEffect(drawX, drawY, width, height,
            AnimationUtils::pulse(pulseTimer, 4.0f));

        // 速度轨迹效果
        for (int i = 1; i <= 3; i++) {
            float trailAlpha = 0.3f / i;
            float trailX = drawX - vx * 0.01f * i;
            float trailY = drawY - vy * 0.01f * i;

            COLORREF trailColor = DrawUtils::blendColor(Theme::PLAYER_SPEED_EFFECT,
                RGB(255, 255, 255), trailAlpha);
            setfillcolor(trailColor);
            solidrectangle((int)trailX, (int)trailY,
                (int)(trailX + width), (int)(trailY + height));
        }
    }

    if (hasShieldActive) {
        // 使用Theme.cpp中的drawShieldEffect
        DrawUtils::drawShieldEffect(drawX + width / 2, drawY + height / 2,
            25, AnimationUtils::pulse(pulseTimer, 3.0f));

        // 额外的护盾光环
        float shieldPulse = AnimationUtils::pulse(pulseTimer, 2.0f);
        DrawUtils::drawGlowCircle((int)(drawX + width / 2), (int)(drawY + height / 2),
            (int)(20 + 5 * shieldPulse), Theme::SHIELD_GLOW, 0.4f);
    }

    // 绘制玩家光晕（根据状态）
    COLORREF glowColor = Theme::PLAYER_MAIN;
    float glowIntensity = 0.3f;

    if (speedBoostTimer > 0) {
        glowColor = Theme::SPEED_GLOW;
        glowIntensity = 0.6f;
    }
    if (hasShieldActive) {
        glowColor = Theme::SHIELD_GLOW;
        glowIntensity = 0.5f;
    }

    // 连击光晕
    if (comboCount > 5) {
        glowColor = DrawUtils::getComboColor(comboCount);
        glowIntensity = 0.4f + 0.3f * AnimationUtils::pulse(pulseTimer, 5.0f);
    }

    DrawUtils::drawGlowRect((int)drawX, (int)drawY, (int)width, (int)height,
        glowColor, glowIntensity);

    // 绘制阴影
    setfillcolor(RGB(50, 50, 50));
    solidrectangle((int)(drawX + 2), (int)(drawY + 2),
        (int)(drawX + width + 2), (int)(drawY + height + 2));

    // 绘制玩家主体（使用颜色动画）
    COLORREF playerColor = Theme::PLAYER_MAIN;

    if (speedBoostTimer > 0) {
        playerColor = AnimationUtils::colorPulse(Theme::PLAYER_MAIN,
            Theme::PLAYER_SPEED_EFFECT, pulseTimer, 4.0f);
    }
    if (hasShieldActive) {
        playerColor = AnimationUtils::colorPulse(Theme::PLAYER_MAIN,
            Theme::PLAYER_SHIELD_EFFECT, pulseTimer, 3.0f);
    }

    setfillcolor(playerColor);
    solidrectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));

    // 绘制高光
    setfillcolor(RGB(255, 255, 255));
    solidrectangle((int)(drawX + 2), (int)(drawY + 2),
        (int)(drawX + width - 2), (int)(drawY + 8));

    // 绘制边框
    setlinecolor(RGB(255, 255, 255));
    setlinestyle(PS_SOLID, 2);
    rectangle((int)drawX, (int)drawY, (int)(drawX + width), (int)(drawY + height));

    // 绘制粒子效果
    drawParticles(offsetX, offsetY);
}

void Player::handleInput() {
    // 移动输入
    if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) {
        moveLeft();
    }
    if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000) {
        moveRight();
    }

    // 跳跃输入
    static bool spacePressed = false;
    bool spaceDown = GetAsyncKeyState(VK_SPACE) & 0x8000;

    if (spaceDown && !spacePressed) {
        jump();
    }
    spacePressed = spaceDown;
}

void Player::reset() {
    x = 100;
    y = 400;
    vx = 0;
    vy = 0;
    onGround = false;
    wasOnGround = false;
    jumpCount = 0;
    pulseTimer = 0.0f;

    // 重置道具效果
    speedBoostTimer = 0.0f;
    shieldTimer = 0.0f;
    hasShieldActive = false;

    // 重置连击
    resetCombo();

    // 清除粒子
    particles.clear();

    // 重置震动
    shakeIntensity = 0.0f;
    shakeTimer = 0.0f;
}

void Player::checkBounds(int windowWidth, int windowHeight) {
    // 左右边界
    if (x < 0) {
        x = 0;
        vx = 0;
    }
    if (x + width > windowWidth) {
        x = windowWidth - width;
        vx = 0;
    }

    // 上边界
    if (y < -height) {
        y = -height;
        vy = 0;
    }
}