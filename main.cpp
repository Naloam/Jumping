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
    int score;
    int maxHeight;
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

public:
    Game() : currentState(MENU), player(100, 400), score(0), maxHeight(0), camera_y(0), fadeAlpha(0),
        cameraTargetY(0), cameraSpeed(3.0f), cameraDeadZone(80.0f),
        worldSpeed(0), baseWorldSpeed(50.0f), gameTime(0),
        spaceWasPressed(false), escWasPressed(false),
        highestPlatformY(0), platformSpawnThreshold(300.0f) {
        srand((unsigned int)time(nullptr));
        initializePlatforms();
        killZone = player.getY() + 300.0f;
    }

    void initializePlatforms() {
        platforms.clear();

        // 地面平台
        platforms.push_back(Platform(0, WINDOW_HEIGHT - 40, WINDOW_WIDTH, 40, NORMAL));

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
            camera_y += (cameraTargetY - camera_y) * cameraSpeed * deltaTime;
        }

        if (camera_y < 0) camera_y = 0;
    }

    void updateGame(float deltaTime) {
        player.handleInput();
        player.update(deltaTime);

        updateCamera(deltaTime);
        updateWorldMovement(deltaTime);

        // 更新背景滚动
        background.update(deltaTime, worldSpeed);

        // 更新平台预览
        platformPreview.update(platforms, camera_y);

        generateNewPlatforms();
        cleanupOldPlatforms();

        // 平台更新
        for (auto& platform : platforms) {
            platform.update(deltaTime);
        }

        // 碰撞检测 - 改进版本
        checkCollisions();

        player.checkBounds(WINDOW_WIDTH, WINDOW_HEIGHT);

        // 分数计算
        float playerRealHeight = -(player.getY() - 400);
        if (playerRealHeight > maxHeight) {
            maxHeight = (int)playerRealHeight;
            score = maxHeight / 10;
        }

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
                // 护盾保护，传送到安全位置
                player.setPosition(player.getX(), killZone - 100);
            }
        }
    }

    void updateWorldMovement(float deltaTime) {
        gameTime += deltaTime;

        float timeSpeedMultiplier = 1.0f + (gameTime / 30.0f) * 0.5f;
        float scoreSpeedMultiplier = 1.0f + (score / 100.0f) * 0.2f;

        worldSpeed = baseWorldSpeed * timeSpeedMultiplier * scoreSpeedMultiplier;
        killZone = camera_y + WINDOW_HEIGHT + 100;

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
        for (auto& platform : platforms) {
            if (platform.checkCollision(player.getX(), player.getY(), player.getWidth(), player.getHeight())) {
                if (player.getY() < platform.getY()) {
                    // 修复：使用Player的实际垂直速度
                    float playerVY = player.getVY();
                    float newY = platform.handleCollision(player.getX(), player.getY(),
                        player.getWidth(), player.getHeight(),
                        playerVY);
                    player.setPosition(player.getX(), newY);
                    player.setVY(playerVY);  // 设置修改后的垂直速度
                    player.setOnGround(true);

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
                }
            }
        }
    }

    void resetGame() {
        player.reset();
        score = 0;
        maxHeight = 0;
        camera_y = 0;
        cameraTargetY = 0;
        gameTime = 0;
        worldSpeed = 0;
        killZone = player.getY() + 300.0f;
        initializePlatforms();
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
        // 完全透明背景 - 只绘制文字，不绘制背景矩形

        // 为文字添加描边效果增强可读性
        settextcolor(RGB(0, 0, 0)); // 黑色描边
        settextstyle(22, 0, L"Arial");

        // 绘制文字描边（偏移1像素绘制多次）
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

        DrawUtils::drawSoftShadowRect(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 120, 300, 240, 20, Theme::PRIMARY);

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
        outtextxy(heightX, WINDOW_HEIGHT / 2 - 10, finalHeightText.c_str());

        wstring survivalTimeText = L"Survival Time: " + to_wstring((int)gameTime) + L"s";
        int timeWidth = textwidth(survivalTimeText.c_str());
        int timeX = (WINDOW_WIDTH - timeWidth) / 2;
        outtextxy(timeX, WINDOW_HEIGHT / 2 + 20, survivalTimeText.c_str());

        wstring maxComboText = L"Max Combo: " + to_wstring(player.getComboCount()) + L"x";
        int comboWidth = textwidth(maxComboText.c_str());
        int comboX = (WINDOW_WIDTH - comboWidth) / 2;
        outtextxy(comboX, WINDOW_HEIGHT / 2 + 50, maxComboText.c_str());

        settextstyle(18, 0, L"Arial");
        settextcolor(Theme::ACCENT);
        wstring restartText = L"SPACE to return to menu";
        int restartWidth = textwidth(restartText.c_str());
        int restartX = (WINDOW_WIDTH - restartWidth) / 2;
        outtextxy(restartX, WINDOW_HEIGHT / 2 + 80, restartText.c_str());

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