// Repro variant: GLOBAL QTabBar QSS + inline stylesheet WITH close-button rule
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QWidget>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // global QSS like style.qss (QTabBar::tab WITHOUT close-button rule)
    app.setStyleSheet(
        "QTabBar::tab {"
        "    background-color: #2d2d30;"
        "    color: #d4d4d4;"
        "    padding: 8px 12px;"
        "    border: 1px solid #3e3e42;"
        "    border-bottom: none;"
        "    border-top-left-radius: 4px;"
        "    border-top-right-radius: 4px;"
        "}"
        "QTabBar::tab:selected { background-color: #1e1e1e; color: #007acc; }"
        "QTabBar::tab:hover { background-color: #3e3e42; }"
    );

    QMainWindow window;
    window.resize(1200, 800);

    QTabWidget *tabs = new QTabWidget(&window);
    tabs->setTabsClosable(true);
    tabs->setMovable(true);
    tabs->setDocumentMode(true);
    // inline stylesheet like editorTabs (WITH close-button rule)
    tabs->setStyleSheet(
        "QTabWidget::pane { border: none; }"
        "QTabBar::tab { background: #2D2D2D; color: #999999; padding: 6px 12px; margin-right: 2px; border: none; font-size: 12px; }"
        "QTabBar::tab:selected { background: #1E1E1E; color: #FFFFFF; border-top: 2px solid #007ACC; }"
        "QTabBar::tab:hover { background: #3C3C3C; }"
        "QTabBar::close-button { image: none; background: none; padding: 0px; }"
    );

    QWidget *welcome = new QWidget();
    tabs->addTab(welcome, "欢迎");

    window.setCentralWidget(tabs);
    window.show();

    qDebug() << "repro started (global QSS + inline close-button rule)";
    return app.exec();
}
