#ifndef YCODE_CANVAS2D_H
#define YCODE_CANVAS2D_H

#include <cstdint>

namespace ycode {

struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

class Canvas2D {
public:
    Canvas2D(void* nativeDc, int width, int height);

    int width() const;
    int height() const;

    void fillRect(float x, float y, float width, float height, Color color);
    void strokeRect(float x, float y, float width, float height, Color color, int thickness = 1);

private:
    void* nativeDc_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

} // namespace ycode

#endif // YCODE_CANVAS2D_H
