// Repro variant: inline tab qss, closable(false)
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
    // NOTE: closable NOT set
    tabs->setStyleSheet(
        "QTabBar::tab { background: #2D2D2D; color: #999999; padding: 6px 12px; border: none; }"
        "QTabBar::tab:selected { background: #1E1E1E; color: #FFFFFF; border-top: 2px solid #007ACC; }"
    );

    tabs->addTab(new QWidget(), "终端");
    tabs->addTab(new QTextEdit(), "输出");
    tabs->addTab(new QWidget(), "问题");

    window.setCentralWidget(tabs);
    window.show();

    qDebug() << "repro started (closable false)";
    return app.exec();
}
