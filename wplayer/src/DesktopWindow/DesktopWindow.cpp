#include "DesktopWindow.h"
#include "../../../lyrics/src/lyrics.h"
#include <algorithm>

#undef max
#undef min

#ifdef _WIN64

#ifdef _DEBUG
#pragma comment(lib, "../x64/Debug/lyrics.lib")
#else
#pragma comment(lib, "../x64/Release/lyrics.lib")
#endif

#else

#ifdef _DEBUG
#pragma comment(lib, "../Debug/lyrics.lib")
#else
#pragma comment(lib, "../Release/lyrics.lib")
#endif

#endif

bool DesktopWindow::init() {
    WNDCLASSW wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ::DefWindowProcW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = s_hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_SIZEALL);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = L"DesktopLyricsWindow";

    if (!::RegisterClassW(&wc) && ::GetLastError() != ERROR_ALREADY_REGISTERED) {
        return false;
    }

    RECT screenRect;
    int x, y, width = 900, height = 150;
    ::SystemParametersInfoW(SPI_GETWORKAREA, 0, &screenRect, 0);
    x = (screenRect.right - screenRect.left - width) >> 1;
    y = screenRect.bottom - screenRect.top - height;

    _hSelf = ::CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
        wc.lpszClassName, nullptr,
        WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS,
        x, y, width, height, NULL, NULL, s_hInstance, nullptr);
    ::SetWindowLongPtrW(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ::SetWindowLongPtrW(_hSelf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&DesktopWindow::LyricsWndProc));

    _hWndTool = ::CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName, nullptr,
        WS_POPUP | WS_CLIPSIBLINGS | WS_THICKFRAME,
        x, y, width, height, NULL, NULL, s_hInstance, nullptr);
    ::SetWindowLongPtrW(_hWndTool, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ::SetWindowLongPtrW(_hWndTool, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&DesktopWindow::ToolWndProc));
    ::SetClassLongPtrW(_hWndTool, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(::GetStockObject(BLACK_BRUSH)));

    ::SetLayeredWindowAttributes(_hWndTool, 0, 40, LWA_ALPHA);

    _mouseEvent.cbSize = sizeof(_mouseEvent);
    _mouseEvent.dwFlags = 0;
    _mouseEvent.hwndTrack = NULL;
    _mouseEvent.dwHoverTime = HOVER_DEFAULT;

    ::ShowWindow(_hSelf, SW_SHOW);
    ::ShowWindow(_hWndTool, SW_SHOW);

    _logFont.lfHeight = 64;
    _logFont.lfWidth = 0;
    _logFont.lfEscapement = 0;
    _logFont.lfOrientation = 0;
    _logFont.lfWeight = FW_BOLD;
    _logFont.lfItalic = FALSE;
    _logFont.lfUnderline = FALSE;
    _logFont.lfStrikeOut = FALSE;
    _logFont.lfCharSet = DEFAULT_CHARSET;
    _logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    _logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    _logFont.lfQuality = DEFAULT_QUALITY;
    _logFont.lfPitchAndFamily = FF_MODERN;
    wcsncpy(_logFont.lfFaceName, L"微软雅黑", LF_FACESIZE);

    _colorPast.push_back(RGB(0x00, 0x80, 0xF0));
    _colorFuture.push_back(RGB(0xF0, 0xF0, 0xF0));

    _countdownChar = L'●';

    return (_hSelf != NULL && _hWndTool != NULL);
}

LRESULT CALLBACK DesktopWindow::LyricsWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    DesktopWindow *thiz = reinterpret_cast<DesktopWindow *>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return thiz->runLyricsProc(hwnd, message, wParam, lParam);
}

LRESULT DesktopWindow::runLyricsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_MOUSEMOVE:
        ::ShowWindow(_hWndTool, SW_SHOWNORMAL);
        return 0;

    case WM_CLOSE:
        ::ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        return 0;

    default:
        break;
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK DesktopWindow::ToolWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    DesktopWindow *thiz = reinterpret_cast<DesktopWindow *>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return thiz->runToolProc(hwnd, message, wParam, lParam);
}

