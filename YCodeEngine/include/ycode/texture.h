#ifndef YCODE_TEXTURE_H
#define YCODE_TEXTURE_H

#include <string>

namespace ycode {

/// 2D 贴图：从图片文件（PNG/BMP/JPG/GIF 等）加载位图。
/// Windows 下基于 GDI+ 解码并转为 GDI 位图；其它平台 loadFromFile 返回 false。
class Texture2D {
public:
    Texture2D() = default;
    ~Texture2D();
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    /// 从图片文件加载。成功返回 true，失败（文件不存在/格式不支持）返回 false。
    bool loadFromFile(const std::string& path);

    int width() const { return width_; }
    int height() const { return height_; }
    bool valid() const { return handle_ != nullptr; }

    /// 底层位图句柄（Windows 为 HBITMAP），供 Canvas2D 绘制使用。
    void* nativeHandle() const { return handle_; }

    void destroy();

private:
    void release();
    void* handle_ = nullptr; // Windows: HBITMAP
    int width_ = 0;
    int height_ = 0;
};

} // namespace ycode

#endif // YCODE_TEXTURE_H
