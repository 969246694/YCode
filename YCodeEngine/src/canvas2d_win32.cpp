#include "ycode/canvas2d.h"
#include "ycode/texture.h"

#include <algorithm>
#include <windows.h>

namespace ycode {
namespace {

RECT makeRect(float x, float y, float width, float height)
{
    int left = static_cast<int>(x);
    int top = static_cast<int>(y);
    int right = static_cast<int>(x + width);
    int bottom = static_cast<int>(y + height);
    if (right < left)
        std::swap(left, right);
    if (bottom < top)
        std::swap(top, bottom);
    return RECT{left, top, right, bottom};
}

COLORREF toColorRef(Color color)
{
    return RGB(color.r, color.g, color.b);
}

} // namespace

Canvas2D::Canvas2D(void* nativeDc, int width, int height)
    : nativeDc_(nativeDc),
      width_(width),
      height_(height)
{
}

int Canvas2D::width() const
{
    return width_;
}

int Canvas2D::height() const
{
    return height_;
}

void Canvas2D::fillRect(float x, float y, float width, float height, Color color)
{
    HDC dc = static_cast<HDC>(nativeDc_);
    if (!dc)
        return;

    RECT rect = makeRect(x, y, width, height);
    HBRUSH brush = CreateSolidBrush(toColorRef(color));
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

void Canvas2D::strokeRect(float x, float y, float width, float height, Color color, int thickness)
{
    HDC dc = static_cast<HDC>(nativeDc_);
    if (!dc)
        return;

    RECT rect = makeRect(x, y, width, height);
    HPEN pen = CreatePen(PS_SOLID, std::max(1, thickness), toColorRef(color));
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

void Canvas2D::drawImage(const Texture2D& texture, float x, float y, float width, float height)
{
    HDC dc = static_cast<HDC>(nativeDc_);
    HBITMAP hbitmap = static_cast<HBITMAP>(texture.nativeHandle());
    if (!dc || !hbitmap)
        return;

    HDC memDc = CreateCompatibleDC(dc);
    if (!memDc)
        return;
    HGDIOBJ oldBitmap = SelectObject(memDc, hbitmap);

    BITMAP bm;
    if (GetObject(hbitmap, sizeof(bm), &bm) != 0)
    {
        StretchBlt(dc,
                   static_cast<int>(x), static_cast<int>(y),
                   static_cast<int>(width), static_cast<int>(height),
                   memDc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    }

    SelectObject(memDc, oldBitmap);
    DeleteDC(memDc);
}

void Canvas2D::drawText(const std::string& text, float x, float y, Color color, float fontSize)
{
    HDC dc = static_cast<HDC>(nativeDc_);
    if (!dc || text.empty())
        return;

    HFONT font = CreateFontA(-static_cast<int>(fontSize), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH, "Consolas");
    HGDIOBJ oldFont = SelectObject(dc, font);
    int oldMode = SetBkMode(dc, TRANSPARENT);
    COLORREF oldColor = SetTextColor(dc, toColorRef(color));
    TextOutA(dc, static_cast<int>(x), static_cast<int>(y), text.c_str(), static_cast<int>(text.size()));
    SetTextColor(dc, oldColor);
    SetBkMode(dc, oldMode);
    SelectObject(dc, oldFont);
    DeleteObject(font);
}

} // namespace ycode