LRESULT DesktopWindow::runToolProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    //case WM_NCHITTEST: {
    //    ::ShowWindow(hwnd, SW_SHOWNORMAL);
    //    LRESULT ret = DefWindowProc(hwnd, message, wParam, lParam);
    //    return (ret == HTCAPTION) ? HTBORDER : ret;
    //    return ret;
    //}

    //case WM_NCCALCSIZE: {
    //    LPNCCALCSIZE_PARAMS lpNCSizeParams = (LPNCCALCSIZE_PARAMS)lParam;
    //    if (wParam) {
    //        lpNCSizeParams->rgrc[2] = lpNCSizeParams->rgrc[1];
    //        lpNCSizeParams->rgrc[1] = lpNCSizeParams->rgrc[0];
    //    }
    //    return TRUE;
    //}

    case WM_LBUTTONDOWN:
        return ::DefWindowProcW(hwnd, WM_SYSCOMMAND, static_cast<WPARAM>(SC_MOVE | HTCAPTION), 0);

    //case WM_NCACTIVATE:
    //    if (!wParam)
    //        ::ShowWindow(hwnd, SW_SHOWNORMAL);
    //    return ::DefWindowProcW(hwnd, message, wParam, lParam);
    
    //case WM_SYSCOMMAND: {
    //    LRESULT ret = ::DefWindowProcW(hwnd, message, wParam, lParam);
    //    ::ShowWindow(hwnd, SW_SHOWNORMAL);  // 避免点到边框上闪烁
    //    return ret;
    //}

    case WM_NCMOUSEMOVE:
        if (_trackedFlag == -1) {  // 取消监测客户区
            //printf("cancel track c\n");
            _mouseEvent.hwndTrack = hwnd;
            _mouseEvent.dwFlags = TME_LEAVE | TME_CANCEL;
            ::TrackMouseEvent(&_mouseEvent);
            _trackedFlag = 0;
        }
        if (_trackedFlag == 0) {  // 监测非客户区
            //printf("start track c\n");
            _mouseEvent.hwndTrack = hwnd;
            _mouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
            ::TrackMouseEvent(&_mouseEvent);
            ::SystemParametersInfoW(SPI_SETDRAGFULLWINDOWS, TRUE, nullptr, 0);
            _trackedFlag = 1;
        }
        return 0;

    case WM_MOUSEMOVE:
        if (_trackedFlag == 1) {  // 取消监测非客户区
            //printf("cancel track nc\n");
            _mouseEvent.hwndTrack = hwnd;
            _mouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT | TME_CANCEL;
            ::TrackMouseEvent(&_mouseEvent);
            _trackedFlag = 0;
        }
        if (_trackedFlag == 0) {  // 监测客户区
            //printf("start track c\n");
            _mouseEvent.hwndTrack = hwnd;
            _mouseEvent.dwFlags = TME_LEAVE;
            ::TrackMouseEvent(&_mouseEvent);
            ::SystemParametersInfoW(SPI_SETDRAGFULLWINDOWS, TRUE, nullptr, 0);
            _trackedFlag = -1;
        }
        return 0;

    case WM_MOUSELEAVE:  // 离开客户区
        // 监测非客户区
        //printf("start track nc\n");
        _mouseEvent.hwndTrack = hwnd;
        _mouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
        ::TrackMouseEvent(&_mouseEvent);
        ::SystemParametersInfoW(SPI_SETDRAGFULLWINDOWS, TRUE, nullptr, 0);
        _trackedFlag = 1;
        return 0;

    case WM_NCMOUSELEAVE:  // 离开非客户区
        //printf("end track nc\n");
        ::ShowWindow(hwnd, SW_HIDE);
        ::SystemParametersInfoW(SPI_SETDRAGFULLWINDOWS, FALSE, nullptr, 0);
        _trackedFlag = 0;
        return 0;

    case WM_MOVING:  // 移动
    case WM_SIZING: {  // 缩放
        ::ShowWindow(hwnd, SW_SHOWNORMAL);

        // 使歌词窗口随之移动/缩放
        LPCRECT pRect = reinterpret_cast<LPCRECT>(lParam);
        ::MoveWindow(_hSelf, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top, FALSE);
    }
    return TRUE;

    case WM_KILLFOCUS:
        ::ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_GETMINMAXINFO: {
        // 限制最小尺寸
        LPMINMAXINFO pMinMaxInfo = reinterpret_cast<LPMINMAXINFO>(lParam);

        ::ShowWindow(hwnd, SW_SHOWNORMAL);

        pMinMaxInfo->ptMinTrackSize.x = 420;
        pMinMaxInfo->ptMinTrackSize.y = 65;
        pMinMaxInfo->ptMaxTrackSize.y = 290;
    }
    return 0;

    case WM_CLOSE:
        ::ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        return 0;

    default:
        break;
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

