// Repro variant: inline tab qss, documentMode(false), closable(true)
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
    // NOTE: documentMode NOT set
    tabs->setStyleSheet(
        "QTabBar::tab { background: #2D2D2D; color: #999999; padding: 6px 12px; border: none; }"
        "QTabBar::tab:selected { background: #1E1E1E; color: #FFFFFF; border-top: 2px solid #007ACC; }"
    );

    QWidget *welcome = new QWidget();
    tabs->addTab(welcome, "欢迎");

    window.setCentralWidget(tabs);
    window.show();

    qDebug() << "repro started (documentMode false)";
    return app.exec();
}
