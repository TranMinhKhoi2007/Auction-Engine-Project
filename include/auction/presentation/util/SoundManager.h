#pragma once

#include <QObject>
#include <cstdint>
#include <vector>

namespace auction::presentation {

// Quản lý hiệu ứng âm thanh sàn đấu giá (tiếng búa gõ, đếm ngược, tiếng chuông)
class SoundManager : public QObject {
    Q_OBJECT
public:
    static SoundManager &instance();

    bool isMuted() const { return muted_; }
    void setMuted(bool mute);

    // Tiếng búa gỗ gõ đe khi chốt phiên (Gavel strike)
    void playGavel();

    // Tiếng tick nhịp đồng hồ đếm ngược 10 giây cuối
    void playCountdownTick();

    // Tiếng chuông chúc mừng khi thắng phiên đấu giá
    void playVictory();

    // Tiếng chuông tinh gọn khi đặt giá thành công
    void playBidSuccess();

signals:
    void muteChanged(bool muted);

private:
    SoundManager();
    ~SoundManager() override = default;

    void playWav(const std::vector<uint8_t> &wavData);
    static std::vector<uint8_t> generateGavelWav();
    static std::vector<uint8_t> generateTickWav();
    static std::vector<uint8_t> generateVictoryWav();
    static std::vector<uint8_t> generateBidWav();

    bool muted_{false};
    std::vector<uint8_t> gavelWav_;
    std::vector<uint8_t> tickWav_;
    std::vector<uint8_t> victoryWav_;
    std::vector<uint8_t> bidWav_;
};

} // namespace auction::presentation
