#ifndef YCODE_WINDOW_H
#define YCODE_WINDOW_H

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

    Window();
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool create(const WindowConfig& config, std::string* error = nullptr);
    void pollEvents();
    void close();

    bool isOpen() const;
    const WindowConfig& config() const;

private:
    std::unique_ptr<Impl> impl_;
    WindowConfig config_;
};

} // namespace ycode

#endif // YCODE_WINDOW_H