void DesktopWindow::toggleLock() {
    LONG_PTR style = ::GetWindowLongPtrW(_hSelf, GWL_EXSTYLE);
    if (style & WS_EX_TRANSPARENT) {
        ::SetWindowLongPtrW(_hSelf, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
    }
    else {
        ::SetWindowLongPtrW(_hSelf, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
    }
}

bool DesktopWindow::isLock() const {
    return (::GetWindowLongPtrW(_hSelf, GWL_EXSTYLE) & WS_EX_TRANSPARENT);
}

static std::wstring UTF8ToWChar(const std::string &utf8) {
    std::wstring ret;
    int len = ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (len > 0) {
        ret.resize(len);
        ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &ret[0], len);
        ret.resize(wcslen(ret.c_str()));
    }
    return ret;
}

bool DesktopWindow::openMatchedLyrics(LPCWSTR fileName) {
    _lyrics.sentences.clear();
    _lyrics.tags.clear();
    _lyrics.total_time = 0;
    _lyrics.offset = 0;

    LPCWSTR dot = wcsrchr(fileName, L'.');
    if (dot == nullptr) {
        return false;
    }

    WCHAR name[MAX_PATH] = L"";
    wcsncpy(name, fileName, dot - fileName);
    wcscat(name, L".krc");

    FILE *fp = _wfopen(name, L"rb");
    if (fp == nullptr) {
        return false;
    }
    fseek(fp, SEEK_SET, SEEK_END);
    long size = ftell(fp);
    std::vector<uint8_t> fileData(size);

    fseek(fp, SEEK_SET, SEEK_SET);
    fread(fileData.data(), 1, size, fp);

    fclose(fp);

    lyrics_detail_t lyrics;
    if (!lyrics_decode(fileData, &lyrics)) {
        return false;
    }

    // 转换
    _lyrics.total_time = lyrics.total_time;
    _lyrics.offset = lyrics.offset;

    for (const auto &kv : lyrics.tags) {
        _lyrics.tags.insert(std::make_pair(UTF8ToWChar(kv.first), UTF8ToWChar(kv.second)));
    }

    _lyrics.sentences.reserve(lyrics.sentences.size());
    for (size_t i = 0, cnt = lyrics.sentences.size(); i < cnt; ++i) {
        const lyrics_sentence_t &sentence = lyrics.sentences[i];

        _lyrics.sentences.emplace_back();
        LyricsSentence &sentenceW = _lyrics.sentences.back();

        sentenceW.duration = sentence.duration;
        sentenceW.start_time = sentence.start_time;
        sentenceW.words.reserve(sentence.words.size());

        for (size_t k = 0, cnt1 = sentence.words.size(); k < cnt1; ++k) {
            const lyrics_word_t &word = sentence.words[k];

            sentenceW.words.emplace_back();
            LyricsWord &wordW = sentenceW.words.back();

            wordW.start_time = word.start_time;
            wordW.duration = word.duration;
            wordW.text = UTF8ToWChar(word.text);
            wordW.phonetic = UTF8ToWChar(word.phonetic);

            // 将带空格结尾的转换为一个need_space标记
            if (!wordW.text.empty() && wordW.text.back() == L' ') {
                wordW.text.pop_back();
                wordW.need_space = true;
            }

            wordW.text.reserve();
            wordW.phonetic.reserve();
        }

        // 分配上下行及倒计时
        const LyricsSentence *prev = (i > 0) ? &_lyrics.sentences[i - 1] : nullptr;
        if (prev == nullptr || sentenceW.start_time > prev->start_time + prev->duration + 5000) {
            sentenceW.need_countdown = true;
            sentenceW.is_top = false;
        }
        else {
            sentenceW.need_countdown = false;
            sentenceW.is_top = !prev->is_top;
        }
        prev = &sentenceW;
    }

    _title.clear();
    LPCWSTR p = wcsrchr(fileName, L'\\');
    if (p != nullptr) {
        _title.assign(p + 1, dot);
    }

    forceRefresh(true);
    _timeOffset = 0;

    return true;
}

void DesktopWindow::forceRefresh(bool clean) {
    // 清除上次绘制标记
    _prevDrawInfos.first.sentence = nullptr;
    _prevDrawInfos.second.sentence = nullptr;
    _displayState = DISPLAY_STATE::NONE;
    if (clean) {
        clearDraw();
    }
}

void DesktopWindow::refreshLyrics(int time) {
    if (!::IsWindowVisible(_hSelf)) {
        return;
    }

    if (_lyrics.sentences.empty()) {
        if (_displayState != DISPLAY_STATE::EMPTY) {
            clearDraw();
            _displayState = DISPLAY_STATE::EMPTY;
        }
        return;
    }

    time += _timeOffset + _lyrics.offset;

    if ((_align & LYRICS_SINGLE_LINE) == 0) {
        DrawInfo info1, info2;
        calculateSentenceIndex2(time, &info1, &info2);
        if (info1.sentence != nullptr || info2.sentence != nullptr) {
            _displayState = DISPLAY_STATE::LRYICS;
            // 重复判断
            if (memcmp(&_prevDrawInfos.first, &info1, sizeof(info1)) == 0 && memcmp(&_prevDrawInfos.second, &info2, sizeof(info2)) == 0) {
                return;
            }
            drawSentence(&info1, &info2);
            return;
        }
    }
    else {
        DrawInfo info;
        calculateSentenceIndex1(time, &info);
        if (info.sentence != nullptr) {
            _displayState = DISPLAY_STATE::LRYICS;
            // 重复判断
            if (memcmp(&_prevDrawInfos.first, &info, sizeof(info)) == 0) {
                return;
            }
            drawSentence(&info, nullptr);
            return;
        }
    }

    if (_displayState == DISPLAY_STATE::TITLE) {
        return;
    }

    _displayState = DISPLAY_STATE::TITLE;
    drawInfo(_title);

}

int DesktopWindow::calculateWordIndex(int time, const LyricsSentence &sentence, int *progress) {
    int deltaTime = time - sentence.start_time;  // 当前时间与这一句开始相差时间
    if (deltaTime < 0) {  // 这一句还没开始
        *progress = 0x1000;
        return (deltaTime / 1000 - 1);  // 倒计时秒数
    }

    for (size_t i = 0, cnt = sentence.words.size(); i < cnt; ++i) {  // 遍历各个字
        const LyricsWord &word = sentence.words.at(i);
        if (deltaTime >= word.start_time) {  // 至少已经开始这个字
            if (deltaTime < word.start_time + word.duration) {  // 这个字还没结束
                *progress = (deltaTime - word.start_time) * 0x1000 / word.duration;  // 计算这个字显示比例
                return static_cast<int>(i);  // 定位到这个字
            }
        }
        else {  // 还没开始这个字
            *progress = 0x1000;
            return static_cast<int>(i - 1);  // 定位到前一个字
        }
    }

    // 当前一句已结束
    *progress = 0x1000;
    return static_cast<int>(sentence.words.size() - 1);
}

void DesktopWindow::calculateSentenceIndex1(int time, DrawInfo *info) {
    const std::vector<LyricsSentence> &sentences = _lyrics.sentences;
    size_t i;
    size_t cnt = sentences.size();
    for (i = 0;  i < cnt; ++i) {
        const LyricsSentence &sentence = sentences[i];
        if (time + 8000 < sentence.start_time) {  // 这一句尚未开始（提前8秒）
            break;
        }

        if (time > sentence.start_time + sentence.duration) {  // 这一句已结束
            continue;
        }

        // 这一句尚未结束
        info->sentence = &sentence;  // 保存当前一句
        info->word = calculateWordIndex(time, sentence, &info->progress);
        return;
    }

    // 这一句尚未开始或者全部结束
    info->sentence = nullptr;
    if (i > 0) {  // 有上一句
        const LyricsSentence &prev = sentences[i - 1];
        if (time < prev.start_time + prev.duration + 3000) {  // 上一句推迟3秒
            info->sentence = &prev;  // 保存上一句
            info->word = calculateWordIndex(time, *info->sentence, &info->progress);
        }
    }
}

void DesktopWindow::calculateSentenceIndex2(int time, DrawInfo *info1, DrawInfo *info2) {
    const std::vector<LyricsSentence> &sentences = _lyrics.sentences;
    size_t i;
    size_t cnt = sentences.size();
    for (i = 0;  i < cnt; ++i) {
        const LyricsSentence &sentence = sentences[i];
        if (time + 8000 < sentence.start_time) {  // 这一句尚未开始（提前8秒）
            break;
        }

        if (time > sentence.start_time + sentence.duration) {  // 这一句已结束
            continue;
        }

        // 这一句尚未结束
        info1->sentence = &sentence;  // 保存当前一句
        info1->word = calculateWordIndex(time, sentence, &info1->progress);

        if (i + 1 < cnt) {  // 有下一句
            const LyricsSentence &next = sentences[i + 1];
            if (time >= next.start_time) {  // 下一句已开始（不提前）
                info2->sentence = &next;  // 保存下一句
            }
            else {  // 下一句尚未开始
                if (time > sentence.start_time + sentence.duration / 2 || info1->word >= (int)(sentence.words.size() >> 1)) {  // 这一句时间或者字数已过半
                    if (next.start_time < sentence.start_time + sentence.duration + 5000) {  // 下一句开始与这一句结束相差小于5秒
                        info2->sentence = &next;  // 保存下一句
                    }
                    else {  // 下一句开始与这一句结束相差大于5秒
                        info2->sentence = nullptr;
                    }
                }
                else {  // 这一句时间和字数未过半
                    if (i > 0) {  // 有上一句
                        const LyricsSentence &prev = sentences[i - 1];
                        if (sentence.start_time < prev.start_time + prev.duration + 5000) {  // 上一句结束与这一句开始相差小于5秒
                            info2->sentence = &prev;  // 保存上一句
                        }
                        else {  // 上一句结束与这一句开始相差大于5秒
                            info2->sentence = nullptr;
                        }
                    }
                    else {  // 没有上一句
                        info2->sentence = nullptr;
                    }
                }
            }
        }
        else {  // 没有下一句
            if (i > 0) {
                const LyricsSentence &prev = sentences[i - 1];
                if (!sentence.need_countdown && time < prev.start_time + prev.duration + 3000) {  // 上一句推迟3秒
                    info2->sentence = &prev;  // 保存上一句
                }
                else {  // 上一句结束超过3秒
                    info2->sentence = nullptr;
                }
            }
            else {  // 没有上一句
                info2->sentence = nullptr;
            }
        }

        if (info2->sentence != nullptr) {
            info2->word = calculateWordIndex(time, *info2->sentence, &info2->progress);
        }
        return;
    }

    // 这一句尚未开始或者全部结束
    info1->sentence = nullptr;
    if (i > 0) {  // 有上一句
        const LyricsSentence &prev = sentences[i - 1];
        if (time < prev.start_time + prev.duration + 3000) {  // 上一句推迟3秒
            info2->sentence = &prev;  // 保存上一句
            info2->word = calculateWordIndex(time, *info2->sentence, &info2->progress);
        }
        else {  // 上一句结束超过3秒
            info2->sentence = nullptr;
        }
    }
    else {
        info2->sentence = nullptr;
    }

    if (info2->sentence != nullptr && info1->sentence == nullptr) {  // 因为绘制的时候，1不检测空
        std::swap(*info1, *info2);
    }
}

// 计算这一句需要的长度
static std::pair<LONG, LONG> calculateSentenceWidth(const DrawInfo &info, HDC hdcText, HDC hdcPhonetic, const TEXTMETRICW &tm1, const TEXTMETRICW &tm2, std::vector<LyricsWordDetail> *details) {
    details->reserve(info.sentence->words.size());
    std::vector<WORD> textIndices, phoneticIndices;

    LONG totalWidth = 0, demarcation = 0;
    LONG prevPhoneticRight = -tm2.tmAveCharWidth;
    for (size_t i = 0, cnt = info.sentence->words.size(); i < cnt; ++i) {
        const LyricsWord &word = info.sentence->words.at(i);
        const std::wstring &text = word.text;
        const std::wstring &phonetic = word.phonetic;

        // 文本索引
        textIndices.resize(text.length());
        if (!::GetGlyphIndicesW(hdcText, (LPCWSTR)text.c_str(), (int)textIndices.size(), textIndices.data(), 0)) {
            continue;
        }

        // 文本的尺寸
        SIZE textSize = { 0, 0 };
        if (!::GetTextExtentPointI(hdcText, textIndices.data(), (int)textIndices.size(), &textSize)) {
            continue;
        }

        // 音译的索引和尺寸
        phoneticIndices.resize(phonetic.length());
        SIZE phoneticSize = { 0, 0 };
        (void)(::GetGlyphIndicesW(hdcPhonetic, (LPCWSTR)phonetic.c_str(), (int)phoneticIndices.size(), phoneticIndices.data(), 0)
            && ::GetTextExtentPointI(hdcPhonetic, phoneticIndices.data(), (int)phoneticIndices.size(), &phoneticSize));

        // 这个词画出来的最大宽度
        const LONG wordMaxWidth = std::max(textSize.cx, phoneticSize.cx);

        // 非ascii字符不加空格
        if (!phonetic.empty() && phonetic[0] < 0x7F) {
            LONG delta = (totalWidth + (wordMaxWidth - phoneticSize.cx) / 2) - prevPhoneticRight - tm2.tmAveCharWidth / 2;
            if (delta < 0) {
                totalWidth -= delta;
            }
        }

        // 文本和音译各自的起始x坐标
        const LONG xPosWord = totalWidth + (wordMaxWidth - textSize.cx) / 2;
        const LONG xPosPhonetics = totalWidth + (wordMaxWidth - phoneticSize.cx) / 2;

        // 进度
        if (static_cast<int>(i) == info.word) {
            demarcation = totalWidth + info.progress * wordMaxWidth / 0x1000;
        }

        // 保存
        details->emplace_back();
        LyricsWordDetail &lwd = details->back();
        lwd.text_indices.swap(textIndices);
        lwd.phonetic_indices.swap(phoneticIndices);
        lwd.text_size = textSize;
        lwd.phonetic_size = phoneticSize;
        lwd.text_pos = xPosWord;
        lwd.phonetic_pos = xPosPhonetics;
        lwd.max_width = wordMaxWidth;

        prevPhoneticRight = totalWidth + phoneticSize.cx;
        totalWidth += wordMaxWidth;
        if (word.need_space) {
            totalWidth += tm1.tmAveCharWidth;
        }
    }

    return std::make_pair(totalWidth, demarcation);
}

// 画一句
static RECT setPixelDataForSentence(DWORD *pixelData, LONG maxWidth, LONG maxHeight, unsigned align,
    const DrawInfo &info, HDC hdcText, HDC hdcPhonetic, const TEXTMETRICW &tm1, const TEXTMETRICW &tm2,
    const std::vector<DWORD> &colorPast1, const std::vector<DWORD> &colorFuture1,
    const std::vector<DWORD> &colorPast2, const std::vector<DWORD> &colorFuture2) {
    std::vector<LyricsWordDetail> details;
    const std::pair<LONG, LONG> ret = calculateSentenceWidth(info, hdcText, hdcPhonetic, tm1, tm2, &details);
    const LONG totalWidth = ret.first;
    const LONG demarcation = ret.second;

    LONG yPosText, yPosPhonetic;
    if ((align & LYRICS_SINGLE_LINE) || !info.sentence->is_top) {
        yPosText = maxHeight - tm1.tmHeight;
        yPosPhonetic = maxHeight - tm1.tmHeight - tm2.tmHeight;
    }
    else {
        yPosText = tm2.tmHeight;
        yPosPhonetic = 0;
    }

    LONG xPos;
    if ((align & LYRICS_CENTER) == LYRICS_CENTER) {
        xPos = (maxWidth - totalWidth) / 2;
    }
    else if ((align & LYRICS_LEFT) == LYRICS_LEFT) {
        xPos = 0;
    }
    else if ((align & LYRICS_RIGHT) == LYRICS_RIGHT) {
        xPos = maxWidth - totalWidth;
    }
    else {
        if (align & LYRICS_SINGLE_LINE) {
            xPos = (maxWidth - totalWidth) / 2;
        }
        else {
            if (info.sentence->is_top || info.sentence->need_countdown) {
                xPos = 0;
            }
            else {
                xPos = maxWidth - totalWidth;
            }
        }
    }

    // 太长
    if (totalWidth > maxWidth) {
        xPos = std::max(xPos, 0L);

        const LONG remainWidth = maxWidth / 4;
        if (demarcation > maxWidth - remainWidth) {
            xPos = std::max(maxWidth - totalWidth, xPos - (demarcation - maxWidth + remainWidth));
        }
    }

    std::vector<BYTE> buff;

    GLYPHMETRICS gm;
    const MAT2 mat2 = { {0, 1}, {0, 0}, {0, 0}, {0, 1} };

    for (size_t i = 0, detailCnt = details.size(); i < detailCnt; ++i) {
        const LyricsWordDetail &lwd = details[i];
        LONG offset = xPos;
        for (size_t k = 0, indiexCnt = lwd.text_indices.size(); k < indiexCnt; ++k) {
            WORD idx = lwd.text_indices.at(k);
            memset(&gm, 0, sizeof(gm));
            DWORD buffSize = ::GetGlyphOutlineW(hdcText, idx, GGO_GLYPH_INDEX | GGO_GRAY8_BITMAP, &gm, 0, nullptr, &mat2);
            if (buffSize == GDI_ERROR) {
                continue;
            }

            if (buffSize == 0) {
                offset += gm.gmCellIncX;
                continue;
            }

            buff.resize(buffSize);
            if (::GetGlyphOutlineW(hdcText, idx, GGO_GLYPH_INDEX | GGO_GRAY8_BITMAP, &gm, buffSize, buff.data(), &mat2) == GDI_ERROR) {
                continue;
            }

            DrawSupport::setPixelDataForChar(pixelData, tm1, gm, buff, maxWidth, maxHeight, lwd.text_pos + offset, yPosText, demarcation + xPos, colorPast1, colorFuture1);

            offset += gm.gmCellIncX;
        }

        offset = xPos;
        for (size_t k = 0, cnt = lwd.phonetic_indices.size(); k < cnt; ++k) {
            WORD idx = lwd.phonetic_indices.at(k);
            memset(&gm, 0, sizeof(gm));
            DWORD buffSize = ::GetGlyphOutlineW(hdcPhonetic, idx, GGO_GLYPH_INDEX | GGO_GRAY8_BITMAP, &gm, 0, nullptr, &mat2);
            if (buffSize == GDI_ERROR) {
                continue;
            }

            if (buffSize == 0) {
                offset += gm.gmCellIncX;
                continue;
            }

            buff.resize(buffSize);
            if (::GetGlyphOutlineW(hdcPhonetic, idx, GGO_GLYPH_INDEX | GGO_GRAY8_BITMAP, &gm, buffSize, buff.data(), &mat2) == GDI_ERROR) {
                continue;
            }

            DrawSupport::setPixelDataForChar(pixelData, tm2, gm, buff, maxWidth, maxHeight, lwd.phonetic_pos + offset, yPosPhonetic, demarcation + xPos, colorPast2, colorFuture2);

            offset += gm.gmCellIncX;
        }
    }

    return RECT{ xPos, yPosPhonetic, xPos + totalWidth, yPosPhonetic + tm1.tmHeight + tm2.tmHeight };
}

static void setPixelDataForCountingdown(DWORD *pixelData, LONG maxWidth, LONG maxHeight, LONG yPos, HDC hdc, WCHAR ch, int cnt, const TEXTMETRICW &tm1, const std::vector<DWORD> &color) {
    std::vector<BYTE> buff;

    GLYPHMETRICS gm;
    const MAT2 mat2 = { {0, 1}, {0, 0}, {0, 0}, {0, 1} };

    DWORD buffSize = ::GetGlyphOutlineW(hdc, ch, GGO_GRAY8_BITMAP, &gm, 0, nullptr, &mat2);
    if (buffSize == GDI_ERROR) {
        return;
    }

    if (buffSize == 0) {
        return;
    }

    buff.resize(buffSize);
    if (::GetGlyphOutlineW(hdc, ch, GGO_GRAY8_BITMAP, &gm, buffSize, buff.data(), &mat2) == GDI_ERROR) {
        return;
    }

    unsigned int const rowSize = buffSize / gm.gmBlackBoxY;
    LONG totalWidth = 0;
    for (int i = 0; i < cnt; ++i) {
        DrawSupport::setPixelDataForChar(pixelData, tm1, gm, buff, maxWidth, maxHeight, totalWidth, yPos, INT_MAX, color, color);
        totalWidth += gm.gmCellIncX;
    }
}

void DesktopWindow::drawSentence(const DrawInfo *info1, const DrawInfo *info2) {
    RECT clientRect;
    ::GetClientRect(_hSelf, &clientRect);

    const LONG maxWidth = clientRect.right - clientRect.left;
    const LONG maxHeight = clientRect.bottom - clientRect.top;

    HDC hdc = ::GetDC(_hSelf);
    HDC hdcMem = ::CreateCompatibleDC(hdc);

    LOGFONTW lf;
    memcpy(&lf, &_logFont, sizeof(lf));

    HFONT hFont1 = ::CreateFontIndirectW(&lf);
    HFONT hOldFont1 = reinterpret_cast<HFONT>(::SelectObject(hdc, hFont1));

    TEXTMETRICW tm1;
    ::GetTextMetricsW(hdc, &tm1);

    lf.lfHeight /= 2;
    HFONT hFont2 = ::CreateFontIndirectW(&lf);
    HFONT hOldFont2 = reinterpret_cast<HFONT>(::SelectObject(hdcMem, hFont2));

    TEXTMETRICW tm2;
    ::GetTextMetricsW(hdcMem, &tm2);

    BITMAPINFO bi = { 0 };
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = maxWidth;
    bi.bmiHeader.biHeight = maxHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = maxWidth * maxHeight * 4;

    DWORD *pixelData;
    HBITMAP hBitmap = ::CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void **)&pixelData, NULL, 0);
    HBITMAP hOldBitmap = reinterpret_cast<HBITMAP>(::SelectObject(hdcMem, hBitmap));

    std::vector<DWORD> colorPast1 = DrawSupport::gradientColor(_colorPast, tm1.tmHeight);
    std::vector<DWORD> colorFuture1 = DrawSupport::gradientColor(_colorFuture, tm1.tmHeight);
    std::vector<DWORD> colorPast2 = DrawSupport::gradientColor(_colorPast, tm2.tmHeight);
    std::vector<DWORD> colorFuture2 = DrawSupport::gradientColor(_colorFuture, tm2.tmHeight);

    static const BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    const SIZE clientSize = { maxWidth, maxHeight };
    const POINT pointSrc = { 0, 0 };
    RECT dirtyRect = { 0, 0, 0, 0 };
    UPDATELAYEREDWINDOWINFO ulwInfo;
    ulwInfo.cbSize = sizeof(ulwInfo);
    ulwInfo.hdcDst = hdc;
    ulwInfo.pptDst = nullptr;
    ulwInfo.psize = &clientSize;
    ulwInfo.hdcSrc = hdcMem;
    ulwInfo.pptSrc = &pointSrc;
    ulwInfo.crKey = 0;
    ulwInfo.pblend = &blend;
    ulwInfo.prcDirty = &dirtyRect;
    ulwInfo.dwFlags = ULW_ALPHA;

    // 相同就不画了
    bool countdown = false;
    if (memcmp(&_prevDrawInfos.first, info1, sizeof(*info1)) != 0 && memcmp(&_prevDrawInfos.second, info1, sizeof(*info1)) != 0) {
        RECT ret = setPixelDataForSentence(pixelData, maxWidth, maxHeight, _align, *info1, hdc, hdcMem, tm1, tm2, colorPast1, colorFuture1, colorPast2, colorFuture2);
        memcpy(&_prevDrawInfos.first, info1, sizeof(*info1));

        dirtyRect.left = 0;
        dirtyRect.top = ret.top;
        dirtyRect.right = clientSize.cx;
        dirtyRect.bottom = ret.top + tm1.tmHeight + tm2.tmHeight;
        ::UpdateLayeredWindowIndirect(_hSelf, &ulwInfo);

        if (!(_align & LYRICS_SINGLE_LINE) && info1->sentence->need_countdown && info1->word <= 0) {
            int cnt = std::min(-info1->word, 5);
            countdown = true;
            setPixelDataForCountingdown(pixelData, maxWidth, maxHeight, ret.left, hdc, _countdownChar, cnt, tm1, colorPast1);

            dirtyRect.left = 0;
            dirtyRect.top = 0;
            dirtyRect.right = clientSize.cx;
            dirtyRect.bottom = tm1.tmHeight + tm2.tmHeight;
            ::UpdateLayeredWindowIndirect(_hSelf, &ulwInfo);
        }
    }

    // 相同就不画了
    if (!countdown && info2 != nullptr
        && memcmp(&_prevDrawInfos.first, info2, sizeof(*info2)) != 0 && memcmp(&_prevDrawInfos.second, info2, sizeof(*info2)) != 0) {
        RECT ret = { 0 };
        if (info2->sentence != nullptr) {
            ret = setPixelDataForSentence(pixelData, maxWidth, maxHeight, _align, *info2, hdc, hdcMem, tm1, tm2, colorPast1, colorFuture1, colorPast2, colorFuture2);
        }
        else {
            if ((_align & LYRICS_SINGLE_LINE) || info1->sentence->is_top) {
                ret.top = maxHeight - tm1.tmHeight - tm2.tmHeight;
            }
            else {
                ret.top = 0;
            }
        }
        memcpy(&_prevDrawInfos.second, info2, sizeof(*info2));

        dirtyRect.left = 0;
        dirtyRect.top = ret.top;
        dirtyRect.right = clientSize.cx;
        dirtyRect.bottom = ret.top + tm1.tmHeight + tm2.tmHeight;
        ::UpdateLayeredWindowIndirect(_hSelf, &ulwInfo);
    }

    ::SelectObject(hdcMem, hOldBitmap);
    ::DeleteObject(hBitmap);

    ::SelectObject(hdcMem, hOldFont2);
    ::DeleteObject(hFont2);

    ::DeleteDC(hdcMem);

    ::SelectObject(hdc, hOldFont1);
    ::DeleteObject(hFont1);

    ::ReleaseDC(_hSelf, hdc);
}

