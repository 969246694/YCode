#include "MainWindow.h"
#include "ChatWidget.h"
#include "CodeEditor.h"
#include "AgentManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QLabel>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCloseEvent>
#include <QSplitter>
#include <QTabBar>
#include <QIcon>
#include <QShortcut>
#include <QInputDialog>
#include <QApplication>
#include <QCoreApplication>
#include <QMenuBar>
#include <QScrollBar>
#include <QTextBlock>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>
#include <QProcessEnvironment>
#include <QVector>

void MainWindow::setupUI()
{
    // 中央组件 — 水平分割器
    mainSplitter = new QSplitter(Qt::Horizontal, this);

    // 左侧面板 (文件浏览器/搜索)
    leftSidebar = new QWidget();
    QVBoxLayout *leftSideLayout = new QVBoxLayout(leftSidebar);
    leftSideLayout->setContentsMargins(0, 0, 0, 0);
    leftSideLayout->setSpacing(0);

    sidebarTitle = new QLabel("  资源管理器");
    sidebarTitle->setStyleSheet(
        "QLabel {"
        "    color: #CCCCCC;"
        "    font-size: 11px;"
        "    font-weight: bold;"
        "    text-transform: uppercase;"
        "    padding: 10px 0px;"
        "    letter-spacing: 0.5px;"
        "}");
    leftSideLayout->addWidget(sidebarTitle);

    // 左侧标签页 (Explorer / Search)
    leftSidebarTabs = new QTabWidget();
    leftSidebarTabs->setTabPosition(QTabWidget::North);
    leftSidebarTabs->setStyleSheet(
        "QTabWidget::pane { border: none; }"
        "QTabBar::tab { "
        "    padding: 6px 16px; "
        "    color: #999999; "
        "    font-size: 12px; "
        "    border: none; "
        "    background: transparent; "
        "}"
        "QTabBar::tab:selected { "
        "    color: #FFFFFF; "
        "    border-bottom: 2px solid #007ACC; "
        "}"
    );
    leftSideLayout->addWidget(leftSidebarTabs);

    mainSplitter->addWidget(leftSidebar);

    // 编辑区 + 底部面板 (垂直分割)
    editorSplitter = new QSplitter(Qt::Vertical);
    editorSplitter->setStyleSheet("QSplitter::handle { background: #3C3C3C; height: 2px; }");

    // 编辑区
    editorArea = new QWidget();
    QVBoxLayout *editorLayout = new QVBoxLayout(editorArea);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);

    // 搜索栏 (初始隐藏)
    searchBar = new QWidget();
    searchBar->setVisible(false);
    searchBar->setStyleSheet(
        "QWidget { background-color: #252526; border-bottom: 1px solid #3C3C3C; }");
    editorLayout->addWidget(searchBar);

    // 编辑器标签页
    editorTabs = new QTabWidget();
    editorTabs->setTabsClosable(true);
    editorTabs->setMovable(true);
    editorTabs->setDocumentMode(true);
    editorTabs->setStyleSheet(
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

    connect(editorTabs, &QTabWidget::currentChanged, this, &MainWindow::onEditorCursorChanged);
    connect(editorTabs->tabBar(), &QTabBar::tabCloseRequested, [this](int index)
            {
        QWidget *widget = editorTabs->widget(index);
        editorTabs->removeTab(index);
        delete widget; });

    editorLayout->addWidget(editorTabs);

    editorSplitter->addWidget(editorArea);

    // 底部面板
    bottomPanel = new QWidget();
    bottomPanel->setVisible(showBottomPanel);
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomPanel);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);

    bottomTabs = new QTabWidget();
    bottomTabs->setStyleSheet(
        "QTabWidget::pane { border: none; background: #1E1E1E; }"
        "QTabBar::tab { "
        "    padding: 4px 16px;"
        "    color: #999999;"
        "    font-size: 11px;"
        "    border: none;"
        "    background: transparent;"
        "}"
        "QTabBar::tab:selected { "
        "    color: #FFFFFF;"
        "    border-bottom: 2px solid #007ACC;"
        "}"
    );
    bottomLayout->addWidget(bottomTabs);

    editorSplitter->addWidget(bottomPanel);
    editorSplitter->setSizes({700, 200});

    mainSplitter->addWidget(editorSplitter);

    // 聊天面板 (右侧)
    chatPanel = new QWidget();
    QVBoxLayout *chatLayout = new QVBoxLayout(chatPanel);
    chatLayout->setContentsMargins(0, 0, 0, 0);
    chatLayout->setSpacing(0);

    chatTitle = new QLabel("  AI 对话");
    chatTitle->setStyleSheet(
        "QLabel {"
        "    color: #CCCCCC;"
        "    font-size: 11px;"
        "    font-weight: bold;"
        "    padding: 10px 0px;"
        "    letter-spacing: 0.5px;"
        "}");
    chatLayout->addWidget(chatTitle);

    chatDisplay = new ChatWidget();
    chatLayout->addWidget(chatDisplay);

    // 输入区域
    chatInputArea = new QWidget();
    chatInputArea->setStyleSheet("QWidget { background: #252526; border-top: 1px solid #3C3C3C; }");
    QVBoxLayout *inputLayout = new QVBoxLayout(chatInputArea);
    inputLayout->setContentsMargins(8, 8, 8, 8);
    inputLayout->setSpacing(6);

    inputField = new QLineEdit();
    inputField->setPlaceholderText("输入问题... (Enter 发送)");
    inputField->setStyleSheet(
        "QLineEdit {"
        "    background: #3C3C3C;"
        "    color: #CCCCCC;"
        "    border: 1px solid #555555;"
        "    border-radius: 4px;"
        "    padding: 8px 12px;"
        "    font-size: 13px;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid #007ACC;"
        "}"
    );

    sendButton = new QPushButton("发送");
    sendButton->setStyleSheet(
        "QPushButton {"
        "    background: #007ACC;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background: #1A8AD4;"
        "}"
        "QPushButton:pressed {"
        "    background: #005999;"
        "}"
        "QPushButton:disabled {"
        "    background: #3E3E42;"
        "    color: #858585;"
        "}"
    );
    sendButton->setEnabled(false); // 输入为空时不可发送

    QHBoxLayout *inputRow = new QHBoxLayout();
    inputRow->addWidget(inputField);
    inputRow->addWidget(sendButton);
    inputLayout->addLayout(inputRow);

    chatLayout->addWidget(chatInputArea);

    mainSplitter->addWidget(chatPanel);

    // 分割比例: 左侧面板 250px, 编辑区 600px, 聊天 350px
    mainSplitter->setSizes({250, 700, 400});
    mainSplitter->setStretchFactor(1, 1);  // 编辑区拉伸

    setCentralWidget(mainSplitter);

    // 连接信号
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(inputField, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);
    connect(inputField, &QLineEdit::textChanged, this, [this](const QString &text) {
        sendButton->setEnabled(!text.trimmed().isEmpty());
    });
}

