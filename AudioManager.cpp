#include "AudioManager.h"
#include <mmsystem.h>
#include <sstream>
#include <iostream>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

AudioManager* AudioManager::instance = nullptr;

AudioManager& AudioManager::getInstance() {
    if (!instance) {
        instance = new AudioManager();
    }
    return *instance;
}

AudioManager::AudioManager()
    : audioEnabled(true), masterVolume(1.0f), musicVolume(0.7f), sfxVolume(0.8f),
    currentBackgroundMusic(SoundType::MENU_MUSIC), backgroundMusicPlaying(false) {
    initializeSoundPaths();
}

AudioManager::~AudioManager() {
    cleanup();
}

std::string AudioManager::getAudioType(const std::string& filepath) {
    if (filepath.find(".mp3") != std::string::npos) {
        return "mpegvideo";
    }
    else if (filepath.find(".wav") != std::string::npos) {
        return "waveaudio";
    }
    else {
        return "waveaudio";
    }
}

bool AudioManager::fileExists(const std::string& filepath) {
    DWORD attributes = GetFileAttributesA(filepath.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES &&
        !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

std::string AudioManager::findAudioFile(const std::string& basePath) {
    // 优先查找MP3，然后WAV
    std::string mp3Path = basePath + ".mp3";
    std::string wavPath = basePath + ".wav";

    if (fileExists(mp3Path)) {
        return mp3Path;
    }
    else if (fileExists(wavPath)) {
        return wavPath;
    }
    else {
        return ""; // 文件不存在
    }
}

void AudioManager::initializeSoundPaths() {
    // 自动检测可用的音频格式
    soundPaths[SoundType::BACKGROUND_MUSIC] = findAudioFile("sounds/background_music");
    soundPaths[SoundType::MENU_MUSIC] = findAudioFile("sounds/menu_music");
    soundPaths[SoundType::JUMP] = findAudioFile("sounds/jump");
    soundPaths[SoundType::LAND] = findAudioFile("sounds/land");
    soundPaths[SoundType::COIN_COLLECT] = findAudioFile("sounds/coin");
    soundPaths[SoundType::ITEM_COLLECT] = findAudioFile("sounds/item");
    soundPaths[SoundType::SPRING_BOUNCE] = findAudioFile("sounds/spring");
    soundPaths[SoundType::PLATFORM_BREAK] = findAudioFile("sounds/break");
    soundPaths[SoundType::OBSTACLE_HIT] = findAudioFile("sounds/hit");
    soundPaths[SoundType::GAME_OVER] = findAudioFile("sounds/game_over");
    soundPaths[SoundType::BUTTON_HOVER] = findAudioFile("sounds/button_hover");
    soundPaths[SoundType::BUTTON_CLICK] = findAudioFile("sounds/button_click");
    soundPaths[SoundType::SHIELD_ACTIVATE] = findAudioFile("sounds/shield");
    soundPaths[SoundType::INVINCIBILITY] = findAudioFile("sounds/invincibility");
    soundPaths[SoundType::COMBO_SOUND] = findAudioFile("sounds/combo");
    soundPaths[SoundType::DAMAGE_SOUND] = findAudioFile("sounds/damage");

    // 移除空路径
    for (auto it = soundPaths.begin(); it != soundPaths.end();) {
        if (it->second.empty()) {
            it = soundPaths.erase(it);
        }
        else {
            ++it;
        }
    }
}

std::string AudioManager::getSoundAlias(SoundType type) {
    std::stringstream ss;
    ss << "sound_" << static_cast<int>(type);
    return ss.str();
}

void AudioManager::initialize() {
    if (!audioEnabled) return;
    cleanup();
    // 初始化完成
}

void AudioManager::cleanup() {
    if (!audioEnabled) return;

    stopAllSounds();

    // 清理所有音频别名
    for (auto& pair : soundAliases) {
        std::string command = "close " + pair.second;
        mciSendStringA(command.c_str(), nullptr, 0, nullptr);
    }

    soundAliases.clear();
    activeSounds.clear();
}

void AudioManager::setMasterVolume(float volume) {
    masterVolume = std::max(0.0f, std::min(1.0f, volume));
}

void AudioManager::setMusicVolume(float volume) {
    musicVolume = std::max(0.0f, std::min(1.0f, volume));
}

void AudioManager::setSFXVolume(float volume) {
    sfxVolume = std::max(0.0f, std::min(1.0f, volume));
}

void AudioManager::playSound(SoundType type, bool loop) {
    if (!audioEnabled) return;

    auto it = soundPaths.find(type);
    if (it == soundPaths.end()) {
        // 使用系统音效作为替代
        switch (type) {
        case SoundType::JUMP:
            PlaySound(TEXT("SystemHand"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::LAND:
            PlaySound(TEXT("SystemDefault"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::COIN_COLLECT:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::ITEM_COLLECT:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::OBSTACLE_HIT:
            PlaySound(TEXT("SystemExclamation"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::DAMAGE_SOUND:
            PlaySound(TEXT("SystemExclamation"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::BUTTON_CLICK:
            PlaySound(TEXT("SystemDefault"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::GAME_OVER:
            PlaySound(TEXT("SystemCriticalStop"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::COMBO_SOUND:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::INVINCIBILITY:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::SHIELD_ACTIVATE:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::BUTTON_HOVER:
            PlaySound(TEXT("SystemDefault"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        default:
            break;
        }
        return;
    }

    std::string alias = getSoundAlias(type);
    std::string filepath = it->second;

    // 修复：检查文件是否存在
    if (!fileExists(filepath)) {
        // 文件不存在，使用系统音效
        switch (type) {
        case SoundType::JUMP:
            PlaySound(TEXT("SystemHand"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::LAND:
            PlaySound(TEXT("SystemDefault"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::COIN_COLLECT:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::ITEM_COLLECT:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::OBSTACLE_HIT:
            PlaySound(TEXT("SystemExclamation"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::DAMAGE_SOUND:
            PlaySound(TEXT("SystemExclamation"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::BUTTON_CLICK:
            PlaySound(TEXT("SystemDefault"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::GAME_OVER:
            PlaySound(TEXT("SystemCriticalStop"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::COMBO_SOUND:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::INVINCIBILITY:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::SHIELD_ACTIVATE:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::BUTTON_HOVER:
            PlaySound(TEXT("SystemDefault"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        default:
            break;
        }
        return;
    }

    // 停止之前的同类型音效
    if (soundAliases.find(type) != soundAliases.end()) {
        stopSound(type);
    }

    // 修复：支持多种音频格式
    std::string audioType = getAudioType(filepath);
    std::string openCommand = "open \"" + filepath + "\" type " + audioType + " alias " + alias;
    MCIERROR result = mciSendStringA(openCommand.c_str(), nullptr, 0, nullptr);

    if (result == 0) {
        soundAliases[type] = alias;

        // 设置音量
        int volume = static_cast<int>(masterVolume * sfxVolume * 1000);
        std::string volumeCommand = "setaudio " + alias + " volume to " + std::to_string(volume);
        mciSendStringA(volumeCommand.c_str(), nullptr, 0, nullptr);

        // 播放音频
        std::string playCommand = "play " + alias;
        if (loop) {
            playCommand += " repeat";
        }

        result = mciSendStringA(playCommand.c_str(), nullptr, 0, nullptr);
        if (result == 0) {
            activeSounds.push_back(alias);
        }
    }
    else {
        // 如果MCI播放失败，回退到系统音效
        switch (type) {
        case SoundType::JUMP:
            PlaySound(TEXT("SystemHand"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::LAND:
            PlaySound(TEXT("SystemDefault"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::COIN_COLLECT:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::ITEM_COLLECT:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::OBSTACLE_HIT:
            PlaySound(TEXT("SystemExclamation"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::DAMAGE_SOUND:
            PlaySound(TEXT("SystemExclamation"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::BUTTON_CLICK:
            PlaySound(TEXT("SystemDefault"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::GAME_OVER:
            PlaySound(TEXT("SystemCriticalStop"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::COMBO_SOUND:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::INVINCIBILITY:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::SHIELD_ACTIVATE:
            PlaySound(TEXT("SystemNotification"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        case SoundType::BUTTON_HOVER:
            PlaySound(TEXT("SystemDefault"), NULL, SND_ALIAS | SND_ASYNC);
            break;
        default:
            break;
        }
    }
}

void AudioManager::playBackgroundMusic(SoundType type, bool loop) {
    if (!audioEnabled) return;

    // 停止当前背景音乐
    stopBackgroundMusic();

    currentBackgroundMusic = type;
    playSound(type, loop);
    backgroundMusicPlaying = true;
}

void AudioManager::stopSound(SoundType type) {
    if (!audioEnabled) return;

    auto it = soundAliases.find(type);
    if (it != soundAliases.end()) {
        std::string alias = it->second;

        std::string stopCommand = "stop " + alias;
        mciSendStringA(stopCommand.c_str(), nullptr, 0, nullptr);

        std::string closeCommand = "close " + alias;
        mciSendStringA(closeCommand.c_str(), nullptr, 0, nullptr);

        auto activeIt = std::find(activeSounds.begin(), activeSounds.end(), alias);
        if (activeIt != activeSounds.end()) {
            activeSounds.erase(activeIt);
        }

        soundAliases.erase(it);

        if (type == currentBackgroundMusic) {
            backgroundMusicPlaying = false;
        }
    }
}

void AudioManager::stopBackgroundMusic() {
    if (!audioEnabled) return;

    if (backgroundMusicPlaying) {
        stopSound(currentBackgroundMusic);
        backgroundMusicPlaying = false;
    }
}

void AudioManager::stopAllSounds() {
    if (!audioEnabled) return;

    for (auto& pair : soundAliases) {
        std::string alias = pair.second;
        std::string stopCommand = "stop " + alias;
        mciSendStringA(stopCommand.c_str(), nullptr, 0, nullptr);

        std::string closeCommand = "close " + alias;
        mciSendStringA(closeCommand.c_str(), nullptr, 0, nullptr);
    }

    soundAliases.clear();
    activeSounds.clear();
    backgroundMusicPlaying = false;
}

bool AudioManager::isPlaying(SoundType type) {
    if (!audioEnabled) return false;

    auto it = soundAliases.find(type);
    if (it != soundAliases.end()) {
        std::string alias = it->second;
        char buffer[256];
        std::string statusCommand = "status " + alias + " mode";
        MCIERROR result = mciSendStringA(statusCommand.c_str(), buffer, sizeof(buffer), nullptr);

        if (result == 0) {
            std::string status(buffer);
            return status.find("playing") != std::string::npos;
        }
    }
    return false;
}

void AudioManager::pauseBackgroundMusic() {
    if (!audioEnabled || !backgroundMusicPlaying) return;

    auto it = soundAliases.find(currentBackgroundMusic);
    if (it != soundAliases.end()) {
        std::string pauseCommand = "pause " + it->second;
        mciSendStringA(pauseCommand.c_str(), nullptr, 0, nullptr);
    }
}

void AudioManager::resumeBackgroundMusic() {
    if (!audioEnabled || !backgroundMusicPlaying) return;

    auto it = soundAliases.find(currentBackgroundMusic);
    if (it != soundAliases.end()) {
        std::string resumeCommand = "resume " + it->second;
        mciSendStringA(resumeCommand.c_str(), nullptr, 0, nullptr);
    }
}

void AudioManager::setAudioEnabled(bool enabled) {
    if (audioEnabled && !enabled) {
        stopAllSounds();
    }
    audioEnabled = enabled;

    if (enabled) {
        initialize();
    }
}

void AudioManager::cleanupFinishedSounds() {
    for (auto it = activeSounds.begin(); it != activeSounds.end();) {
        char buffer[256];
        std::string statusCommand = "status " + *it + " mode";
        MCIERROR result = mciSendStringA(statusCommand.c_str(), buffer, sizeof(buffer), nullptr);

        if (result != 0 || std::string(buffer).find("stopped") != std::string::npos) {
            std::string closeCommand = "close " + *it;
            mciSendStringA(closeCommand.c_str(), nullptr, 0, nullptr);
            it = activeSounds.erase(it);
        }
        else {
            ++it;
        }
    }
}

void AudioManager::onGameStart() {
    if (!audioEnabled) return;
    playBackgroundMusic(SoundType::BACKGROUND_MUSIC, true);
}

void AudioManager::onGameOver() {
    if (!audioEnabled) return;
    playSound(SoundType::GAME_OVER, false);
    Sleep(1000);
    stopBackgroundMusic();
}

void AudioManager::onMenuEnter() {
    if (!audioEnabled) return;
    playBackgroundMusic(SoundType::MENU_MUSIC, true);
}

void AudioManager::onGamePause() {
    if (!audioEnabled) return;
    pauseBackgroundMusic();
}

void AudioManager::onGameResume() {
    if (!audioEnabled) return;
    resumeBackgroundMusic();
}