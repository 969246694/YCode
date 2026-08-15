#include "ycode/audio.h"

#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

namespace ycode {

AudioPlayer::~AudioPlayer()
{
    stop();
}

bool AudioPlayer::playWav(const std::string& path, bool loop)
{
    if (path.empty())
        return false;

    DWORD flags = SND_FILENAME | SND_ASYNC | SND_NODEFAULT;
    if (loop)
        flags |= SND_LOOP;
    return PlaySoundA(path.c_str(), nullptr, flags) != 0;
}

void AudioPlayer::stop()
{
    PlaySoundA(nullptr, nullptr, 0);
}

} // namespace ycode