// ============================================================
// 活动栏 (最左侧图标条)
// ============================================================

void MainWindow::setupActivityBar()
{
    activityBar = new QToolBar("活动栏", this);
    activityBar->setMovable(false);
    activityBar->setFloatable(false);
    activityBar->setOrientation(Qt::Vertical);
    activityBar->setIconSize(QSize(24, 24));
    activityBar->setStyleSheet(
        "QToolBar {"
        "    background: #333333;"
        "    border: none;"
        "    spacing: 4px;"
        "    padding: 8px 0px;"
        "}"
        "QToolButton {"
        "    background: transparent;"
        "    border: none;"
        "    border-left: 2px solid transparent;"
        "    padding: 8px;"
        "    margin: 2px 0px;"
        "    color: #858585;"
        "    font-size: 20px;"
        "}"
        "QToolButton:hover {"
        "    color: #FFFFFF;"
        "    background: #3C3C3C;"
        "}"
        "QToolButton:checked, QToolButton[active=\"true\"] {"
        "    color: #FFFFFF;"
        "    border-left: 2px solid #007ACC;"
        "    background: #37373D;"
        "}"
    );

    btnExplorer = new QToolButton();
    btnExplorer->setText("📁");
    btnExplorer->setToolTip("资源管理器 (Ctrl+Shift+E)");
    btnExplorer->setCheckable(true);
    btnExplorer->setChecked(true);
    btnExplorer->setProperty("active", "true");

    btnSearch = new QToolButton();
    btnSearch->setText("🔍");
    btnSearch->setToolTip("搜索 (Ctrl+Shift+F)");
    btnSearch->setCheckable(true);

    btnChat = new QToolButton();
    btnChat->setText("💬");
    btnChat->setToolTip("AI 对话 (Ctrl+Shift+L)");
    btnChat->setCheckable(true);

    btnSettings = new QToolButton();
    btnSettings->setText("⚙️");
    btnSettings->setToolTip("设置");

    activityBar->addWidget(btnExplorer);
    activityBar->addSeparator();
    activityBar->addWidget(btnSearch);
    activityBar->addWidget(btnChat);
    activityBar->addSeparator();
    activityBar->addWidget(btnSettings);

    addToolBar(Qt::LeftToolBarArea, activityBar);

    // 连接活动栏按钮
    connect(btnExplorer, &QToolButton::clicked, this, &MainWindow::showFileExplorer);
    connect(btnSearch, &QToolButton::clicked, this, &MainWindow::showSearchPanel);
    connect(btnChat, &QToolButton::clicked, this, &MainWindow::showChatPanel);
    connect(btnSettings, &QToolButton::clicked, this, &MainWindow::showApiSettings);
}

