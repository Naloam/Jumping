#pragma once
#include <windows.h>
#include <string>
#include <map>
#include <vector>

enum class SoundType {
    BACKGROUND_MUSIC,
    MENU_MUSIC,
    JUMP,
    LAND,
    COIN_COLLECT,
    ITEM_COLLECT,
    SPRING_BOUNCE,
    PLATFORM_BREAK,
    OBSTACLE_HIT,
    GAME_OVER,
    BUTTON_HOVER,
    BUTTON_CLICK,
    SHIELD_ACTIVATE,
    INVINCIBILITY,
    COMBO_SOUND,
    DAMAGE_SOUND
};

class AudioManager {
private:
    static AudioManager* instance;
    bool audioEnabled;
    float masterVolume;
    float musicVolume;
    float sfxVolume;

    std::map<SoundType, std::string> soundPaths;
    std::map<SoundType, std::string> soundAliases;
    std::vector<std::string> activeSounds;

    SoundType currentBackgroundMusic;
    bool backgroundMusicPlaying;

	AudioManager();

    // 内部方法
    void initializeSoundPaths();
    std::string getSoundAlias(SoundType type);
    void cleanupFinishedSounds();
    std::string getAudioType(const std::string& filepath);
    bool fileExists(const std::string& filepath);
    std::string findAudioFile(const std::string& basePath);

public:
    static AudioManager& getInstance();

    // 初始化和清理
    void initialize();
    void cleanup();

    // 音量控制
    void setMasterVolume(float volume);
    void setMusicVolume(float volume);
    void setSFXVolume(float volume);
    float getMasterVolume() const { return masterVolume; }
    float getMusicVolume() const { return musicVolume; }
    float getSFXVolume() const { return sfxVolume; }

    // 音频播放
    void playSound(SoundType type, bool loop = false);
    void playBackgroundMusic(SoundType type, bool loop = true);
    void stopSound(SoundType type);
    void stopBackgroundMusic();
    void stopAllSounds();

    // 音频状态
    bool isPlaying(SoundType type);
    void pauseBackgroundMusic();
    void resumeBackgroundMusic();

    // 启用/禁用音频
    void setAudioEnabled(bool enabled);
    bool isAudioEnabled() const { return audioEnabled; }

    // 游戏状态音频管理
    void onGameStart();
    void onGameOver();
    void onMenuEnter();
    void onGamePause();
    void onGameResume();

    ~AudioManager();
};