#ifndef YCODE_WINDOW_H
#define YCODE_WINDOW_H

#include "ycode/input.h"

#include <functional>
#include <memory>
#include <string>

namespace ycode {

struct WindowConfig {
    std::string title = "YCode Game";
    int width = 1280;
    int height = 720;
    bool visible = true;
};

class Window {
public:
    struct Impl;

    /// Called during WM_PAINT.  hdc is the device context (HDC on Win32),
    /// width/height are the client area dimensions.
    using PaintHandler = std::function<void(void* hdc, int width, int height)>;

    Window();
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool create(const WindowConfig& config, std::string* error = nullptr);
    void pollEvents();
    void close();

    bool isOpen() const;
    const WindowConfig& config() const;

    // ---- Input -----------------------------------------------------------
    /// Returns true while the virtual key is held down.
    bool isKeyDown(int virtualKey) const;
    bool isKeyDown(Key key) const
    {
        return isKeyDown(static_cast<int>(key));
    }

    /// Returns true only on the first frame the key transitions to down.
    /// Cleared each frame by endFrame().
    bool wasKeyPressed(int virtualKey) const;
    bool wasKeyPressed(Key key) const
    {
        return wasKeyPressed(static_cast<int>(key));
    }

    /// Must be called once per frame (after pollEvents / before next tick)
    /// to clear per-frame input flags.
    void endFrame();

    // ---- Rendering -------------------------------------------------------
    /// Set a callback that will be invoked inside WM_PAINT.
    /// The callback receives the native HDC and client-area dimensions.
    void setPaintHandler(PaintHandler handler);

    /// Trigger a repaint (InvalidateRect on Win32).
    void invalidate();

    /// Returns the native window handle (HWND on Win32).
    void* getNativeHandle() const;

private:
    std::unique_ptr<Impl> impl_;
    WindowConfig config_;
};

} // namespace ycode

#endif // YCODE_WINDOW_H
