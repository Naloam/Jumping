#include "Player.h"
#include "Platform.h" 
#include "Theme.h"
#include <vector>
#include <string>
#include <graphics.h>
#include <windows.h>
#include <ctime>
#include <cstdlib>

using namespace std;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

enum GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

class PlatformGenerator {
private:
    static const float MAX_JUMP_HEIGHT;    // 玩家最大跳跃高度
    static const float MAX_JUMP_DISTANCE;  // 玩家最大水平跳跃距离

public:
    // 根据难度获取随机平台类型
    PlatformType getRandomType(float difficulty) {
        int rand_val = rand() % 100;

        if (difficulty < 0.3f) {
            // 低难度：主要是普通平台
            if (rand_val < 80) return NORMAL;
            else if (rand_val < 95) return MOVING;
            else return BREAKABLE;
        }
        else if (difficulty < 0.7f) {
            // 中等难度
            if (rand_val < 60) return NORMAL;
            else if (rand_val < 85) return MOVING;
            else return BREAKABLE;
        }
        else {
            // 高难度
            if (rand_val < 40) return NORMAL;
            else if (rand_val < 70) return MOVING;
            else return BREAKABLE;
        }
    }

    Platform generateNextPlatform(const Platform& lastPlatform, float currentHeight, float difficulty) {
        // 计算安全的垂直距离
        float verticalGap = 60.0f + (difficulty * 20.0f);  // 随难度增加间距
        verticalGap = std::min(verticalGap, MAX_JUMP_HEIGHT * 0.8f);  // 确保可跳跃

        // 计算安全的水平距离
        float horizontalGap = 50.0f + (rand() % 100);  // 50-150像素随机
        horizontalGap = std::min(horizontalGap, MAX_JUMP_DISTANCE * 0.7f);

        // 确保平台在屏幕范围内
        float newX = lastPlatform.getX() + (rand() % 2 == 0 ? 1 : -1) * horizontalGap;
        newX = std::max(50.0f, std::min(newX, (float)WINDOW_WIDTH - 150.0f));

        float newY = lastPlatform.getY() - verticalGap;

        return Platform(newX, newY, 80 + rand() % 60, 20, getRandomType(difficulty));
    }

    // 生成随机平台（用于初始化和动态生成）
    Platform generateRandomPlatform(float y, float difficulty) {
        float x = 50.0f + rand() % (WINDOW_WIDTH - 200);
        float width = 80.0f + rand() % 80;  // 80-160像素宽度
        return Platform(x, y, width, 20, getRandomType(difficulty));
    }
};

// 静态常量定义
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

    // UI相关
    float fadeAlpha;

    // 相机相关
    float cameraTargetY;          // 相机目标位置
    float cameraSpeed;            // 相机跟随速度
    float cameraDeadZone;         // 相机死区（屏幕中线附近不移动的区域）

    // 游戏状态相关
    float worldSpeed;           // 世界上升速度
    float baseWorldSpeed;      // 基础上升速度
    float gameTime;            // 游戏时间
    float killZone;            // 死亡线位置

    // 输入状态管理
    bool spaceWasPressed;
    bool escWasPressed;

    // 平台生成器
    PlatformGenerator platformGenerator;

    // 平台生成相关
    float highestPlatformY;      // 最高平台的Y坐标
    float platformSpawnThreshold; // 平台生成阈值

