#include "Player.h"
#include "Platform.h" 
#include "Theme.h"
#include <vector>
#include <string>
#include <graphics.h>
#include <windows.h>
#include <ctime>
#include <cstdlib>
#include <algorithm>

using namespace std;

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;

enum GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

class BackgroundScrolling {
private:
    struct BackgroundLayer {
        float y;
        float speed;
        COLORREF color;
        int height;
    };

    vector<BackgroundLayer> layers;

public:
    BackgroundScrolling() {
        // 添加多层背景
        layers.push_back({ 0, 0.1f, RGB(220, 235, 245), 100 });
        layers.push_back({ 100, 0.2f, RGB(210, 225, 240), 120 });
        layers.push_back({ 220, 0.3f, RGB(200, 215, 235), 140 });
    }

    void update(float deltaTime, float cameraSpeed) {
        for (auto& layer : layers) {
            layer.y += cameraSpeed * layer.speed * deltaTime;
        }
    }

    void draw(float cameraY) {
        for (const auto& layer : layers) {
            float drawY = layer.y - cameraY;

            // 绘制重复的背景层
            for (int i = -2; i <= 3; i++) {
                float layerY = drawY + i * layer.height;
                if (layerY < WINDOW_HEIGHT + 50 && layerY > -layer.height - 50) {
                    setfillcolor(layer.color);
                    solidrectangle(0, (int)layerY, WINDOW_WIDTH, (int)(layerY + layer.height));
                }
            }
        }
    }
};

class PlatformPreview {
private:
    struct PreviewPlatform {
        float x, y;
        float width;
        PlatformType type;
        float alpha;
    };

    vector<PreviewPlatform> previews;

public:
    void update(const vector<Platform>& platforms, float cameraY) {
        previews.clear();

        for (const auto& platform : platforms) {
            float screenY = platform.getY() - cameraY;

            // 为即将出现在屏幕上方的平台添加预览
            if (screenY < -50 && screenY > -200) {
                float alpha = 1.0f - (abs(screenY + 50) / 150.0f);
                previews.push_back({
                    platform.getX(), platform.getY(),
                    platform.getWidth(), platform.getType(),
                    alpha * 0.5f
                    });
            }
        }
    }

    void draw(float cameraY) {
        for (const auto& preview : previews) {
            float drawY = preview.y - cameraY;

            COLORREF previewColor;
            switch (preview.type) {
            case NORMAL: previewColor = Theme::PLATFORM_NORMAL; break;
            case MOVING: previewColor = Theme::PLATFORM_MOVING; break;
            case BREAKABLE: previewColor = Theme::PLATFORM_BREAKABLE; break;
            case SPRING: previewColor = Theme::PLATFORM_SPRING; break;
            }

            // 绘制半透明预览
            int r = (int)(GetRValue(previewColor) * preview.alpha);
            int g = (int)(GetGValue(previewColor) * preview.alpha);
            int b = (int)(GetBValue(previewColor) * preview.alpha);

            setfillcolor(RGB(r, g, b));
            setlinestyle(PS_DOT, 1);
            setlinecolor(RGB(r, g, b));

            rectangle((int)preview.x, (int)drawY,
                (int)(preview.x + preview.width), (int)(drawY + 20));
        }
    }
};

class PlatformGenerator {
private:
    static const float MAX_JUMP_HEIGHT;
    static const float MAX_JUMP_DISTANCE;

public:
    PlatformType getRandomType(float difficulty) {
        int rand_val = rand() % 100;

        if (difficulty < 0.3f) {
            if (rand_val < 70) return NORMAL;
            else if (rand_val < 85) return MOVING;
            else if (rand_val < 95) return BREAKABLE;
            else return SPRING;
        }
        else if (difficulty < 0.7f) {
            if (rand_val < 50) return NORMAL;
            else if (rand_val < 75) return MOVING;
            else if (rand_val < 90) return BREAKABLE;
            else return SPRING;
        }
        else {
            if (rand_val < 35) return NORMAL;
            else if (rand_val < 60) return MOVING;
            else if (rand_val < 85) return BREAKABLE;
            else return SPRING;
        }
    }

    Platform generateNextPlatform(const Platform& lastPlatform, float currentHeight, float difficulty) {
        float verticalGap = 60.0f + (difficulty * 20.0f);
        verticalGap = std::min(verticalGap, MAX_JUMP_HEIGHT * 0.8f);

        float horizontalGap = 50.0f + (rand() % 100);
        horizontalGap = std::min(horizontalGap, MAX_JUMP_DISTANCE * 0.7f);

        float newX = lastPlatform.getX() + (rand() % 2 == 0 ? 1 : -1) * horizontalGap;
        newX = std::max(50.0f, std::min(newX, (float)WINDOW_WIDTH - 150.0f));

        float newY = lastPlatform.getY() - verticalGap;

        return Platform(newX, newY, 80 + rand() % 60, 20, getRandomType(difficulty));
    }

