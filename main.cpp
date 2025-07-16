#include "Player.h"
#include "Platform.h" 
#include "Theme.h"
#include "AudioManager.h"
#include <vector>
#include <string>
#include <cmath>
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
    HELP,
    AUDIO_SETTINGS,
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

    std::vector<Obstacle> obstacles;
    std::vector<Coin> coins;

    // 新增：生成器
    float obstacleSpawnTimer;
    float coinSpawnTimer;
    float obstacleSpawnRate;
    float coinSpawnRate;

    struct Button {
        int x, y, width, height;
        wstring text;
        bool isHovered;

        Button(int x, int y, int width, int height, const wstring& text)
            : x(x), y(y), width(width), height(height), text(text), isHovered(false) {
        }

        bool isPointInside(int px, int py) const {
            return px >= x && px <= x + width && py >= y && py <= y + height;
        }
    };

    // 新增：菜单按钮
    Button startButton;
    Button helpButton;
    Button backButton;  // 帮助页面的返回按钮

    // 新增：鼠标状态
    bool mouseWasPressed;
    int mouseX, mouseY;

    float helpScrollOffset;      // 帮助页面滚动偏移
    float maxHelpScrollOffset;   // 最大滚动偏移
    const float HELP_SCROLL_SPEED = 30.0f;  // 滚动速度

    AudioManager& audioManager;
    // 新增：音频设置相关变量
    Button audioSettingsButton;
    Button backFromAudioButton;
    Button muteButton;
    Button masterVolumeDownButton;
    Button masterVolumeUpButton;
    Button musicVolumeDownButton;
    Button musicVolumeUpButton;
    Button sfxVolumeDownButton;
    Button sfxVolumeUpButton;

