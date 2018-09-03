#include "MainWindow.h"
#include <commdlg.h>
#include <CommCtrl.h>
#include <shellapi.h>
#include "../base/Common.h"
#include "../LyricsSettingDialog/LyricsSettingDialog.h"

#define APP_NAME L"wplayer"

#define IDB_MUTE 0x2000
#define IDB_PLAY 0x2001
#define IDB_PREV 0x2002
#define IDB_NEXT 0x2003
#define IDB_STOP 0x2004

#define IDM_FILE_LOAD 0x3000
#define IDM_FILE_CLOSE 0x3001
#define IDM_FILE_EXIT 0x3002

#define IDM_SETTING_DESKTOP 0x3100
#define IDM_SETTING_LOCK 0x3101
#define IDM_SETTING_STYLE 0x3102
#define IDM_SETTING_ALL_REPEAT 0x3110
#define IDM_SETTING_ORDER 0x3111
#define IDM_SETTING_REPEAT_ONCE 0x3112
#define IDM_SETTING_ONCE 0x3113
#define IDM_SETTING_SHUFFLE 0x3114

#define IDM_OPERATE_PLAY IDB_PLAY
#define IDM_OPERATE_STOP IDB_STOP
#define IDM_OPERATE_FORWARD 0x3200
#define IDM_OPERATE_BACKWARD 0x3201
#define IDM_OPERATE_PREV IDB_PREV
#define IDM_OPERATE_NEXT IDB_NEXT
#define IDM_OPERATE_AHEAD_100MS 0x3210
#define IDM_OPERATE_AHEAD_200MS 0x3211
#define IDM_OPERATE_AHEAD_500MS 0x3212
#define IDM_OPERATE_AHEAD_1S 0x3213
#define IDM_OPERATE_AHEAD_2S 0x3214
#define IDM_OPERATE_AHEAD_5S 0x3215
#define IDM_OPERATE_DELAY_100MS 0x3220
#define IDM_OPERATE_DELAY_200MS 0x3221
#define IDM_OPERATE_DELAY_500MS 0x3222
#define IDM_OPERATE_DELAY_1S 0x3223
#define IDM_OPERATE_DELAY_2S 0x3224
#define IDM_OPERATE_DELAY_5S 0x3225
#define IDM_OPERATE_DELETE 0x3202

#define IDM_TOOL_EDITLYRICS 0x3300
#define IDM_TOOL_EDITID3 0x3301
#define IDM_TOOL_DIRECTORY 0x3302

#define IDM_HELP_ABOUT 0x3400
#define IDM_HELP_MANUAL 0x3401

#define IDM_NOTIFY_RECOVERY 0x3500

#define IDT_PLAYER_TIMER 0x4000

#define PROGRESS_SCALE 1000
#define FRAME_DELAY 30