    Platform generateRandomPlatform(float y, float difficulty) {
        float x = 50.0f + rand() % (WINDOW_WIDTH - 200);
        float width = 80.0f + rand() % 80;
        return Platform(x, y, width, 20, getRandomType(difficulty));
    }
};

const float PlatformGenerator::MAX_JUMP_HEIGHT = 150.0f;
const float PlatformGenerator::MAX_JUMP_DISTANCE = 200.0f;

class Game {
private:
    GameState currentState;
    Player player;
    vector<Platform> platforms;
    long long score;
    long long maxHeight;
    float initialPlayerY;   // 记录初始Y位置
    float camera_y;

    // 新增系统
    BackgroundScrolling background;
    PlatformPreview platformPreview;

    // UI相关
    float fadeAlpha;

    // 相机相关
    float cameraTargetY;
    float cameraSpeed;
    float cameraDeadZone;
    float maxCameraSpeed; 
    float cameraSpeedLimit;

    // 游戏状态相关
    float worldSpeed;
    float baseWorldSpeed;
    float gameTime;
    float killZone;

    // 输入状态管理
    bool spaceWasPressed;
    bool escWasPressed;

    // 平台生成器
    PlatformGenerator platformGenerator;

    // 平台生成相关
    float highestPlatformY;
    float platformSpawnThreshold;

    // 新增：平滑镜头速度控制
    float smoothCameraSpeed;
    float cameraSpeedAcceleration;
    float maxSafeCameraSpeed;

    // 新增：玩家垂直速度统计
    std::vector<float> playerVerticalSpeedSamples;
    float averagePlayerVerticalSpeed;
    float lastPlayerY;
    float playerSpeedSampleTime;

    // 新增：世界速度平滑控制
    float maxWorldSpeed;
    float worldSpeedSmoothing;

    // 最后接触的平台信息（用于复活）
    struct LastPlatformInfo {
        float x, y;
        float width, height;
        bool isValid;

        LastPlatformInfo() : x(0), y(0), width(100), height(20), isValid(false) {}
    } lastSafePlatform;

public:
    Game() : currentState(MENU), player(100, 400), score(0), maxHeight(0), camera_y(0), fadeAlpha(0),
        cameraTargetY(0), cameraSpeed(3.0f), cameraDeadZone(80.0f),
        maxCameraSpeed(4.5f), cameraSpeedLimit(6.0f), 
        worldSpeed(0), baseWorldSpeed(20.0f), gameTime(0),
        spaceWasPressed(false), escWasPressed(false),
        highestPlatformY(0), platformSpawnThreshold(30.0f),
        // 新增：初始化平滑镜头控制
        smoothCameraSpeed(3.0f),
        cameraSpeedAcceleration(0.5f),
        maxSafeCameraSpeed(8.0f), // 安全的最大镜头速度

        // 新增：初始化玩家速度统计
        averagePlayerVerticalSpeed(0.0f),
        lastPlayerY(0.0f),
        playerSpeedSampleTime(0.0f),

        // 新增：初始化世界速度控制
        maxWorldSpeed(60.0f), // 世界速度上限
        worldSpeedSmoothing(2.0f) {
        srand((unsigned int)time(nullptr));
        initializePlatforms();{
            srand((unsigned int)time(nullptr));
            initializePlatforms();
            positionPlayerOnStartPlatform();
            initialPlayerY = player.getY();
            lastPlayerY = initialPlayerY;
            killZone = player.getY() + 300.0f;
        }

        // 确保玩家出生在起始平台上
        positionPlayerOnStartPlatform();

        // 记录初始Y位置
        initialPlayerY = player.getY();

        killZone = player.getY() + 300.0f;
    }

    void initializePlatforms() {
        platforms.clear();

        // 地面平台
        platforms.push_back(Platform(0, WINDOW_HEIGHT - 40, WINDOW_WIDTH, 40, NORMAL));

        // 添加一个固定的起始平台，确保玩家有地方站立
        float startPlatformY = WINDOW_HEIGHT - 120;
        float startPlatformX = WINDOW_WIDTH / 2 - 75; // 居中位置
        platforms.push_back(Platform(startPlatformX, startPlatformY, 150, 20, NORMAL));

        // 随机生成初始平台
        float currentY = WINDOW_HEIGHT - 100;
        highestPlatformY = currentY;

        for (int i = 0; i < 15; i++) {
            currentY -= 80 + rand() % 60;
            Platform newPlatform = platformGenerator.generateRandomPlatform(currentY, 0.2f);
            platforms.push_back(newPlatform);

            if (currentY < highestPlatformY) {
                highestPlatformY = currentY;
            }
        }
    }