public:
    Game() : currentState(MENU), player(100, 400), score(0), maxHeight(0), camera_y(0), fadeAlpha(0),
        cameraTargetY(0), cameraSpeed(3.0f), cameraDeadZone(80.0f),
        maxCameraSpeed(4.5f), cameraSpeedLimit(600.0f),
        worldSpeed(0), baseWorldSpeed(20.0f), gameTime(0),
        spaceWasPressed(false), escWasPressed(false),
        highestPlatformY(0), platformSpawnThreshold(30.0f),
        smoothCameraSpeed(3.0f),
        cameraSpeedAcceleration(0.5f),
        maxSafeCameraSpeed(8.0f),
        averagePlayerVerticalSpeed(0.0f),
        lastPlayerY(0.0f),
        playerSpeedSampleTime(0.0f),
        maxWorldSpeed(60.0f),
        worldSpeedSmoothing(2.0f),
        obstacleSpawnTimer(0.0f),
        coinSpawnTimer(0.0f),
        obstacleSpawnRate(5.0f),
        coinSpawnRate(6.0f),
        helpScrollOffset(0.0f), maxHelpScrollOffset(0.0f),
        startButton(WINDOW_WIDTH / 2 - 100, 300, 200, 50, L"Start Game"),
        helpButton(WINDOW_WIDTH / 2 - 100, 370, 200, 50, L"Help"),
        audioSettingsButton(WINDOW_WIDTH / 2 - 100, 440, 200, 50, L"Audio Settings"),  // 新增
        backButton(WINDOW_WIDTH / 2 - 100, 650, 200, 50, L"Back to Menu"),
        muteButton(WINDOW_WIDTH / 2 - 100, 180, 200, 50, L"Toggle Mute"),
        masterVolumeDownButton(WINDOW_WIDTH / 2 - 250, 260, 100, 35, L"Master -"),
        masterVolumeUpButton(WINDOW_WIDTH / 2 + 150, 260, 100, 35, L"Master +"),
        musicVolumeDownButton(WINDOW_WIDTH / 2 - 250, 320, 100, 35, L"Music -"),
        musicVolumeUpButton(WINDOW_WIDTH / 2 + 150, 320, 100, 35, L"Music +"),
        sfxVolumeDownButton(WINDOW_WIDTH / 2 - 250, 380, 100, 35, L"SFX -"),
        sfxVolumeUpButton(WINDOW_WIDTH / 2 + 150, 380, 100, 35, L"SFX +"),
        backFromAudioButton(WINDOW_WIDTH / 2 - 100, 650, 200, 50, L"Back to Menu"),

        mouseWasPressed(false), mouseX(0), mouseY(0),
        audioManager(AudioManager::getInstance()) {

        srand((unsigned int)time(nullptr));
        // 初始化音频系统
        audioManager.initialize();
        audioManager.onMenuEnter();  // 播放菜单音乐
        initializePlatforms();
        positionPlayerOnStartPlatform();

        // 修复：使用地面作为基准点，而不是起始平台
        initialPlayerY = WINDOW_HEIGHT - 40;  // 地面平台的顶部

        lastPlayerY = player.getY();
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
		case HELP:
            updateHelp();
			break;
        case AUDIO_SETTINGS:
            updateAudioSettings();
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

    // 在Game类中添加音频设置方法
    void toggleAudio() {
        audioManager.setAudioEnabled(!audioManager.isAudioEnabled());
    }

    void adjustMasterVolume(float delta) {
        float newVolume = audioManager.getMasterVolume() + delta;
        audioManager.setMasterVolume(newVolume);
    }

    void adjustMusicVolume(float delta) {
        float newVolume = audioManager.getMusicVolume() + delta;
        audioManager.setMusicVolume(newVolume);
    }

    void adjustSFXVolume(float delta) {
        float newVolume = audioManager.getSFXVolume() + delta;
        audioManager.setSFXVolume(newVolume);
    }

    // 在main函数中添加音频控制快捷键
    void handleAudioControls() {
        static bool mReleased = true;
        static bool nReleased = true;
        static bool bReleased = true;
        static bool vReleased = true;  // 新增：音量调节键

        bool mPressed = GetAsyncKeyState('M') & 0x8000;  // 静音/取消静音
        bool nPressed = GetAsyncKeyState('N') & 0x8000;  // 降低音量
        bool bPressed = GetAsyncKeyState('B') & 0x8000;  // 增加音量
        bool vPressed = GetAsyncKeyState('V') & 0x8000;  // 打开音频设置

        if (mPressed && mReleased) {
            toggleAudio();
            mReleased = false;
        }
        if (!mPressed) mReleased = true;

        if (nPressed && nReleased) {
            adjustMasterVolume(-0.1f);
            nReleased = false;
        }
        if (!nPressed) nReleased = true;

        if (bPressed && bReleased) {
            adjustMasterVolume(0.1f);
            bReleased = false;
        }
        if (!bPressed) bReleased = true;

        // 新增：V键打开音频设置
        if (vPressed && vReleased && currentState == MENU) {
            currentState = AUDIO_SETTINGS;
            vReleased = false;
        }
        if (!vPressed) vReleased = true;
    }

    // 新增：更新音频设置状态
    void updateAudioSettings() {
        static bool backReleased = true;
        static bool mouseReleased = true;

        bool backPressed = GetAsyncKeyState(VK_BACK) & 0x8000;
        bool mousePressed = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

        // 更新按钮悬停状态
        backFromAudioButton.isHovered = backFromAudioButton.isPointInside(mouseX, mouseY);
        muteButton.isHovered = muteButton.isPointInside(mouseX, mouseY);
        masterVolumeDownButton.isHovered = masterVolumeDownButton.isPointInside(mouseX, mouseY);
        masterVolumeUpButton.isHovered = masterVolumeUpButton.isPointInside(mouseX, mouseY);
        musicVolumeDownButton.isHovered = musicVolumeDownButton.isPointInside(mouseX, mouseY);
        musicVolumeUpButton.isHovered = musicVolumeUpButton.isPointInside(mouseX, mouseY);
        sfxVolumeDownButton.isHovered = sfxVolumeDownButton.isPointInside(mouseX, mouseY);
        sfxVolumeUpButton.isHovered = sfxVolumeUpButton.isPointInside(mouseX, mouseY);

        // 按钮点击处理
        if (mousePressed && mouseReleased) {
            if (backFromAudioButton.isHovered) {
                audioManager.playSound(SoundType::BUTTON_CLICK, false);
                currentState = MENU;
            }
            else if (muteButton.isHovered) {
                audioManager.playSound(SoundType::BUTTON_CLICK, false);
                toggleAudio();
            }
            else if (masterVolumeDownButton.isHovered) {
                audioManager.playSound(SoundType::BUTTON_CLICK, false);
                adjustMasterVolume(-0.1f);
            }
            else if (masterVolumeUpButton.isHovered) {
                audioManager.playSound(SoundType::BUTTON_CLICK, false);
                adjustMasterVolume(0.1f);
            }
            else if (musicVolumeDownButton.isHovered) {
                audioManager.playSound(SoundType::BUTTON_CLICK, false);
                adjustMusicVolume(-0.1f);
            }
            else if (musicVolumeUpButton.isHovered) {
                audioManager.playSound(SoundType::BUTTON_CLICK, false);
                adjustMusicVolume(0.1f);
            }
            else if (sfxVolumeDownButton.isHovered) {
                audioManager.playSound(SoundType::BUTTON_CLICK, false);
                adjustSFXVolume(-0.1f);
            }
            else if (sfxVolumeUpButton.isHovered) {
                audioManager.playSound(SoundType::BUTTON_CLICK, false);
                adjustSFXVolume(0.1f);
            }
            mouseReleased = false;
        }
        if (!mousePressed) mouseReleased = true;

        // 返回菜单
        if (backPressed && backReleased) {
            audioManager.playSound(SoundType::BUTTON_CLICK, false);
            currentState = MENU;
            backReleased = false;
        }
        if (!backPressed) backReleased = true;
    }

    // 新增：绘制音频设置界面
    void drawAudioSettings() {
        // 绘制背景
        setbkcolor(Theme::BACKGROUND);
        cleardevice();

        // 绘制标题
        settextcolor(RGB(255, 255, 255));
        settextstyle(48, 0, L"Arial");
        wstring title = L"Audio Settings";
        int titleWidth = textwidth(title.c_str());
        int titleX = (WINDOW_WIDTH - titleWidth) / 2;
        outtextxy(titleX, 80, title.c_str());

        // 绘制音频状态
        settextstyle(24, 0, L"Arial");
        wstring audioStatus = audioManager.isAudioEnabled() ? L"Audio: ON" : L"Audio: OFF";
        COLORREF statusColor = audioManager.isAudioEnabled() ? RGB(0, 255, 0) : RGB(255, 0, 0);
        settextcolor(statusColor);
        int statusWidth = textwidth(audioStatus.c_str());
        outtextxy((WINDOW_WIDTH - statusWidth) / 2, 150, audioStatus.c_str());

        // 修改：调整音量条和文字布局，为按钮留出更多空间
        settextcolor(RGB(255, 255, 255));
        settextstyle(20, 0, L"Arial");

        // 音量条的左右边距增加，为按钮留出空间
        int volumeBarX = WINDOW_WIDTH / 2 - 150;  // 增加左边距
        int volumeBarWidth = 300;  // 增加宽度
        int volumeBarHeight = 20;

        // 主音量 - 调整位置和间距
        wstring masterVolumeText = L"Master Volume: " + std::to_wstring((int)(audioManager.getMasterVolume() * 100)) + L"%";
        int masterTextWidth = textwidth(masterVolumeText.c_str());
        outtextxy((WINDOW_WIDTH - masterTextWidth) / 2, 240, masterVolumeText.c_str());  // 向上移动
        drawVolumeBar(volumeBarX, 265, volumeBarWidth, volumeBarHeight, audioManager.getMasterVolume());

        // 音乐音量 - 调整位置和间距
        wstring musicVolumeText = L"Music Volume: " + std::to_wstring((int)(audioManager.getMusicVolume() * 100)) + L"%";
        int musicTextWidth = textwidth(musicVolumeText.c_str());
        outtextxy((WINDOW_WIDTH - musicTextWidth) / 2, 300, musicVolumeText.c_str());  // 向上移动
        drawVolumeBar(volumeBarX, 325, volumeBarWidth, volumeBarHeight, audioManager.getMusicVolume());

        // 音效音量 - 调整位置和间距
        wstring sfxVolumeText = L"SFX Volume: " + std::to_wstring((int)(audioManager.getSFXVolume() * 100)) + L"%";
        int sfxTextWidth = textwidth(sfxVolumeText.c_str());
        outtextxy((WINDOW_WIDTH - sfxTextWidth) / 2, 360, sfxVolumeText.c_str());  // 向上移动
        drawVolumeBar(volumeBarX, 385, volumeBarWidth, volumeBarHeight, audioManager.getSFXVolume());

        // 绘制按钮 - 现在按钮位置已经调整，不会遮挡文字
        drawButton(muteButton, RGB(100, 100, 100), RGB(150, 150, 150), RGB(255, 255, 255));
        drawButton(masterVolumeDownButton, RGB(80, 80, 80), RGB(120, 120, 120), RGB(255, 255, 255));
        drawButton(masterVolumeUpButton, RGB(80, 80, 80), RGB(120, 120, 120), RGB(255, 255, 255));
        drawButton(musicVolumeDownButton, RGB(80, 80, 80), RGB(120, 120, 120), RGB(255, 255, 255));
        drawButton(musicVolumeUpButton, RGB(80, 80, 80), RGB(120, 120, 120), RGB(255, 255, 255));
        drawButton(sfxVolumeDownButton, RGB(80, 80, 80), RGB(120, 120, 120), RGB(255, 255, 255));
        drawButton(sfxVolumeUpButton, RGB(80, 80, 80), RGB(120, 120, 120), RGB(255, 255, 255));
        drawButton(backFromAudioButton, RGB(100, 100, 100), RGB(150, 150, 150), RGB(255, 255, 255));

        // 绘制快捷键提示 - 向下移动以适应新布局
        settextcolor(RGB(150, 150, 150));
        settextstyle(16, 0, L"Arial");
        vector<wstring> shortcuts = {
            L"M: Toggle Mute",
            L"N: Volume Down",
            L"B: Volume Up",
            L"Backspace: Return to Menu"
        };

        int shortcutY = 500;  // 向下移动
        for (const auto& shortcut : shortcuts) {
            int shortcutWidth = textwidth(shortcut.c_str());
            outtextxy((WINDOW_WIDTH - shortcutWidth) / 2, shortcutY, shortcut.c_str());
            shortcutY += 20;
        }
    }

    // 新增：绘制音量条
    void drawVolumeBar(int x, int y, int width, int height, float volume) {
        // 背景
        setfillcolor(RGB(50, 50, 50));
        solidrectangle(x, y, x + width, y + height);

        // 音量条
        int volumeWidth = (int)(width * volume);
        COLORREF volumeColor = RGB(0, 255, 0);
        if (volume > 0.8f) volumeColor = RGB(255, 255, 0);
        if (volume > 0.9f) volumeColor = RGB(255, 0, 0);

        setfillcolor(volumeColor);
        solidrectangle(x, y, x + volumeWidth, y + height);

        // 边框
        setlinecolor(RGB(200, 200, 200));
        setlinestyle(PS_SOLID, 1);
        rectangle(x, y, x + width, y + height);
    }

    void updateInputState() {
        bool spacePressed = GetAsyncKeyState(VK_SPACE) & 0x8000;
        bool escPressed = GetAsyncKeyState(VK_ESCAPE) & 0x8000;
        spaceWasPressed = spacePressed;
        escWasPressed = escPressed;

        // 鼠标输入处理
        bool mousePressed = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
        mouseWasPressed = mousePressed;

        // 获取鼠标位置
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        ScreenToClient(GetHWnd(), &cursorPos);
        mouseX = cursorPos.x;
        mouseY = cursorPos.y;

        // 修复：改进滚动处理（仅在帮助页面时）
        if (currentState == HELP) {
            // 添加按键释放检查，防止连续触发
            static bool upReleased = true;
            static bool downReleased = true;
            static bool priorReleased = true;
            static bool nextReleased = true;

            bool upPressed = GetAsyncKeyState(VK_UP) & 0x8000;
            bool downPressed = GetAsyncKeyState(VK_DOWN) & 0x8000;
            bool priorPressed = GetAsyncKeyState(VK_PRIOR) & 0x8000;  // Page Up
            bool nextPressed = GetAsyncKeyState(VK_NEXT) & 0x8000;    // Page Down

            // 向上滚动
            if ((upPressed && upReleased) || (priorPressed && priorReleased)) {
                float scrollAmount = upPressed ? HELP_SCROLL_SPEED * 0.5f : HELP_SCROLL_SPEED;
                helpScrollOffset -= scrollAmount;
                if (helpScrollOffset < 0) helpScrollOffset = 0;

                upReleased = false;
                priorReleased = false;
            }

            // 向下滚动
            if ((downPressed && downReleased) || (nextPressed && nextReleased)) {
                float scrollAmount = downPressed ? HELP_SCROLL_SPEED * 0.5f : HELP_SCROLL_SPEED;
                helpScrollOffset += scrollAmount;
                // 重要：确保 maxHelpScrollOffset 在绘制时正确计算
                if (maxHelpScrollOffset > 0 && helpScrollOffset > maxHelpScrollOffset) {
                    helpScrollOffset = maxHelpScrollOffset;
                }

                downReleased = false;
                nextReleased = false;
            }

            // 更新按键释放状态
            if (!upPressed) upReleased = true;
            if (!downPressed) downReleased = true;
            if (!priorPressed) priorReleased = true;
            if (!nextPressed) nextReleased = true;
        }
    }

    void updateMenu() {
        static bool spaceReleased = true;
        static bool escReleased = true;
        static bool hReleased = true;
        static bool vReleased = true;  // 新增：V键
        static bool mouseReleased = true;

        bool spacePressed = GetAsyncKeyState(VK_SPACE) & 0x8000;
        bool escPressed = GetAsyncKeyState(VK_ESCAPE) & 0x8000;
        bool hPressed = GetAsyncKeyState('H') & 0x8000;
        bool vPressed = GetAsyncKeyState('V') & 0x8000;  // 新增：V键
        bool mousePressed = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

        // 更新按钮悬停状态
        bool wasStartHovered = startButton.isHovered;
        bool wasHelpHovered = helpButton.isHovered;
        bool wasAudioHovered = audioSettingsButton.isHovered;  // 新增

        startButton.isHovered = startButton.isPointInside(mouseX, mouseY);
        helpButton.isHovered = helpButton.isPointInside(mouseX, mouseY);
        audioSettingsButton.isHovered = audioSettingsButton.isPointInside(mouseX, mouseY);  // 新增

        // 播放悬停音效
        if (!wasStartHovered && startButton.isHovered) {
            audioManager.playSound(SoundType::BUTTON_HOVER, false);
        }
        if (!wasHelpHovered && helpButton.isHovered) {
            audioManager.playSound(SoundType::BUTTON_HOVER, false);
        }
        if (!wasAudioHovered && audioSettingsButton.isHovered) {  // 新增
            audioManager.playSound(SoundType::BUTTON_HOVER, false);
        }

        // 按钮点击处理
        if (mousePressed && mouseReleased) {
            if (startButton.isHovered) {
                audioManager.playSound(SoundType::BUTTON_CLICK, false);
                currentState = PLAYING;
                audioManager.onGameStart();
                resetGame();
            }
            else if (helpButton.isHovered) {
                audioManager.playSound(SoundType::BUTTON_CLICK, false);
                currentState = HELP;
            }
            else if (audioSettingsButton.isHovered) {  // 新增
                audioManager.playSound(SoundType::BUTTON_CLICK, false);
                currentState = AUDIO_SETTINGS;
            }
            mouseReleased = false;
        }
        if (!mousePressed) mouseReleased = true;

        // 键盘快捷键
        if (spacePressed && spaceReleased) {
            currentState = PLAYING;
            audioManager.onGameStart();
            resetGame();
            spaceReleased = false;
        }
        if (!spacePressed) spaceReleased = true;

        if (hPressed && hReleased) {
            audioManager.playSound(SoundType::BUTTON_CLICK, false);
            currentState = HELP;
            hReleased = false;
        }
        if (!hPressed) hReleased = true;

        if (vPressed && vReleased) {  // 新增：V键进入音频设置
            audioManager.playSound(SoundType::BUTTON_CLICK, false);
            currentState = AUDIO_SETTINGS;
            vReleased = false;
        }
        if (!vPressed) vReleased = true;

        if (escPressed && escReleased) {
            audioManager.cleanup();
            closegraph();
            exit(0);
        }
        if (!escPressed) escReleased = true;
    }

    void updateHelp() {
        static bool backspaceReleased = true;
        static bool mouseReleased = true;

        bool backspacePressed = GetAsyncKeyState(VK_BACK) & 0x8000;
        bool mousePressed = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

        // 更新返回按钮悬停状态
        bool wasBackHovered = backButton.isHovered;
        Button fixedBackButton = backButton;
        fixedBackButton.y = WINDOW_HEIGHT - 80;
        backButton.isHovered = fixedBackButton.isPointInside(mouseX, mouseY);

        // 新增：播放悬停音效
        if (!wasBackHovered && backButton.isHovered) {
            audioManager.playSound(SoundType::BUTTON_HOVER, false);
        }

        // 返回菜单
        if ((backspacePressed && backspaceReleased) ||
            (mousePressed && mouseReleased && backButton.isHovered)) {
            audioManager.playSound(SoundType::BUTTON_CLICK, false);  // 新增：按钮点击音效
            currentState = MENU;
            audioManager.onMenuEnter();  // 新增：切换到菜单音乐
            helpScrollOffset = 0.0f;
            backspaceReleased = false;
            mouseReleased = false;
        }
        if (!backspacePressed) backspaceReleased = true;
        if (!mousePressed) mouseReleased = true;
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

        /*if (camera_y < 0) camera_y = 0;*/
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

        // 修复：添加障碍物和金币系统更新
        updateObstacles(deltaTime);
        updateCoins(deltaTime);
        spawnObstacles(deltaTime);
        spawnCoins(deltaTime);

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

        // 修复：添加障碍物和金币碰撞检测
        checkObstacleCollisions();
        checkCoinCollection();

        // 碰撞检测
        checkCollisions();

        player.checkBounds(WINDOW_WIDTH, WINDOW_HEIGHT);

        // 分数计算
        updateScore();

        // 暂停检查
        static bool pReleased = true;
        bool pPressed = GetAsyncKeyState('P') & 0x8000;

        if (pPressed && pReleased) {
            audioManager.onGamePause();
            currentState = PAUSED;
            pReleased = false;
        }
        if (!pPressed) pReleased = true;

        // 游戏结束检查
        if (player.getY() > killZone || player.isDead()) {
            if (player.canTakeDamage()) {
                audioManager.onGameOver();
                currentState = GAME_OVER;
            }
            else if (!player.isDead()) {
                respawnPlayerToSafePlatform();
            }
            else {
                audioManager.onGameOver();
                currentState = GAME_OVER;
            }
        }
    }

    void updateObstacles(float deltaTime) {
        // 更新所有障碍物
        for (auto& obstacle : obstacles) {
            obstacle.update(deltaTime, worldSpeed);
        }

        // 移除非活跃的障碍物
        obstacles.erase(
            std::remove_if(obstacles.begin(), obstacles.end(),
                [](const Obstacle& obs) { return !obs.isActive(); }),
            obstacles.end()
        );
    }

    void updateCoins(float deltaTime) {
        // 更新所有金币
        for (auto& coin : coins) {
            coin.update(deltaTime, worldSpeed);

            // 如果玩家有磁场效果，应用磁化
            if (player.hasMagneticFieldActive()) {
                coin.applyMagnetism(player.getX() + player.getWidth() / 2,
                    player.getY() + player.getHeight() / 2,
                    player.getMagnetRadius(), deltaTime);
            }
        }

        // 移除已收集的金币
        coins.erase(
            std::remove_if(coins.begin(), coins.end(),
                [](const Coin& coin) { return coin.isCollected(); }),
            coins.end()
        );
    }

    void spawnObstacles(float deltaTime) {
        obstacleSpawnTimer += deltaTime;

        if (obstacleSpawnTimer >= obstacleSpawnRate) {
            float difficulty = std::min(1.0f, gameTime / 60.0f);  // 1分钟内达到最大难度

            // 根据难度调整生成率（更频繁）
            obstacleSpawnRate = std::max(1.0f, 4.0f - difficulty * 2.0f);  // 从4秒降到1秒

            // 随机选择障碍物类型
            ObstacleType type = static_cast<ObstacleType>(rand() % 6);

            // 随机位置（在屏幕上方生成）
            float spawnX = 50.0f + rand() % (WINDOW_WIDTH - 150);
            float spawnY = camera_y - 200;  // 在相机上方200像素处生成

            obstacles.push_back(Obstacle(spawnX, spawnY, type));
            obstacleSpawnTimer = 0.0f;
        }
    }

    void spawnCoins(float deltaTime) {
        coinSpawnTimer += deltaTime;

        if (coinSpawnTimer >= coinSpawnRate) {
            // 修复：调整金币生成频率
            coinSpawnRate = 4.0f + (rand() % 4); // 4-7秒生成一个金币

            // 在平台附近生成金币
            if (!platforms.empty()) {
                // 寻找没有道具的普通平台
                std::vector<int> availablePlatforms;
                for (size_t i = 0; i < platforms.size(); i++) {
                    if (platforms[i].getItem() == nullptr &&
                        platforms[i].getType() == NORMAL &&
                        platforms[i].getY() < camera_y + 200 &&  // 确保在相机视野内
                        platforms[i].getY() > camera_y - 400) {  // 不要太远
                        availablePlatforms.push_back((int)i);
                    }
                }

                if (!availablePlatforms.empty()) {
                    int randomIndex = availablePlatforms[rand() % availablePlatforms.size()];
                    const Platform& platform = platforms[randomIndex];

                    float coinX = platform.getX() + platform.getWidth() / 2;
                    float coinY = platform.getY() - 30;

                    int coinValue = 10 + rand() % 15; // 10-25分
                    coins.push_back(Coin(coinX, coinY, coinValue));
                }
            }

            coinSpawnTimer = 0.0f;
        }
    }

    void checkObstacleCollisions() {
        for (auto& obstacle : obstacles) {
            if (obstacle.isActive() &&
                obstacle.checkCollision(player.getX(), player.getY(),
                    player.getWidth(), player.getHeight())) {

                // 如果障碍物被冻结，跳过伤害
                if (player.hasObstaclesFrozen()) continue;

                // 只有在可以受伤害时才造成伤害
                if (player.canTakeDamage()) {
                    player.takeDamage((int)obstacle.getDamage());

                    // 播放受伤音效
                    audioManager.playSound(SoundType::OBSTACLE_HIT, false);
                    // 添加屏幕震动
                    player.addScreenShake(3.0f);
                }

                break;
            }
        }
    }

    void checkCoinCollection() {
        for (auto& coin : coins) {
            if (!coin.isCollected() &&
                coin.checkCollision(player.getX(), player.getY(),
                    player.getWidth(), player.getHeight())) {

                // 收集金币
                player.collectCoin(coin.getValue());
                coin.collect();

                // 播放金币收集音效
                audioManager.playSound(SoundType::COIN_COLLECT, false);

                // 添加收集特效
                player.addScreenShake(1.0f);

                // 立即退出循环，避免重复收集
                break;
            }
        }
    }

    // 更新分数系统
    void updateScore() {
        // 使用地面作为基准计算高度
        float currentHeightFloat = initialPlayerY - player.getY();

        // 确保高度为正值
        if (currentHeightFloat < 0) currentHeightFloat = 0;

        // 转换为 long long，避免精度损失
        long long currentHeight = (long long)std::round(currentHeightFloat);

        // 更新最大高度 - 确保没有上限
        if (currentHeight > maxHeight) {
            maxHeight = currentHeight;
        }

        // 总分数计算
        long long heightScore = maxHeight / 5;
        long long bonusScore = player.getBonusScore();
        long long comboBonus = player.getComboCount() * 10;

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
        float timeSpeedMultiplier = 1.0f + (gameTime / 60.0f) * 0.3f;
        float scoreSpeedMultiplier = 1.0f + (score / 500.0f) * 0.1f;

        // 计算目标世界速度
        float targetWorldSpeed = baseWorldSpeed * timeSpeedMultiplier * scoreSpeedMultiplier;

        // 确保世界速度不会过快，避免影响高度计算
        if (averagePlayerVerticalSpeed > 0) {
            float maxAllowedWorldSpeed = averagePlayerVerticalSpeed * 0.6f; // 降低到60%
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
        static bool pReleased = true;
        static bool escReleased = true;

        bool pPressed = GetAsyncKeyState('P') & 0x8000;
        bool escPressed = GetAsyncKeyState(VK_ESCAPE) & 0x8000;

        if (pPressed && pReleased) {
            audioManager.onGameResume();
            currentState = PLAYING;
            pReleased = false;
        }
        if (!pPressed) pReleased = true;

        if (escPressed && escReleased) {
            audioManager.onMenuEnter();
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
        Platform* landedPlatform = nullptr;  // 记录着陆的平台

        for (auto& platform : platforms) {
            // 跳过已破碎的平台的碰撞检测
            if (platform.isBrokenPlatform()) {
                continue;
            }

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
                bool isSpringTriggered = false;

                platform.handleCollision(player.getX(), player.getY(), player.getWidth(), player.getHeight(), playerVY);

                // 检查是否是弹簧平台触发
                if (platform.getType() == SPRING && playerVY < 0) {
                    isSpringTriggered = true;
                }

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

                // 修复：弹簧平台也应该被认为是地面，但弹簧触发时不设置onGround
                if (platform.getType() == SPRING) {
                    if (isSpringTriggered) {
                        // 弹簧触发时，给玩家一个短暂的地面状态，然后立即弹起
                        foundGroundCollision = true;
                        player.addBonusScore(25);
                        // 注意：不要立即设置为false，让Player的update方法处理
                    }
                    else {
                        // 弹簧平台未触发时，正常当作地面
                        foundGroundCollision = true;
                    }
                }
                else {
                    foundGroundCollision = true;
                }

                landedPlatform = &platform;  // 记录着陆平台

                // 收集道具 - 恢复所有道具类型处理
                Item* item = platform.collectItem();
                if (item) {
                    switch (item->type) {
                    case SPEED_BOOST:
                        player.applySpeedBoost();
                        break;
                    case SHIELD:
                        player.applyShield();
                        break;
                    case HEALTH_BOOST:
                        player.applyHealthBoost();
                        break;
                    case INVINCIBILITY:
                        player.applyInvincibility();
                        break;
                    case DOUBLE_JUMP:
                        player.applyDoubleJump();
                        break;
                    case SLOW_TIME:
                        player.applySlowTime();
                        break;
                    case MAGNETIC_FIELD:
                        player.applyMagneticField();
                        break;
                    case FREEZE_OBSTACLES:
                        player.applyFreezeObstacles();
                        break;
                    case COIN:
                        player.collectCoin(item->value);
                        break;
                    default:
                        break;
                    }
                }

                break;
            }
        }

        // 设置地面状态
        player.setOnGround(foundGroundCollision);

        // 如果玩家着陆在平台上，更新combo系统
        if (foundGroundCollision && landedPlatform != nullptr) {
            player.updateComboSystem(landedPlatform->getY());
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

        // 重置镜头平滑控制
        smoothCameraSpeed = 3.0f;

        // 重置玩家速度统计
        playerVerticalSpeedSamples.clear();
        averagePlayerVerticalSpeed = 0.0f;
        playerSpeedSampleTime = 0.0f;

        // 重置障碍物和金币系统
        obstacles.clear();
        coins.clear();
        obstacleSpawnTimer = 0.0f;
        coinSpawnTimer = 0.0f;
        obstacleSpawnRate = 5.0f;
        coinSpawnRate = 6.0f;

        initializePlatforms();
        positionPlayerOnStartPlatform();

        // 修复：保持使用地面作为基准
        initialPlayerY = WINDOW_HEIGHT - 40;  // 地面平台的顶部

        lastPlayerY = player.getY();
        killZone = player.getY() + 300.0f;
    }

    void render() {
        BeginBatchDraw();

        // 应用屏幕震动（仅在游戏中）
        float shakeX = 0, shakeY = 0;
        if (currentState == PLAYING) {
            player.getShakeOffset(shakeX, shakeY);
        }

        setbkcolor(Theme::BACKGROUND);
        cleardevice();

        switch (currentState) {
        case MENU:
            drawMenu();
            break;
        case HELP:        
            drawHelp();
            break;
		case AUDIO_SETTINGS:
            drawAudioSettings();
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
        // 绘制背景渐变
        for (int i = 0; i < WINDOW_HEIGHT; i++) {
            float ratio = (float)i / WINDOW_HEIGHT;
            COLORREF bgColor = DrawUtils::interpolateColor(
                Theme::BACKGROUND,
                DrawUtils::adjustBrightness(Theme::BACKGROUND, 0.8f),
                ratio
            );
            setlinecolor(bgColor);
            line(0, i, WINDOW_WIDTH, i);
        }

        // 绘制游戏标题
        settextcolor(Theme::PRIMARY_DARK);
        settextstyle(60, 0, L"Arial");
        wstring title = L"Jump Game";
        int titleWidth = textwidth(title.c_str());
        int titleX = (WINDOW_WIDTH - titleWidth) / 2;

        // 标题阴影
        settextcolor(RGB(100, 100, 100));
        outtextxy(titleX + 3, 120 + 3, title.c_str());

        // 标题主体
        settextcolor(Theme::PRIMARY);
        outtextxy(titleX, 120, title.c_str());

        // 绘制副标题
        settextcolor(Theme::TEXT_SECONDARY);
        settextstyle(20, 0, L"Arial");
        wstring subtitle = L"A Challenging Platform Adventure";
        int subtitleWidth = textwidth(subtitle.c_str());
        int subtitleX = (WINDOW_WIDTH - subtitleWidth) / 2;
        outtextxy(subtitleX, 200, subtitle.c_str());

        // 绘制按钮
        drawButton(startButton, Theme::PRIMARY, Theme::PRIMARY_LIGHT, RGB(255, 255, 255));
        drawButton(helpButton, Theme::SECONDARY, Theme::PRIMARY_LIGHT, RGB(255, 255, 255));
        drawButton(audioSettingsButton, RGB(100, 150, 200), RGB(150, 200, 255), RGB(255, 255, 255));

        // 绘制控制提示
        settextcolor(Theme::TEXT_DISABLED);
        settextstyle(16, 0, L"Arial");
        vector<wstring> hints = {
            L"Press SPACE or click Start to begin",
            L"Press H or click Help for instructions",
			L"Press V or click for Audio Settings",
            L"Press ESC to exit"
        };

        int hintY = 520;
        for (const auto& hint : hints) {
            int hintWidth = textwidth(hint.c_str());
            int hintX = (WINDOW_WIDTH - hintWidth) / 2;
            outtextxy(hintX, hintY, hint.c_str());
            hintY += 25;
        }

        // 绘制版本信息
        settextcolor(Theme::TEXT_DISABLED);
        settextstyle(14, 0, L"Arial");
        wstring version = L"Version 1.0 - EasyX Graphics";
        int versionWidth = textwidth(version.c_str());
        outtextxy(WINDOW_WIDTH - versionWidth - 20, WINDOW_HEIGHT - 30, version.c_str());
    }

    void drawHelp() {
        // 绘制背景
        setbkcolor(Theme::BACKGROUND);
        cleardevice();

        // 创建滚动视口
        int viewportY = -(int)helpScrollOffset;
        int contentStartY = viewportY;

        // 绘制标题
        settextcolor(Theme::PRIMARY);
        settextstyle(40, 0, L"Arial");
        wstring title = L"Game Help";
        int titleWidth = textwidth(title.c_str());
        int titleX = (WINDOW_WIDTH - titleWidth) / 2;
        if (contentStartY + 30 > -50 && contentStartY + 30 < WINDOW_HEIGHT + 50) {
            outtextxy(titleX, contentStartY + 30, title.c_str());
        }

        // 绘制帮助内容
        int startY = contentStartY + 100;
        int lineHeight = 28;  // 增加行高
        int sectionSpacing = 35;  // 增加段落间距
        int currentY = startY;

        // 游戏玩法说明
        if (currentY > -50 && currentY < WINDOW_HEIGHT + 50) {
            settextcolor(Theme::PRIMARY_DARK);
            settextstyle(24, 0, L"Arial");
            outtextxy(50, currentY, L"How to Play:");
        }
        currentY += 45;  // 增加标题后的间距

        settextcolor(Theme::TEXT_PRIMARY);
        settextstyle(16, 0, L"Arial");
        vector<wstring> gameplayInstructions = {
            L"• Use A/D or Arrow Keys to move left and right",
            L"• Press SPACE to jump (supports double jump with power-up)",
            L"• Collect items on platforms for special abilities",
            L"• Avoid obstacles that fall from above",
            L"• Build combo by jumping to higher platforms",
            L"• Survive as long as possible and reach maximum height!"
        };

        for (const auto& instruction : gameplayInstructions) {
            if (currentY > -50 && currentY < WINDOW_HEIGHT + 50) {
                outtextxy(70, currentY, instruction.c_str());
            }
            currentY += lineHeight;
        }

        currentY += sectionSpacing;

        // 道具说明 - 改善排版
        if (currentY > -50 && currentY < WINDOW_HEIGHT + 50) {
            settextcolor(Theme::PRIMARY_DARK);
            settextstyle(24, 0, L"Arial");
            outtextxy(50, currentY, L"Power-ups:");
        }
        currentY += 45;

        settextcolor(Theme::TEXT_PRIMARY);
        settextstyle(16, 0, L"Arial");

        // 道具信息结构
        struct ItemInfo {
            wstring name;
            wstring description;
            COLORREF color;
        };

        vector<ItemInfo> items = {
            {L"Speed Boost", L"Increases movement speed for 5 seconds", Theme::ITEM_SPEED},
            {L"Shield", L"Protects from damage and enables revival", Theme::ITEM_SHIELD},
            {L"Double Jump", L"Enables triple jump for 10 seconds", Theme::ITEM_DOUBLE_JUMP},
            {L"Slow Time", L"Slows down time for easier navigation", Theme::ITEM_SLOW_TIME},
            {L"Magnetic Field", L"Attracts nearby coins automatically", Theme::ITEM_MAGNETIC_FIELD},
            {L"Freeze Obstacles", L"Freezes all obstacles for 10 seconds", Theme::ITEM_FREEZE_OBSTACLES},
            {L"Health Boost", L"Restores 2 health points instantly", Theme::ITEM_HEALTH_BOOST},
            {L"Invincibility", L"Temporary immunity to all damage", Theme::ITEM_INVINCIBILITY},
            {L"Coin", L"Increases score and currency", Theme::ITEM_COIN}
        };

        int col1X = 70;
        int col2X = 620;  // 调整第二列位置
        int itemsPerColumn = 5;
        int itemLineHeight = 38;  // 增加道具间行高

        for (size_t i = 0; i < items.size(); i++) {
            int drawX = (i < itemsPerColumn) ? col1X : col2X;
            int drawY = currentY + (i % itemsPerColumn) * itemLineHeight;

            if (drawY > -50 && drawY < WINDOW_HEIGHT + 50) {
                // 绘制道具颜色指示器
                setfillcolor(items[i].color);
                solidcircle(drawX, drawY + 10, 8);  // 增大指示器

                // 绘制道具名称
                settextcolor(items[i].color);
                settextstyle(16, 0, L"Arial");
                outtextxy(drawX + 25, drawY, items[i].name.c_str());

                // 绘制道具描述
                settextcolor(Theme::TEXT_SECONDARY);
                settextstyle(14, 0, L"Arial");
                outtextxy(drawX + 25, drawY + 20, items[i].description.c_str());
            }
        }

        currentY += itemsPerColumn * itemLineHeight + sectionSpacing;

        // 障碍物说明
        if (currentY > -50 && currentY < WINDOW_HEIGHT + 50) {
            settextcolor(Theme::PRIMARY_DARK);
            settextstyle(24, 0, L"Arial");
            outtextxy(50, currentY, L"Obstacles:");
        }
        currentY += 45;

        settextcolor(Theme::TEXT_PRIMARY);
        settextstyle(16, 0, L"Arial");

        vector<wstring> obstacles = {
            L"• Spikes: Static ground hazards - 1 damage",
            L"• Fireballs: Fall from above - 2 damage",
            L"• Lasers: Move horizontally - 3 damage",
            L"• Rotating Saws: Spin in place - 2 damage",
            L"• Falling Rocks: Accelerate downward - 1 damage",
            L"• Moving Walls: Slide across screen - 1 damage"
        };

        for (const auto& obstacle : obstacles) {
            if (currentY > -50 && currentY < WINDOW_HEIGHT + 50) {
                outtextxy(70, currentY, obstacle.c_str());
            }
            currentY += lineHeight;
        }

        currentY += sectionSpacing;

        // 平台类型说明
        if (currentY > -50 && currentY < WINDOW_HEIGHT + 50) {
            settextcolor(Theme::PRIMARY_DARK);
            settextstyle(24, 0, L"Arial");
            outtextxy(50, currentY, L"Platform Types:");
        }
        currentY += 45;

        settextcolor(Theme::TEXT_PRIMARY);
        settextstyle(16, 0, L"Arial");

        vector<wstring> platformTypes = {
            L"• Normal Platforms: Standard jumping platforms",
            L"• Moving Platforms: Slide back and forth",
            L"• Breakable Platforms: Break after being stepped on",
            L"• Spring Platforms: Launch you higher when stepped on"
        };

        for (const auto& platformType : platformTypes) {
            if (currentY > -50 && currentY < WINDOW_HEIGHT + 50) {
                outtextxy(70, currentY, platformType.c_str());
            }
            currentY += lineHeight;
        }

        currentY += sectionSpacing;

        // 修复：重新计算最大滚动偏移，确保正确性
        int totalContentHeight = currentY + 150;  // 内容总高度
        int availableHeight = WINDOW_HEIGHT - 160;  // 可用显示区域（减去固定按钮和提示的空间）
        maxHelpScrollOffset = std::max(0.0f, (float)(totalContentHeight - availableHeight));

        // 绘制返回按钮（固定位置，不受滚动影响）
        Button fixedBackButton = backButton;
        fixedBackButton.y = WINDOW_HEIGHT - 80;  // 固定在底部
        drawButton(fixedBackButton, Theme::PRIMARY, Theme::PRIMARY_LIGHT, RGB(255, 255, 255));

        // 绘制滚动指示器
        if (maxHelpScrollOffset > 0) {
            // 绘制滚动条
            float scrollBarHeight = 200.0f;
            float scrollBarY = 100.0f;
            float scrollBarX = WINDOW_WIDTH - 20.0f;

            // 滚动条背景
            setfillcolor(RGB(200, 200, 200));
            solidrectangle((int)scrollBarX, (int)scrollBarY, (int)scrollBarX + 10, (int)(scrollBarY + scrollBarHeight));

            // 修复：改进滚动条滑块计算
            float contentRatio = (float)availableHeight / totalContentHeight;  // 可见内容比例
            float thumbHeight = std::max(20.0f, scrollBarHeight * contentRatio);  // 滑块高度，最小20像素
            float scrollProgress = helpScrollOffset / maxHelpScrollOffset;  // 滚动进度
            float thumbY = scrollBarY + scrollProgress * (scrollBarHeight - thumbHeight);

            setfillcolor(Theme::PRIMARY);
            solidrectangle((int)scrollBarX, (int)thumbY, (int)scrollBarX + 10, (int)(thumbY + thumbHeight));

            // 绘制滚动提示
            settextcolor(Theme::TEXT_DISABLED);
            settextstyle(14, 0, L"Arial");
            outtextxy(WINDOW_WIDTH - 150, WINDOW_HEIGHT - 50, L"Use ↑↓ or PgUp/PgDn to scroll");
        }

        // 修改：更新底部提示文字
        settextcolor(Theme::TEXT_DISABLED);
        settextstyle(16, 0, L"Arial");
        wstring backHint = L"Press Backspace or click Back to return to menu";  // 修改提示文字
        int backHintWidth = textwidth(backHint.c_str());
        int backHintX = (WINDOW_WIDTH - backHintWidth) / 2;
        outtextxy(backHintX, WINDOW_HEIGHT - 30, backHint.c_str());
    }

    void drawButton(const Button& button, COLORREF normalColor, COLORREF hoverColor, COLORREF textColor) {
        // 只绘制在视口内的按钮
        if (button.y < -button.height || button.y > WINDOW_HEIGHT) {
            return;
        }

        // 选择颜色
        COLORREF buttonColor = button.isHovered ? hoverColor : normalColor;

        // 绘制按钮背景
        DrawUtils::drawSoftShadowRect(button.x, button.y, button.width, button.height, 10, buttonColor, 3);

        // 绘制按钮边框
        setlinecolor(button.isHovered ? Theme::PRIMARY_LIGHT : Theme::PRIMARY_DARK);
        setlinestyle(PS_SOLID, 2);
        rectangle(button.x, button.y, button.x + button.width, button.y + button.height);

        // 绘制按钮文字
        settextcolor(textColor);
        settextstyle(24, 0, L"Arial");
        int textWidth = textwidth(button.text.c_str());
        int textHeight = textheight(button.text.c_str());
        int textX = button.x + (button.width - textWidth) / 2;
        int textY = button.y + (button.height - textHeight) / 2;
        outtextxy(textX, textY, button.text.c_str());

        // 悬停时添加光晕效果
        if (button.isHovered) {
            DrawUtils::drawGlowRect(button.x - 2, button.y - 2, button.width + 4, button.height + 4,
                Theme::PRIMARY_LIGHT, 0.3f);
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

        // 绘制障碍物
        for (const auto& obstacle : obstacles) {
            float drawY = obstacle.getY() - camera_y;
            if (drawY > -100 && drawY < WINDOW_HEIGHT + 100) {
                obstacle.drawWithOffset(shakeX, -camera_y + shakeY);
            }
        }

        // 绘制金币
        for (const auto& coin : coins) {
            float drawY = coin.getY() - camera_y;
            if (drawY > -50 && drawY < WINDOW_HEIGHT + 50) {
                coin.drawWithOffset(shakeX, -camera_y + shakeY);
            }
        }

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

    // 修复 drawGameUI 函数
    void drawGameUI() {
        // 绘制描边文字的辅助函数
        auto drawTextWithOutline = [&](const wstring& text, int x, int y, COLORREF textColor) {
            // 黑色描边
            settextcolor(RGB(0, 0, 0));
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx != 0 || dy != 0) {
                        outtextxy(x + dx, y + dy, text.c_str());
                    }
                }
            }
            // 主文字
            settextcolor(textColor);
            outtextxy(x, y, text.c_str());
            };

        // 设置字体
        settextstyle(22, 0, L"Arial");

        // 左侧UI布局 - 所有信息都在左侧显示
        int startX = 30;
        int startY = 30;
        int lineHeight = 30;

        // 分数显示
        wstring scoreText = L"Score: " + to_wstring(score);
        drawTextWithOutline(scoreText, startX, startY, RGB(255, 255, 255));

        // 高度显示
        wstring heightText = L"Height: " + to_wstring(maxHeight);
        drawTextWithOutline(heightText, startX, startY + lineHeight, RGB(100, 200, 255));

        // 道具收集数
        wstring itemText = L"Items: " + to_wstring(player.getItemsCollected());
        drawTextWithOutline(itemText, startX, startY + lineHeight * 2, RGB(255, 200, 100));

        // 时间显示
        wstring timeText = L"Time: " + to_wstring((int)gameTime) + L"s";
        drawTextWithOutline(timeText, startX, startY + lineHeight * 3, RGB(200, 255, 200));

        // 连击显示
        if (player.getComboCount() > 1) {
            wstring comboText = L"Combo: " + to_wstring(player.getComboCount()) + L"x";
            COLORREF comboColor = DrawUtils::getComboColor(player.getComboCount());
            drawTextWithOutline(comboText, startX, startY + lineHeight * 4, comboColor);
        }

        // 生命值显示
        settextstyle(18, 0, L"Arial");
        int healthY = startY + lineHeight * 5;

        // 绘制生命值背景
        setfillcolor(RGB(50, 50, 50));
        solidrectangle(startX, healthY, startX + 200, healthY + 25);

        // 绘制生命值条
        float healthPercentage = (float)player.getHealth() / player.getMaxHealth();
        COLORREF healthColor;
        if (healthPercentage > 0.6f) {
            healthColor = RGB(0, 255, 0);
        }
        else if (healthPercentage > 0.3f) {
            healthColor = RGB(255, 255, 0);
        }
        else {
            healthColor = RGB(255, 0, 0);
        }

        setfillcolor(healthColor);
        solidrectangle(startX + 5, healthY + 5,
            (int)(startX + 5 + 190 * healthPercentage),
            healthY + 20);

        // 生命值文字
        wstring healthText = L"Health: " + to_wstring(player.getHealth()) + L"/" + to_wstring(player.getMaxHealth());
        drawTextWithOutline(healthText, startX + 10, healthY + 6, RGB(255, 255, 255));

        // 金币显示
        wstring coinText = L"Coins: " + to_wstring(player.getCoins());
        drawTextWithOutline(coinText, startX, healthY + 35, RGB(255, 215, 0));

        // 道具状态显示 - 也在左侧
        settextstyle(16, 0, L"Arial");
        int effectY = healthY + 65;

        if (player.hasSpeedBoost()) {
            drawTextWithOutline(L"Speed Boost Active", startX, effectY, Theme::ITEM_SPEED);
            effectY += 20;
        }

        if (player.hasShield()) {
            drawTextWithOutline(L"Shield Active", startX, effectY, Theme::ITEM_SHIELD);
            effectY += 20;
        }

        if (player.hasInvincibilityActive()) {
            drawTextWithOutline(L"Invincibility Active", startX, effectY, RGB(255, 215, 0));
            effectY += 20;
        }

        if (player.hasDoubleJumpActive()) {
            drawTextWithOutline(L"Double Jump Active", startX, effectY, RGB(100, 255, 100));
            effectY += 20;
        }

        if (player.hasSlowTimeActive()) {
            drawTextWithOutline(L"Slow Time Active", startX, effectY, RGB(100, 100, 255));
            effectY += 20;
        }

        if (player.hasMagneticFieldActive()) {
            drawTextWithOutline(L"Magnetic Field Active", startX, effectY, RGB(255, 100, 255));
            effectY += 20;
        }

        if (player.hasObstaclesFrozen()) {
            drawTextWithOutline(L"Obstacles Frozen", startX, effectY, RGB(100, 255, 255));
            effectY += 20;
        }

        // 控制提示 - 移到右下角
        settextstyle(14, 0, L"Arial");
        int controlX = WINDOW_WIDTH - 220;
        int controlY = WINDOW_HEIGHT - 100;

        vector<wstring> controls = {
            L"A/D: Move",
            L"SPACE: Jump",
            L"P: Pause",
            L"ESC: Exit"
        };

        for (size_t i = 0; i < controls.size(); i++) {
            drawTextWithOutline(controls[i], controlX, controlY + (int)i * 18, Theme::TEXT_DISABLED);
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
        wstring resumeText = L"P to resume";
        int resumeWidth = textwidth(resumeText.c_str());
        int resumeX = (WINDOW_WIDTH - resumeWidth) / 2;
        outtextxy(resumeX, WINDOW_HEIGHT / 2 + 5, resumeText.c_str());

        wstring menuText = L"ESC to return to menu";
        int menuWidth = textwidth(menuText.c_str());
        int menuX = (WINDOW_WIDTH - menuWidth) / 2;
        outtextxy(menuX, WINDOW_HEIGHT / 2 + 30, menuText.c_str());
    }

    void drawGameOver() {
        // 半透明背景
        setfillcolor(DrawUtils::blendColor(RGB(0, 0, 0), RGB(255, 255, 255), 0.8f));
        solidrectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        // 扩大结算面板
        int panelWidth = 450;
        int panelHeight = 400;
        int panelX = WINDOW_WIDTH / 2 - panelWidth / 2;
        int panelY = WINDOW_HEIGHT / 2 - panelHeight / 2;

        // 绘制结算面板背景
        DrawUtils::drawSoftShadowRect(panelX, panelY, panelWidth, panelHeight, 20, Theme::PRIMARY);

        // 标题
        settextcolor(WHITE);
        settextstyle(45, 0, L"Arial");
        wstring gameOverText = L"Game Over";
        int gameOverWidth = textwidth(gameOverText.c_str());
        int gameOverX = (WINDOW_WIDTH - gameOverWidth) / 2;
        outtextxy(gameOverX, panelY + 30, gameOverText.c_str());

        // 统计信息
        settextstyle(22, 0, L"Arial");
        settextcolor(Theme::PRIMARY_LIGHT);

        int statY = panelY + 100;
        int lineHeight = 35;

        // 最终得分
        wstring finalScoreText = L"Final Score: " + to_wstring(score);
        int scoreWidth = textwidth(finalScoreText.c_str());
        outtextxy((WINDOW_WIDTH - scoreWidth) / 2, statY, finalScoreText.c_str());

        // 最大高度
        wstring maxHeightText = L"Max Height: " + to_wstring(maxHeight) + L" pixels";
        int heightWidth = textwidth(maxHeightText.c_str());
        outtextxy((WINDOW_WIDTH - heightWidth) / 2, statY + lineHeight, maxHeightText.c_str());

        // 道具收集统计
        wstring itemsText = L"Items Collected: " + to_wstring(player.getItemsCollected());
        int itemsWidth = textwidth(itemsText.c_str());
        outtextxy((WINDOW_WIDTH - itemsWidth) / 2, statY + lineHeight * 2, itemsText.c_str());

        // 存活时间
        int minutes = (int)gameTime / 60;
        int seconds = (int)gameTime % 60;
        wstring survivalTimeText = L"Survival Time: " + to_wstring(minutes) + L":" +
            (seconds < 10 ? L"0" : L"") + to_wstring(seconds);
        int timeWidth = textwidth(survivalTimeText.c_str());
        outtextxy((WINDOW_WIDTH - timeWidth) / 2, statY + lineHeight * 3, survivalTimeText.c_str());

        // 最高连击
        wstring maxComboText = L"Max Combo: " + to_wstring(player.getComboCount()) + L"x";
        int comboWidth = textwidth(maxComboText.c_str());
        outtextxy((WINDOW_WIDTH - comboWidth) / 2, statY + lineHeight * 4, maxComboText.c_str());

        // 金币收集
        wstring coinsText = L"Coins Collected: " + to_wstring(player.getCoins());
        int coinsWidth = textwidth(coinsText.c_str());
        outtextxy((WINDOW_WIDTH - coinsWidth) / 2, statY + lineHeight * 5, coinsText.c_str());

        // 评级系统
        settextstyle(28, 0, L"Arial");
        wstring rank = L"Rank: ";
        COLORREF rankColor = RGB(255, 255, 255);

        if (maxHeight >= 5000) {
            rank += L"S+ Master";
            rankColor = RGB(255, 215, 0);  // 金色
        }
        else if (maxHeight >= 3000) {
            rank += L"A+ Expert";
            rankColor = RGB(255, 100, 100);  // 红色
        }
        else if (maxHeight >= 1500) {
            rank += L"B+ Advanced";
            rankColor = RGB(100, 255, 100);  // 绿色
        }
        else if (maxHeight >= 800) {
            rank += L"C+ Skilled";
            rankColor = RGB(100, 100, 255);  // 蓝色
        }
        else {
            rank += L"D Beginner";
            rankColor = RGB(200, 200, 200);  // 灰色
        }

        settextcolor(rankColor);
        int rankWidth = textwidth(rank.c_str());
        outtextxy((WINDOW_WIDTH - rankWidth) / 2, statY + lineHeight * 6, rank.c_str());

        // 操作提示
        settextstyle(24, 0, L"Arial");
        settextcolor(Theme::WARNING);
        wstring restartText = L"SPACE - Return to Menu";
        int restartWidth = textwidth(restartText.c_str());
        outtextxy((WINDOW_WIDTH - restartWidth) / 2, panelY + panelHeight - 60, restartText.c_str());

        wstring exitText = L"ESC - Exit Game";
        int exitWidth = textwidth(exitText.c_str());
        outtextxy((WINDOW_WIDTH - exitWidth) / 2, panelY + panelHeight - 30, exitText.c_str());
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

        // 在游戏循环中调用音频控制
        game.handleAudioControls();

        Sleep(16);
    }

    closegraph();
    return 0;
}