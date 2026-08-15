#ifndef YCODE_AUDIO_H
#define YCODE_AUDIO_H

#include <string>

namespace ycode {

/// 简单的音频播放（当前支持 WAV，Windows 下用 PlaySound 异步播放）。
class AudioPlayer {
public:
    AudioPlayer() = default;
    ~AudioPlayer();

    /// 播放 WAV 文件（异步）。loop=true 时循环播放。返回是否成功。
    bool playWav(const std::string& path, bool loop = false);

    /// 停止当前播放。
    void stop();
};

} // namespace ycode

#endif // YCODE_AUDIO_H