    // 将玩家定位到起始平台上
    void positionPlayerOnStartPlatform() {
        // 找到起始平台（第二个平台，因为第一个是地面）
        if (platforms.size() >= 2) {
            const Platform& startPlatform = platforms[1];
            float platformCenterX = startPlatform.getX() + startPlatform.getWidth() / 2;
            float platformTop = startPlatform.getY() - player.getHeight();

            player.setPosition(platformCenterX - player.getWidth() / 2, platformTop);
            player.setOnGround(true);

            // 记录为安全平台
            lastSafePlatform.x = startPlatform.getX();
            lastSafePlatform.y = startPlatform.getY();
            lastSafePlatform.width = startPlatform.getWidth();
            lastSafePlatform.height = startPlatform.getHeight();
            lastSafePlatform.isValid = true;
        }
    }

    void generateNewPlatforms() {
        if (camera_y < highestPlatformY + platformSpawnThreshold) {
            int numNewPlatforms = 5 + rand() % 4;
            float currentDifficulty = std::min(1.0f, gameTime / 60.0f);

            for (int i = 0; i < numNewPlatforms; i++) {
                float newY = highestPlatformY - (80 + rand() % 80);
                Platform newPlatform = platformGenerator.generateRandomPlatform(newY, currentDifficulty);
                platforms.push_back(newPlatform);
                highestPlatformY = newY;
            }
        }
    }

    void cleanupOldPlatforms() {
        float cleanupThreshold = camera_y + WINDOW_HEIGHT + 200;
        platforms.erase(
            std::remove_if(platforms.begin(), platforms.end(),
                [cleanupThreshold](const Platform& platform) {
                    return platform.getY() > cleanupThreshold;
                }),
            platforms.end()
        );
    }

    void update(float deltaTime) {
        updateInputState();

        switch (currentState) {
        case MENU:
            updateMenu();
            break;
        case PLAYING:
            updateGame(deltaTime);
            break;
        case PAUSED:
            updatePause();
            break;
        case GAME_OVER:
            updateGameOver();
            break;
        }
    }

    void updateInputState() {
        bool spacePressed = GetAsyncKeyState(VK_SPACE) & 0x8000;
        bool escPressed = GetAsyncKeyState(VK_ESCAPE) & 0x8000;
        spaceWasPressed = spacePressed;
        escWasPressed = escPressed;
    }

    void updateMenu() {
        static bool spaceReleased = true;
        static bool escReleased = true;

        bool spacePressed = GetAsyncKeyState(VK_SPACE) & 0x8000;
        bool escPressed = GetAsyncKeyState(VK_ESCAPE) & 0x8000;

        if (spacePressed && spaceReleased) {
            currentState = PLAYING;
            resetGame();
            spaceReleased = false;
        }
        if (!spacePressed) spaceReleased = true;

        if (escPressed && escReleased) {
            closegraph();
            exit(0);
        }
        if (!escPressed) escReleased = true;
    }

    void updateCamera(float deltaTime) {
        float playerScreenY = player.getY() - camera_y;
        float screenCenterY = WINDOW_HEIGHT / 2.0f;

        if (playerScreenY < screenCenterY - cameraDeadZone) {
            cameraTargetY = player.getY() - screenCenterY;

            // 计算当前高度和难度
            float currentHeight = initialPlayerY - player.getY();
            float difficultyFactor = std::min(1.0f, currentHeight / 1000.0f); // 1000像素内达到最大难度

            // 平滑的镜头速度增长
            float targetCameraSpeed = cameraSpeed + difficultyFactor * cameraSpeedAcceleration;
            targetCameraSpeed = std::min(targetCameraSpeed, maxSafeCameraSpeed);

            // 使用指数平滑来过渡镜头速度
            smoothCameraSpeed = smoothCameraSpeed * 0.98f + targetCameraSpeed * 0.02f;

            // 计算镜头移动量
            float cameraMovement = (cameraTargetY - camera_y) * smoothCameraSpeed * deltaTime;

            // 应用最大速度限制
            float maxMovement = maxSafeCameraSpeed * deltaTime;
            if (std::abs(cameraMovement) > maxMovement) {
                cameraMovement = (cameraMovement > 0) ? maxMovement : -maxMovement;
            }

            camera_y += cameraMovement;
        }

        if (camera_y < 0) camera_y = 0;
    }

