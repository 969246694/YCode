// Repro variant: closable(true) + close-button QSS rule (like the real app)
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QWidget>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.resize(1200, 800);

    QTabWidget *tabs = new QTabWidget(&window);
    tabs->setTabsClosable(true);
    tabs->setMovable(true);
    tabs->setDocumentMode(true);
    tabs->setStyleSheet(
        "QTabWidget::pane { border: none; }"
        "QTabBar::tab { "
        "    background: #2D2D2D;"
        "    color: #999999;"
        "    padding: 6px 12px;"
        "    margin-right: 2px;"
        "    border: none;"
        "    font-size: 12px;"
        "}"
        "QTabBar::tab:selected { "
        "    background: #1E1E1E;"
        "    color: #FFFFFF;"
        "    border-top: 2px solid #007ACC;"
        "}"
        "QTabBar::tab:hover { "
        "    background: #3C3C3C;"
        "}"
        "QTabBar::close-button { "
        "    image: none;"
        "    background: none;"
        "    padding: 0px;"
        "}"
    );

    QWidget *welcome = new QWidget();
    tabs->addTab(welcome, "欢迎");

    window.setCentralWidget(tabs);
    window.show();

    qDebug() << "repro started (closable + close-button QSS)";
    return app.exec();
}
