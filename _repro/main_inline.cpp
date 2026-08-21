// Repro variant: inline stylesheet on the tab widget (like the real app), NO global QSS
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QWidget>
#include <QTextEdit>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.resize(1200, 800);

    QTabWidget *tabs = new QTabWidget(&window);
    tabs->setTabsClosable(true);
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
    );

    QWidget *welcome = new QWidget();
    welcome->setStyleSheet("QWidget { background: #1E1E1E; }");
    tabs->addTab(welcome, "欢迎");

    window.setCentralWidget(tabs);
    window.show();

    qDebug() << "repro started (inline tab qss)";
    return app.exec();
}
