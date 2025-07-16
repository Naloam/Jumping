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

    // �ڲ�����
    void initializeSoundPaths();
    std::string getSoundAlias(SoundType type);
    void cleanupFinishedSounds();
    std::string getAudioType(const std::string& filepath);
    bool fileExists(const std::string& filepath);
    std::string findAudioFile(const std::string& basePath);

public:
    static AudioManager& getInstance();

    // ��ʼ��������
    void initialize();
    void cleanup();

    // ��������
    void setMasterVolume(float volume);
    void setMusicVolume(float volume);
    void setSFXVolume(float volume);
    float getMasterVolume() const { return masterVolume; }
    float getMusicVolume() const { return musicVolume; }
    float getSFXVolume() const { return sfxVolume; }

    // ��Ƶ����
    void playSound(SoundType type, bool loop = false);
    void playBackgroundMusic(SoundType type, bool loop = true);
    void stopSound(SoundType type);
    void stopBackgroundMusic();
    void stopAllSounds();

    // ��Ƶ״̬
    bool isPlaying(SoundType type);
    void pauseBackgroundMusic();
    void resumeBackgroundMusic();

    // ����/������Ƶ
    void setAudioEnabled(bool enabled);
    bool isAudioEnabled() const { return audioEnabled; }

    // ��Ϸ״̬��Ƶ����
    void onGameStart();
    void onGameOver();
    void onMenuEnter();
    void onGamePause();
    void onGameResume();

    ~AudioManager();
};