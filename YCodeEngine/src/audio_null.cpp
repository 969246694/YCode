#include "ycode/audio.h"

namespace ycode {

AudioPlayer::~AudioPlayer()
{
}

bool AudioPlayer::playWav(const std::string&, bool)
{
    return false; // 非 Windows 平台暂不支持
}

void AudioPlayer::stop()
{
}

} // namespace ycode
