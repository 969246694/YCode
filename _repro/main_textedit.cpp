// Repro variant: QTextEdit as the tab instead of plain QWidget
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QWidget>
#include <QTextEdit>
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
    QTextEdit *edit = new QTextEdit();
    edit->setPlainText("hello");
    tabs->addTab(edit, "文件");
    tabs->addTab(new QTextEdit(), "文件2");

    window.setCentralWidget(tabs);
    window.show();

    qDebug() << "repro started (QTextEdit tabs)";
    return app.exec();
}