int MainWindow::run(HINSTANCE hInstance, int iCmdShow) {
    s_hInstance = hInstance;

    WNDCLASSW wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = L"WPlayerMainWindow";

    if (!::RegisterClassW(&wc) && ::GetLastError() != ERROR_ALREADY_REGISTERED) {
        return false;
    }

    RECT screenRect;
    ::SystemParametersInfoW(SPI_GETWORKAREA, 0, &screenRect, 0);
    int xScreen = screenRect.right - screenRect.left;
    int yScreen = screenRect.bottom - screenRect.top;

    int width = 300, height = 400;

    HWND hwnd = ::CreateWindowExW(0, wc.lpszClassName, APP_NAME,
        WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
        (xScreen - width) / 2, (yScreen - height) / 2, width, height,
        NULL, NULL, hInstance, this);

    ::ShowWindow(hwnd, iCmdShow);
    ::UpdateWindow(hwnd);

    MSG msg;
    while (::GetMessageW(&msg, NULL, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CREATE) {
        MainWindow *thiz = reinterpret_cast<MainWindow *>(((LPCREATESTRUCTW)lParam)->lpCreateParams);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(thiz));

        thiz->_hSelf = hwnd;
        thiz->initMenu();
        thiz->initWidgets();
        return 0;
    }
    MainWindow *thiz = reinterpret_cast<MainWindow *>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return thiz->runProc(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = ::BeginPaint(hwnd, &ps);

        RECT rect;
        ::GetClientRect(hwnd, &rect);
        ::FillRect(hdc, &rect, reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));
        ::EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND:
        onCommand(wParam);
        return 0;

    case WM_INITMENUPOPUP:
        onInitMenuPopup(reinterpret_cast<HMENU>(wParam));
        return 0;

    case WM_NOTIFY: {
        LONG_PTR userData = ::GetWindowLongPtrW(reinterpret_cast<LPNMHDR>(lParam)->hwndFrom, GWLP_USERDATA);
        if (userData != 0) {
            PlayListView *listView = reinterpret_cast<PlayListView *>(userData);
            listView->onNotify(wParam, lParam);
            return 0;
        }
        break;
    }

    case WM_HSCROLL: {
        LONG_PTR userData = ::GetWindowLongPtrW(reinterpret_cast<HWND>(lParam), GWLP_USERDATA);
        if (userData != 0) {
            TrackBar *trackBar = reinterpret_cast<TrackBar *>(userData);
            if (reinterpret_cast<HWND>(lParam) == trackBar->getHWnd()) {
                trackBar->onHScroll(wParam, lParam);
                return 0;
            }
        }
        break;
    }

    case WM_TIMER:
        onTimer();
        return 0;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

void MainWindow::initWidgets() {
    RECT rect;
    ::GetClientRect(_hSelf, &rect);
    LONG width = rect.right - rect.left;
    LONG height = rect.bottom - rect.top;

    _listView.init(_hSelf, POINT{ 10, 120 }, SIZE{ width - 20, height - 130 });

    _progressTrack.init(_hSelf, POINT{ 10, 30 }, SIZE{ width - 20, 20 });
    _volumeTrack.init(_hSelf, POINT{ 10, 80 }, SIZE{ 110, 20 });
    ::SendMessageW(_volumeTrack.getHWnd(), TBM_SETRANGE, static_cast<WPARAM>(FALSE), MAKELPARAM(0, 100));
    ::SendMessageW(_volumeTrack.getHWnd(), TBM_SETPOS, static_cast<WPARAM>(TRUE), static_cast<LPARAM>(50));

    _progressTrack.setTrackingListener([this](TrackBar *, LONG_PTR pos) {
        refreshCurrentTime(pos * PROGRESS_SCALE);
    });
    _progressTrack.setPosChangedListener([this](TrackBar *, LONG_PTR pos) {
        _player.seekTo(_hSelf, static_cast<DWORD>(pos * PROGRESS_SCALE), _playing);
        _desktopWindow.forceRefresh(true);
    });
    _volumeTrack.setTrackingListener([this](TrackBar *, LONG_PTR pos) {
        _volume = static_cast<int>(pos) * 10;
        if (!_muting) {
            _player.setVolume(_hSelf, _volume);
        }
    });
    _volumeTrack.setPosChangedListener([this](TrackBar *, LONG_PTR pos) {
        _volume = static_cast<int>(pos) * 10;
        if (!_muting) {
            _player.setVolume(_hSelf, _volume);
        }
    });

    _hPlayButton = Common::createButton(_hSelf, POINT{ (width - 40) >> 1, 60 }, SIZE{ 40, 40 }, L">", reinterpret_cast<HMENU>(IDB_PLAY));
    _hPrevButton = Common::createButton(_hSelf, POINT{ width - 115, 65 }, SIZE{ 30, 30 }, L"|<<", reinterpret_cast<HMENU>(IDB_PREV));
    _hNextButton = Common::createButton(_hSelf, POINT{ width - 80, 65 }, SIZE{ 30, 30 }, L">>|", reinterpret_cast<HMENU>(IDB_NEXT));
    _hStopButton = Common::createButton(_hSelf, POINT{ width - 45, 65 }, SIZE{ 30, 30 }, L"■", reinterpret_cast<HMENU>(IDB_STOP));
    _hMuteButton = Common::createCheckBox(_hSelf, POINT{ 20, 60 }, SIZE{ 50, 20 }, L"静音", reinterpret_cast<HMENU>(IDB_MUTE));

    _hCurrentTime = Common::createStatic(_hSelf, POINT{ 15, 10 }, SIZE{ 70, 20 }, L"00:00.00");
    _hTotalTime = Common::createStatic(_hSelf, POINT{ width - 85, 10 }, SIZE{ 70, 20 }, L"00:00.00");
    ::SetWindowLongPtrW(_hTotalTime, GWL_STYLE, ::GetWindowLongPtrW(_hTotalTime , GWL_STYLE) | SS_RIGHT);

    HFONT hFont = reinterpret_cast<HFONT>(::SendMessageW(_listView.getHWnd(), WM_GETFONT, 0, 0));
    ::SendMessageW(_hPlayButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(FALSE));
    ::SendMessageW(_hPrevButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(FALSE));
    ::SendMessageW(_hNextButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(FALSE));
    ::SendMessageW(_hStopButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(FALSE));
    ::SendMessageW(_hMuteButton, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(FALSE));
    ::SendMessageW(_hCurrentTime, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(FALSE));
    ::SendMessageW(_hTotalTime, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), static_cast<LPARAM>(FALSE));

    _desktopWindow.init();
}

