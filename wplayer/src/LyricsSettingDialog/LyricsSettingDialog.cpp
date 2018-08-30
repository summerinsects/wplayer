#include "LyricsSettingDialog.h"
#include <CommCtrl.h>
#include <commdlg.h>
#include <algorithm>

#pragma comment(lib, "msimg32.lib")  // for ::AlphaBlend

#define IDS_SETFONT 0x5100
#define IDM_APPEND 0x6000
#define IDM_DELETE 0x6001
#define IDM_MODIFY 0x6002
#define IDM_MOVE_UP 0x6003
#define IDM_MOVE_DOWN 0x6004

#define IDB_LYRICS_SEPARATE 0x7000
#define IDB_LYRICS_LEFT 0x7001
#define IDB_LYRICS_RIGHT 0x7002
#define IDB_LYRICS_CENTER 0x7003

#define IDB_LYRICS_DOUBLE_LINES 0x7004
#define IDB_LYRICS_SINGLE_LINE 0x7005

static bool chooseColor(HWND hwndOwner, LPCOLORREF lpColor);
static bool chooseFont(HWND hwndOwner, LPLOGFONTW lpLogFont);
static void insertColor(HWND hListView, int idx, COLORREF color);
static void deleteColor(HWND hListView, int idx);
static void updateColor(HWND hListView, int idx, COLORREF color);

bool LyricsSettingDialog::show(HWND hwndOwner, DrawSupport::DrawParam *param) {
    _drawParam = *param;

    WORD tempData[128];
    LPDLGTEMPLATEW lpDlgTemp;

    memset(tempData, 0, sizeof(tempData));
    lpDlgTemp = reinterpret_cast<LPDLGTEMPLATEW>(tempData);

    lpDlgTemp->style = WS_CAPTION | WS_SYSMENU | DS_CENTER | DS_MODALFRAME;
    lpDlgTemp->dwExtendedStyle = WS_EX_TOOLWINDOW;
    lpDlgTemp->cdit = 0;
    lpDlgTemp->x = 0;
    lpDlgTemp->y = 0;
    lpDlgTemp->cx = 240;
    lpDlgTemp->cy = 220;

    INT_PTR ret = ::DialogBoxIndirectParamW(s_hInstance, lpDlgTemp, hwndOwner, &LyricsSettingDialog::DialogProc, reinterpret_cast<LPARAM>(this));

    if (ret == IDOK) {
        *param = _drawParam;
        return true;
    }

    return false;
}

INT_PTR CALLBACK LyricsSettingDialog::DialogProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_INITDIALOG) {
        LyricsSettingDialog *thiz = reinterpret_cast<LyricsSettingDialog *>(lParam);
        ::SetWindowLongPtrW(hDialog, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(thiz));
        thiz->_hSelf = hDialog;
        thiz->init();
        return TRUE;
    }

    LyricsSettingDialog *thiz = reinterpret_cast<LyricsSettingDialog *>(::GetWindowLongPtrW(hDialog, GWLP_USERDATA));
    return thiz->runProc(hDialog, message, wParam, lParam);
}