void DesktopWindow::drawInfo(const std::wstring &text) {
    RECT clientRect;
    ::GetClientRect(_hSelf, &clientRect);

    const LONG maxWidth = clientRect.right - clientRect.left;
    const LONG maxHeight = clientRect.bottom - clientRect.top;

    HDC hdc = ::GetDC(_hSelf);
    HDC hdcMem = ::CreateCompatibleDC(hdc);

    HFONT hFont1 = ::CreateFontIndirectW(&_logFont);
    HFONT hOldFont1 = reinterpret_cast<HFONT>(::SelectObject(hdc, hFont1));

    TEXTMETRICW tm1;
    ::GetTextMetricsW(hdc, &tm1);

    BITMAPINFO bi = { 0 };
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = maxWidth;
    bi.bmiHeader.biHeight = maxHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = maxWidth * maxHeight * 4;

    DWORD *pixelData;
    HBITMAP hBitmap = ::CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void **)&pixelData, NULL, 0);
    HBITMAP hOldBitmap = reinterpret_cast<HBITMAP>(::SelectObject(hdcMem, hBitmap));

    do {
        if (text.empty()) {
            break;
        }

        // 文本索引
        std::vector<WORD> textIndices;
        textIndices.resize(text.length());
        if (!::GetGlyphIndicesW(hdc, (LPCWSTR)text.c_str(), (int)textIndices.size(), textIndices.data(), 0)) {
            break;
        }

        // 文本的尺寸
        SIZE textSize = { 0, 0 };
        if (!::GetTextExtentPointI(hdc, textIndices.data(), (int)textIndices.size(), &textSize)) {
            break;
        }

        std::vector<DWORD> colorPast1 = DrawSupport::gradientColor(_colorPast, tm1.tmHeight);

        GLYPHMETRICS gm;
        const MAT2 mat2 = { {0, 1}, {0, 0}, {0, 0}, {0, 1} };

        std::vector<BYTE> buff;

        const LONG yPos = (_align & LYRICS_SINGLE_LINE) == LYRICS_SINGLE_LINE ? maxHeight - tm1.tmHeight : 0;
        LONG xPos = (maxWidth - textSize.cx) / 2;
        if (textSize.cx > maxWidth) {
            xPos = std::max(xPos, 0L);

            // TODO: 太长滚动
            //const LONG remainWidth = maxWidth / 4;
            //if (demarcation > maxWidth - remainWidth) {
            //    xPos = std::max(maxWidth - totalWidth, xPos - (demarcation - maxWidth + remainWidth));
            //}
        }

        LONG offset = 0;
        for (size_t i = 0, cnt = textIndices.size(); i < cnt; ++i) {
            WORD idx = textIndices[i];
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

            DrawSupport::setPixelDataForChar(pixelData, tm1, gm, buff, maxWidth, maxHeight, xPos + offset, yPos, INT_MAX, colorPast1, colorPast1);

            offset += gm.gmCellIncX;
        }
    } while (0);

    static const BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    const SIZE clientSize = { maxWidth, maxHeight };
    const POINT pointSrc = { 0, 0 };
    UPDATELAYEREDWINDOWINFO ulwInfo;
    ulwInfo.cbSize = sizeof(ulwInfo);
    ulwInfo.hdcDst = hdc;
    ulwInfo.pptDst = nullptr;
    ulwInfo.psize = &clientSize;
    ulwInfo.hdcSrc = hdcMem;
    ulwInfo.pptSrc = &pointSrc;
    ulwInfo.crKey = 0;
    ulwInfo.pblend = &blend;
    ulwInfo.dwFlags = ULW_ALPHA;

    RECT dirtyRect = { 0, 0, clientSize.cx, clientSize.cy };
    ulwInfo.prcDirty = &dirtyRect;
    ::UpdateLayeredWindowIndirect(_hSelf, &ulwInfo);

    ::SelectObject(hdcMem, hOldBitmap);
    ::DeleteObject(hBitmap);

    ::DeleteDC(hdcMem);

    ::SelectObject(hdc, hOldFont1);
    ::DeleteObject(hFont1);

    ::ReleaseDC(_hSelf, hdc);
}