void MainWindow::initMenu() {
    HMENU hFileMenu = ::CreatePopupMenu();
    HMENU hSettingMenu = ::CreatePopupMenu();
    HMENU hOperateMenu = ::CreatePopupMenu();
    HMENU hToolMenu = ::CreatePopupMenu();
    HMENU hHelpMenu = ::CreatePopupMenu();

    HMENU hPlayModeMenu = ::CreatePopupMenu();
    HMENU hLyricsMenu = ::CreatePopupMenu();
    HMENU hAheadMenu = ::CreatePopupMenu();
    HMENU hDelayMenu = ::CreatePopupMenu();

    HMENU hPlayerMenu = ::CreateMenu();

    ::AppendMenuW(hPlayerMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFileMenu), L"文件(&F)");
    ::AppendMenuW(hPlayerMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSettingMenu), L"设置(&S)");
    ::AppendMenuW(hPlayerMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hOperateMenu), L"操作(&O)");
    ::AppendMenuW(hPlayerMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hToolMenu), L"工具(&T)");
    ::AppendMenuW(hPlayerMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hHelpMenu), L"帮助(&H)");

    ::AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_LOAD, L"载入(&L)...");
    ::AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_CLOSE, L"关闭(&C)");
    ::AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"退出(&X)");

    ::AppendMenuW(hSettingMenu, MF_STRING, IDM_SETTING_DESKTOP, L"桌面歌词(&D)");
    ::AppendMenuW(hSettingMenu, MF_STRING, IDM_SETTING_LOCK, L"锁定歌词(&L)");
    ::AppendMenuW(hSettingMenu, MF_STRING, IDM_SETTING_STYLE, L"歌词样式(&S)...");
    ::AppendMenuW(hSettingMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(hSettingMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hPlayModeMenu), L"播放模式(&M)");

    ::AppendMenuW(hPlayModeMenu, MF_STRING, IDM_SETTING_ALL_REPEAT, L"列表循环");
    ::AppendMenuW(hPlayModeMenu, MF_STRING, IDM_SETTING_ORDER, L"顺序播放");
    ::AppendMenuW(hPlayModeMenu, MF_STRING, IDM_SETTING_REPEAT_ONCE, L"单曲循环");
    ::AppendMenuW(hPlayModeMenu, MF_STRING, IDM_SETTING_ONCE, L"单曲播放");
    ::AppendMenuW(hPlayModeMenu, MF_STRING, IDM_SETTING_SHUFFLE, L"随机播放");

    ::AppendMenuW(hOperateMenu, MF_STRING, IDM_OPERATE_PLAY, L"播放/暂停(&P)");
    ::AppendMenuW(hOperateMenu, MF_STRING, IDM_OPERATE_STOP, L"停止(&T)");
    ::AppendMenuW(hOperateMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(hOperateMenu, MF_STRING, IDM_OPERATE_FORWARD, L"前进(&F)");
    ::AppendMenuW(hOperateMenu, MF_STRING, IDM_OPERATE_BACKWARD, L"后退(&B)");
    ::AppendMenuW(hOperateMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(hOperateMenu, MF_STRING, IDM_OPERATE_PREV, L"上一曲(&V)");
    ::AppendMenuW(hOperateMenu, MF_STRING, IDM_OPERATE_NEXT, L"下一曲(&N)");
    ::AppendMenuW(hOperateMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(hOperateMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hAheadMenu), L"歌词提前(&A)");
    ::AppendMenuW(hOperateMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hDelayMenu), L"歌词延后(&D)");

    ::AppendMenuW(hAheadMenu, MF_STRING, IDM_OPERATE_AHEAD_100MS, L"提前0.1秒");
    ::AppendMenuW(hAheadMenu, MF_STRING, IDM_OPERATE_AHEAD_200MS, L"提前0.2秒");
    ::AppendMenuW(hAheadMenu, MF_STRING, IDM_OPERATE_AHEAD_500MS, L"提前0.5秒");
    ::AppendMenuW(hAheadMenu, MF_STRING, IDM_OPERATE_AHEAD_1S, L"提前1秒");
    ::AppendMenuW(hAheadMenu, MF_STRING, IDM_OPERATE_AHEAD_2S, L"提前2秒");
    ::AppendMenuW(hAheadMenu, MF_STRING, IDM_OPERATE_AHEAD_5S, L"提前5秒");

    ::AppendMenuW(hDelayMenu, MF_STRING, IDM_OPERATE_DELAY_100MS, L"延后0.1秒");
    ::AppendMenuW(hDelayMenu, MF_STRING, IDM_OPERATE_DELAY_200MS, L"延后0.2秒");
    ::AppendMenuW(hDelayMenu, MF_STRING, IDM_OPERATE_DELAY_500MS, L"延后0.5秒");
    ::AppendMenuW(hDelayMenu, MF_STRING, IDM_OPERATE_DELAY_1S, L"延后1秒");
    ::AppendMenuW(hDelayMenu, MF_STRING, IDM_OPERATE_DELAY_2S, L"延后2秒");
    ::AppendMenuW(hDelayMenu, MF_STRING, IDM_OPERATE_DELAY_5S, L"延后5秒");

    ::AppendMenuW(hToolMenu, MF_STRING, IDM_TOOL_DIRECTORY, L"转到目录(&D)...");
    ::AppendMenuW(hToolMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(hToolMenu, MF_STRING, IDM_TOOL_EDITID3, L"编辑ID3v1(&E)...");
    ::AppendMenuW(hToolMenu, MF_STRING, IDM_TOOL_EDITLYRICS, L"制作歌词(&M)...");

    ::AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"关于(&A)...");
    ::AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_MANUAL, L"说明(&M)...");

    ::SetMenu(_hSelf, hPlayerMenu);

    _hSettingMenu = hSettingMenu;
    _hPlayModeMenu = hPlayModeMenu;
    _hNotifyMenu = NULL;
}

