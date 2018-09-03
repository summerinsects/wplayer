#include "PlayListView.h"
#include <CommCtrl.h>
#include <shellapi.h>
#include <time.h>

static struct {
    int width;
    WCHAR text[13];
} s_colInfo[] = {
    { 210, L"文件(0)" },
    { 60, L"时长" },
    { 70, L"文件大小" }
};

bool PlayListView::init(HWND hPerent, const POINT &pos, const SIZE &size) {
    _hSelf = ::CreateWindowExW(WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES,
        WC_LISTVIEWW, nullptr,
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        pos.x, pos.y, size.cx, size.cy,
        hPerent, NULL, s_hInstance, nullptr);

    ::SetWindowLongPtrW(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ::SendMessageW(_hSelf, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, static_cast<LPARAM>(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES));

    _defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(_hSelf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PlayListView::ListViewProc)));

    initColumns();
    loadPlayList();

    return _hSelf != nullptr;
}

void PlayListView::initColumns() {
    LVCOLUMNW listCol;
    listCol.mask = LVCF_TEXT | LVCF_WIDTH;
    listCol.cchTextMax = 128;

    for (size_t i = 0, cnt = _countof(s_colInfo); i < cnt; ++i) {
        listCol.cx = s_colInfo[i].width;
        listCol.pszText = s_colInfo[i].text;
        ::SendMessageW(_hSelf, LVM_INSERTCOLUMNW, static_cast<WPARAM>(i), reinterpret_cast<LPARAM>(&listCol));
    }
}

LRESULT CALLBACK PlayListView::ListViewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PlayListView *thiz = reinterpret_cast<PlayListView *>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return thiz->runProc(hwnd, message, wParam, lParam);
}

LRESULT PlayListView::runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DROPFILES) {  // 拦截文件拉拽消息
        HDROP hDrop = reinterpret_cast<HDROP>(wParam);
        UINT count = ::DragQueryFileW(hDrop, 0xFFFFFFFFU, nullptr, 0);  // 获取文件总数

        WCHAR fileName[_MAX_PATH];
        for (UINT i = 0; i < count; ++i) {
            ::DragQueryFileW(hDrop, i, fileName, _MAX_PATH - 1);  // 逐个获取
            WCHAR *dot = wcsrchr(fileName, L'.');

            if (dot == nullptr) {  // 无扩展名
                continue;
            }

            // TODO:
            if (wcscmp(dot, L".mp3") == 0) {  // 检测扩展名
                insertListItem(fileName);  // 添加文件
            }
        }

        ::DragFinish(hDrop);

        return 0;
    }

    return ::CallWindowProcW(_defaultProc, hwnd, message, wParam, lParam);
}

bool PlayListView::insertListItem(LPCWSTR fileName) {
    WCHAR shortName[MAX_PATH];
    if (LPCWSTR p = wcsrchr(fileName, L'\\')) {
        wcsncpy(shortName, p + 1, MAX_PATH);
    }
    else {
        wcsncpy(shortName, fileName, MAX_PATH);
    }

    // 插入行
    LVITEMW listItem;

    listItem.mask = LVIF_TEXT | LVIF_PARAM;
    listItem.cchTextMax = _MAX_PATH - 1;
    listItem.pszText = shortName;
    listItem.iItem = static_cast<int>(_files.size());
    listItem.iSubItem = 0;//IDX_FILENAME;
    listItem.lParam = 0;// (LPARAM)gs_ppFileList[gs_iListCount];
    ::SendMessageW(_hSelf, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&listItem));

    _selectedIdx = static_cast<int>(_files.size());
    _files.push_back(fileName);

    // 更新头
    WCHAR textBuf[128];
    _snwprintf(textBuf, sizeof(shortName), L"文件(%zu)", _files.size());

    LVCOLUMNW listCol;
    listCol.mask = LVCF_TEXT;
    listCol.cchTextMax = 128;

    listCol.pszText = textBuf;
    ::SendMessageW(_hSelf, LVM_SETCOLUMNW, static_cast<WPARAM>(0), reinterpret_cast<LPARAM>(&listCol));

    return true;
}

void PlayListView::onNotify(WPARAM wParam, LPARAM lParam) {
    switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
    case LVN_COLUMNCLICK:
        if (_files.empty()) {
            break;
        }
        //sortWithCol(((LPNMLISTVIEW)lParam)->iSubItem);  // 排序
        break;

    case NM_CLICK: {  // 单击
        LPNMITEMACTIVATE ia = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
        _selectedIdx = ia->iItem;
        break;
    }

    case NM_DBLCLK: {  // 双击
        LPNMITEMACTIVATE ia = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
        _selectedIdx = ia->iItem;
        _playingIdx = _selectedIdx;
        HWND hParent = ::GetParent(_hSelf);
        ::SendMessageW(hParent, WM_COMMAND, MAKEWPARAM(/*IDB_STOP*/0x2004, 0), 0);  // 停止
        ::SendMessageW(hParent, WM_COMMAND, MAKEWPARAM(/*IDB_PLAY*/0x2001, 0), 0);  // 播放
        break;
    }

    case NM_RCLICK: {  // 右键
        LPNMITEMACTIVATE ia = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
        break;
    }

    default:
        break;
    }
}

LPCWSTR PlayListView::getCurrentFile() {
    if (_files.empty()) {
        return nullptr;
    }

    if (_playingIdx < 0) _playingIdx = 0;
    return _files.at(_playingIdx).c_str();
}

LPCWSTR PlayListView::getNextFile(PLAY_MODE mode, bool manual) {
    if (_files.empty()) {
        return nullptr;
    }

    switch (mode) {
    case PLAY_MODE::ALL_REPEAT:
    case PLAY_MODE::REPEAT_ONCE:
    case PLAY_MODE::ONCE:
        if (static_cast<size_t>(++_playingIdx) >= _files.size()) {
            _playingIdx = 0;
        }
        return _files.at(_playingIdx).c_str();
    case PLAY_MODE::ORDER:
        if (manual || static_cast<size_t>(_playingIdx + 1) < _files.size()) {
            return _files.at(++_playingIdx).c_str();
        }
        return nullptr;
    case PLAY_MODE::SHUFFLE:
        srand(static_cast<unsigned>(time(nullptr)));
        _playingIdx = static_cast<int>(rand() % _files.size());
        return _files.at(_playingIdx).c_str();
    default:
        return nullptr;
    }
}

LPCWSTR PlayListView::getPrevFile(PLAY_MODE mode) {
    if (_files.empty()) {
        return nullptr;
    }

    if (mode != PLAY_MODE::SHUFFLE) {
        if (--_playingIdx < 0) {
            _playingIdx = static_cast<int>(_files.size()) - 1;
        }
        return _files.at(_playingIdx).c_str();
    }
    else {
        srand(static_cast<unsigned>(time(nullptr)));
        _playingIdx = static_cast<int>(rand() % _files.size());
        return _files.at(_playingIdx).c_str();
    }
}