// ============================================================
// 文件浏览器 (QTreeView)
// ============================================================

void MainWindow::setupFileExplorer()
{
    fileSystemModel = new QFileSystemModel(this);
    QString rootPath = activeWorkspacePath();
    fileSystemModel->setRootPath(rootPath);
    fileSystemModel->setFilter(QDir::NoDot | QDir::AllDirs | QDir::Files);
    fileSystemModel->setNameFilters(QStringList()
                                    << "*.cpp" << "*.h" << "*.hpp" << "*.c"
                                    << "*.py" << "*.js" << "*.ts" << "*.lua"
                                    << "*.txt" << "*.md" << "*.json" << "*.xml" << "*.yaml" << "*.yml"
                                    << "*.cmake" << "*.bat" << "*.sh"
                                    << "*.ui" << "*.qrc" << "*.pro" << "*.pri"
                                    << "*.rc" << "*.ico" << "*.png" << "*.jpg" << "*.jpeg"
                                    << "*.glsl" << "*.vert" << "*.frag" << "*.hlsl" << "*.ycode");
    fileSystemModel->setNameFilterDisables(false);

    fileTreeView = new QTreeView();
    fileTreeView->setModel(fileSystemModel);
    fileTreeView->setRootIndex(fileSystemModel->index(rootPath));
    fileTreeView->setHeaderHidden(true);
    fileTreeView->setAnimated(true);
    fileTreeView->setIndentation(16);
    fileTreeView->setSortingEnabled(true);
    fileTreeView->sortByColumn(0, Qt::AscendingOrder);

    // 隐藏大小、类型等列，只显示文件名
    for (int i = 1; i < fileSystemModel->columnCount(); ++i)
        fileTreeView->hideColumn(i);

    fileTreeView->setStyleSheet(
        "QTreeView {"
        "    background: #252526;"
        "    color: #CCCCCC;"
        "    border: none;"
        "    font-size: 13px;"
        "    outline: none;"
        "}"
        "QTreeView::item {"
        "    padding: 4px 8px;"
        "    border: none;"
        "}"
        "QTreeView::item:hover {"
        "    background: #2A2D2E;"
        "}"
        "QTreeView::item:selected {"
        "    background: #37373D;"
        "    color: #FFFFFF;"
        "}"
        "QTreeView::branch:has-children:!has-siblings:closed,"
        "QTreeView::branch:closed:has-children:has-siblings {"
        "    border-image: none;"
        "}"
        "QTreeView::branch:open:has-children:!has-siblings,"
        "QTreeView::branch:open:has-children:has-siblings {"
        "    border-image: none;"
        "}"
    );

    // 文件夹和文件图标 (使用 Unicode)
    fileSystemModel->setData(fileSystemModel->index(rootPath), "📂", Qt::DecorationRole);

    connect(fileTreeView, &QTreeView::doubleClicked, this, &MainWindow::onFileTreeDoubleClicked);

    // 添加到左侧面板的第一个标签页
    leftSidebarTabs->addTab(fileTreeView, "📁 文件");
}

