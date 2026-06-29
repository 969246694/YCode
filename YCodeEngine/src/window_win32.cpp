#include "ycode/window.h"

#include <unordered_set>
#include <utility>
#include <windows.h>

namespace ycode {

struct Window::Impl {
    HWND hwnd = nullptr;
    bool open = false;

    // Input state
    std::unordered_set<int> keysDown;
    std::unordered_set<int> keysPressed;

    // Paint callback
    Window::PaintHandler paintHandler;
};

namespace {

const wchar_t* kWindowClassName = L"YCodeEngineWindow";

std::wstring toWide(const std::string& text)
{
    if (text.empty())
        return std::wstring();

    int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (size <= 0)
        return std::wstring(text.begin(), text.end());

    std::wstring wide(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), size);
    if (!wide.empty() && wide.back() == L'\0')
        wide.pop_back();
    return wide;
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Window::Impl* impl = reinterpret_cast<Window::Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_NCCREATE:
    {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    case WM_CLOSE:
        if (impl)
            impl->open = false;
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        if (impl)
        {
            impl->hwnd = nullptr;
            impl->open = false;
        }
        return 0;

    // ---- Keyboard input --------------------------------------------------
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        int vk = static_cast<int>(wParam);
        // Suppress auto-repeat: only flag as "pressed" on first transition
        bool wasDown = (lParam & (1 << 30)) != 0; // bit 30 = previous key state
        if (impl)
        {
            if (!wasDown)
                impl->keysPressed.insert(vk);
            impl->keysDown.insert(vk);
        }
        return 0;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (impl)
        {
            impl->keysDown.erase(static_cast<int>(wParam));
        }
        return 0;

    // ---- Painting --------------------------------------------------------
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        // Default background
        HBRUSH background = CreateSolidBrush(RGB(18, 18, 24));
        FillRect(dc, &rect, background);
        DeleteObject(background);

        // If the user provided a paint handler, call it.
        // Otherwise, draw the placeholder text.
        if (impl && impl->paintHandler)
        {
            impl->paintHandler(dc, width, height);
        }
        else
        {
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(235, 235, 245));
            DrawTextW(dc, L"YCode Engine", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

bool registerWindowClass(HINSTANCE instance)
{
    static bool registered = false;
    if (registered)
        return true;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kWindowClassName;

    registered = RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    return registered;
}

} // namespace

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
    close();
    config_ = config;

    if (config.width <= 0 || config.height <= 0)
    {
        if (error)
            *error = "Window width and height must be greater than zero";
        return false;
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!registerWindowClass(instance))
    {
        if (error)
            *error = "Failed to register Win32 window class";
        return false;
    }

    RECT rect = {0, 0, config.width, config.height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    std::wstring title = toWide(config.title);
    impl_->hwnd = CreateWindowExW(
        0,
        kWindowClassName,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        instance,
        impl_.get());

    if (!impl_->hwnd)
    {
        if (error)
            *error = "Failed to create Win32 window";
        return false;
    }

    SetWindowTextW(impl_->hwnd, title.c_str());
    impl_->open = true;
    if (config.visible)
    {
        ShowWindow(impl_->hwnd, SW_SHOW);
        UpdateWindow(impl_->hwnd);
    }
    return true;
}

void Window::pollEvents()
{
    MSG message;
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

void Window::close()
{
    if (impl_ && impl_->hwnd)
        DestroyWindow(impl_->hwnd);
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

// ---- Input ---------------------------------------------------------------

bool Window::isKeyDown(int virtualKey) const
{
    return impl_ && impl_->keysDown.count(virtualKey) > 0;
}

bool Window::wasKeyPressed(int virtualKey) const
{
    return impl_ && impl_->keysPressed.count(virtualKey) > 0;
}

void Window::endFrame()
{
    if (impl_)
        impl_->keysPressed.clear();
}

// ---- Rendering -----------------------------------------------------------

void Window::setPaintHandler(PaintHandler handler)
{
    if (impl_)
        impl_->paintHandler = std::move(handler);
}

void Window::invalidate()
{
    if (impl_ && impl_->hwnd)
        InvalidateRect(impl_->hwnd, nullptr, FALSE);
}

void* Window::getNativeHandle() const
{
    return impl_ ? impl_->hwnd : nullptr;
}

} // namespace ycode