    // 新增：更新玩家垂直速度统计
    void updatePlayerVerticalSpeedStats(float deltaTime) {
        playerSpeedSampleTime += deltaTime;

        // 每0.1秒采样一次玩家垂直速度
        if (playerSpeedSampleTime >= 0.1f) {
            float currentPlayerY = player.getY();
            float verticalSpeed = (lastPlayerY - currentPlayerY) / playerSpeedSampleTime; // 向上为正

            // 只记录向上的速度（跳跃时）
            if (verticalSpeed > 0) {
                playerVerticalSpeedSamples.push_back(verticalSpeed);

                // 保持最近50个样本
                if (playerVerticalSpeedSamples.size() > 50) {
                    playerVerticalSpeedSamples.erase(playerVerticalSpeedSamples.begin());
                }

                // 计算平均速度
                float sum = 0;
                for (float speed : playerVerticalSpeedSamples) {
                    sum += speed;
                }
                averagePlayerVerticalSpeed = sum / playerVerticalSpeedSamples.size();
            }

            lastPlayerY = currentPlayerY;
            playerSpeedSampleTime = 0.0f;
        }
    }

    void updateGame(float deltaTime) {
        player.handleInput();
        player.update(deltaTime);

        updateCamera(deltaTime);
        updateWorldMovement(deltaTime);

        // 更新背景滚动（使用平滑的世界速度）
        background.update(deltaTime, worldSpeed);

        // 更新平台预览
        platformPreview.update(platforms, camera_y);

        generateNewPlatforms();
        cleanupOldPlatforms();

        // 平台更新
        for (auto& platform : platforms) {
            platform.update(deltaTime);
        }

        // 碰撞检测
        checkCollisions();

        player.checkBounds(WINDOW_WIDTH, WINDOW_HEIGHT);

        // 分数计算
        updateScore();

        // 暂停检查
        static bool escReleased = true;
        bool escPressed = GetAsyncKeyState(VK_ESCAPE) & 0x8000;

        if (escPressed && escReleased) {
            currentState = PAUSED;
            escReleased = false;
        }
        if (!escPressed) escReleased = true;

        // 游戏结束检查
        if (player.getY() > killZone) {
            if (player.canTakeDamage()) {
                currentState = GAME_OVER;
            }
            else {
                respawnPlayerToSafePlatform();
            }
        }
    }

    // 更新分数系统
    void updateScore() {
        // 高度分数计算（使用double避免精度损失）
        double currentHeight = initialPlayerY - player.getY(); // 简化计算

        if (currentHeight > (double)maxHeight) {
            maxHeight = (long long)currentHeight;
        }

        // 总分数 = 高度分数 + 道具奖励分数 + 连击奖励
        long long heightScore = maxHeight / 5;  // 每5像素1分，提高分数增长速度
        long long bonusScore = player.getBonusScore();
        long long comboBonus = player.getComboCount() * 10; // 连击额外奖励

        score = heightScore + bonusScore + comboBonus;
    }

    // 护盾复活逻辑
    void respawnPlayerToSafePlatform() {
        if (lastSafePlatform.isValid) {
            // 将玩家传送到最后的安全平台上
            float respawnX = lastSafePlatform.x + lastSafePlatform.width / 2 - player.getWidth() / 2;
            float respawnY = lastSafePlatform.y - player.getHeight();

            player.setPosition(respawnX, respawnY);
            player.setVY(0); // 停止下降
            player.setOnGround(true);

            // 添加复活特效
            player.createShieldActivateEffect();
            player.addScreenShake(3.0f);

            // 护盾消耗：复活后失去护盾
            player.consumeShield();
            
        }
        else {
            // 如果没有记录的安全平台，传送到起始位置
            positionPlayerOnStartPlatform();
            player.consumeShield();
        }
    }

    void updateWorldMovement(float deltaTime) {
        gameTime += deltaTime;

        // 更新玩家垂直速度统计
        updatePlayerVerticalSpeedStats(deltaTime);

        // 基础速度增长（更温和）
        float timeSpeedMultiplier = 1.0f + (gameTime / 60.0f) * 0.3f; // 降低时间倍数
        float scoreSpeedMultiplier = 1.0f + (score / 500.0f) * 0.1f; // 降低分数倍数

        // 计算目标世界速度
        float targetWorldSpeed = baseWorldSpeed * timeSpeedMultiplier * scoreSpeedMultiplier;

        // 确保世界速度不超过玩家平均垂直速度的80%
        if (averagePlayerVerticalSpeed > 0) {
            float maxAllowedWorldSpeed = averagePlayerVerticalSpeed * 0.8f;
            targetWorldSpeed = std::min(targetWorldSpeed, maxAllowedWorldSpeed);
        }

        // 应用绝对上限
        targetWorldSpeed = std::min(targetWorldSpeed, maxWorldSpeed);

        // 使用平滑过渡到目标速度
        worldSpeed = worldSpeed * (1.0f - worldSpeedSmoothing * deltaTime) +
            targetWorldSpeed * (worldSpeedSmoothing * deltaTime);

        // 更新死亡区域
        killZone = camera_y + WINDOW_HEIGHT + 100;

        // 移动平台
        for (auto& platform : platforms) {
            platform.moveY(worldSpeed * deltaTime);
        }

        highestPlatformY += worldSpeed * deltaTime;
    }