void DesktopWindow::clearDraw() {
    RECT clientRect;
    ::GetClientRect(_hSelf, &clientRect);

    const LONG maxWidth = clientRect.right - clientRect.left;
    const LONG maxHeight = clientRect.bottom - clientRect.top;

    HDC hdc = ::GetDC(_hSelf);
    HDC hdcMem = ::CreateCompatibleDC(hdc);

    BITMAPINFO bi = { 0 };
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = maxWidth;
    bi.bmiHeader.biHeight = maxHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = maxWidth * maxHeight * 4;

    DWORD *pixelData;
    HBITMAP hBitmap = ::CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void **)&pixelData, NULL, 0);
    HBITMAP hOldBitmap = reinterpret_cast<HBITMAP>(::SelectObject(hdcMem, hBitmap));

    static const BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    const SIZE clientSize = { maxWidth, maxHeight };
    const POINT pointSrc = { 0, 0 };
    UPDATELAYEREDWINDOWINFO ulwInfo;
    ulwInfo.cbSize = sizeof(ulwInfo);
    ulwInfo.hdcDst = hdc;
    ulwInfo.pptDst = nullptr;
    ulwInfo.psize = &clientSize;
    ulwInfo.hdcSrc = hdcMem;
    ulwInfo.pptSrc = &pointSrc;
    ulwInfo.crKey = 0;
    ulwInfo.pblend = &blend;
    ulwInfo.dwFlags = ULW_ALPHA;

    RECT dirtyRect = { 0, 0, clientSize.cx, clientSize.cy };
    ulwInfo.prcDirty = &dirtyRect;
    ::UpdateLayeredWindowIndirect(_hSelf, &ulwInfo);

    ::SelectObject(hdcMem, hOldBitmap);
    ::DeleteObject(hBitmap);

    ::DeleteDC(hdcMem);

    ::ReleaseDC(_hSelf, hdc);
}