// ============================================================
// 编辑器区域
// ============================================================

void MainWindow::setupEditorArea()
{
    // 编辑器标签页已在 setupUI() 中创建
    // 这里可以添加欢迎页
}

// ============================================================
// 聊天面板
// ============================================================

void MainWindow::setupChatPanel()
{
    // 聊天面板已在 setupUI() 中创建
}

// ============================================================
// 搜索栏
// ============================================================

void MainWindow::setupSearchBar()
{
    QHBoxLayout *searchLayout = new QHBoxLayout(searchBar);
    searchLayout->setContentsMargins(8, 4, 8, 4);
    searchLayout->setSpacing(6);

    searchInput = new QLineEdit();
    searchInput->setPlaceholderText("搜索...");
    searchInput->setMaximumWidth(250);
    searchInput->setStyleSheet(
        "QLineEdit {"
        "    background: #3C3C3C;"
        "    color: #CCCCCC;"
        "    border: 1px solid #555555;"
        "    padding: 4px 8px;"
        "    font-size: 13px;"
        "}"
        "QLineEdit:focus { border: 1px solid #007ACC; }"
    );

    replaceInput = new QLineEdit();
    replaceInput->setPlaceholderText("替换为...");
    replaceInput->setMaximumWidth(250);
    replaceInput->setStyleSheet(searchInput->styleSheet());

    btnSearchNext = new QPushButton("↓");
    btnSearchNext->setToolTip("查找下一个 (Enter)");
    btnSearchNext->setMaximumWidth(30);
    btnSearchPrev = new QPushButton("↑");
    btnSearchPrev->setToolTip("查找上一个 (Shift+Enter)");
    btnSearchPrev->setMaximumWidth(30);
    btnReplace = new QPushButton("替换");
    btnReplace->setToolTip("替换当前");
    btnReplaceAll = new QPushButton("全部替换");
    btnReplaceAll->setToolTip("替换所有匹配项");
    btnCloseSearch = new QPushButton("✕");
    btnCloseSearch->setToolTip("关闭搜索");
    btnCloseSearch->setMaximumWidth(30);

    QString btnStyle =
        "QPushButton {"
        "    background: #3C3C3C;"
        "    color: #CCCCCC;"
        "    border: 1px solid #555555;"
        "    border-radius: 3px;"
        "    padding: 4px 10px;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover { background: #505050; }"
        "QPushButton:pressed { background: #007ACC; }";

    btnSearchNext->setStyleSheet(btnStyle);
    btnSearchPrev->setStyleSheet(btnStyle);
    btnReplace->setStyleSheet(btnStyle);
    btnReplaceAll->setStyleSheet(btnStyle);
    btnCloseSearch->setStyleSheet(btnStyle);

    searchLayout->addWidget(searchInput);
    searchLayout->addWidget(replaceInput);
    searchLayout->addWidget(btnSearchNext);
    searchLayout->addWidget(btnSearchPrev);
    searchLayout->addWidget(btnReplace);
    searchLayout->addWidget(btnReplaceAll);
    searchLayout->addStretch();
    searchLayout->addWidget(btnCloseSearch);

    connect(btnSearchNext, &QPushButton::clicked, this, &MainWindow::searchNext);
    connect(btnSearchPrev, &QPushButton::clicked, this, &MainWindow::searchPrevious);
    connect(btnReplace, &QPushButton::clicked, this, &MainWindow::replaceCurrent);
    connect(btnReplaceAll, &QPushButton::clicked, this, &MainWindow::replaceAll);
    connect(btnCloseSearch, &QPushButton::clicked, this, &MainWindow::hideSearchBar);
    connect(searchInput, &QLineEdit::returnPressed, this, &MainWindow::searchNext);
}

// ============================================================
// 底部面板 (终端 + 问题)
// ============================================================

