#ifndef __PLAY_LIST_VIEW_H__
#define __PLAY_LIST_VIEW_H__

#include "../base/Window.h"
#include <vector>
#include <string>

class PlayListView : public Window {
public:
    bool init(HWND hPerent, const POINT &pos, const SIZE &size);

    void onNotify(WPARAM wParam, LPARAM lParam);
    bool insertListItem(LPCWSTR fileName);
    LPCWSTR getSelectedFile() const;

protected:
    void initColumns();

    static LRESULT CALLBACK ListViewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    std::vector<std::wstring> _files;
    int _selectedIdx = -1;
};

#endif
