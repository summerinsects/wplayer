#ifndef __DRAW_SUPPORT_H__
#define __DRAW_SUPPORT_H__

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>

#define LYRICS_SEPARATE 0x0
#define LYRICS_LEFT 0x1
#define LYRICS_RIGHT 0x2
#define LYRICS_CENTER 0x3

#define LYRICS_DOUBLE_LINES 0x00
#define LYRICS_SINGLE_LINE 0x10

namespace DrawSupport {
    struct DrawParam {
        LOGFONTW logFont;
        std::vector<COLORREF> colorPast;
        std::vector<COLORREF> colorFuture;
        unsigned align = 0;
        bool singleLine = false;
    };

    __forceinline DWORD convertRGBtoBGR(COLORREF c) {
        return ((c & 0xFF) << 16) | (c & 0xFF00) | ((c & 0xFF0000) >> 16);
    }
    std::vector<DWORD> gradientColor(const std::vector<COLORREF> &colors, LONG height);

    void setPixelDataForChar(DWORD *pixelData, const TEXTMETRICW &tm, const GLYPHMETRICS &gm, const std::vector<BYTE> &buff,
        LONG maxWidth, LONG maxHeight, LONG xPos, LONG yPos, LONG demarcation,
        const std::vector<DWORD> &colorPast, const std::vector<DWORD> &colorFuture);
}

#endif
