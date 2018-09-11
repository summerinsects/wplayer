#include "LyricsEditorDialog.h"
#include <CommCtrl.h>
#include <commdlg.h>
#include <Richedit.h>

void LyricsEditorDialog::show(HWND hwndOwner) {
    WORD tempData[128];
    LPDLGTEMPLATEW lpDlgTemp;

    memset(tempData, 0, sizeof(tempData));
    lpDlgTemp = reinterpret_cast<LPDLGTEMPLATEW>(tempData);

    lpDlgTemp->style = WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_MODALFRAME;
    lpDlgTemp->dwExtendedStyle = WS_EX_TOOLWINDOW;
    lpDlgTemp->cdit = 0;
    lpDlgTemp->x = 0;
    lpDlgTemp->y = 0;
    lpDlgTemp->cx = 260;
    lpDlgTemp->cy = 280;

    ::CreateDialogIndirectParamW(s_hInstance, lpDlgTemp, hwndOwner, &LyricsEditorDialog::DialogProc, reinterpret_cast<LPARAM>(this));
    ::ShowWindow(_hSelf, SW_SHOW);
}

INT_PTR CALLBACK LyricsEditorDialog::DialogProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_INITDIALOG) {
        LyricsEditorDialog *thiz = reinterpret_cast<LyricsEditorDialog *>(lParam);
        ::SetWindowLongPtrW(hDialog, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(thiz));
        thiz->_hSelf = hDialog;
        thiz->init();
        return TRUE;
    }

    LyricsEditorDialog *thiz = reinterpret_cast<LyricsEditorDialog *>(::GetWindowLongPtrW(hDialog, GWLP_USERDATA));
    return thiz->runProc(hDialog, message, wParam, lParam);
}

INT_PTR LyricsEditorDialog::runProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_GETMINMAXINFO: {
        LPMINMAXINFO mmi = reinterpret_cast<LPMINMAXINFO>(lParam);
        mmi->ptMinTrackSize.x = 500;
        mmi->ptMinTrackSize.y = 500;
        return TRUE;
    }

    case WM_SIZE: {
        WORD width = LOWORD(lParam);
        WORD height = HIWORD(lParam);
        ::MoveWindow(_hStatic, 10, 10, width - 20, 80, TRUE);
        ::MoveWindow(_hListView, 10, 100, width - 20, 80, TRUE);
        ::MoveWindow(_hRichEdit, 10, 190, width - 20, height - 200, TRUE);
        return TRUE;
    }

    case WM_CLOSE:
        ::DestroyWindow(hDialog);
        _hSelf = NULL;
        return TRUE;

    default:
        break;
    }

    return FALSE;
}

void LyricsEditorDialog::init() {
    ::SendMessageW(_hSelf, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"歌词编辑器"));

    HMENU hEditorMenu = CreateMenu();
    HMENU hEditMenu = CreatePopupMenu();
    HMENU hFileMenu = CreatePopupMenu();
    HMENU hSettingMenu = CreatePopupMenu();
    HMENU hHelpMenu = CreatePopupMenu();

    ::AppendMenuW(hEditorMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFileMenu), L"文件(&F)");
    ::AppendMenuW(hEditorMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hEditMenu), L"编辑(&E)");
    ::AppendMenuW(hEditorMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSettingMenu), L"设置(&S)");
    ::AppendMenuW(hEditorMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hHelpMenu), L"帮助(&H)");

    ::SetMenu(_hSelf, hEditorMenu);

    _hStatic = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"static", nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_OWNERDRAW,
        0, 0, 0, 0,
        _hSelf, NULL, s_hInstance, nullptr);

    _hListView = ::CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr,
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SINGLESEL,
        0, 0, 0, 0, _hSelf, NULL, s_hInstance, nullptr);

    _hRichEdit = ::CreateWindowExW(WS_EX_CLIENTEDGE, RICHEDIT_CLASSW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | WS_HSCROLL | WS_VSCROLL
        | ES_MULTILINE | ES_NOHIDESEL | ES_DISABLENOSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        0, 0, 0, 0,
        _hSelf, NULL, s_hInstance, nullptr);
    ::SendMessageW(_hRichEdit, EM_LIMITTEXT, static_cast<WPARAM>(0xFFFFFFFFU), 0);
    ::SendMessageW(_hRichEdit, EM_SETEVENTMASK, 0, static_cast<LPARAM>(ENM_SELCHANGE | ENM_UPDATE));

    HFONT hFont = reinterpret_cast<HFONT>(::SendMessageW(_hListView, WM_GETFONT, 0, 0));
    ::SendMessageW(_hRichEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(FALSE));
}
