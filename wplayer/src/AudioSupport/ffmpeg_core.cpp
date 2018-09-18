#include "ffmpeg_core.h"

#include <vector>
#include <thread>

#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")

#pragma warning(disable:4996)

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

#ifdef _WIN64
#pragma comment(lib, "../wplayer/src/AudioSupport/x64/lib/avcodec.lib")
#pragma comment(lib, "../wplayer/src/AudioSupport/x64/lib/avutil.lib")
#pragma comment(lib, "../wplayer/src/AudioSupport/x64/lib/avformat.lib")
#pragma comment(lib, "../wplayer/src/AudioSupport/x64/lib/swresample.lib")
#else
#pragma comment(lib, "../wplayer/src/AudioSupport/x86/lib/avcodec.lib")
#pragma comment(lib, "../wplayer/src/AudioSupport/x86/lib/avutil.lib")
#pragma comment(lib, "../wplayer/src/AudioSupport/x86/lib/avformat.lib")
#pragma comment(lib, "../wplayer/src/AudioSupport/x86/lib/swresample.lib")
#endif

namespace ffmpeg_core {

#define BUFFER_COUNT 2
#define BUFFER_LEN 7056 //(int)((double)44100 * 1 / 25 + 0.5) * 4;

    struct AudioState {
        AVFormatContext *fmtCtx = nullptr;
        int audioIndex = -1;

        HWAVEOUT hWaveOut = NULL;
        WORD channels = 0;
        WAVEHDR waveHdr[BUFFER_COUNT];
        std::vector<BYTE> buf[BUFFER_COUNT];

        volatile uint64_t time = 0;
        volatile uint64_t bytes = 0;
        uint64_t duration = 0;
        std::thread renderThread;
        volatile bool shouldQuit = false;
        volatile bool finished = false;
        volatile int64_t seekTo = 0;
        volatile bool shouldSeek = false;
    };
    static AudioState s_as;

