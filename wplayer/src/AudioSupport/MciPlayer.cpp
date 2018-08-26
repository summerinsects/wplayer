#include <windows.h>
#include <mmsystem.h>
#include <digitalv.h>
#include "MciPlayer.h"

#pragma comment(lib, "winmm.lib")

#define SHOW_ERROR_MESSAGE

#ifdef SHOW_ERROR_MESSAGE
#define ERROR_MESSAGE_BOX(hwnd, error) \
    do { \
        WCHAR errText[128]; \
        ::mciGetErrorStringW(error, errText, 127); \
        ::MessageBoxW(hwnd, errText, L"MCI错误", MB_ICONHAND); \
    } while (0)
#else
#define ERROR_MESSAGE_BOX(...)
#endif

bool MciPlayer::openFile(HWND hwnd, LPCWSTR fileName) {
    MCI_OPEN_PARMSW openParms;
    openParms.dwCallback = 0;
    openParms.lpstrAlias = nullptr;
    openParms.lpstrDeviceType = reinterpret_cast<LPCWSTR>(MCI_DEVTYPE_WAVEFORM_AUDIO);
    openParms.lpstrElementName = fileName;
    openParms.wDeviceID = 0;

    MCIERROR err = ::mciSendCommandW(0, MCI_OPEN, MCI_OPEN_ELEMENT, reinterpret_cast<DWORD_PTR>(&openParms));
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        _deviceID = 0;
        return false;
    }
    _deviceID = openParms.wDeviceID;

    return true;
}

bool MciPlayer::closeFile(HWND hwnd) {
    if (_deviceID == 0) {
        return FALSE;
    }

    MCIERROR err = ::mciSendCommandW(_deviceID, MCI_CLOSE, 0, 0);
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        return FALSE;
    }
    _deviceID = 0;

    return TRUE;
}

DWORD_PTR MciPlayer::getLength(HWND hwnd) {
    MCI_STATUS_PARMS statusParms;
    statusParms.dwCallback = 0;
    statusParms.dwItem = MCI_STATUS_LENGTH;
    statusParms.dwReturn = 0;
    statusParms.dwTrack = 0;

    MCIERROR err = ::mciSendCommandW(_deviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&statusParms));
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        return 0;
    }

    return statusParms.dwReturn;
}

DWORD_PTR MciPlayer::getPos(HWND hwnd) {
    MCI_STATUS_PARMS statusParms;
    statusParms.dwCallback = 0;
    statusParms.dwItem = MCI_STATUS_POSITION;
    statusParms.dwReturn = 0;
    statusParms.dwTrack = 0;

    MCIERROR err = ::mciSendCommandW(_deviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&statusParms));
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        return (DWORD_PTR)-1;
    }

    return statusParms.dwReturn;
}

bool MciPlayer::play(HWND hwnd) {
    if (_deviceID == 0) {
        return false;
    }

    MCI_PLAY_PARMS playParms;
    playParms.dwCallback = 0;
    playParms.dwFrom = 0;
    playParms.dwTo = 0;

    MCIERROR err = ::mciSendCommandW(_deviceID, MCI_PLAY, 0, reinterpret_cast<DWORD_PTR>(&playParms));
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        return false;
    }

    return true;
}

bool MciPlayer::pause(HWND hwnd) {
    if (_deviceID == 0) {
        return false;
    }

    MCI_GENERIC_PARMS genericParms;
    genericParms.dwCallback = 0;
    MCIERROR err = ::mciSendCommandW(_deviceID, MCI_PAUSE, 0, reinterpret_cast<DWORD_PTR>(&genericParms));
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        return false;
    }

    return true;
}

bool MciPlayer::stop(HWND hwnd) {
    if (_deviceID == 0) {
        return false;
    }

    MCI_GENERIC_PARMS genericParms;
    genericParms.dwCallback = 0;
    MCIERROR err = ::mciSendCommandW(_deviceID, MCI_STOP, 0, reinterpret_cast<DWORD_PTR>(&genericParms));
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        return false;
    }

    return true;
}

bool MciPlayer::goForward(HWND hwnd, DWORD milliSec, bool play) {
    if (_deviceID == 0) {
        return false;
    }

    MCI_STATUS_PARMS statusParms;
    statusParms.dwItem = MCI_STATUS_POSITION;
    statusParms.dwCallback = 0;
    statusParms.dwReturn = 0;
    statusParms.dwTrack = 0;

    MCIERROR err = ::mciSendCommandW(_deviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&statusParms));
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        return false;
    }

    MCI_SEEK_PARMS seekParms;
    seekParms.dwCallback = 0;
    seekParms.dwTo = static_cast<DWORD>(statusParms.dwReturn) + milliSec;

    err = ::mciSendCommandW(_deviceID, MCI_SEEK, MCI_TO, reinterpret_cast<DWORD_PTR>(&seekParms));
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        return false;
    }

    return (play ? this->play(hwnd) : true);
}

bool MciPlayer::goBackward(HWND hwnd, DWORD milliSec, bool play) {
    if (_deviceID == 0) {
        return false;
    }

    MCI_STATUS_PARMS statusParms;
    statusParms.dwItem = MCI_STATUS_POSITION;
    statusParms.dwCallback = 0;
    statusParms.dwReturn = 0;
    statusParms.dwTrack = 0;

    MCIERROR err = ::mciSendCommandW(_deviceID, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&statusParms));
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        return false;
    }

    MCI_SEEK_PARMS seekParms;
    seekParms.dwCallback = 0;
    seekParms.dwTo = (statusParms.dwReturn > milliSec) ? (static_cast<DWORD>(statusParms.dwReturn) - milliSec) : 0;

    err = ::mciSendCommandW(_deviceID, MCI_SEEK, MCI_TO, reinterpret_cast<DWORD_PTR>(&seekParms));
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        return false;
    }

    return (play ? this->play(hwnd) : true);
}

bool MciPlayer::seekTo(HWND hwnd, DWORD milliSec, bool play) {
    if (_deviceID == 0) {
        return false;
    }

    MCI_SEEK_PARMS seekParms;
    seekParms.dwCallback = 0;
    seekParms.dwTo = milliSec;

    MCIERROR err = ::mciSendCommandW(_deviceID, MCI_SEEK, MCI_TO, reinterpret_cast<DWORD_PTR>(&seekParms));
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        return false;
    }

    return (play ? this->play(hwnd) : true);
}

bool MciPlayer::setVolume(HWND hwnd, DWORD volume) {
    if (_deviceID == 0) {
        return false;
    }

    MCI_DGV_SETAUDIO_PARMS setParms;
    setParms.dwItem = MCI_DGV_SETAUDIO_VOLUME;
    setParms.dwValue = volume;

    MCIERROR err = ::mciSendCommandW(_deviceID, MCI_SETAUDIO, MCI_DGV_SETAUDIO_VALUE | MCI_DGV_SETAUDIO_ITEM,
        reinterpret_cast<DWORD_PTR>(&setParms));
    if (err != 0) {
        ERROR_MESSAGE_BOX(hwnd, err);
        return false;
    }

    return true;
}
