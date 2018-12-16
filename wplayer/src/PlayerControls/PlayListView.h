#ifndef __PLAY_LIST_VIEW_H__
#define __PLAY_LIST_VIEW_H__

#include "../base/Window.h"
#include <vector>
#include <string>

enum class PLAY_MODE {
    ALL_REPEAT = 0,
    ORDER,
    REPEAT_ONCE,
    ONCE,
    SHUFFLE
};

class PlayListView : public Window {
public:
    bool init(HWND hPerent, const POINT &pos, const SIZE &size);

    void onNotify(WPARAM wParam, LPARAM lParam);
    bool insertListItem(LPCWSTR fileName);
    LPCWSTR getCurrentFile();
    LPCWSTR getNextFile(PLAY_MODE mode, bool manual);
    LPCWSTR getPrevFile(PLAY_MODE mode);
    LPCWSTR getSelectedFile() const;

protected:
    void initColumns();
    void refreshSelectedIndex(int idx);
    bool loadPlayList();

    static LRESULT CALLBACK ListViewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    std::vector<std::wstring> _files;
    int _selectedIdx = -1;
    int _playingIdx = -1;
};

#endif