void MainWindow::setupBottomPanel()
{
    // 终端标签页
    QWidget *terminalTab = new QWidget();
    QVBoxLayout *terminalLayout = new QVBoxLayout(terminalTab);
    terminalLayout->setContentsMargins(0, 0, 0, 0);
    terminalLayout->setSpacing(0);

    terminalOutput = new QPlainTextEdit();
    terminalOutput->setReadOnly(true);
    terminalOutput->setStyleSheet(
        "QPlainTextEdit {"
        "    background: #1E1E1E;"
        "    color: #00FF00;"
        "    border: none;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 12px;"
        "    padding: 8px;"
        "    selection-background-color: #264F78;"
        "}"
    );
    terminalOutput->appendPlainText("YCode Terminal v1.0.0");
    terminalOutput->appendPlainText("输入命令并按回车执行...");
    terminalOutput->appendPlainText("");

    terminalInput = new QLineEdit();
    terminalInput->setPlaceholderText("> 输入命令...");
    terminalInput->setStyleSheet(
        "QLineEdit {"
        "    background: #252526;"
        "    color: #00FF00;"
        "    border: none;"
        "    border-top: 1px solid #3C3C3C;"
        "    padding: 6px 8px;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 12px;"
        "}"
    );

    connect(terminalInput, &QLineEdit::returnPressed, [this]()
            {
        QString cmd = terminalInput->text();
        if (cmd.isEmpty()) return;
        terminalOutput->appendPlainText("> " + cmd);
        terminalInput->clear();

        // 执行简单命令
        QProcess proc;
        proc.setWorkingDirectory(activeWorkspacePath());
        proc.start("cmd", QStringList() << "/c" << cmd);
        proc.waitForFinished(5000);
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
        QString err = QString::fromLocal8Bit(proc.readAllStandardError());
        if (!output.isEmpty())
            terminalOutput->appendPlainText(output);
        if (!err.isEmpty())
            terminalOutput->appendHtml("<span style='color:#FF5555'>" + err.toHtmlEscaped() + "</span>");

        QScrollBar *sb = terminalOutput->verticalScrollBar();
        sb->setValue(sb->maximum()); });

    terminalLayout->addWidget(terminalOutput);
    terminalLayout->addWidget(terminalInput);
    bottomTabs->addTab(terminalTab, "终端");

    // 问题标签页
    problemsList = new QListWidget();
    problemsList->setStyleSheet(
        "QListWidget {"
        "    background: #1E1E1E;"
        "    color: #CCCCCC;"
        "    border: none;"
        "    font-size: 12px;"
        "}"
        "QListWidget::item { padding: 4px 8px; }"
        "QListWidget::item:hover { background: #2A2D2E; }"
    );
    problemsList->addItem("✅ 没有检测到问题");
    bottomTabs->addTab(problemsList, "问题");

    // 输出标签页
    outputLog = new QPlainTextEdit();
    outputLog->setReadOnly(true);
    outputLog->setStyleSheet(
        "QPlainTextEdit {"
        "    background: #1E1E1E;"
        "    color: #CCCCCC;"
        "    border: none;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 12px;"
        "    padding: 8px;"
        "}"
    );
    outputLog->appendPlainText("YCode 输出日志...");
    bottomTabs->addTab(outputLog, "输出");
}

// ============================================================
// 状态栏
// ============================================================

void MainWindow::setupStatusBar()
{
    statusBar = new QStatusBar(this);
    statusBar->setStyleSheet(
        "QStatusBar {"
        "    background: #007ACC;"
        "    color: #FFFFFF;"
        "    border: none;"
        "    padding: 2px 8px;"
        "    font-size: 12px;"
        "}"
        "QStatusBar::item { border: none; }"
    );

    statusMessage = new QLabel("就绪");
    statusMessage->setStyleSheet("color: #FFFFFF; padding: 0px 8px;");
    statusBar->addWidget(statusMessage, 1);

    lineColLabel = new QLabel("行 1, 列 1");
    lineColLabel->setStyleSheet("color: #FFFFFF; padding: 0px 12px;");
    statusBar->addPermanentWidget(lineColLabel);

    languageLabel = new QLabel("纯文本");
    languageLabel->setStyleSheet("color: #FFFFFF; padding: 0px 12px;");
    statusBar->addPermanentWidget(languageLabel);

    encodingLabel = new QLabel("UTF-8");
    encodingLabel->setStyleSheet("color: #FFFFFF; padding: 0px 12px;");
    statusBar->addPermanentWidget(encodingLabel);

    setStatusBar(statusBar);
}

