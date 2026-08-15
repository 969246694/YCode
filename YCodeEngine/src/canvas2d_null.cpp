#include "ycode/canvas2d.h"
#include "ycode/texture.h"

namespace ycode {

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

void Canvas2D::fillRect(float, float, float, float, Color)
{
}

void Canvas2D::strokeRect(float, float, float, float, Color, int)
{
}

void Canvas2D::drawImage(const Texture2D&, float, float, float, float)
{
}

void Canvas2D::drawText(const std::string&, float, float, Color, float)
{
}

} // namespace ycode
