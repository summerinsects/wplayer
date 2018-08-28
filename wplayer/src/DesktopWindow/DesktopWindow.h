#ifndef __DESKTOP_WINDOW_H__
#define __DESKTOP_WINDOW_H__

#include "../base/Window.h"
#include <vector>
#include <string>
#include <unordered_map>

struct LyricsWord {
    std::wstring text;
    std::wstring phonetic;
    int start_time = 0;
    int duration = 0;
    bool need_space = false;
};

struct LyricsSentence {
    int start_time = 0;
    int duration = 0;
    std::vector<LyricsWord> words;
    std::wstring translation;

    bool is_top = false;
    bool need_countdown = false;
};

struct LyricsDetail {
    std::unordered_map<std::wstring, std::wstring> tags;
    unsigned total_time = 0;
    int offset = 0;
    std::vector<LyricsSentence> sentences;
};

struct DrawInfo {
    const LyricsSentence *sentence = nullptr;
    int word = 0;
    int progress = 0;  // 0x1000规整
};

struct LyricsWordDetail {
    std::vector<WORD> text_indices;
    std::vector<WORD> phonetic_indices;
    SIZE text_size;
    SIZE phonetic_size;
    LONG text_pos;
    LONG phonetic_pos;
    LONG max_width;
};

#define LYRICS_SEPARATE 0x0
#define LYRICS_LEFT 0x1
#define LYRICS_RIGHT 0x2
#define LYRICS_CENTER 0x3

#define LYRICS_DOUBLE_LINES 0x00
#define LYRICS_SINGLE_LINE 0x10

class DesktopWindow : public Window {
public:
    bool init();

    void toggleLock();
    bool isLock() const;

    bool openMatchedLyrics(LPCWSTR fileName);
    void refreshLyrics(int time);
    void forceRefresh();

private:
    HWND _hWndTool = NULL;

    static LRESULT CALLBACK LyricsWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ToolWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    LRESULT CALLBACK runLyricsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK runToolProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    TRACKMOUSEEVENT _mouseEvent;
    int _trackedFlag = 0;  // -1 客户区 0 无 1 非客客户区

    LyricsDetail _lyrics;

    std::pair<DrawInfo, DrawInfo> _prevDrawInfos;

    LOGFONTW _logFont;
    std::vector<COLORREF> _colorPast;
    std::vector<COLORREF> _colorFuture;
    unsigned _align = 0;
    bool _singleLine = false;
    WCHAR _countdownChar = 0;

    enum class DISPLAY_STATE {
        NONE = 0, EMPTY, LRYICS, TITLE, MADE_BY
    };
    DISPLAY_STATE _displayState = DISPLAY_STATE::NONE;

    std::wstring _title;

    int calculateWordIndex(int time, const LyricsSentence &sentence, int *progress);
    void calculateSentenceIndex1(int time, DrawInfo *info);
    void calculateSentenceIndex2(int time, DrawInfo *info1, DrawInfo *info2);
    void drawSentence(const DrawInfo *info1, const DrawInfo *info2);
    void drawInfo(const std::wstring &text);
    void clearDraw();
};

#endif