void MainWindow::onInitMenuPopup(HMENU hMenu) {
    if (hMenu == _hSettingMenu || hMenu == _hNotifyMenu) {
        ::CheckMenuItem(hMenu, IDM_SETTING_DESKTOP, ::IsWindowVisible(_desktopWindow.getHWnd()) ? MF_CHECKED : MF_UNCHECKED);
        ::CheckMenuItem(hMenu, IDM_SETTING_LOCK, _desktopWindow.isLock() ? MF_CHECKED : MF_UNCHECKED);
    }
    else if (hMenu == _hPlayModeMenu) {
        ::CheckMenuRadioItem(hMenu, IDM_SETTING_ALL_REPEAT, IDM_SETTING_SHUFFLE,
            IDM_SETTING_ALL_REPEAT + static_cast<int>(_playMode), MF_BYCOMMAND);
    }
}

void MainWindow::onCommand(WPARAM wParam) {
    switch (LOWORD(wParam)) {
    case IDM_FILE_LOAD:
        loadFile();
        break;

    case IDM_FILE_CLOSE:
        ::ShowWindow(_hSelf, SW_HIDE);
        break;

    case IDM_FILE_EXIT:
        ::DestroyWindow(_hSelf);
        break;

    case IDM_SETTING_DESKTOP:
        ::ShowWindow(_desktopWindow.getHWnd(), ::IsWindowVisible(_desktopWindow.getHWnd()) ? SW_HIDE : SW_SHOW);
        break;

    case IDM_SETTING_ALL_REPEAT:
        _playMode = PLAY_MODE::ALL_REPEAT;
        break;

    case IDM_SETTING_ORDER:
        _playMode = PLAY_MODE::ORDER;
        break;

    case IDM_SETTING_REPEAT_ONCE:
        _playMode = PLAY_MODE::REPEAT_ONCE;
        break;

    case IDM_SETTING_ONCE:
        _playMode = PLAY_MODE::ONCE;
        break;

    case IDM_SETTING_SHUFFLE:
        _playMode = PLAY_MODE::SHUFFLE;
        break;

    case IDM_SETTING_LOCK:
        _desktopWindow.toggleLock();
        break;

    case IDM_SETTING_STYLE: {
        LyricsSettingDialog lsd;
        DrawSupport::DrawParam param = _desktopWindow.getDrawParam();
        if (lsd.show(_hSelf, &param)) {
            _desktopWindow.setDrawParam(std::move(param));
        }
        break;
    }
    case IDM_OPERATE_FORWARD:
        break;

    case IDM_OPERATE_BACKWARD:
        break;

    case IDM_OPERATE_AHEAD_100MS:
        _desktopWindow.setLyricsOffset(100);
        break;

    case IDM_OPERATE_AHEAD_200MS:
        _desktopWindow.setLyricsOffset(200);
        break;

    case IDM_OPERATE_AHEAD_500MS:
        _desktopWindow.setLyricsOffset(500);
        break;

    case IDM_OPERATE_AHEAD_1S:
        _desktopWindow.setLyricsOffset(1000);
        break;

    case IDM_OPERATE_AHEAD_2S:
        _desktopWindow.setLyricsOffset(2000);
        break;

    case IDM_OPERATE_AHEAD_5S:
        _desktopWindow.setLyricsOffset(5000);
        break;

    case IDM_OPERATE_DELAY_100MS:
        _desktopWindow.setLyricsOffset(-100);
        break;

    case IDM_OPERATE_DELAY_200MS:
        _desktopWindow.setLyricsOffset(-200);
        break;

    case IDM_OPERATE_DELAY_500MS:
        _desktopWindow.setLyricsOffset(-500);
        break;

    case IDM_OPERATE_DELAY_1S:
        _desktopWindow.setLyricsOffset(-1000);
        break;

    case IDM_OPERATE_DELAY_2S:
        _desktopWindow.setLyricsOffset(-2000);
        break;

    case IDM_OPERATE_DELAY_5S:
        _desktopWindow.setLyricsOffset(-5000);
        break;

    case IDM_OPERATE_DELETE:
        break;

    case IDM_TOOL_EDITLYRICS:
        break;

    case IDM_TOOL_EDITID3:
        break;

    case IDM_TOOL_DIRECTORY: {
        LPCWSTR file = _listView.getCurrentFile();
        if (file != nullptr) {
            std::wstring cmd = L"/e,/select, ";
            cmd.append(file);
            ::ShellExecuteW(_hSelf, L"open", L"explorer", cmd.c_str(), nullptr, SW_SHOW);
        }
        break;
    }

    case IDM_HELP_ABOUT:
        ::MessageBoxW(_hSelf, L"By: summer_insects", L"关于", MB_ICONASTERISK);
        break;

    case IDM_HELP_MANUAL:
        ::MessageBoxW(_hSelf,
            L"1.歌词文件的路径应与歌曲文件相同，且文件名也必须相同\n\n"
            L"2.歌词提前/延后可调整0.1、0.2、0.5、1、2、5秒，如需更精确的调整可打开歌词后编辑\n\n",
            L"说明", MB_OK);
        break;

    case IDM_NOTIFY_RECOVERY:
        ::SetForegroundWindow(_hSelf);
        ::ShowWindow(_hSelf, SW_SHOWNORMAL);
        break;

    case IDB_MUTE:
        toggleMute();
        break;

    case IDB_PLAY:
        onPlay();
        break;

    case IDB_PREV:
        playSpecifiedFile(_listView.getPrevFile(_playMode));
        break;

    case IDB_NEXT:
        playSpecifiedFile(_listView.getNextFile(_playMode, true));
        break;

    case IDB_STOP:
        stop();
        break;

    default:
        break;
    }
}