    static void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
        UNREFERENCED_PARAMETER(hWaveOut);
        UNREFERENCED_PARAMETER(dwInstance);
        UNREFERENCED_PARAMETER(dwParam2);
        if (uMsg == WOM_DONE) {
            ::SetEvent(reinterpret_cast<HANDLE>(reinterpret_cast<WAVEHDR *>(dwParam1)->dwUser));
        }
    }

    static bool open(const char *strPath, AudioState *as) {
        as->fmtCtx = avformat_alloc_context();
        if (avformat_open_input(&as->fmtCtx, strPath, nullptr, nullptr) < 0) {
            return false;
        }

        if (avformat_find_stream_info(as->fmtCtx, nullptr) < 0) {
            return false;
        }

        as->audioIndex = -1;
        for (int i = 0; i < (int)as->fmtCtx->nb_streams; ++i) {
            if (as->fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                as->audioIndex = i;
                break;
            }
        }

        if (as->audioIndex == -1) {
            return false;
        }

        AVStream *stream = as->fmtCtx->streams[as->audioIndex];

        const AVRational &base = stream->time_base;
        as->duration = stream->duration / base.num * 1000 / base.den;

        AVCodecContext *codecCtx = stream->codec;
        AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);
        if (codec == nullptr) {
            return false;
        }

        if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
            return false;
        }

        as->channels = static_cast<WORD>(av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO));

        WAVEFORMATEX waveFormatex = { 0 };
        waveFormatex.wFormatTag = WAVE_FORMAT_PCM;
        waveFormatex.wBitsPerSample = 16;
        waveFormatex.nSamplesPerSec = 44100;
        waveFormatex.nChannels = as->channels;
        waveFormatex.nBlockAlign = waveFormatex.nChannels * waveFormatex.wBitsPerSample / 8;
        waveFormatex.nAvgBytesPerSec = waveFormatex.nSamplesPerSec * waveFormatex.nBlockAlign;

        if (MMSYSERR_NOERROR != ::waveOutOpen(&as->hWaveOut, WAVE_MAPPER, &waveFormatex, reinterpret_cast<DWORD_PTR>(&waveOutProc), 0, CALLBACK_FUNCTION)) {
            return false;
        }

        for (int i = 0; i < BUFFER_COUNT; ++i) {
            memset(&as->waveHdr[i], 0, sizeof(WAVEHDR));
            as->buf[i].resize(BUFFER_LEN);

            as->waveHdr[i].lpData = reinterpret_cast<LPSTR>(as->buf[i].data());
            as->waveHdr[i].dwBufferLength = BUFFER_LEN;
            as->waveHdr[i].dwUser = reinterpret_cast<DWORD_PTR>(::CreateEventW(nullptr, TRUE, TRUE, nullptr));
            ::waveOutPrepareHeader(as->hWaveOut, &as->waveHdr[i], sizeof(WAVEHDR));
        }

        return true;
    }

    static void close(AudioState *as) {
        avformat_close_input(&as->fmtCtx);
        avformat_free_context(as->fmtCtx);

        if (as->hWaveOut != NULL) {
            for (int i = 0; i < BUFFER_COUNT; ++i) {
                ::CloseHandle(reinterpret_cast<HANDLE>(as->waveHdr[i].dwUser));
                ::waveOutUnprepareHeader(as->hWaveOut, &as->waveHdr[i], sizeof(WAVEHDR));
            }

            ::waveOutClose(as->hWaveOut);
            as->hWaveOut = NULL;
        }
    }

    static void AudioRenderThread(AudioState *as) {
        AVStream *stream = as->fmtCtx->streams[as->audioIndex];
        const AVRational &base = stream->time_base;

        AVPacket *packet = av_packet_alloc();
        AVFrame *frame = av_frame_alloc();

        AVCodecContext *codecCtx = stream->codec;
        SwrContext *swrCtx = swr_alloc_set_opts(nullptr, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100,
            av_get_default_channel_layout(codecCtx->channels),
            codecCtx->sample_fmt, codecCtx->sample_rate, 0, nullptr);
        swr_init(swrCtx);

        unsigned avail = 0;
        uint8_t *buf = nullptr;
        int bufIdx = 0;
        while (!as->shouldQuit) {
            if (as->shouldSeek) {
                int64_t pts = as->seekTo * 1000;
                if (av_seek_frame(as->fmtCtx, -1, pts, AVSEEK_FLAG_ANY) >= 0) {
                    avcodec_flush_buffers(codecCtx);
                    as->shouldSeek = false;
                }
            }

            if (av_read_frame(as->fmtCtx, packet) >= 0) {
                if (packet->stream_index == as->audioIndex) {

                    as->time = (base.den == 0 ? -1 : packet->pts * base.num * 1000 / base.den);
                    MMTIME mmt = { TIME_BYTES };
                    ::waveOutGetPosition(s_as.hWaveOut, &mmt, sizeof(mmt));
                    as->bytes = mmt.u.cb;

                    while (!as->shouldQuit && !as->shouldSeek) {

                        int gotFrame;
                        int ret = avcodec_decode_audio4(codecCtx, frame, &gotFrame, packet);

                        if (ret < 0) {
                            if (!gotFrame) {
                                break;
                            }

                            packet->size = 0;
                            continue;
                        }

                        int sampnum = 0;
                        while (!as->shouldQuit && !as->shouldSeek) {
                            WAVEHDR &waveHdr = as->waveHdr[bufIdx];
                            if (avail == 0) {
                                ::WaitForSingleObject(reinterpret_cast<HANDLE>(waveHdr.dwUser), INFINITE);
                                avail = waveHdr.dwBufferLength;
                                buf = reinterpret_cast<uint8_t *>(waveHdr.lpData);
                            }
                            sampnum = swr_convert(swrCtx, &buf, avail / 4, const_cast<const uint8_t **>(frame->extended_data), frame->nb_samples);
                            frame->extended_data = nullptr;
                            frame->nb_samples = 0;
                            avail -= sampnum * 4;
                            buf += sampnum * 4;

                            if (avail == 0) {
                                ::ResetEvent(reinterpret_cast<HANDLE>(waveHdr.dwUser));
                                ::waveOutWrite(as->hWaveOut, &waveHdr, sizeof(WAVEHDR));
                                if (++bufIdx == BUFFER_COUNT) {
                                    bufIdx = 0;
                                }
                            }

                            if (sampnum <= 0) {
                                break;
                            }
                        }
                        packet->data += ret;
                        packet->size -= ret;

                        if (packet->size <= 0) {
                            break;
                        }
                    }
                }
                av_packet_unref(packet);
            }
            else {
                break;
            }
        }

        av_packet_free(&packet);
        av_frame_free(&frame);
        swr_free(&swrCtx);

        as->finished = true;
    }

    bool play(const char *strPath) {
        stop();

        s_as.shouldQuit = false;
        s_as.finished = false;

        av_register_all();

        if (!open(strPath, &s_as)) {
            return false;
        }

        s_as.renderThread = std::thread(AudioRenderThread, &s_as);
        return true;
    }

    bool stop() {
        resume();

        s_as.shouldQuit = true;
        if (s_as.renderThread.joinable()) {
            s_as.renderThread.join();
        }

        close(&s_as);
        return true;
    }

    bool isFinish() {
        return s_as.finished;
    }

    bool pause() {
        if (s_as.hWaveOut != NULL) {
            ::waveOutPause(s_as.hWaveOut);
        }
        return true;
    }

    bool resume() {
        if (s_as.hWaveOut != NULL) {
            ::waveOutRestart(s_as.hWaveOut);
        }
        return true;
    }

    uint64_t getLength() {
        return s_as.duration;
    }

    uint64_t getPos() {
        if (s_as.hWaveOut == NULL) {
            return 0;
        }

        MMTIME mmt = { TIME_BYTES };
        MMRESULT ret = ::waveOutGetPosition(s_as.hWaveOut, &mmt, sizeof(mmt));
        if (ret != MMSYSERR_NOERROR) {
            return s_as.time;

        }

        //int64_t o = (mmt.u.cb - s_as.bytes) * 1000 / (44100 * s_as.channels * 16 / 8);
        int64_t offset = (mmt.u.cb - s_as.bytes) * 5 / (441 * s_as.channels);
        return s_as.time + offset;
    }

    bool seekTo(int64_t milliSec) {
        s_as.shouldSeek = true;
        s_as.seekTo = milliSec;
        return true;
    }

    bool setVolume(uint32_t volume) {
        if (s_as.hWaveOut == NULL) {
            return false;
        }

        return (MMSYSERR_NOERROR == ::waveOutSetVolume(s_as.hWaveOut, (volume << 16) | volume));
    }
}
