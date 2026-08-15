#include "ycode/texture.h"

namespace ycode {

Texture2D::~Texture2D()
{
    release();
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : handle_(other.handle_), width_(other.width_), height_(other.height_)
{
    other.handle_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
{
    if (this != &other)
    {
        release();
        handle_ = other.handle_;
        width_ = other.width_;
        height_ = other.height_;
        other.handle_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

bool Texture2D::loadFromFile(const std::string&)
{
    return false; // 非 Windows 平台暂不支持
}

void Texture2D::destroy()
{
    release();
}

void Texture2D::release()
{
    handle_ = nullptr;
    width_ = 0;
    height_ = 0;
}

} // namespace ycode
