#include <QApplication>
#include <QStyleFactory>
#include <QPropertyAnimation>
#include <QAbstractAnimation>
#include "MainWindow.h"

#ifdef _WIN32
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

namespace {
// Windows 10/11 深色标题栏：与应用的深色主题保持一致
void enableDarkTitleBar(QWidget *window)
{
#ifdef _WIN32
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd)
        return;
    BOOL dark = TRUE;
    // DWMWA_USE_IMMERSIVE_DARK_MODE：Win10 1903+ 为 20，早期版本为 19
    HRESULT hr = DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
    if (FAILED(hr))
        DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
#else
    Q_UNUSED(window);
#endif
}
} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("YCode");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Yiyangzai");

    // 设置现代风格
    app.setStyle(QStyleFactory::create("Fusion"));

    // 设置调色板（深色主题）
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);

    MainWindow window;
    window.show();
    enableDarkTitleBar(&window);

    // 窗口启动淡入
    QPropertyAnimation *fade = new QPropertyAnimation(&window, "windowOpacity");
    fade->setDuration(300);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->start(QAbstractAnimation::DeleteWhenStopped);

    return app.exec();
}