INT_PTR LyricsSettingDialog::runProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDS_SETFONT: {
            if (chooseFont(hDialog, &_drawParam.logFont)) {
                ::InvalidateRect(_hStaticExample, nullptr, TRUE);
            }
            return TRUE;
        }

        case IDM_APPEND: {
            std::vector<COLORREF> *colors = reinterpret_cast<std::vector<COLORREF> *>(::GetWindowLongPtrW(_hCurrentList, GWLP_USERDATA));
            if (colors != nullptr) {
                COLORREF color;
                if (chooseColor(hDialog, &color)) {
                    colors->push_back(color);
                    insertColor(_hCurrentList, static_cast<int>(colors->size()), color);
                    ::InvalidateRect(_hStaticExample, nullptr, TRUE);
                }
            }
            return TRUE;
        }

        case IDM_DELETE: {
            std::vector<COLORREF> *colors = reinterpret_cast<std::vector<COLORREF> *>(::GetWindowLongPtrW(_hCurrentList, GWLP_USERDATA));
            if (colors != nullptr) {
                LRESULT selected = ::SendMessageW(_hCurrentList, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), static_cast<LPARAM>(LVNI_SELECTED));
                if (selected == -1 && colors->size() > 1) {
                    return TRUE;
                }
                colors->erase(colors->begin() + selected);
                deleteColor(_hCurrentList, static_cast<int>(selected));
                ::InvalidateRect(_hStaticExample, nullptr, TRUE);
            }
            return TRUE;
        }

        case IDM_MODIFY: {
            std::vector<COLORREF> *colors = reinterpret_cast<std::vector<COLORREF> *>(::GetWindowLongPtrW(_hCurrentList, GWLP_USERDATA));
            if (colors != nullptr) {
                LRESULT selected = ::SendMessageW(_hCurrentList, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), static_cast<LPARAM>(LVNI_SELECTED));
                if (selected == -1) {
                    return TRUE;
                }

                COLORREF color;
                if (chooseColor(hDialog, &color)) {
                    colors->at(selected) = color;
                    ::InvalidateRect(_hStaticExample, nullptr, TRUE);
                }
            }
            return TRUE;
        }

        case IDM_MOVE_UP: {
            std::vector<COLORREF> *colors = reinterpret_cast<std::vector<COLORREF> *>(::GetWindowLongPtrW(_hCurrentList, GWLP_USERDATA));
            if (colors != nullptr) {
                LRESULT selected = ::SendMessageW(_hCurrentList, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), static_cast<LPARAM>(LVNI_SELECTED));
                if (selected == -1 && selected > 0) {
                    return TRUE;
                }

                std::swap(colors->at(selected - 1), colors->at(selected));
                updateColor(_hCurrentList, static_cast<int>(selected), colors->at(selected));
                updateColor(_hCurrentList, static_cast<int>(selected - 1), colors->at(selected - 1));
                ::InvalidateRect(_hStaticExample, nullptr, TRUE);
            }
            return TRUE;
        }

        case IDM_MOVE_DOWN: {
            std::vector<COLORREF> *colors = reinterpret_cast<std::vector<COLORREF> *>(::GetWindowLongPtrW(_hCurrentList, GWLP_USERDATA));
            if (colors != nullptr) {
                LRESULT selected = ::SendMessageW(_hCurrentList, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), static_cast<LPARAM>(LVNI_SELECTED));
                if (selected == -1 && selected + 1 < static_cast<LRESULT>(colors->size())) {
                    return TRUE;
                }

                std::swap(colors->at(selected), colors->at(selected + 1));
                updateColor(_hCurrentList, static_cast<int>(selected), colors->at(selected));
                updateColor(_hCurrentList, static_cast<int>(selected + 1), colors->at(selected + 1));
                ::InvalidateRect(_hStaticExample, nullptr, TRUE);
            }
            return TRUE;
        }

        case IDB_LYRICS_SEPARATE:
            _drawParam.align &= 0xF0;
            return TRUE;

        case IDB_LYRICS_LEFT:
            _drawParam.align &= 0xF0;
            _drawParam.align |= LYRICS_LEFT;
            return TRUE;

        case IDB_LYRICS_RIGHT:
            _drawParam.align &= 0xF0;
            _drawParam.align |= LYRICS_RIGHT;
            return TRUE;

        case IDB_LYRICS_CENTER:
            _drawParam.align &= 0xF0;
            _drawParam.align |= LYRICS_CENTER;
            return TRUE;

        case IDB_LYRICS_DOUBLE_LINES:
            _drawParam.align &= 0x0F;
            return TRUE;

        case IDB_LYRICS_SINGLE_LINE:
            _drawParam.align &= 0x0F;
            _drawParam.align |= LYRICS_SINGLE_LINE;
            return TRUE;

        case IDOK:
            ::EndDialog(hDialog, IDOK);
            return TRUE;

        case IDCANCEL:
            ::EndDialog(hDialog, IDCANCEL);
            return TRUE;

        default:
            break;
        }
        break;

    case WM_NOTIFY: {
        HWND hListView = reinterpret_cast<LPNMHDR>(lParam)->hwndFrom;

        switch (((LPNMHDR)lParam)->code) {
        case NM_RCLICK: {
            LPNMITEMACTIVATE ia = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
            POINT point = { ia->ptAction.x, ia->ptAction.y };
            ::ClientToScreen(hListView, &point);

            HMENU hPopupMenu = ::CreatePopupMenu();
            ::AppendMenuW(hPopupMenu, MF_STRING, IDM_APPEND, L"添加");
            ::AppendMenuW(hPopupMenu, MF_STRING, IDM_DELETE, L"删除");
            ::AppendMenuW(hPopupMenu, MF_STRING, IDM_MODIFY, L"更改");
            ::AppendMenuW(hPopupMenu, MF_STRING, IDM_MOVE_UP, L"上移");
            ::AppendMenuW(hPopupMenu, MF_STRING, IDM_MOVE_DOWN, L"下移");

            _hCurrentList = hListView;
            ::TrackPopupMenuEx(hPopupMenu, TPM_LEFTALIGN | TPM_TOPALIGN, point.x, point.y, hDialog, nullptr);
            ::DestroyMenu(hPopupMenu);
            return TRUE;
        }
        case NM_CUSTOMDRAW: {
            LPNMLVCUSTOMDRAW cd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);
            LRESULT result = CDRF_DODEFAULT;
            switch (cd->nmcd.dwDrawStage) {
            case  CDDS_PREPAINT:
                result = CDRF_NOTIFYITEMDRAW;
                break;
            case CDDS_ITEMPREPAINT:
                LVITEMW listItem;
                listItem.mask = LVIF_PARAM;
                listItem.iSubItem = 0;
                listItem.iItem = static_cast<int>(cd->nmcd.dwItemSpec);
                ::SendMessageW(hListView, LVM_GETITEM, 0, reinterpret_cast<LPARAM>(&listItem));

                cd->clrTextBk = static_cast<COLORREF>(listItem.lParam);
                result = CDRF_NOTIFYSUBITEMDRAW;
                break;
            case (CDDS_ITEMPREPAINT | CDDS_SUBITEM):
                result = CDRF_DODEFAULT;
                break;
            default:
                break;
            }
            ::SetWindowLongPtrW(hDialog, DWLP_MSGRESULT, static_cast<LONG_PTR>(result));
            return TRUE;
        }
        default:
            break;
        }
        break;
    }

    case WM_INITMENUPOPUP: {
        const std::vector<COLORREF> *colors = reinterpret_cast<std::vector<COLORREF> *>(::GetWindowLongPtrW(_hCurrentList, GWLP_USERDATA));
        if (colors != nullptr) {
            LRESULT selected = ::SendMessageW(_hCurrentList, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), static_cast<LPARAM>(LVNI_SELECTED));

            HMENU hRightMenu = reinterpret_cast<HMENU>(wParam);
            if (selected == -1) {
                ::EnableMenuItem(hRightMenu, IDM_APPEND, MF_ENABLED);
                ::EnableMenuItem(hRightMenu, IDM_DELETE, MF_GRAYED);
                ::EnableMenuItem(hRightMenu, IDM_MODIFY, MF_GRAYED);
                ::EnableMenuItem(hRightMenu, IDM_MOVE_UP, MF_GRAYED);
                ::EnableMenuItem(hRightMenu, IDM_MOVE_DOWN, MF_GRAYED);
            }
            else {
                LRESULT cnt = static_cast<LRESULT>(colors->size());
                ::EnableMenuItem(hRightMenu, IDM_APPEND, MF_GRAYED);
                ::EnableMenuItem(hRightMenu, IDM_DELETE, cnt > 1 ? MF_ENABLED : MF_GRAYED);
                ::EnableMenuItem(hRightMenu, IDM_MODIFY, MF_ENABLED);
                ::EnableMenuItem(hRightMenu, IDM_MOVE_UP, selected > 0 ? MF_ENABLED : MF_GRAYED);
                ::EnableMenuItem(hRightMenu, IDM_MOVE_DOWN, selected + 1 < cnt ? MF_ENABLED : MF_GRAYED);
            }
        }
        return TRUE;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        if (dis->CtlType == ODT_STATIC) {
            drawSample(dis->hDC, dis->rcItem);
            return TRUE;
        }
        break;
    }

    case WM_CLOSE:
        ::EndDialog(hDialog, IDCLOSE);
        return TRUE;

    default:
        break;
    }

    return FALSE;
}

