#ifndef __FFMPEG_CORE_H__
#define __FFMPEG_CORE_H__

#include <stdint.h>

namespace ffmpeg_core {
    bool play(const char *strPath);
    bool stop();
    bool isFinish();

    bool pause();
    bool resume();

    uint64_t getLength();
    uint64_t getPos();

    bool seekTo(int64_t milliSec);

    static const uint16_t MAX_VOLUME = 0xFFFFU;

    bool setVolume(uint32_t volume);
}

#endif
