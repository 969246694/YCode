#include "ycode/canvas2d.h"

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

} // namespace ycode
