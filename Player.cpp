#include "Player.h"
#include "Theme.h"
#include "AudioManager.h"
#include <graphics.h>
#include <windows.h>
#include <cmath>
#include <cstdlib>

// 静态常量定义
const float Player::GRAVITY = 600.0f;
const float Player::JUMP_SPEED = -400.0f;
const float Player::MOVE_SPEED = 300.0f;
const float Player::FRICTION = 0.8f;
const float Player::MAX_FALL_SPEED = 500.0f;

Player::Player(float x, float y)
    : x(x), y(y), vx(0), vy(0), width(30), height(30),
    onGround(false), wasOnGround(false), jumpCount(0), maxJumps(2),
    currentColor(Theme::PLAYER_MAIN), pulseTimer(0.0f),
    speedBoostTimer(0.0f), shieldTimer(0.0f), hasShieldActive(false), shieldUsed(false),
    doubleJumpTimer(0.0f), slowTimeTimer(0.0f), magneticFieldTimer(0.0f), freezeObstaclesTimer(0.0f),
    invincibilityTimer(0.0f), hasInvincibility(false),
    health(3), maxHealth(3), invulnerabilityTimer(0.0f), coins(0),
    timeScaleFactor(1.0f), magnetRadius(0.0f), hasDoubleJump(false), obstaclesFrozen(false),
    comboCount(0), comboTimer(0.0f), lastLandingTime(0.0f),
    lastPlatformY(0.0f), hasValidLastPlatform(false), currentPlatformY(0.0f), 
    shakeIntensity(0.0f), shakeTimer(0.0f),
    bonusScore(0), itemsCollected(0) {
}