    void updatePause() {
        static bool spaceReleased = true;
        static bool escReleased = true;

        bool spacePressed = GetAsyncKeyState(VK_SPACE) & 0x8000;
        bool escPressed = GetAsyncKeyState(VK_ESCAPE) & 0x8000;

        if (spacePressed && spaceReleased) {
            currentState = PLAYING;
            spaceReleased = false;
        }
        if (!spacePressed) spaceReleased = true;

        if (escPressed && escReleased) {
            currentState = MENU;
            escReleased = false;
        }
        if (!escPressed) escReleased = true;
    }

    void updateGameOver() {
        static bool spaceReleased = true;
        static bool escReleased = true;

        bool spacePressed = GetAsyncKeyState(VK_SPACE) & 0x8000;
        bool escPressed = GetAsyncKeyState(VK_ESCAPE) & 0x8000;

        if (spacePressed && spaceReleased) {
            currentState = MENU;
            spaceReleased = false;
        }
        if (!spacePressed) spaceReleased = true;

        if (escPressed && escReleased) {
            closegraph();
            exit(0);
        }
        if (!escPressed) escReleased = true;
    }

    void checkCollisions() {
        bool foundGroundCollision = false;

        for (auto& platform : platforms) {
            // 获取玩家和平台的边界
            float playerLeft = player.getX();
            float playerRight = player.getX() + player.getWidth();
            float playerTop = player.getY();
            float playerBottom = player.getY() + player.getHeight();

            float platformLeft = platform.getX();
            float platformRight = platform.getX() + platform.getWidth();
            float platformTop = platform.getY();

            // 检查水平重叠
            bool horizontalOverlap = (playerRight > platformLeft) && (playerLeft < platformRight);

            // 检查垂直碰撞（玩家从上方接触）
            bool verticalCollision = (playerBottom >= platformTop) && (playerBottom <= platformTop + 15);

            // 对于弹簧平台，放宽条件确保能够触发
            if (platform.getType() == SPRING) {
                verticalCollision = (playerBottom >= platformTop) && (playerBottom <= platformTop + 20);
            }

            if (horizontalOverlap && verticalCollision) {
                // 计算碰撞后的位置
                float newY = platformTop - player.getHeight();

                // 处理平台特殊效果
                float playerVY = player.getVY();
                platform.handleCollision(player.getX(), player.getY(), player.getWidth(), player.getHeight(), playerVY);

                player.setPosition(player.getX(), newY);
                player.setVY(playerVY); 

                // 记录最后接触的安全平台（只记录普通平台和弹簧平台）
                if (platform.getType() == NORMAL || platform.getType() == SPRING) {
                    lastSafePlatform.x = platform.getX();
                    lastSafePlatform.y = platform.getY();
                    lastSafePlatform.width = platform.getWidth();
                    lastSafePlatform.height = platform.getHeight();
                    lastSafePlatform.isValid = true;
                }

                // 对于弹簧平台，不要立即设置为onGround，让玩家弹起
                if (platform.getType() == SPRING && playerVY < 0) {
                    foundGroundCollision = false;  // 弹簧时不在地面
                    // 弹簧平台额外加分
                    player.addBonusScore(25);
                }
                else {
                    foundGroundCollision = true;
                }

                // 收集道具
                Item* item = platform.collectItem();
                if (item) {
                    switch (item->type) {
                    case SPEED_BOOST:
                        player.applySpeedBoost();
                        break;
                    case SHIELD:
                        player.applyShield();
                        break;
                    default:
                        break;
                    }
                }

                break;  // 只处理第一个有效碰撞
            }
        }

        // 设置地面状态
        player.setOnGround(foundGroundCollision);
    }

    void resetGame() {
        player.reset();
        score = 0;
        maxHeight = 0;
        camera_y = 0;
        cameraTargetY = 0;
        gameTime = 0;
        worldSpeed = 0;

        // 重置镜头平滑控制
        smoothCameraSpeed = 3.0f;

        // 重置玩家速度统计
        playerVerticalSpeedSamples.clear();
        averagePlayerVerticalSpeed = 0.0f;
        playerSpeedSampleTime = 0.0f;

        initializePlatforms();
        positionPlayerOnStartPlatform();

        initialPlayerY = player.getY();
        lastPlayerY = initialPlayerY;
        killZone = player.getY() + 300.0f;
    }

