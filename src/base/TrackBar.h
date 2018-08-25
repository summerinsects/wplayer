#ifndef __TRACK_BAR_H__
#define __TRACK_BAR_H__

#include "Window.h"
#include <functional>

#undef max
#undef min

class TrackBar : public Window {
public:
    bool init(HWND hPerent, const POINT &pos, const SIZE &size);

    bool setRange(WORD min, WORD max);
    void onHScroll(WPARAM wParam, LPARAM lParam);

    void setPosChangedListener(std::function<void (TrackBar *thiz, LONG_PTR pos)> onPosChanged) { _onPosChanged.swap(onPosChanged); }
    void setTrackingListener(std::function<void (TrackBar *thiz, LONG_PTR pos)> onTracking) { _onTracking.swap(onTracking); }

protected:
    std::function<void (TrackBar *thiz, LONG_PTR pos)> _onPosChanged;
    std::function<void (TrackBar *thiz, LONG_PTR pos)> _onTracking;

    bool _isTracking = false;

    static LRESULT CALLBACK TrackBarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};

#endif