static void deleteColor(HWND hListView, int idx) {
    ::SendMessageW(hListView, LVM_DELETEITEM, static_cast<WPARAM>(idx), 0);
}

static void updateColor(HWND hListView, int idx, COLORREF color) {
    LVITEMW listItem;

    listItem.mask = LVIF_PARAM;
    listItem.iSubItem = 0;
    listItem.iItem = idx;
    listItem.lParam = static_cast<LPARAM>(color);
    ::SendMessageW(hListView, LVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&listItem));
}

static void insertColor(HWND hListView, int idx, COLORREF color) {
    LVITEMW listItem;

    listItem.mask = LVIF_PARAM;
    listItem.iSubItem = 0;
    listItem.iItem = idx;
    listItem.lParam = static_cast<LPARAM>(color);
    ::SendMessageW(hListView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&listItem));
}

static void setColorsForListView(HWND hListView, const std::vector<COLORREF> &colors) {
    LVITEMW listItem;

    listItem.mask = LVIF_PARAM;
    listItem.iSubItem = 0;
    for (size_t i = 0, cnt = colors.size(); i < cnt; ++i) {
        listItem.iItem = static_cast<int>(i);
        listItem.lParam = static_cast<LPARAM>(colors[i]);
        ::SendMessageW(hListView, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&listItem));
    }
}