bool MainWindow::loadFile() {
    WCHAR allFileName[512 * _MAX_PATH] = L"";
    WCHAR fileName[_MAX_PATH], *p;
    OPENFILENAMEW ofn = {0};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = _hSelf;
    ofn.hInstance = s_hInstance;
    ofn.lpstrFilter = L"音频文件(*.mp3)\0*.mp3\0\0";
    ofn.lpstrFile = allFileName;
    ofn.nMaxFile = 512 * _MAX_PATH - 1;
    ofn.lpstrTitle = L"载入";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY
        | OFN_LONGNAMES | OFN_EXPLORER | OFN_ALLOWMULTISELECT;

    if (!::GetOpenFileNameW(&ofn)) {
        return false;
    }

    if (allFileName[ofn.nFileOffset - 1] == L'\\') {
        return _listView.insertListItem(allFileName);
    }

    for (p = allFileName + ofn.nFileOffset; *p != L'\0'; ) {
        wcsncpy(fileName, allFileName, ofn.nFileOffset);
        wcscat(fileName, L"\\");
        wcscat(fileName, p);
        p += wcslen(p) + 1;

        if (!_listView.insertListItem(fileName)) {
            return false;
        }
    }

    return true;
}

bool MainWindow::onPlay() {
    if (_playing) {
        return pause();
    }
    else {
        if (_opened) {
            return resume();
        }
        else {
            return playSpecifiedFile(_listView.getCurrentFile());
        }
    }
}