// ============================================================
// 菜单栏 (增强版)
// ============================================================

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);
    menuBar->setStyleSheet(
        "QMenuBar { background: #3C3C3C; color: #CCCCCC; border: none; padding: 2px; }"
        "QMenuBar::item { padding: 4px 12px; }"
        "QMenuBar::item:selected { background: #505050; }"
        "QMenu { background: #252526; color: #CCCCCC; border: 1px solid #3C3C3C; padding: 4px; }"
        "QMenu::item { padding: 6px 30px 6px 20px; }"
        "QMenu::item:selected { background: #094771; }"
        "QMenu::separator { height: 1px; background: #3C3C3C; margin: 4px 10px; }"
    );

    // 文件菜单
    QMenu *fileMenu = menuBar->addMenu("文件(&F)");
    fileMenu->addAction("📄 新建文件", this, &MainWindow::newFile, QKeySequence::New);
    fileMenu->addAction("📂 打开文件...", this, &MainWindow::openFile, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("💾 保存", this, &MainWindow::saveFile, QKeySequence::Save);
    fileMenu->addAction("💾 另存为...", this, &MainWindow::saveAsFile, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("❌ 关闭编辑器", [this]()
                        {
        int idx = editorTabs->currentIndex();
        if (idx >= 0) {
            QWidget *w = editorTabs->widget(idx);
            editorTabs->removeTab(idx);
            delete w;
        } }, QKeySequence::Close);
    fileMenu->addSeparator();
    fileMenu->addAction("🚪 退出", this, &QWidget::close, QKeySequence::Quit);

    // 编辑菜单
    QMenu *editMenu = menuBar->addMenu("编辑(&E)");
    editMenu->addAction("🔍 查找", this, &MainWindow::showSearchBar, QKeySequence::Find);
    editMenu->addAction("🔄 替换", [this]()
                        { showSearchBar();
        replaceInput->setFocus(); }, QKeySequence::Replace);
    editMenu->addSeparator();
    editMenu->addAction("📋 清空对话", this, &MainWindow::clearChat);

    // 视图菜单
    QMenu *viewMenu = menuBar->addMenu("视图(&V)");
    viewMenu->addAction("📁 资源管理器", this, &MainWindow::showFileExplorer, QKeySequence("Ctrl+Shift+E"));
    viewMenu->addAction("🔍 搜索", this, &MainWindow::showSearchPanel, QKeySequence("Ctrl+Shift+F"));
    viewMenu->addAction("💬 AI 对话", this, &MainWindow::showChatPanel, QKeySequence("Ctrl+Shift+L"));
    viewMenu->addSeparator();
    viewMenu->addAction("📟 切换底部面板", this, &MainWindow::toggleBottomPanel, QKeySequence("Ctrl+J"));
    viewMenu->addSeparator();
    viewMenu->addAction("⌨️ 命令面板...", [this]()
                        {
        // Trigger command palette
        QString cmd = QInputDialog::getText(this, "命令面板", "输入命令:",
                                            QLineEdit::Normal, QString(), nullptr,
                                            Qt::Dialog | Qt::WindowCloseButtonHint);
        if (cmd.isEmpty()) return;
        // Simple command handling
        if (cmd == "toggle terminal" || cmd == "terminal") toggleBottomPanel();
        else if (cmd == "search") showSearchBar();
        else if (cmd == "new file") newFile();
        else if (cmd == "open file") openFile();
        else statusMessage->setText("未知命令: " + cmd); }, QKeySequence("Ctrl+Shift+P"));

    // 游戏开发菜单
    QMenu *gameMenu = menuBar->addMenu("游戏开发(&G)");
    gameMenu->addAction("🎮 新建 YCode 游戏项目...", this, &MainWindow::createGameProject);
    gameMenu->addAction("📂 打开 YCode 游戏项目...", this, &MainWindow::openGameProject);
    gameMenu->addSeparator();
    gameMenu->addAction("🔨 构建当前游戏项目", this, &MainWindow::buildGameProject);
    gameMenu->addAction("▶️ 运行当前游戏项目", this, &MainWindow::runGameProject);
    gameMenu->addAction("🔁 实时预览（自动重建+重启）", this, &MainWindow::runGamePreview);
    gameMenu->addAction("⏹ 停止预览", this, &MainWindow::stopGamePreview);
    gameMenu->addSeparator();
    gameMenu->addAction("🛠 构建 YCode Engine", this, &MainWindow::buildYCodeEngine);
    gameMenu->addAction("📂 打开引擎源码目录", this, &MainWindow::openYCodeEngineFolder);
    gameMenu->addSeparator();
    gameMenu->addAction("🤖 启动 AI 游戏开发模式", this, &MainWindow::sendGameDevPrompt);

    // 设置菜单
    QMenu *settingsMenu = menuBar->addMenu("设置(&S)");
    settingsMenu->addAction("🔑 API 设置...", this, &MainWindow::showApiSettings);
    settingsMenu->addAction("🎨 面板主题...", this, &MainWindow::showPanelThemeSettings);

    // 帮助菜单
    QMenu *helpMenu = menuBar->addMenu("帮助(&H)");
    helpMenu->addAction("检查更新...", this, &MainWindow::checkForUpdates);
    helpMenu->addSeparator();
    helpMenu->addAction("ℹ️ 关于 YCode", [this]()
                        { QMessageBox::about(this, "关于 YCode",
                                             "YCode v2.0 - VSCode Style\n\n"
                                             "基于 DeepSeek V4-Pro 的 AI 编程助手\n"
                                             "内置 C++ 语法高亮、树形文件浏览器、\n"
                                             "终端、搜索替换等功能\n\n"
                                             "© 2026 Yiyangzai"); });

    setMenuBar(menuBar);
}

void MainWindow::showApiSettings()
{
    QInputDialog inputDlg(this);
    inputDlg.setWindowTitle("YCode 设置");
    inputDlg.setLabelText("DeepSeek API Key (仅当前会话保存；推荐使用 DEEPSEEK_API_KEY 环境变量):");
    inputDlg.setTextValue(apiKey);
    inputDlg.setStyleSheet(QString(
        "QInputDialog { background: %1; }"
        "QLabel { color: %2; }"
        "QLineEdit { background: %3; color: %2; border: 1px solid %4; }")
        .arg(panelTheme.elevatedBackground,
             panelTheme.textColor,
             panelTheme.panelBackground,
             panelTheme.borderColor));

    if (inputDlg.exec() != QDialog::Accepted)
        return;

    apiKey = inputDlg.textValue();
    saveSettings();
    if (agentManager)
    {
        agentManager->stop();
        delete agentManager;
    }

    agentManager = new AgentManager(currentProjectPath, apiKey, this);
    agentManager->setWorkspacePath(workspacePath);
    connectAgentSignals();
    if (!apiKey.isEmpty())
        agentManager->start();
    statusMessage->setText("API Key 已更新");
}

void MainWindow::setupCommandPalette()
{
    // Ctrl+Shift+P 已在菜单中绑定
    // Ctrl+F 搜索
    QShortcut *findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, this, &MainWindow::showSearchBar);

    // Ctrl+H 替换
    QShortcut *replaceShortcut = new QShortcut(QKeySequence::Replace, this);
    connect(replaceShortcut, &QShortcut::activated, [this]()
            {
        showSearchBar();
        replaceInput->setFocus(); });

    // Escape 关闭搜索
    QShortcut *escShortcut = new QShortcut(QKeySequence("Escape"), this);
    connect(escShortcut, &QShortcut::activated, [this]()
            {
        if (searchBar->isVisible())
            hideSearchBar(); });

    // Ctrl+J 切换底部面板
    QShortcut *bottomShortcut = new QShortcut(QKeySequence("Ctrl+J"), this);
    connect(bottomShortcut, &QShortcut::activated, this, &MainWindow::toggleBottomPanel);
}

// ============================================================
// 文件树双击
// ============================================================
