#ifndef __MCI_PLAYER_H__
#define __MCI_PLAYER_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mciapi.h>

class MciPlayer {
public:
    bool openFile(HWND hwnd, LPCWSTR fileName);  // 打开设备
    bool closeFile(HWND hwnd);  // 关闭设备
    DWORD_PTR getLength(HWND hwnd);  // 获取文件长度
    DWORD_PTR getPos(HWND hwnd);  // 获取当前位置

    bool play(HWND hwnd);  // 播放
    bool pause(HWND hwnd);  // 暂停
    bool stop(HWND hwnd);  // 停止
    bool goForward(HWND hwnd, DWORD milliSec, bool play);  // 前进
    bool goBackward(HWND hwnd, DWORD milliSec, bool play);  // 后退
    bool seekTo(HWND hwnd, DWORD milliSec, bool play);  // 跳转到
    bool setVolume(HWND hwnd, DWORD volume);  // 设置音量

private:
    MCIDEVICEID _deviceID = 0;
};

#endif
