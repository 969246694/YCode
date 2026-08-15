#include "ycode/texture.h"

#include <memory>
#include <windows.h>
#include <wingdi.h> // META_* 常量（WIN32_LEAN_AND_MEAN 下 windows.h 不包含 wingdi.h）
#include <objidl.h> // IStream（GDI+ 头文件依赖）
#include <gdiplus.h>

namespace ycode {
namespace {

bool ensureGdiplus()
{
    static bool ready = []() {
        Gdiplus::GdiplusStartupInput input;
        ULONG_PTR token = 0;
        return Gdiplus::GdiplusStartup(&token, &input, nullptr) == Gdiplus::Ok;
    }();
    return ready;
}

} // namespace

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

bool Texture2D::loadFromFile(const std::string& path)
{
    release();

    if (!ensureGdiplus())
        return false;

    std::wstring widePath;
    {
        int size = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
        if (size <= 0)
            return false;
        widePath.resize(static_cast<size_t>(size - 1));
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &widePath[0], size);
    }

    std::unique_ptr<Gdiplus::Bitmap> bitmap(Gdiplus::Bitmap::FromFile(widePath.c_str()));
    if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok)
        return false;

    int w = bitmap->GetWidth();
    int h = bitmap->GetHeight();
    if (w <= 0 || h <= 0)
        return false;

    HBITMAP hbitmap = nullptr;
    if (bitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hbitmap) != Gdiplus::Ok || !hbitmap)
        return false;

    handle_ = hbitmap;
    width_ = w;
    height_ = h;
    return true;
}

void Texture2D::destroy()
{
    release();
}

void Texture2D::release()
{
    if (handle_)
    {
        DeleteObject(static_cast<HBITMAP>(handle_));
        handle_ = nullptr;
    }
    width_ = 0;
    height_ = 0;
}

} // namespace ycode
