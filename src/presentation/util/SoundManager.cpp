#include "auction/presentation/util/SoundManager.h"

#include <QSettings>
#include <cmath>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

namespace auction::presentation {

static constexpr double kPi = 3.14159265358979323846;

static std::vector<uint8_t> makeWavHeader(const std::vector<int16_t> &pcm, int sampleRate = 44100) {
    uint32_t dataSize = static_cast<uint32_t>(pcm.size() * sizeof(int16_t));
    uint32_t chunkSize = 36 + dataSize;
    std::vector<uint8_t> wav(44 + dataSize);
    std::memcpy(&wav[0], "RIFF", 4);
    std::memcpy(&wav[4], &chunkSize, 4);
    std::memcpy(&wav[8], "WAVEfmt ", 8);
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels = 1; // mono
    uint32_t byteRate = sampleRate * numChannels * 2;
    uint16_t blockAlign = numChannels * 2;
    uint16_t bitsPerSample = 16;
    std::memcpy(&wav[16], &subchunk1Size, 4);
    std::memcpy(&wav[20], &audioFormat, 2);
    std::memcpy(&wav[22], &numChannels, 2);
    std::memcpy(&wav[24], &sampleRate, 4);
    std::memcpy(&wav[28], &byteRate, 4);
    std::memcpy(&wav[32], &blockAlign, 2);
    std::memcpy(&wav[34], &bitsPerSample, 2);
    std::memcpy(&wav[36], "data", 4);
    std::memcpy(&wav[40], &dataSize, 4);
    std::memcpy(&wav[44], pcm.data(), dataSize);
    return wav;
}

SoundManager &SoundManager::instance() {
    static SoundManager s_instance;
    return s_instance;
}

SoundManager::SoundManager() {
    QSettings settings;
    muted_ = settings.value("sound/muted", false).toBool();

    gavelWav_ = generateGavelWav();
    tickWav_ = generateTickWav();
    victoryWav_ = generateVictoryWav();
    bidWav_ = generateBidWav();
}

void SoundManager::setMuted(bool mute) {
    if (muted_ != mute) {
        muted_ = mute;
        QSettings settings;
        settings.setValue("sound/muted", muted_);
        emit muteChanged(muted_);
    }
}

void SoundManager::playGavel() {
    playWav(gavelWav_);
}

void SoundManager::playCountdownTick() {
    playWav(tickWav_);
}

void SoundManager::playVictory() {
    playWav(victoryWav_);
}

void SoundManager::playBidSuccess() {
    playWav(bidWav_);
}

void SoundManager::playWav(const std::vector<uint8_t> &wavData) {
    if (muted_ || wavData.empty())
        return;
#ifdef _WIN32
    PlaySoundW(reinterpret_cast<LPCWSTR>(wavData.data()), NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#endif
}

// Tổng hợp âm thanh tiếng búa gỗ gõ đe (Double wooden strike: Cạch... Cạch!)
std::vector<uint8_t> SoundManager::generateGavelWav() {
    constexpr int sampleRate = 44100;
    constexpr double duration = 0.5;
    const int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<int16_t> pcm(totalSamples, 0);

    const double strikeTimes[2] = {0.0, 0.16};
    for (double strikeTime : strikeTimes) {
        const int startSample = static_cast<int>(strikeTime * sampleRate);
        const int strikeLen = static_cast<int>(sampleRate * 0.28);
        for (int i = 0; i < strikeLen && (startSample + i) < totalSamples; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            // Thành phần đập (Thud low-freq + Thùng gỗ resonance + Click đanh)
            double low = std::sin(2.0 * kPi * 180.0 * t) * std::exp(-t * 28.0);
            double mid = std::sin(2.0 * kPi * 420.0 * t) * std::exp(-t * 36.0);
            double high = std::sin(2.0 * kPi * 980.0 * t) * std::exp(-t * 60.0);
            double noise = ((std::rand() % 2000 - 1000) / 1000.0) * std::exp(-t * 120.0);

            double sample = (low * 0.45 + mid * 0.35 + high * 0.15 + noise * 0.05);
            int32_t val = static_cast<int32_t>(sample * 30000.0);
            val = std::clamp(val, -32767, 32767);

            pcm[startSample + i] = static_cast<int16_t>(std::clamp(pcm[startSample + i] + val, -32767, 32767));
        }
    }

    return makeWavHeader(pcm, sampleRate);
}

// Tổng hợp âm thanh nhịp đếm ngược gỗ (Woodblock click)
std::vector<uint8_t> SoundManager::generateTickWav() {
    constexpr int sampleRate = 44100;
    constexpr double duration = 0.05;
    const int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<int16_t> pcm(totalSamples, 0);

    for (int i = 0; i < totalSamples; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double sample = std::sin(2.0 * kPi * 1050.0 * t) * std::exp(-t * 85.0);
        int32_t val = static_cast<int32_t>(sample * 24000.0);
        pcm[i] = static_cast<int16_t>(std::clamp(val, -32767, 32767));
    }

    return makeWavHeader(pcm, sampleRate);
}

// Tổng hợp chuông chúc mừng thắng phiên
std::vector<uint8_t> SoundManager::generateVictoryWav() {
    constexpr int sampleRate = 44100;
    constexpr double duration = 0.75;
    const int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<int16_t> pcm(totalSamples, 0);

    const double notes[4] = {523.25, 659.25, 783.99, 1046.50}; // C5, E5, G5, C6
    const double times[4] = {0.0, 0.12, 0.24, 0.36};

    for (int n = 0; n < 4; ++n) {
        int start = static_cast<int>(times[n] * sampleRate);
        int len = totalSamples - start;
        double freq = notes[n];
        for (int i = 0; i < len; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            double sample = std::sin(2.0 * kPi * freq * t) * std::exp(-t * 7.5);
            int32_t val = static_cast<int32_t>(sample * 14000.0);
            pcm[start + i] = static_cast<int16_t>(std::clamp(pcm[start + i] + val, -32767, 32767));
        }
    }

    return makeWavHeader(pcm, sampleRate);
}

// Tổng hợp tiếng chuông đặt giá thành công
std::vector<uint8_t> SoundManager::generateBidWav() {
    constexpr int sampleRate = 44100;
    constexpr double duration = 0.18;
    const int totalSamples = static_cast<int>(sampleRate * duration);
    std::vector<int16_t> pcm(totalSamples, 0);

    for (int i = 0; i < totalSamples; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double freq = 1200.0 + (t / duration) * 600.0;
        double sample = std::sin(2.0 * kPi * freq * t) * std::exp(-t * 22.0);
        int32_t val = static_cast<int32_t>(sample * 22000.0);
        pcm[i] = static_cast<int16_t>(std::clamp(val, -32767, 32767));
    }

    return makeWavHeader(pcm, sampleRate);
}

} // namespace auction::presentation
