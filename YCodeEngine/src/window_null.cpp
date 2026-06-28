#include "ycode/window.h"

namespace ycode {

struct Window::Impl {
    bool open = false;
};

Window::Window()
    : impl_(std::make_unique<Impl>())
{
}

Window::~Window()
{
    close();
}

bool Window::create(const WindowConfig& config, std::string* error)
{
    config_ = config;
    if (config.width <= 0 || config.height <= 0)
    {
        if (error)
            *error = "Window width and height must be greater than zero";
        return false;
    }

    impl_->open = true;
    return true;
}

void Window::pollEvents()
{
}

void Window::close()
{
    if (impl_)
        impl_->open = false;
}

bool Window::isOpen() const
{
    return impl_ && impl_->open;
}

const WindowConfig& Window::config() const
{
    return config_;
}

} // namespace ycode