    void render() {
        BeginBatchDraw();

        // 应用屏幕震动
        float shakeX, shakeY;
        player.getShakeOffset(shakeX, shakeY);

        setbkcolor(Theme::BACKGROUND);
        cleardevice();

        switch (currentState) {
        case MENU:
            drawMenu();
            break;
        case PLAYING:
            drawGame(shakeX, shakeY);
            break;
        case PAUSED:
            drawGame(shakeX, shakeY);
            drawPause();
            break;
        case GAME_OVER:
            drawGame(shakeX, shakeY);
            drawGameOver();
            break;
        }

        EndBatchDraw();
    }

    void drawMenu() {
        DrawUtils::drawSoftShadowRect(WINDOW_WIDTH / 2 - 150, 150, 300, 80, 10, Theme::PRIMARY);

        settextcolor(WHITE);
        settextstyle(40, 0, L"Arial");
        wstring title = L"Jump Game";
        int titleWidth = textwidth(title.c_str());
        int titleX = (WINDOW_WIDTH - titleWidth) / 2;
        outtextxy(titleX, 170, title.c_str());

        settextstyle(20, 0, L"Arial");
        settextcolor(Theme::TEXT_SECONDARY);

        vector<wstring> instructions = {
            L"Press SPACE to start",
            L"A/D or Arrow Keys to move",
            L"SPACE to jump",
            L"Collect items for power-ups",
            L"ESC to exit"
        };

        int startY = 280;
        for (size_t i = 0; i < instructions.size(); i++) {
            int textWidth = textwidth(instructions[i].c_str());
            int textX = (WINDOW_WIDTH - textWidth) / 2;
            outtextxy(textX, startY + (int)i * 30, instructions[i].c_str());
        }
    }