void Player::update(float deltaTime) {
    // 应用时间缩放
    float effectiveDeltaTime = deltaTime * timeScaleFactor;

    pulseTimer += effectiveDeltaTime;

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

    // 更新新道具效果
    if (doubleJumpTimer > 0) {
        doubleJumpTimer -= deltaTime;
        if (doubleJumpTimer <= 0) {
            hasDoubleJump = false;
            maxJumps = 2;  // 恢复到2段跳
        }
    }

    if (slowTimeTimer > 0) {
        slowTimeTimer -= deltaTime;
        if (slowTimeTimer <= 0) {
            timeScaleFactor = 1.0f;  // 恢复正常时间
        }
    }

    if (magneticFieldTimer > 0) {
        magneticFieldTimer -= deltaTime;
        if (magneticFieldTimer <= 0) {
            magnetRadius = 0.0f;
        }
    }

    if (freezeObstaclesTimer > 0) {
        freezeObstaclesTimer -= deltaTime;
        if (freezeObstaclesTimer <= 0) {
            obstaclesFrozen = false;
        }
    }

    if (invulnerabilityTimer > 0) {
        invulnerabilityTimer -= deltaTime;
    }

    // 更新无敌计时器
    if (invincibilityTimer > 0) {
        invincibilityTimer -= deltaTime;
        if (invincibilityTimer <= 0) {
            hasInvincibility = false;
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

    // 重力应用
    if (!onGround) {
        vy += GRAVITY * effectiveDeltaTime;
        if (vy > MAX_FALL_SPEED) {
            vy = MAX_FALL_SPEED;
        }
    }
    else {
        // 只有当垂直速度向下时才停止垂直移动
        if (vy > 0) {
            vy = 0;
        }
        // 如果vy < 0（向上运动），保持不变，让玩家可以弹起或跳跃
    }

    // 摩擦力
    vx *= FRICTION;

    // 更新位置
    x += vx * effectiveDeltaTime;
    y += vy * effectiveDeltaTime;

    // 如果玩家开始向上移动，应该离开地面
    if (vy < 0) {
        onGround = false;
    }

    // 更新粒子
    updateParticles(effectiveDeltaTime);
}

void Player::setOnGround(bool grounded) {
    wasOnGround = onGround;
    onGround = grounded;

    if (onGround && !wasOnGround && vy > 0) {
        jumpCount = 0;

        // 播放着陆音效
        AudioManager::getInstance().playSound(SoundType::LAND, false);

        createLandingParticles();

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

        // 播放跳跃音效
        try {
            AudioManager::getInstance().playSound(SoundType::JUMP, false);
        }catch(...){}
        createJumpParticles();
        addScreenShake(1.5f);

        // 跳跃时重置连击计时器（保持连击）
        if (comboCount > 0) {
            comboTimer = 3.0f;  // 重置连击计时器
        }

        // 二段跳的特殊效果
        if (jumpCount > 1) {
            createDoubleJumpParticles();
        }
    }
}

void Player::updateComboSystem(float platformY) {
    currentPlatformY = platformY;

    // 检查是否应该增加combo
    if (shouldIncrementCombo(platformY)) {
        comboCount++;
        comboTimer = 3.0f;  // 3秒内必须继续跳跃才能保持连击

        // 连击特效
        if (comboCount > 3) {
            AudioManager::getInstance().playSound(SoundType::COMBO_SOUND, false);
            createComboEffect();
        }
    }

    // 更新上一个平台记录
    lastPlatformY = platformY;
    hasValidLastPlatform = true;
}

// 判断是否应该增加combo
bool Player::shouldIncrementCombo(float newPlatformY) {
    // 如果没有有效的上一个平台记录，不增加combo
    if (!hasValidLastPlatform) {
        return false;
    }

    // 如果在同一个平台上（Y坐标相同或非常接近），不增加combo
    if (abs(newPlatformY - lastPlatformY) < 5.0f) {
        return false;
    }

    // 如果当前平台高度不大于起跳平台高度，不增加combo
    // 注意：Y坐标值越小表示越高，所以newPlatformY应该小于lastPlatformY
    if (newPlatformY >= lastPlatformY) {
        return false;
    }

    return true;
}

// 重置Combo系统
void Player::resetComboSystem() {
    hasValidLastPlatform = false;
    lastPlatformY = 0.0f;
    currentPlatformY = 0.0f;
}

void Player::applySpeedBoost() {
    speedBoostTimer = 5.0f;

    // 播放道具收集音效
    AudioManager::getInstance().playSound(SoundType::ITEM_COLLECT, false);

    createSpeedBoostEffect();
    addBonusScore(50);
    incrementItemsCollected();
}

void Player::applyShield() {
    hasShieldActive = true;
    shieldTimer = 10.0f;
    shieldUsed = false;

    AudioManager::getInstance().playSound(SoundType::SHIELD_ACTIVATE, false);

    createShieldActivateEffect();
    addBonusScore(100);
    incrementItemsCollected();
}

void Player::applyInvincibility() {
    invincibilityTimer = 8.0f;
    hasInvincibility = true;

    // 播放无敌音效
    AudioManager::getInstance().playSound(SoundType::INVINCIBILITY, false);

    addBonusScore(150);
    incrementItemsCollected();
    createInvincibilityEffect();
}

// 添加奖励分数
void Player::addBonusScore(int points) {
    bonusScore += points;

    // 根据连击倍数增加额外分数
    if (comboCount > 1) {
        int comboBonus = points * (comboCount - 1) / 2; // 连击奖励递减
        bonusScore += comboBonus;
    }
}

// 消耗护盾（用于复活）
void Player::consumeShield() {
    hasShieldActive = false;
    shieldTimer = 0.0f;
    shieldUsed = true;
}

void Player::addCombo() {
    // 这个方法现在被updateComboSystem替代，保留这个方法以保持兼容性，但不执行任何操作
}

void Player::resetCombo() {
    comboCount = 0;
    comboTimer = 0.0f;
    resetComboSystem();  // 同时重置combo系统
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

// 无敌激活特效
void Player::createInvincibilityEffect() {
    for (int i = 0; i < 30; i++) {
        float angle = (float)i / 30.0f * 6.28f;
        float radius = 30 + rand() % 20;
        float px = x + width / 2 + cos(angle) * radius;
        float py = y + height / 2 + sin(angle) * radius;

        particles.push_back(Particle(
            px, py,
            cos(angle) * 30, sin(angle) * 30,
            2.5f,
            RGB(255, 215, 0)  // 金色无敌特效
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

// 二段跳粒子效果
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

// 速度粒子效果
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

// 速度提升激活特效
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

// 护盾激活特效
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

// 连击特效
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

    // 无敌效果绘制
    if (hasInvincibilityActive()) {
        // 绘制无敌光环
        float invincibilityPulse = AnimationUtils::pulse(pulseTimer, 4.0f);
        COLORREF invincibilityColor = RGB(255, 215, 0);  // 金色

        // 绘制多层无敌光环
        for (int i = 0; i < 3; i++) {
            float radius = 35 + i * 10 + invincibilityPulse * 5;
            float alpha = 0.3f - i * 0.1f;
            DrawUtils::drawGlowCircle((int)(drawX + width / 2), (int)(drawY + height / 2),
                (int)radius, invincibilityColor, alpha);
        }

        // 绘制星星特效
        for (int i = 0; i < 8; i++) {
            float angle = (float)i / 8.0f * 6.28f + pulseTimer * 2.0f;
            float starRadius = 40 + sin(pulseTimer * 3.0f + i) * 10;
            float starX = drawX + width / 2 + cos(angle) * starRadius;
            float starY = drawY + height / 2 + sin(angle) * starRadius;
            DrawUtils::drawSparkle(starX, starY, 6.0f, invincibilityColor, pulseTimer + i);
        }
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
    // 无敌状态光晕
    if (hasInvincibilityActive()) {
        glowColor = RGB(255, 215, 0);  // 金色光晕
        glowIntensity = 0.8f;
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
    // 无敌状态颜色
    if (hasInvincibilityActive()) {
        playerColor = AnimationUtils::colorPulse(Theme::PLAYER_MAIN,
            RGB(255, 215, 0), pulseTimer, 5.0f);
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
    shieldUsed = false;

    // 重置新道具效果
    doubleJumpTimer = 0.0f;
    slowTimeTimer = 0.0f;
    magneticFieldTimer = 0.0f;
    freezeObstaclesTimer = 0.0f;

    // 重置无敌状态
    invincibilityTimer = 0.0f;
    hasInvincibility = false;

    // 重置玩家状态
    health = maxHealth = 3;
    invulnerabilityTimer = 0.0f;
    coins = 0;

    // 重置道具效果强度
    timeScaleFactor = 1.0f;
    magnetRadius = 0.0f;
    hasDoubleJump = false;
    obstaclesFrozen = false;
    maxJumps = 2;

    // 重置连击
    resetCombo();

    // 重置分数系统
    bonusScore = 0;
    itemsCollected = 0;

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
    /*if (y < -height) {
        y = -height;
        vy = 0;
    }*/
}

void Player::applyDoubleJump() {
    doubleJumpTimer = 10.0f;  // 10秒额外跳跃
    hasDoubleJump = true;
    maxJumps = 3;  // 增加到3段跳
    addBonusScore(75);
    incrementItemsCollected();
}

void Player::applySlowTime() {
    slowTimeTimer = 8.0f;  // 8秒时间减缓
    timeScaleFactor = 0.5f;  // 时间减缓到50%
    addBonusScore(100);
    incrementItemsCollected();
}

void Player::applyMagneticField() {
    magneticFieldTimer = 15.0f;  // 15秒磁场效果
    magnetRadius = 150.0f;  // 磁场半径
    addBonusScore(80);
    incrementItemsCollected();
}

void Player::applyHealthBoost() {
    heal(2);  // 恢复2点生命
    addBonusScore(60);
    incrementItemsCollected();
}

void Player::applyFreezeObstacles() {
    freezeObstaclesTimer = 10.0f;  // 10秒冻结障碍物
    obstaclesFrozen = true;
    addBonusScore(120);
    incrementItemsCollected();
}

void Player::collectCoin(int value) {
    coins += value;
    addBonusScore(value);
    // 添加收集特效
    createSpeedBoostEffect(); // 重用特效
}

void Player::takeDamage(int damage) {
    if (invulnerabilityTimer > 0) return;

    health -= damage;
    if (health < 0) health = 0;

    // 播放受伤音效
    AudioManager::getInstance().playSound(SoundType::DAMAGE_SOUND, false);

    // 设置无敌时间
    invulnerabilityTimer = 1.0f;

    // 添加受伤特效
    addScreenShake(4.0f);
}

// 添加死亡检查函数
bool Player::isDead() const {
    return health <= 0;
}

void Player::heal(int amount) {
    health += amount;
    if (health > maxHealth) health = maxHealth;

    // 添加治疗特效
    createShieldActivateEffect();
}