bool MainWindow::pause() {
    if (! _player.pause(_hSelf)) {
        return false;
    }

    ::SendMessageW(_hPlayButton, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L">"));
    _playing = false;
    ::KillTimer(_hSelf, IDT_PLAYER_TIMER);
    return true;
}

bool MainWindow::resume() {
    _playing = false;
    if (_muting) {
        if (!_player.setVolume(_hSelf, 0)) {
            return false;
        }
    }
    else {
        _volume = static_cast<int>(::SendMessageW(_volumeTrack.getHWnd(), TBM_GETPOS, 0, 0)) * 10;
        if (!_player.setVolume(_hSelf, static_cast<DWORD>(_volume))) {
            return false;
        }
    }

    if (_player.play(_hSelf)) {
        ::SendMessageW(_hPlayButton, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"||"));
        ::SetTimer(_hSelf, IDT_PLAYER_TIMER, FRAME_DELAY, nullptr);

        _playing = true;
        return true;
    }

    return false;
}

bool MainWindow::stop() {
    if (!_opened) {
        _playing = false;
        return true;
    }

    if (_player.closeFile(_hSelf)) {
        _opened = false;
        _playing = false;
        refreshPlayerControls(0);
        ::SendMessageW(_hPlayButton, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L">"));
        ::KillTimer(_hSelf, IDT_PLAYER_TIMER);
        return true;
    }

    return false;
}

