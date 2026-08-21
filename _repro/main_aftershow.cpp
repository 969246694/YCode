// Repro variant: add tabs AFTER window.show()
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QWidget>
#include <QTextEdit>
#include <QTimer>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setStyleSheet(
        "QTabBar::tab { background: transparent; color: #999; padding: 6px 12px; }"
    );

    QMainWindow window;
    window.resize(1200, 800);

    QTabWidget *tabs = new QTabWidget(&window);
    tabs->setTabsClosable(true);
    tabs->setDocumentMode(true);

    window.setCentralWidget(tabs);
    window.show();

    // add tab AFTER show
    QTimer::singleShot(500, [tabs]() {
        QTextEdit *edit = new QTextEdit();
        edit->setPlainText("hello");
        tabs->addTab(edit, "文件");
        qDebug() << "tab added after show";
    });

    qDebug() << "repro started (tab added after show)";
    return app.exec();
}
