#include "DrawSupport.h"
#include <algorithm>

// 颜色叠加算法
static BYTE inline blendb(BYTE a1, BYTE a2, BYTE c1, BYTE c2) {
    return static_cast<uint8_t>((255U * a1 * c1 - a1 * c1 * a2 + 255U * a2 * c2) / (255U * a1 + 255U * a2 - a1 * a2));
}

static DWORD ColorBlendb(DWORD color1, DWORD color2) {
    const BYTE a1 = (color1 >> 24 & 0xFFU);
    const BYTE a2 = (color2 >> 24 & 0xFFU);
    const BYTE alphaBlend = a1 + a2 - a1 * a2 / 255;
    const BYTE redBlend = blendb(a1, a2, (color1 & 0xFFU), (color2 & 0xFFU));
    const BYTE greenBlend = blendb(a1, a2, (BYTE)((color1 & 0xFF00U) >> 8U), (BYTE)((color2 & 0xFF00U) >> 8U));
    const BYTE blueBlend = blendb(a1, a2, (BYTE)((color1 & 0xFF0000U) >> 16U), (BYTE)((color2 & 0xFF0000U) >> 16U));

    return (alphaBlend) << 24U | (blueBlend) << 16U | (greenBlend) << 8U | redBlend;
}

namespace DrawSupport {

    // 画一个字符
    void setPixelDataForChar(DWORD *pixelData, const TEXTMETRICW &tm, const GLYPHMETRICS &gm, const std::vector<BYTE> &buff,
        LONG maxWidth, LONG maxHeight, LONG xPos, LONG yPos, LONG demarcation,
        const std::vector<DWORD> &colorPast, const std::vector<DWORD> &colorFuture) {
        const size_t rowSize = buff.size() / gm.gmBlackBoxY;

        for (UINT y = 0; y < gm.gmBlackBoxY; ++y) {
            const LONG chy = y + (tm.tmAscent - gm.gmptGlyphOrigin.y);
            const LONG yy = maxHeight - (yPos + chy);
            if (yy < 0) {
                break;
            }
            if (yy >= maxHeight) {
                continue;
            }

            for (UINT x = 0; x < gm.gmBlackBoxX; ++x) {
                const BYTE gray = buff[rowSize * y + x];
                if (gray == 0) {
                    continue;
                }

                const LONG xx = xPos + x + gm.gmptGlyphOrigin.x;
                if (xx < 0) {
                    continue;
                }
                if (xx >= maxWidth) {
                    break;
                }

                DWORD bgColor = pixelData[yy * maxWidth + xx];
                DWORD color = (gray == 0x40) ? 0xFF000000U : (gray * 4) << 24;

                if (yy > 1 && xx + 1 < maxWidth) {
                    pixelData[(yy - 1) * maxWidth + xx + 1] = color;
                }

                bgColor = ColorBlendb(bgColor, color);
                color = (xx < demarcation) ? colorPast[chy] : colorFuture[chy];
                if (gray == 0x40) {
                    color |= 0xFF000000U;
                }
                else {
                    color |= (gray * 4) << 24;
                }

                pixelData[yy * maxWidth + xx] = ColorBlendb(bgColor, color);
            }
        }
    }

    std::vector<DWORD> gradientColor(const std::vector<COLORREF> &colors, LONG height) {
        size_t count = colors.size();
        if (count == 0) {
            return std::vector<DWORD>(height, 0);
        }

        std::vector<DWORD> ret(height, colors[0]);

        if (count > 1) {
            LONG split = height / (static_cast<LONG>(count) - 1);
            COLORREF color0 = colors[0];
            int r0 = GetRValue(color0);
            int g0 = GetGValue(color0);
            int b0 = GetBValue(color0);

            for (size_t i = 1; i < count; ++i) {
                COLORREF color1 = colors[i];
                int r1 = GetRValue(color1);
                int g1 = GetGValue(color1);
                int b1 = GetBValue(color1);

                int ro = r1 - r0;
                int go = g1 - g0;
                int bo = b1 - b0;

                for (LONG k = 0; k < split; ++k) {
                    ret[(i - 1) * split + k] = RGB(r0 + ro * k / split, g0 + go * k / split, b0 + bo * k / split);
                }

                r0 = r1;
                g0 = g1;
                b0 = b1;
            }
        }

        std::transform(ret.begin(), ret.end(), ret.begin(), convertRGBtoBGR);

        return ret;
    }
}