bool MainWindow::playSpecifiedFile(LPCWSTR fileName) {
    do {
        if (_opened && !_player.closeFile(_hSelf)) {
            break;
        }
        _opened = false;
        if (fileName == nullptr) {
            break;
        }

        if (!_player.openFile(_hSelf, fileName)) {
            break;
        }
        //TODO: setNotifyIconText

        _opened = true;
        if (!resume()) {
            break;
        }

        _audioLength = _player.getLength(_hSelf);
        setupProgress();
        _desktopWindow.openMatchedLyrics(fileName);

        return true;

    } while (0);

    ::KillTimer(_hSelf, IDT_PLAYER_TIMER);
    ::SendMessageW(_hSelf, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(APP_NAME));
    //TODO: setNotifyIconText
    stop();

    return false;
}

void MainWindow::refreshCurrentTime(LONG_PTR pos) {
    WCHAR textBuf[32];
    ldiv_t ret1, ret2;

    ret1 = ldiv(static_cast<long>(pos), 60000L);
    ret2 = ldiv(ret1.rem, 1000L);

    _snwprintf(textBuf, 32, L"%.2ld:%.2ld.%.2ld", ret1.quot, ret2.quot, ret2.rem / 10L);
    ::SendMessageW(_hCurrentTime, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(textBuf));
}

void MainWindow::refreshPlayerControls(DWORD_PTR curTime) {
    if (_progressTrack.isTracking()) {
        return;
    }

    refreshCurrentTime(curTime);
    ::SendMessageW(_progressTrack.getHWnd(), TBM_SETPOS, static_cast<WPARAM>(TRUE), static_cast<LPARAM>(curTime / PROGRESS_SCALE));  // 设置进度位置
}

void MainWindow::setupProgress() {
    WCHAR textBuf[32];
    ldiv_t ret1, ret2;

    ret1 = ldiv((long)_audioLength, 60000);
    ret2 = ldiv(ret1.rem, 1000);

    _snwprintf(textBuf, 32, L"%.2ld:%.2ld.%.2ld", ret1.quot, ret2.quot, ret2.rem / 10);

    ::SendMessageW(_hTotalTime, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(textBuf));  // 总时间
    ::SendMessageW(_hCurrentTime, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L"00:00.00"));  // 当前时间

    ::SendMessageW(_progressTrack.getHWnd(), TBM_SETRANGEMAX, static_cast<WPARAM>(TRUE), static_cast<LPARAM>(_audioLength / PROGRESS_SCALE));  // 进度
}

void MainWindow::toggleMute() {
    if (_muting) {
        _volume = static_cast<int>(::SendMessageW(_volumeTrack.getHWnd(), TBM_GETPOS, 0, 0)) * 10;
        if (_player.setVolume(_hSelf, static_cast<DWORD>(_volume))) {
            ::SendMessageW(_hMuteButton, BM_SETCHECK, static_cast<WPARAM>(FALSE), 0);
            _muting = false;
        }
    }
    else {
        if (_player.setVolume(_hSelf, 0)) {
            ::SendMessageW(_hMuteButton, BM_SETCHECK, static_cast<WPARAM>(TRUE), 0);
            _muting = TRUE;
        }
    }
}

bool MainWindow::onTimer() {
    DWORD_PTR curPos = _player.getPos(_hSelf);
    if (curPos == static_cast<DWORD_PTR>(-1)) {
        return false;
    }

    if (_audioLength <= curPos) {  // 已播放完
        if (_playMode != PLAY_MODE::REPEAT_ONCE) {
            stop();
            if (_playMode != PLAY_MODE::ONCE) {
                playSpecifiedFile(_listView.getNextFile(_playMode, false));
            }
        }
        else {
            _player.seekTo(_hSelf, 0, true);
        }
        return false;
    }

    if (::IsWindowVisible(_hSelf)) {
        refreshPlayerControls(curPos);
    }

    _desktopWindow.refreshLyrics((int)curPos);

    return true;
}