    void drawGame(float shakeX = 0, float shakeY = 0) {
        // 绘制背景滚动
        background.draw(camera_y);

        // 绘制平台预览
        platformPreview.draw(camera_y);

        // 绘制平台
        for (const auto& platform : platforms) {
            float drawY = platform.getY() - camera_y;
            if (drawY > -50 && drawY < WINDOW_HEIGHT + 50) {
                platform.drawWithOffset(shakeX, -camera_y + shakeY);
            }
        }

        // 绘制玩家
        player.drawWithOffset(shakeX, -camera_y + shakeY);

        // 绘制死亡线（增强特效）
        float deathLineY = killZone - camera_y;
        if (deathLineY > 0 && deathLineY < WINDOW_HEIGHT + 100) {
            // 计算危险强度
            float dangerIntensity = 1.0f;
            if (deathLineY < WINDOW_HEIGHT) {
                dangerIntensity = 1.0f - (deathLineY / WINDOW_HEIGHT) * 0.5f;
            }

            // 使用Theme.cpp中的drawDangerZone
            DrawUtils::drawDangerZone(deathLineY, dangerIntensity);

            // 额外的警告效果
            if (deathLineY < WINDOW_HEIGHT - 50) {
                // 屏幕边缘红色警告
                COLORREF warningColor = AnimationUtils::colorFlash(RGB(255, 0, 0), RGB(255, 255, 255),
                    dangerIntensity * 0.3f);
                setfillcolor(warningColor);
                solidrectangle(0, 0, WINDOW_WIDTH, 5);
                solidrectangle(0, WINDOW_HEIGHT - 5, WINDOW_WIDTH, WINDOW_HEIGHT);
                solidrectangle(0, 0, 5, WINDOW_HEIGHT);
                solidrectangle(WINDOW_WIDTH - 5, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
            }
        }

        drawGameUI();
    }

    void drawGameUI() {
        // 为文字添加描边效果增强可读性
        settextcolor(RGB(0, 0, 0)); // 黑色描边
        settextstyle(22, 0, L"Arial");

        // 修改显示逻辑，使用long long
        wstring scoreText = L"Score: " + to_wstring(score);
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx != 0 || dy != 0) {
                    outtextxy(30 + dx, 30 + dy, scoreText.c_str());
                }
            }
        }

        wstring heightText = L"Height: " + to_wstring(maxHeight);
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx != 0 || dy != 0) {
                    outtextxy(30 + dx, 60 + dy, heightText.c_str());
                }
            }
        }

        // 新增：显示道具收集数
        wstring itemText = L"Items: " + to_wstring(player.getItemsCollected());
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx != 0 || dy != 0) {
                    outtextxy(30 + dx, 90 + dy, itemText.c_str());
                }
            }
        }

        wstring timeText = L"Time: " + to_wstring((int)gameTime) + L"s";
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx != 0 || dy != 0) {
                    outtextxy(30 + dx, 90 + dy, timeText.c_str());
                }
            }
        }

        // 绘制主文字（白色）
        settextcolor(RGB(255, 255, 255));
        outtextxy(30, 30, scoreText.c_str());
        outtextxy(30, 60, heightText.c_str());
        outtextxy(30, 90, timeText.c_str());
        outtextxy(30, 120, timeText.c_str());

        // 连击显示（带描边）
        if (player.getComboCount() > 1) {
            wstring comboText = L"Combo: " + to_wstring(player.getComboCount()) + L"x";

            // 描边
            settextcolor(RGB(0, 0, 0));
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx != 0 || dy != 0) {
                        outtextxy(30 + dx, 120 + dy, comboText.c_str());
                    }
                }
            }

            // 主文字
            COLORREF comboColor = DrawUtils::getComboColor(player.getComboCount());
            settextcolor(comboColor);
            outtextxy(30, 120, comboText.c_str());
        }

        // 道具状态显示（带描边）
        settextstyle(16, 0, L"Arial");

        if (player.hasSpeedBoost()) {
            wstring speedText = L"⚡ Speed Boost Active";

            // 描边
            settextcolor(RGB(0, 0, 0));
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx != 0 || dy != 0) {
                        outtextxy(30 + dx, 150 + dy, speedText.c_str());
                    }
                }
            }

            // 主文字
            settextcolor(Theme::ITEM_SPEED);
            outtextxy(30, 150, speedText.c_str());
        }

        if (player.hasShield()) {
            wstring shieldText = L"🛡 Shield Active";

            // 描边
            settextcolor(RGB(0, 0, 0));
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx != 0 || dy != 0) {
                        outtextxy(200 + dx, 150 + dy, shieldText.c_str());
                    }
                }
            }

            // 主文字
            settextcolor(Theme::ITEM_SHIELD);
            outtextxy(200, 150, shieldText.c_str());
        }

        // 控制提示 - 移到右下角（带描边）
        settextstyle(14, 0, L"Arial");
        int rightX = WINDOW_WIDTH - 200;

        vector<wstring> controls = {
            L"A/D: Move",
            L"SPACE: Jump",
            L"Collect items for power-ups",
            L"ESC: Pause"
        };

        for (size_t i = 0; i < controls.size(); i++) {
            int yPos = WINDOW_HEIGHT - 120 + (int)i * 20;

            // 描边
            settextcolor(RGB(0, 0, 0));
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx != 0 || dy != 0) {
                        outtextxy(rightX + dx, yPos + dy, controls[i].c_str());
                    }
                }
            }

            // 主文字
            settextcolor(Theme::TEXT_DISABLED);
            outtextxy(rightX, yPos, controls[i].c_str());
        }

        // 连击显示（增强特效）
        if (player.getComboCount() > 1) {
            wstring comboText = L"Combo: " + to_wstring(player.getComboCount()) + L"x";

            // 连击越高，特效越强
            COLORREF comboColor = DrawUtils::getComboColor(player.getComboCount());
            float comboIntensity = std::min(1.0f, player.getComboCount() / 10.0f);

            // 连击光晕背景
            DrawUtils::drawGlowRect(25, 115, 200, 25, comboColor, comboIntensity * 0.3f);

            // 描边
            settextcolor(RGB(0, 0, 0));
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx != 0 || dy != 0) {
                        outtextxy(30 + dx, 120 + dy, comboText.c_str());
                    }
                }
            }

            // 主文字（带脉动效果）
            COLORREF pulsatingComboColor = AnimationUtils::colorPulse(comboColor, RGB(255, 255, 255),
                (float)clock() / CLOCKS_PER_SEC, 3.0f);
            settextcolor(pulsatingComboColor);
            outtextxy(30, 120, comboText.c_str());

            // 高连击时的额外星星特效
            if (player.getComboCount() > 10) {
                for (int i = 0; i < 3; i++) {
                    float starX = 30 + 60 * i;
                    float starY = 105;
                    DrawUtils::drawSparkle(starX, starY, 8.0f, comboColor,
                        (float)clock() / CLOCKS_PER_SEC + i);
                }
            }
        }
    }

    void drawPause() {
        setfillcolor(DrawUtils::blendColor(RGB(0, 0, 0), RGB(255, 255, 255), 0.7f));
        solidrectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        DrawUtils::drawSoftShadowRect(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 60, 200, 120, 15, Theme::PRIMARY);

        settextcolor(WHITE);
        settextstyle(30, 0, L"Arial");
        wstring pauseText = L"PAUSED";
        int pauseWidth = textwidth(pauseText.c_str());
        int pauseX = (WINDOW_WIDTH - pauseWidth) / 2;
        outtextxy(pauseX, WINDOW_HEIGHT / 2 - 30, pauseText.c_str());

        settextstyle(16, 0, L"Arial");
        settextcolor(Theme::PRIMARY_LIGHT);
        wstring resumeText = L"SPACE to resume";
        int resumeWidth = textwidth(resumeText.c_str());
        int resumeX = (WINDOW_WIDTH - resumeWidth) / 2;
        outtextxy(resumeX, WINDOW_HEIGHT / 2 + 5, resumeText.c_str());

        wstring menuText = L"ESC to return to menu";
        int menuWidth = textwidth(menuText.c_str());
        int menuX = (WINDOW_WIDTH - menuWidth) / 2;
        outtextxy(menuX, WINDOW_HEIGHT / 2 + 30, menuText.c_str());
    }

    void drawGameOver() {
        setfillcolor(DrawUtils::blendColor(RGB(0, 0, 0), RGB(255, 255, 255), 0.8f));
        solidrectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        DrawUtils::drawSoftShadowRect(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 100, 300, 240, 20, Theme::PRIMARY);

        settextcolor(WHITE);
        settextstyle(40, 0, L"Arial");
        wstring gameOverText = L"Game Over";
        int gameOverWidth = textwidth(gameOverText.c_str());
        int gameOverX = (WINDOW_WIDTH - gameOverWidth) / 2;
        outtextxy(gameOverX, WINDOW_HEIGHT / 2 - 80, gameOverText.c_str());

        settextstyle(24, 0, L"Arial");
        settextcolor(Theme::PRIMARY_LIGHT);

        wstring finalScoreText = L"Final Score: " + to_wstring(score);
        int scoreWidth = textwidth(finalScoreText.c_str());
        int scoreX = (WINDOW_WIDTH - scoreWidth) / 2;
        outtextxy(scoreX, WINDOW_HEIGHT / 2 - 40, finalScoreText.c_str());

        wstring finalHeightText = L"Max Height: " + to_wstring(maxHeight);
        int heightWidth = textwidth(finalHeightText.c_str());
        int heightX = (WINDOW_WIDTH - heightWidth) / 2;
        outtextxy(heightX, WINDOW_HEIGHT / 2 - 15, finalHeightText.c_str());

        // 新增：显示道具收集统计
        wstring itemsText = L"Items Collected: " + to_wstring(player.getItemsCollected());
        int itemsWidth = textwidth(itemsText.c_str());
        int itemsX = (WINDOW_WIDTH - itemsWidth) / 2;
        outtextxy(itemsX, WINDOW_HEIGHT / 2 + 10, itemsText.c_str());

        wstring survivalTimeText = L"Survival Time: " + to_wstring((int)gameTime) + L"s";
        int timeWidth = textwidth(survivalTimeText.c_str());
        int timeX = (WINDOW_WIDTH - timeWidth) / 2;
        outtextxy(timeX, WINDOW_HEIGHT / 2 + 35, survivalTimeText.c_str());

        wstring maxComboText = L"Max Combo: " + to_wstring(player.getComboCount()) + L"x";
        int comboWidth = textwidth(maxComboText.c_str());
        int comboX = (WINDOW_WIDTH - comboWidth) / 2;
        outtextxy(comboX, WINDOW_HEIGHT / 2 + 60, maxComboText.c_str());

        settextstyle(30, 0, L"Arial");
        settextcolor(Theme::WARNING);
        wstring restartText = L"SPACE to return to menu";
        int restartWidth = textwidth(restartText.c_str());
        int restartX = (WINDOW_WIDTH - restartWidth) / 2;
        outtextxy(restartX, WINDOW_HEIGHT / 2 + 85, restartText.c_str());

        wstring exitText = L"ESC to exit game";
        int exitWidth = textwidth(exitText.c_str());
        int exitX = (WINDOW_WIDTH - exitWidth) / 2;
        outtextxy(exitX, WINDOW_HEIGHT / 2 + 110, exitText.c_str());
    }
};

int main() {
    initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
    setbkmode(TRANSPARENT);
    SetWindowText(GetHWnd(), L"Jump Game EasyX Version");

    Game game;
    clock_t lastTime = clock();

    while (true) {
        if (GetAsyncKeyState(VK_F4) & 0x8000) {
            break;
        }

        clock_t currentTime = clock();
        float deltaTime = (float)(currentTime - lastTime) / CLOCKS_PER_SEC;
        lastTime = currentTime;

        if (deltaTime > 0.033f) deltaTime = 0.033f;

        game.update(deltaTime);
        game.render();

        Sleep(16);
    }

    closegraph();
    return 0;
}