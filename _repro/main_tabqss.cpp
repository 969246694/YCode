// Repro variant: QSS = only QTabBar::tab rule
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
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
    QWidget *welcome = new QWidget();
    QVBoxLayout *wl = new QVBoxLayout(welcome);
    wl->setAlignment(Qt::AlignCenter);
    QLabel *logo = new QLabel("YCode", welcome);
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet("QLabel { font-size: 48px; font-weight: bold; color: #3C3C3C; }");
    wl->addWidget(logo);
    tabs->addTab(welcome, "欢迎");

    window.setCentralWidget(tabs);
    window.show();

    qDebug() << "repro started (QSS: QTabBar::tab only)";
    return app.exec();
}