void LyricsSettingDialog::init() {
    ::SendMessageW(_hSelf, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"设置歌词样式"));

    RECT rect;
    ::GetClientRect(_hSelf, &rect);
    LONG width = rect.right - rect.left;
    LONG height = rect.bottom - rect.top;

    HWND hStaticLabel, hButton, hListView;

    HFONT hFont = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));  // simsun
    //HFONT hFont = static_cast<HFONT>(::GetStockObject(DEVICE_DEFAULT_FONT));  // system
    //HFONT hFont = static_cast<HFONT>(::GetStockObject(SYSTEM_FONT));  // system

    hStaticLabel = ::CreateWindowExW(0, L"static", L"已播放",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        50, 60, 50, 25, _hSelf, NULL, s_hInstance, nullptr);
    ::SendMessageW(hStaticLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));

    hListView = ::CreateWindowExW(WS_EX_CLIENTEDGE/* | LVS_EX_BORDERSELECT*/, WC_LISTVIEWW, nullptr,
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | LVS_NOCOLUMNHEADER | LVS_LIST/* | LVS_OWNERDRAWFIXED*/,
        50, 85, 45, 80, _hSelf, NULL, s_hInstance, nullptr);
    ::SetWindowLongPtrW(hListView, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&_drawParam.colorPast));
    setColorsForListView(hListView, _drawParam.colorPast);

    hStaticLabel = ::CreateWindowExW(0, L"static", L"未播放",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        130, 60, 50, 25, _hSelf, NULL, s_hInstance, nullptr);
    ::SendMessageW(hStaticLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));

    hListView = ::CreateWindowExW(WS_EX_CLIENTEDGE/* | LVS_EX_BORDERSELECT*/, WC_LISTVIEWW, nullptr,
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | LVS_NOCOLUMNHEADER | LVS_LIST/* | LVS_OWNERDRAWFIXED*/,
        130, 85, 45, 80, _hSelf, NULL, s_hInstance, nullptr);
    ::SetWindowLongPtrW(hListView, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&_drawParam.colorFuture));
    setColorsForListView(hListView, _drawParam.colorFuture);

    hStaticLabel = ::CreateWindowExW(0, L"button", L"颜色",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        30, 30, 210, 150, _hSelf, NULL, s_hInstance, nullptr);
    ::SendMessageW(hStaticLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));

    hButton = ::CreateWindowExW(0, L"button", L"行数",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        width - 210, 30, 180, 60, _hSelf, NULL, s_hInstance, nullptr);
    ::SendMessageW(hButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));

    hButton = ::CreateWindowExW(0, L"button", L"双行",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON,
        width - 180, 50, 60, 25, _hSelf, reinterpret_cast<HMENU>(IDB_LYRICS_DOUBLE_LINES), s_hInstance, nullptr);
    ::SendMessageW(hButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));
    if ((_drawParam.align & LYRICS_SINGLE_LINE) == 0) ::SendMessageW(hButton, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0);

    hButton = ::CreateWindowExW(0, L"button", L"单行",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
        width - 105, 50, 60, 25, _hSelf, reinterpret_cast<HMENU>(IDB_LYRICS_SINGLE_LINE), s_hInstance, nullptr);
    ::SendMessageW(hButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));
    if (_drawParam.align & LYRICS_SINGLE_LINE) ::SendMessageW(hButton, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0);

    hButton = ::CreateWindowExW(0, L"button", L"对齐方式",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        width - 210, 120, 180, 100, _hSelf, NULL, s_hInstance, nullptr);
    ::SendMessageW(hButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));

    hButton = ::CreateWindowExW(0, L"button", L"分离",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | WS_GROUP,
        width - 180, 150, 60, 25, _hSelf, reinterpret_cast<HMENU>(IDB_LYRICS_SEPARATE), s_hInstance, nullptr);
    ::SendMessageW(hButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));
    if ((_drawParam.align & 0xF) == LYRICS_SEPARATE) ::SendMessageW(hButton, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0);

    hButton = ::CreateWindowExW(0, L"button", L"左对齐",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
        width - 105, 150, 60, 25, _hSelf, reinterpret_cast<HMENU>(IDB_LYRICS_LEFT), s_hInstance, nullptr);
    ::SendMessageW(hButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));
    if ((_drawParam.align & 0xF) == LYRICS_LEFT) ::SendMessageW(hButton, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0);

    hButton = ::CreateWindowExW(0, L"button", L"居中",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
        width - 180, 180, 60, 25, _hSelf, reinterpret_cast<HMENU>(IDB_LYRICS_CENTER), s_hInstance, nullptr);
    ::SendMessageW(hButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));
    if ((_drawParam.align & 0xF) == LYRICS_CENTER) ::SendMessageW(hButton, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0);

    hButton = ::CreateWindowExW(0, L"button", L"右对齐",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
        width - 105, 180, 60, 25, _hSelf, reinterpret_cast<HMENU>(IDB_LYRICS_RIGHT), s_hInstance, nullptr);
    ::SendMessageW(hButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));
    if ((_drawParam.align & 0xF) == LYRICS_RIGHT) ::SendMessageW(hButton, BM_SETCHECK, static_cast<WPARAM>(BST_CHECKED), 0);

    _hStaticExample = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"static", nullptr,
        WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
        30, height - 185, width - 60, 100, _hSelf, NULL, s_hInstance, nullptr);

    hButton = ::CreateWindowExW(0, L"button", L"字体",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP,
        30, 200, 50, 25, _hSelf, (HMENU)IDS_SETFONT, s_hInstance, nullptr);
    ::SendMessageW(hButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));

    hStaticLabel = ::CreateWindowExW(0, L"static", L"倒计时字符",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        120, 205, 70, 25, _hSelf, NULL, s_hInstance, nullptr);
    ::SendMessageW(hStaticLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));

    _hComboBox = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"combobox", nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | CBS_DROPDOWN,
        195, 200, 45, 30, _hSelf, NULL, s_hInstance, nullptr);
    ::SendMessageW(_hComboBox, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));
    ::SendMessageW(_hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"●"));
    ::SendMessageW(_hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"★"));
    ::SendMessageW(_hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"◆"));
    ::SendMessageW(_hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"■"));
    ::SendMessageW(_hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"▲"));
    //::SendMessageW(_hComboBox, CB_SETCURSEL, (WPARAM)gs_iCountdown, 0);

    hButton = ::CreateWindowExW(0, L"button", L"确定",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | BS_DEFPUSHBUTTON,
        45, height - 55, 75, 25, _hSelf, (HMENU)IDOK, s_hInstance, nullptr);
    ::SendMessageW(hButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));

    hButton = ::CreateWindowExW(0, L"button", L"取消",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS,
        width - 120, height - 55, 75, 25, _hSelf, (HMENU)IDCANCEL, s_hInstance, nullptr);
    ::SendMessageW(hButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(TRUE));
}

