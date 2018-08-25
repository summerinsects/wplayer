#include "PlayListView.h"
#include <CommCtrl.h>
#include <shellapi.h>

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
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | LVS_REPORT | LVS_SHOWSELALWAYS,
        pos.x, pos.y, size.cx, size.cy,
        hPerent, NULL, s_hInstance, nullptr);

    ::SetWindowLongPtrW(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ::SendMessageW(_hSelf, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES));

    _defaultProc = (WNDPROC)::SetWindowLongPtrW(_hSelf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PlayListView::ListViewProc));

    initColumns();

    return _hSelf != nullptr;
}

void PlayListView::initColumns() {
    LVCOLUMNW listCol;
    listCol.mask = LVCF_TEXT | LVCF_WIDTH;
    listCol.cchTextMax = 128;

    for (size_t i = 0, cnt = _countof(s_colInfo); i < cnt; ++i) {
        listCol.cx = s_colInfo[i].width;
        listCol.pszText = s_colInfo[i].text;
        ::SendMessageW(_hSelf, LVM_INSERTCOLUMNW, (WPARAM)i, (LPARAM)&listCol);
    }
}

LRESULT CALLBACK PlayListView::ListViewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PlayListView *thiz = reinterpret_cast<PlayListView *>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return thiz->runProc(hwnd, message, wParam, lParam);
}

LRESULT PlayListView::runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DROPFILES) {  // 拦截文件拉拽消息
        HDROP hDrop = (HDROP)wParam;
        UINT count = ::DragQueryFileW(hDrop, 0xFFFFFFFFU, nullptr, 0);  // 获取文件总数

        WCHAR fileName[_MAX_PATH];
        for (UINT i = 0; i < count; ++i) {
            ::DragQueryFileW(hDrop, i, fileName, _MAX_PATH - 1);  // 逐个获取
            WCHAR *dot = wcsrchr(fileName, L'.');

            if (dot == nullptr) {  // 无扩展名
                continue;
            }

            // TODO:
            //if (_tcscmp(dot, TEXT(".mp3")) == 0) {  // 检测扩展名
            insertListItem(fileName);  // 添加文件
            //}
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
    ::SendMessageW(_hSelf, LVM_INSERTITEMW, 0, (LPARAM)&listItem);

    _files.push_back(fileName);

    // 更新头
    WCHAR textBuf[128];
    _snwprintf(textBuf, sizeof(shortName), L"文件(%zu)", _files.size());

    LVCOLUMNW listCol;
    listCol.mask = LVCF_TEXT;
    listCol.cchTextMax = 128;

    listCol.pszText = textBuf;
    ::SendMessageW(_hSelf, LVM_SETCOLUMNW, (WPARAM)0, (LPARAM)&listCol);

    return true;
}

void PlayListView::onNotify(WPARAM wParam, LPARAM lParam) {
    switch (((LPNMHDR)lParam)->code) {
    case LVN_COLUMNCLICK:
        if (_files.empty()) {
            break;
        }
        //sortWithCol(((LPNMLISTVIEW)lParam)->iSubItem);  // 排序
        break;

    case NM_CLICK: {  // 单击
        LPNMITEMACTIVATE ia = (LPNMITEMACTIVATE)lParam;
        break;
    }

    case NM_DBLCLK: {  // 双击
        LPNMITEMACTIVATE ia = (LPNMITEMACTIVATE)lParam;
        break;
    }

    case NM_RCLICK: {  // 右键
        LPNMITEMACTIVATE ia = (LPNMITEMACTIVATE)lParam;
        break;
    }

    default:
        break;
    }
}