public:
    Game() : currentState(MENU), player(100, 400), score(0), maxHeight(0), camera_y(0), fadeAlpha(0),
        cameraTargetY(0), cameraSpeed(3.0f), cameraDeadZone(80.0f),
        worldSpeed(0), baseWorldSpeed(50.0f), gameTime(0),
        spaceWasPressed(false), escWasPressed(false),
        highestPlatformY(0), platformSpawnThreshold(300.0f) {
        srand((unsigned int)time(nullptr)); // 初始化随机种子
        initializePlatforms();

        // 初始化死亡线位置（在玩家下方一定距离）
        killZone = player.getY() + 300.0f;
    }

    void initializePlatforms() {
        platforms.clear();

        // 地面平台
        platforms.push_back(Platform(0, WINDOW_HEIGHT - 40, WINDOW_WIDTH, 40, NORMAL));

        // 随机生成初始平台
        float currentY = WINDOW_HEIGHT - 100;
        highestPlatformY = currentY;

        for (int i = 0; i < 15; i++) {  // 生成15个初始平台
            currentY -= 80 + rand() % 60;  // 随机间距

            Platform newPlatform = platformGenerator.generateRandomPlatform(currentY, 0.2f);
            platforms.push_back(newPlatform);

            if (currentY < highestPlatformY) {
                highestPlatformY = currentY;
            }
        }
    }

    void generateNewPlatforms() {
        // 当相机上升到一定高度时，生成新平台
        if (camera_y < highestPlatformY + platformSpawnThreshold) {

            // 生成5-8个新平台
            int numNewPlatforms = 5 + rand() % 4;
            float currentDifficulty = std::min(1.0f, gameTime / 60.0f); // 基于时间的难度

            for (int i = 0; i < numNewPlatforms; i++) {
                float newY = highestPlatformY - (80 + rand() % 80); // 随机间距

                Platform newPlatform = platformGenerator.generateRandomPlatform(newY, currentDifficulty);
                platforms.push_back(newPlatform);

                highestPlatformY = newY;
            }
        }
    }

    void cleanupOldPlatforms() {
        // 清理相机下方很远的平台，避免内存占用过多
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
        // 更新输入状态
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
        // 管理按键状态，避免连续触发
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
            // 修复：正确退出游戏
            closegraph();
            exit(0);
        }
        if (!escPressed) escReleased = true;
    }

    void updateCamera(float deltaTime) {
        float playerScreenY = player.getY() - camera_y;
        float screenCenterY = WINDOW_HEIGHT / 2.0f;

        // 当玩家超过屏幕中线上方时，相机开始跟随
        if (playerScreenY < screenCenterY - cameraDeadZone) {
            cameraTargetY = player.getY() - screenCenterY;

            // 平滑插值跟随
            camera_y += (cameraTargetY - camera_y) * cameraSpeed * deltaTime;
        }

        // 限制相机不要过度向上移动
        if (camera_y < 0) camera_y = 0;
    }

    void updateGame(float deltaTime) {
        player.handleInput();
        player.update(deltaTime);

        // 更新相机
        updateCamera(deltaTime);

        // 更新世界移动
        updateWorldMovement(deltaTime);

        // 生成新平台
        generateNewPlatforms();

        // 清理旧平台
        cleanupOldPlatforms();

        // 平台更新
        for (auto& platform : platforms) {
            platform.update(deltaTime);
        }

        // 碰撞检测
        checkCollisions();

        // 边界检查
        player.checkBounds(WINDOW_WIDTH, WINDOW_HEIGHT);

        // 分数计算 - 修正分数计算逻辑
        float playerRealHeight = -(player.getY() - 400); // 相对于初始位置的高度
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

        // 游戏结束检查 - 修正死亡判定
        if (player.getY() > killZone) {
            currentState = GAME_OVER;
        }

        // 简单的死亡判定：如果玩家掉落到初始位置下方很远
        if (player.getY() > 600 + 200) {
            currentState = GAME_OVER;
        }
    }

    void updateWorldMovement(float deltaTime) {
        gameTime += deltaTime;

        // 根据时间和分数计算世界上升速度
        float timeSpeedMultiplier = 1.0f + (gameTime / 30.0f) * 0.5f;  // 每30秒增加50%速度
        float scoreSpeedMultiplier = 1.0f + (score / 100.0f) * 0.2f;   // 每100分增加20%速度

        worldSpeed = baseWorldSpeed * timeSpeedMultiplier * scoreSpeedMultiplier;

        // 更新死亡线位置 - 让死亡线跟随相机
        killZone = camera_y + WINDOW_HEIGHT + 100;

        // 移动所有平台（世界上升效果）
        for (auto& platform : platforms) {
            platform.moveY(worldSpeed * deltaTime);
        }

        // 更新最高平台位置
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

        // 修复：在游戏结束界面也可以用ESC退出
        if (escPressed && escReleased) {
            closegraph();
            exit(0);
        }
        if (!escPressed) escReleased = true;
    }

    void checkCollisions() {
        for (const auto& platform : platforms) {
            if (player.getX() < platform.getX() + platform.getWidth() &&
                player.getX() + player.getWidth() > platform.getX() &&
                player.getY() < platform.getY() + platform.getHeight() &&
                player.getY() + player.getHeight() > platform.getY()) {

                // 只有从上方落下才能站在平台上
                if (player.getY() < platform.getY()) {
                    player.setPosition(player.getX(), platform.getY() - player.getHeight());
                    player.setOnGround(true);
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

        // 重新随机生成平台
        initializePlatforms();
    }

    void render() {
        // 开始批量绘制（双缓冲）
        BeginBatchDraw();

        // 清屏 - 使用极简背景
        setbkcolor(Theme::BACKGROUND);
        cleardevice();

        switch (currentState) {
        case MENU:
            drawMenu();
            break;
        case PLAYING:
            drawGame();
            break;
        case PAUSED:
            drawGame(); // 先绘制游戏画面
            drawPause(); // 再绘制暂停遮罩
            break;
        case GAME_OVER:
            drawGame(); // 先绘制游戏画面
            drawGameOver(); // 再绘制游戏结束遮罩
            break;
        }

        // 结束批量绘制并显示
        EndBatchDraw();
    }

    void drawMenu() {
        // 使用主题色彩绘制极简菜单
        DrawUtils::drawSoftShadowRect(WINDOW_WIDTH / 2 - 150, 150, 300, 80, 10, Theme::PRIMARY);

        settextcolor(WHITE);
        settextstyle(40, 0, L"Arial");
        wstring title = L"Jump Game";
        int titleWidth = textwidth(title.c_str());
        int titleX = (WINDOW_WIDTH - titleWidth) / 2;
        outtextxy(titleX, 170, title.c_str());

        // 指令
        settextstyle(20, 0, L"Arial");
        settextcolor(Theme::TEXT_SECONDARY);

        vector<wstring> instructions = {
            L"Press SPACE to start",
            L"A/D or Arrow Keys to move",
            L"SPACE to jump",
            L"ESC to exit"
        };

        int startY = 280;
        for (int i = 0; i < instructions.size(); i++) {
            int textWidth = textwidth(instructions[i].c_str());
            int textX = (WINDOW_WIDTH - textWidth) / 2;
            outtextxy(textX, startY + i * 30, instructions[i].c_str());
        }
    }

    void drawGame() {
        // 绘制平台（应用相机偏移）
        for (const auto& platform : platforms) {
            float drawY = platform.getY() - camera_y;

            // 只绘制可见的平台
            if (drawY > -50 && drawY < WINDOW_HEIGHT + 50) {
                platform.drawWithOffset(0, -camera_y);
            }
        }

        // 绘制玩家（应用相机偏移）
        player.drawWithOffset(0, -camera_y);

        // 绘制死亡线指示器
        float deathLineY = killZone - camera_y;
        if (deathLineY > 0 && deathLineY < WINDOW_HEIGHT + 100) {
            setlinecolor(RGB(255, 100, 100));
            setlinestyle(PS_SOLID, 3);
            line(0, (int)deathLineY, WINDOW_WIDTH, (int)deathLineY);

            // 死亡线警告文字
            if (deathLineY < WINDOW_HEIGHT && deathLineY > WINDOW_HEIGHT - 100) {
                settextcolor(RGB(255, 150, 150));
                settextstyle(16, 0, L"Arial");
                outtextxy(WINDOW_WIDTH / 2 - 50, (int)deathLineY - 25, L"DANGER ZONE");
            }
        }

        // 绘制调试信息（可选）
        settextcolor(RGB(255, 255, 0));
        settextstyle(14, 0, L"Arial");
        wstring platformCountDebug = L"Platforms: " + to_wstring(platforms.size());
        outtextxy(250, 60, platformCountDebug.c_str());

        // 绘制UI
        drawGameUI();
    }

    void drawGameUI() {
        // 分数显示背景
        DrawUtils::drawSoftShadowRect(20, 20, 200, 100, 8, Theme::SURFACE);

        settextcolor(Theme::TEXT_PRIMARY);
        settextstyle(20, 0, L"Arial");
        wstring scoreText = L"Score: " + to_wstring(score);
        outtextxy(30, 30, scoreText.c_str());

        // 高度显示
        wstring heightText = L"Height: " + to_wstring(maxHeight);
        outtextxy(30, 55, heightText.c_str());

        // 游戏时间显示
        wstring timeText = L"Time: " + to_wstring((int)gameTime) + L"s";
        outtextxy(30, 80, timeText.c_str());

        // 控制提示
        settextstyle(14, 0, L"Arial");
        settextcolor(Theme::TEXT_DISABLED);
        outtextxy(30, WINDOW_HEIGHT - 100, L"A/D: Move");
        outtextxy(30, WINDOW_HEIGHT - 80, L"SPACE: Jump");
        outtextxy(30, WINDOW_HEIGHT - 60, L"ESC: Pause");
    }

    void drawPause() {
        // 半透明遮罩
        setfillcolor(DrawUtils::blendColor(RGB(0, 0, 0), RGB(255, 255, 255), 0.7f));
        solidrectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        // 暂停对话框
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
        // 半透明遮罩
        setfillcolor(DrawUtils::blendColor(RGB(0, 0, 0), RGB(255, 255, 255), 0.8f));
        solidrectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        // 游戏结束对话框
        DrawUtils::drawSoftShadowRect(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 120, 300, 240, 20, Theme::PRIMARY);

        settextcolor(WHITE);
        settextstyle(40, 0, L"Arial");
        wstring gameOverText = L"Game Over";
        int gameOverWidth = textwidth(gameOverText.c_str());
        int gameOverX = (WINDOW_WIDTH - gameOverWidth) / 2;
        outtextxy(gameOverX, WINDOW_HEIGHT / 2 - 80, gameOverText.c_str());

        // 最终分数
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

        settextstyle(18, 0, L"Arial");
        settextcolor(Theme::ACCENT);
        wstring restartText = L"SPACE to return to menu";
        int restartWidth = textwidth(restartText.c_str());
        int restartX = (WINDOW_WIDTH - restartWidth) / 2;
        outtextxy(restartX, WINDOW_HEIGHT / 2 + 60, restartText.c_str());

        wstring exitText = L"ESC to exit game";
        int exitWidth = textwidth(exitText.c_str());
        int exitX = (WINDOW_WIDTH - exitWidth) / 2;
        outtextxy(exitX, WINDOW_HEIGHT / 2 + 90, exitText.c_str());
    }
};

// 修复主循环，添加ESC退出处理
int main() {
    // 初始化图形窗口
    initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
    setbkmode(TRANSPARENT);

    // 设置窗口标题
    SetWindowText(GetHWnd(), L"Jump Game - EasyX Version");

    Game game;

    clock_t lastTime = clock();

    while (true) {
        // 全局ESC检查（紧急退出）
        if (GetAsyncKeyState(VK_F4) & 0x8000) {
            break; // 使用F4作为强制退出键
        }

        clock_t currentTime = clock();
        float deltaTime = (float)(currentTime - lastTime) / CLOCKS_PER_SEC;
        lastTime = currentTime;

        // 限制deltaTime避免大跳跃
        if (deltaTime > 0.033f) deltaTime = 0.033f;

        game.update(deltaTime);
        game.render();

        Sleep(16); // 约60 FPS
    }

    closegraph();
    return 0;
}