static bool chooseColor(HWND hwndOwner, LPCOLORREF lpColor) {
    static COLORREF s_CustColorTable[16] = {
        RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
        RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
        RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
        RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF)
    };

    CHOOSECOLORW cc;
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwndOwner;
    cc.hInstance = NULL;
    cc.lpCustColors = s_CustColorTable;
    cc.rgbResult = *lpColor;
    cc.Flags = CC_RGBINIT | CC_SOLIDCOLOR;

    if (::ChooseColorW(&cc)) {
        *lpColor = cc.rgbResult;
        return true;
    }

    return false;
}

static bool chooseFont(HWND hwndOwner, LPLOGFONTW lpLogFont) {
    CHOOSEFONTW cf;

    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = hwndOwner;
    cf.hDC = NULL;
    cf.lpLogFont = lpLogFont;
    cf.Flags = CF_LIMITSIZE | CF_NOVERTFONTS | CF_INITTOLOGFONTSTRUCT;
    cf.nSizeMin = 16;
    cf.nSizeMax = 72;

    if (::ChooseFontW(&cf)) {
        return true;
    }

    return false;
}

void LyricsSettingDialog::drawSample(HDC hdc, const RECT &rc) {
    ::FillRect(hdc, &rc, reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));

    static const wchar_t text[] = L"歌词效果预览";

    LONG maxWidth = rc.right - rc.left;
    LONG maxHeight = rc.bottom - rc.top;

    BITMAPINFO bi = { 0 };
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = maxWidth;
    bi.bmiHeader.biHeight = maxHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = maxHeight * maxWidth * 4;

    DWORD *pixelData;
    HBITMAP hBitmap = ::CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void **)&pixelData, NULL, 0);

    HFONT hFont = ::CreateFontIndirectW(&_drawParam.logFont);
    HFONT hOldFont = (HFONT)::SelectObject(hdc, hFont);

    std::vector<WORD> indices;
    indices.resize(wcslen(text));
    DWORD ret = ::GetGlyphIndicesW(hdc, text, static_cast<int>(indices.size()), indices.data(), 0);
    if (ret != GDI_ERROR) {
        indices.resize(ret);

        SIZE textSize;
        ::GetTextExtentPointI(hdc, indices.data(), static_cast<int>(ret), &textSize);

        const LONG xPos = (maxWidth - textSize.cx) / 2, yPos = (maxHeight - textSize.cy) / 2;

        TEXTMETRICW tm;
        ::GetTextMetricsW(hdc, &tm);

        std::vector<DWORD> colorPast = DrawSupport::gradientColor(_drawParam.colorPast, tm.tmHeight);
        std::vector<DWORD> colorFuture = DrawSupport::gradientColor(_drawParam.colorFuture, tm.tmHeight);

        GLYPHMETRICS gm;
        const MAT2 mat2 = { {0, 1}, {0, 0}, {0, 0}, {0, 1} };

        std::vector<BYTE> buff;
        LONG offset = 0;
        for (size_t i = 0, cnt = indices.size(); i < cnt; ++i) {
            WORD idx = indices[i];
            memset(&gm, 0, sizeof(gm));

            DWORD buffSize = ::GetGlyphOutlineW(hdc, idx, GGO_GLYPH_INDEX | GGO_GRAY8_BITMAP, &gm, 0, nullptr, &mat2);
            if (buffSize == GDI_ERROR) {
                continue;
            }

            if (buffSize == 0) {
                offset += gm.gmCellIncX;
                continue;
            }

            buff.resize(buffSize);
            if (::GetGlyphOutlineW(hdc, idx, GGO_GLYPH_INDEX | GGO_GRAY8_BITMAP, &gm, buffSize, buff.data(), &mat2) == GDI_ERROR) {
                continue;
            }

            DrawSupport::setPixelDataForChar(pixelData, tm, gm, buff, maxWidth, maxHeight, xPos + offset, yPos, xPos + textSize.cx / 2, colorPast, colorFuture);

            offset += gm.gmCellIncX;
        }

        HDC hdcMem = ::CreateCompatibleDC(hdc);
        HBITMAP hBitmapOld = (HBITMAP)::SelectObject(hdcMem, hBitmap);

        static BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
        ::AlphaBlend(hdc, 0, 0, maxWidth, maxHeight, hdcMem, 0, 0, maxWidth, maxHeight, blend);

        ::SelectObject(hdcMem, hBitmapOld);
        ::DeleteObject(hBitmap);

        ::DeleteDC(hdcMem);
    }

    ::SelectObject(hdc, hOldFont);
    ::DeleteObject(hFont);
}
