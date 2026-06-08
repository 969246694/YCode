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
#include <QMenuBar>
#include <QScrollBar>
#include <QTextBlock>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>

// ============================================================
// 构造函数 & 析构函数
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      agentManager(nullptr),
      showBottomPanel(true),
      selfUpdateInProgress(false),
      currentActivity(0)
{
    setWindowIcon(QIcon("F:/YiyangzaiCode/YCode.ico"));

    QByteArray envApiKey = qgetenv("DEEPSEEK_API_KEY");
    apiKey = QString::fromUtf8(envApiKey);

    setupUI();
    setupActivityBar();
    setupFileExplorer();
    setupEditorArea();
    setupChatPanel();
    setupBottomPanel();
    setupSearchBar();
    setupStatusBar();
    setupMenuBar();
    setupCommandPalette();
    loadSettings();

    setWindowTitle("YCode - AI 编程助手");
    setMinimumSize(1200, 800);
    resize(1400, 900);

    if (apiKey.isEmpty())
    {
        QMessageBox::information(this, "欢迎使用 YCode",
                                 "请先在设置中配置 DeepSeek API Key\n\n"
                                 "你可以在 F:/YiyangzaiCode 设置环境变量 DEEPSEEK_API_KEY");
    }

    // 安装事件过滤器监听编辑器光标变化
    installEventFilter(this);
}

MainWindow::~MainWindow()
{
    saveSettings();
}

// ============================================================
// ★ 连接 AgentManager 信号（抽取为独立函数，方便重建时复用）
// ============================================================
void MainWindow::connectAgentSignals()
{
    if (!agentManager) return;

    connect(agentManager, &AgentManager::outputReceived, this, &MainWindow::onAgentOutput);
    connect(agentManager, &AgentManager::errorOccurred, this, &MainWindow::onAgentError);
    connect(agentManager, &AgentManager::statusChanged, this, &MainWindow::onAgentStatusChanged);
    connect(agentManager, &AgentManager::agentRestarting, this, &MainWindow::onAgentRestarting);
    connect(agentManager, &AgentManager::ycodeSelfUpdateRequested, this, &MainWindow::onYCodeSelfUpdateRequested);
}

// ============================================================
// 主界面布局
// ============================================================

void MainWindow::setupUI()
{
    // 中央组件 — 水平分割器
    mainSplitter = new QSplitter(Qt::Horizontal, this);

    // 左侧面板 (文件浏览器/搜索)
    leftSidebar = new QWidget();
    QVBoxLayout *leftSideLayout = new QVBoxLayout(leftSidebar);
    leftSideLayout->setContentsMargins(0, 0, 0, 0);
    leftSideLayout->setSpacing(0);

    QLabel *sidebarTitle = new QLabel("  资源管理器");
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

    QLabel *chatTitle = new QLabel("  AI 对话");
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
    QWidget *inputArea = new QWidget();
    inputArea->setStyleSheet("QWidget { background: #252526; border-top: 1px solid #3C3C3C; }");
    QVBoxLayout *inputLayout = new QVBoxLayout(inputArea);
    inputLayout->setContentsMargins(8, 8, 8, 8);
    inputLayout->setSpacing(6);

    inputField = new QLineEdit();
    inputField->setPlaceholderText("输入问题... (Ctrl+Enter 发送)");
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
    );

    QHBoxLayout *inputRow = new QHBoxLayout();
    inputRow->addWidget(inputField);
    inputRow->addWidget(sendButton);
    inputLayout->addLayout(inputRow);

    chatLayout->addWidget(inputArea);

    mainSplitter->addWidget(chatPanel);

    // 分割比例: 左侧面板 250px, 编辑区 600px, 聊天 350px
    mainSplitter->setSizes({250, 700, 400});
    mainSplitter->setStretchFactor(1, 1);  // 编辑区拉伸

    setCentralWidget(mainSplitter);

    // 连接信号
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(inputField, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);
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
    connect(btnSettings, &QToolButton::clicked, [this]()
            {
        QInputDialog inputDlg(this);
        inputDlg.setWindowTitle("YCode 设置");
        inputDlg.setLabelText("DeepSeek API Key:");
        inputDlg.setTextValue(apiKey);
        inputDlg.setStyleSheet(
            "QInputDialog { background: #2D2D2D; }"
            "QLabel { color: #CCCCCC; }"
            "QLineEdit { background: #3C3C3C; color: #CCCCCC; }"
        );
        if (inputDlg.exec() == QDialog::Accepted)
        {
            apiKey = inputDlg.textValue();
            saveSettings();
            if (agentManager)
            {
                agentManager->stop();
                delete agentManager;
            }
            agentManager = new AgentManager(currentProjectPath, apiKey, this);
            connectAgentSignals();  // ★ 使用统一信号连接
            if (!apiKey.isEmpty())
                agentManager->start();
            statusMessage->setText("API Key 已更新");
        } });
}

// ============================================================
// 文件浏览器 (QTreeView)
// ============================================================

void MainWindow::setupFileExplorer()
{
    fileSystemModel = new QFileSystemModel(this);
    fileSystemModel->setRootPath(currentProjectPath.isEmpty() ? "F:/YiyangzaiCode" : currentProjectPath);
    fileSystemModel->setFilter(QDir::NoDot | QDir::AllDirs | QDir::Files);
    fileSystemModel->setNameFilters(QStringList()
                                    << "*.cpp" << "*.h" << "*.hpp" << "*.c"
                                    << "*.py" << "*.js" << "*.ts"
                                    << "*.txt" << "*.md" << "*.json" << "*.xml"
                                    << "*.cmake" << "*.bat" << "*.sh"
                                    << "*.ui" << "*.qrc" << "*.pro" << "*.pri"
                                    << "*.rc" << "*.ico" << "*.png");
    fileSystemModel->setNameFilterDisables(false);

    fileTreeView = new QTreeView();
    fileTreeView->setModel(fileSystemModel);
    fileTreeView->setRootIndex(fileSystemModel->index(currentProjectPath.isEmpty() ?
                                                          "F:/YiyangzaiCode" : currentProjectPath));
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
    fileSystemModel->setData(fileSystemModel->index(currentProjectPath), "📂", Qt::DecorationRole);

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
        proc.setWorkingDirectory(currentProjectPath);
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
    QPlainTextEdit *outputLog = new QPlainTextEdit();
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

    // 设置菜单
    QMenu *settingsMenu = menuBar->addMenu("设置(&S)");
    settingsMenu->addAction("🔑 API 设置...", [this]()
                            { btnSettings->click(); });

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

// ============================================================
// 命令面板快捷键
// ============================================================

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

void MainWindow::onFileTreeDoubleClicked(const QModelIndex &index)
{
    QString filePath = fileSystemModel->filePath(index);
    QFileInfo info(filePath);

    if (info.isDir())
        return; // 目录由 QTreeView 自动展开

    if (!info.isFile())
        return;

    // 检查是否已打开
    for (int i = 0; i < editorTabs->count(); ++i)
    {
        CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->widget(i));
        if (editor && editor->filePath() == filePath)
        {
            editorTabs->setCurrentIndex(i);
            return;
        }
    }

    // 打开新标签页
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        CodeEditor *editor = new CodeEditor();
        editor->setPlainText(content);
        editor->setFilePath(filePath);

        QString fileName = info.fileName();
        int tabIndex = editorTabs->addTab(editor, fileName);
        editorTabs->setCurrentIndex(tabIndex);

        // 根据文件扩展名设置语言
        QString suffix = info.suffix().toLower();
        if (suffix == "cpp" || suffix == "c" || suffix == "cc" || suffix == "cxx")
            languageLabel->setText("C++");
        else if (suffix == "h" || suffix == "hpp")
            languageLabel->setText("C++ 头文件");
        else if (suffix == "py")
            languageLabel->setText("Python");
        else if (suffix == "js")
            languageLabel->setText("JavaScript");
        else if (suffix == "ts")
            languageLabel->setText("TypeScript");
        else if (suffix == "json")
            languageLabel->setText("JSON");
        else if (suffix == "md")
            languageLabel->setText("Markdown");
        else if (suffix == "cmake" || suffix == "txt")
            languageLabel->setText("CMake");
        else if (suffix == "bat")
            languageLabel->setText("Batch");
        else
            languageLabel->setText("纯文本");

        updateStatusBar();
    }
    else
    {
        QMessageBox::warning(this, "错误", "无法打开文件: " + filePath);
    }
}

// ============================================================
// 视图切换
// ============================================================

void MainWindow::showFileExplorer()
{
    currentActivity = 0;
    leftSidebar->setVisible(true);
    leftSidebarTabs->setCurrentIndex(0);
    btnExplorer->setProperty("active", "true");
    btnSearch->setProperty("active", "false");
    btnChat->setProperty("active", "false");
    activityBar->style()->unpolish(activityBar);
    activityBar->style()->polish(activityBar);
}

void MainWindow::showSearchPanel()
{
    currentActivity = 1;
    leftSidebar->setVisible(true);

    // 确保搜索标签页存在
    if (leftSidebarTabs->count() < 2)
    {
        searchPage = new QWidget();
        QVBoxLayout *searchLayout = new QVBoxLayout(searchPage);
        searchLayout->setContentsMargins(8, 8, 8, 8);

        QLabel *searchLabel = new QLabel("全局搜索");
        searchLabel->setStyleSheet("color: #CCCCCC; font-size: 13px; font-weight: bold;");
        searchGlobalInput = new QLineEdit();
        searchGlobalInput->setPlaceholderText("输入搜索内容...");
        searchGlobalInput->setStyleSheet(
            "QLineEdit {"
            "    background: #3C3C3C;"
            "    color: #CCCCCC;"
            "    border: 1px solid #555555;"
            "    border-radius: 4px;"
            "    padding: 6px 10px;"
            "    font-size: 12px;"
            "}"
        );

        searchLayout->addWidget(searchLabel);
        searchLayout->addWidget(searchGlobalInput);
        searchLayout->addStretch();
        leftSidebarTabs->addTab(searchPage, "🔍 搜索");
    }

    leftSidebarTabs->setCurrentIndex(1);
    btnExplorer->setProperty("active", "false");
    btnSearch->setProperty("active", "true");
    btnChat->setProperty("active", "false");
    activityBar->style()->unpolish(activityBar);
    activityBar->style()->polish(activityBar);
    searchGlobalInput->setFocus();
}

void MainWindow::showChatPanel()
{
    currentActivity = 2;
    btnExplorer->setProperty("active", "false");
    btnSearch->setProperty("active", "false");
    btnChat->setProperty("active", "true");
    activityBar->style()->unpolish(activityBar);
    activityBar->style()->polish(activityBar);
    chatPanel->setVisible(true);
    inputField->setFocus();
}

void MainWindow::toggleBottomPanel()
{
    showBottomPanel = !showBottomPanel;
    bottomPanel->setVisible(showBottomPanel);

    if (showBottomPanel)
    {
        editorSplitter->setSizes({700, 200});
    }
    else
    {
        editorSplitter->setSizes({editorSplitter->height(), 0});
    }
}

void MainWindow::toggleTerminalTab()
{
    bottomPanel->setVisible(true);
    bottomTabs->setCurrentIndex(0);
    terminalInput->setFocus();
}

void MainWindow::toggleProblemsTab()
{
    bottomPanel->setVisible(true);
    bottomTabs->setCurrentIndex(1);
}

// ============================================================
// 搜索替换功能
// ============================================================

void MainWindow::showSearchBar()
{
    searchBar->setVisible(true);
    searchInput->setFocus();
    searchInput->selectAll();

    // 如果编辑器中有选中文本，自动填充
    CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->currentWidget());
    if (editor)
    {
        QString selected = editor->textCursor().selectedText();
        if (!selected.isEmpty() && !selected.contains('\n'))
        {
            searchInput->setText(selected);
        }
    }
}

void MainWindow::hideSearchBar()
{
    searchBar->setVisible(false);
    // 聚焦回编辑器
    CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->currentWidget());
    if (editor)
        editor->setFocus();
}

void MainWindow::searchNext()
{
    CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->currentWidget());
    if (!editor || searchInput->text().isEmpty())
        return;

    QString searchText = searchInput->text();
    QTextDocument *doc = editor->document();
    QTextCursor cursor = editor->textCursor();

    // 从光标位置开始搜索
    QTextCursor found = doc->find(searchText, cursor);
    if (found.isNull())
    {
        // 从文档开头重新搜索
        QTextCursor startCursor(doc);
        found = doc->find(searchText, startCursor);
        if (found.isNull())
        {
            statusMessage->setText("未找到: " + searchText);
            return;
        }
    }

    editor->setTextCursor(found);
    editor->ensureCursorVisible();
    statusMessage->setText("已找到匹配项");
}

void MainWindow::searchPrevious()
{
    CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->currentWidget());
    if (!editor || searchInput->text().isEmpty())
        return;

    QString searchText = searchInput->text();
    QTextDocument *doc = editor->document();
    QTextCursor cursor = editor->textCursor();

    QTextCursor found = doc->find(searchText, cursor, QTextDocument::FindBackward);
    if (found.isNull())
    {
        // 从文档末尾重新搜索
        QTextCursor endCursor(doc);
        endCursor.movePosition(QTextCursor::End);
        found = doc->find(searchText, endCursor, QTextDocument::FindBackward);
        if (found.isNull())
        {
            statusMessage->setText("未找到: " + searchText);
            return;
        }
    }

    editor->setTextCursor(found);
    editor->ensureCursorVisible();
    statusMessage->setText("已找到匹配项");
}

void MainWindow::replaceCurrent()
{
    CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->currentWidget());
    if (!editor || searchInput->text().isEmpty())
        return;

    QTextCursor cursor = editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == searchInput->text())
    {
        cursor.insertText(replaceInput->text());
        statusMessage->setText("已替换");
    }
    else
    {
        searchNext();
    }
}

void MainWindow::replaceAll()
{
    CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->currentWidget());
    if (!editor || searchInput->text().isEmpty())
        return;

    QString searchText = searchInput->text();
    QString replaceText = replaceInput->text();

    QTextCursor cursor(editor->document());
    int count = 0;

    while (true)
    {
        cursor = editor->document()->find(searchText, cursor);
        if (cursor.isNull())
            break;
        cursor.insertText(replaceText);
        count++;
    }

    statusMessage->setText(QString("已替换 %1 处").arg(count));
}

// ============================================================
// 文件操作
// ============================================================

void MainWindow::openFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "打开文件", currentProjectPath,
                                                    "所有支持的文件 (*.cpp *.h *.hpp *.c *.py *.js *.ts *.txt *.md *.json *.xml);;C++文件 (*.cpp *.h *.hpp);;所有文件 (*.*)");
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "错误", "无法打开文件: " + filePath);
        return;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    CodeEditor *editor = new CodeEditor();
    editor->setPlainText(content);
    editor->setFilePath(filePath);

    QString fileName = QFileInfo(filePath).fileName();
    int index = editorTabs->addTab(editor, fileName);
    editorTabs->setCurrentIndex(index);

    statusMessage->setText("已打开: " + fileName);
    updateStatusBar();
}

void MainWindow::saveFile()
{
    int currentIndex = editorTabs->currentIndex();
    if (currentIndex < 0)
        return;

    CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->widget(currentIndex));
    if (!editor)
        return;

    QString filePath = editor->filePath();
    if (filePath.isEmpty())
    {
        saveAsFile();
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "错误", "无法保存文件: " + filePath);
        return;
    }

    file.write(editor->toPlainText().toUtf8());
    file.close();
    editor->setModified(false);

    statusMessage->setText("已保存: " + QFileInfo(filePath).fileName());
}

void MainWindow::saveAsFile()
{
    int currentIndex = editorTabs->currentIndex();
    if (currentIndex < 0)
        return;

    CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->widget(currentIndex));
    if (!editor)
        return;

    QString filePath = QFileDialog::getSaveFileName(this, "保存文件", currentProjectPath,
                                                    "C++文件 (*.cpp);;头文件 (*.h);;Python (*.py);;文本文件 (*.txt)");
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "错误", "无法保存文件: " + filePath);
        return;
    }

    file.write(editor->toPlainText().toUtf8());
    file.close();
    editor->setFilePath(filePath);
    editor->setModified(false);

    QString fileName = QFileInfo(filePath).fileName();
    editorTabs->setTabText(currentIndex, fileName);
    statusMessage->setText("已保存: " + fileName);
}

void MainWindow::newFile()
{
    CodeEditor *editor = new CodeEditor();
    editor->setPlainText("// 新文件\n\n");
    int index = editorTabs->addTab(editor, "未命名");
    editorTabs->setCurrentIndex(index);
}

// ============================================================
// 聊天功能
// ============================================================

void MainWindow::sendMessage()
{
    QString message = inputField->text().trimmed();
    if (message.isEmpty())
        return;

    inputField->clear();
    appendToChat(message, true);

    if (agentManager && agentManager->isRunning())
    {
        agentManager->sendMessage(message);
        statusMessage->setText("Agent 正在处理...");
    }
    else
    {
        appendToChat("⚠️ Agent 未启动，请检查 API Key 配置", false);
        statusMessage->setText("Agent 未启动");
    }
}

void MainWindow::appendToChat(const QString &message, bool isUser)
{
    if (isUser)
        chatDisplay->appendUserMessage(message);
    else
        chatDisplay->appendAssistantMessage(message);
}

void MainWindow::clearChat()
{
    chatDisplay->clear();
    statusMessage->setText("对话已清空");
}

// ============================================================
// Agent 回调
// ============================================================

void MainWindow::onAgentOutput(const QString &output)
{
    appendToChat(output, false);
    statusMessage->setText("就绪");
}

void MainWindow::onAgentError(const QString &error)
{
    appendToChat("❌ 错误: " + error, false);
    statusMessage->setText("错误: " + error);
}

void MainWindow::onAgentStatusChanged(const QString &status)
{
    statusMessage->setText(status);
}

// ★ 新增：agent 正在重启的回调
void MainWindow::onAgentRestarting()
{
    appendToChat("🔄 Agent 进程正在重启，请稍候...", false);
    statusMessage->setText("Agent 重启中...");
}

void MainWindow::onYCodeSelfUpdateRequested()
{
    appendToChat("YCode 将退出并执行完整自更新：重建 Agent、重建客户端、更新快捷方式，然后重新启动。", false);
    statusMessage->setText("YCode 自更新准备中...");
    startYCodeSelfUpdate();
}

void MainWindow::checkForUpdates()
{
    statusMessage->setText("正在检查更新...");

    bool localOk = false;
    QString localSha = runGitCommand(QStringList() << "rev-parse" << "HEAD", 5000, &localOk).trimmed();
    if (!localOk || localSha.isEmpty())
    {
        QMessageBox::warning(this, "检查更新失败",
                             "当前目录不是有效的 Git 部署，或无法读取本地版本。\n\n"
                             "请使用 git clone 部署项目，或手动从 GitHub 下载最新版。");
        statusMessage->setText("检查更新失败");
        return;
    }

    bool remoteOk = false;
    QString remoteOutput = runGitCommand(QStringList() << "ls-remote" << "origin" << "main", 20000, &remoteOk).trimmed();
    if (!remoteOk || remoteOutput.isEmpty())
    {
        QMessageBox::warning(this, "检查更新失败",
                             "无法访问远端 origin/main。\n\n" + remoteOutput);
        statusMessage->setText("检查更新失败");
        return;
    }

    QString remoteSha = remoteOutput.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).value(0);
    if (remoteSha.isEmpty())
    {
        QMessageBox::warning(this, "检查更新失败", "远端返回内容无法解析:\n\n" + remoteOutput);
        statusMessage->setText("检查更新失败");
        return;
    }

    if (remoteSha == localSha)
    {
        QMessageBox::information(this, "已是最新版",
                                 "当前已经是 origin/main 的最新版。\n\n"
                                 "版本: " + localSha.left(12));
        statusMessage->setText("已是最新版");
        return;
    }

    QMessageBox::StandardButton result = QMessageBox::question(
        this,
        "发现新版本",
        "发现 origin/main 有新版本。\n\n"
        "当前版本: " + localSha.left(12) + "\n"
        "最新版本: " + remoteSha.left(12) + "\n\n"
        "是否立即拉取、重建并重启 YCode？",
        QMessageBox::Yes | QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        statusMessage->setText("已取消更新");
        return;
    }

    startYCodeSelfUpdate(QStringList() << "--pull");
}

bool MainWindow::startYCodeSelfUpdate(const QStringList &arguments)
{
    if (!saveAllModifiedFilesForSelfUpdate())
        return false;

    QString updaterPath = QDir(currentProjectPath).filePath("ycode_self_update.bat");
    if (!QFile::exists(updaterPath))
    {
        appendToChat("错误: 找不到自更新脚本 " + updaterPath, false);
        statusMessage->setText("YCode 自更新失败");
        return false;
    }

    saveSettings();

    if (agentManager)
        agentManager->stop();

    QString command = "\"" + QDir::toNativeSeparators(updaterPath) + "\"";
    for (const QString &argument : arguments)
        command += " " + argument;

    bool started = QProcess::startDetached("cmd.exe", QStringList() << "/c" << command, currentProjectPath);
    if (!started)
    {
        appendToChat("错误: 无法启动自更新脚本 " + updaterPath, false);
        statusMessage->setText("YCode 自更新失败");
        if (agentManager && !apiKey.isEmpty())
            agentManager->start();
        return false;
    }

    selfUpdateInProgress = true;
    statusMessage->setText("YCode 正在退出以完成自更新...");
    QTimer::singleShot(200, qApp, &QApplication::quit);
    return true;
}

// ============================================================
// 编辑器光标变化
// ============================================================

void MainWindow::onEditorCursorChanged()
{
    updateStatusBar();
}

void MainWindow::updateStatusBar()
{
    CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->currentWidget());
    if (editor)
    {
        QTextCursor cursor = editor->textCursor();
        int line = cursor.blockNumber() + 1;
        int col = cursor.columnNumber() + 1;
        lineColLabel->setText(QString("行 %1, 列 %2").arg(line).arg(col));
    }
    else
    {
        lineColLabel->setText("行 1, 列 1");
        languageLabel->setText("纯文本");
    }
}

// ============================================================
// 加载和保存设置
// ============================================================

void MainWindow::loadSettings()
{
    QSettings settings;
    currentProjectPath = settings.value("projectPath", "F:/YiyangzaiCode").toString();

    QByteArray envApiKey = qgetenv("DEEPSEEK_API_KEY");
    if (!envApiKey.isEmpty())
        apiKey = QString::fromUtf8(envApiKey);
    else
        apiKey = settings.value("apiKey", "").toString();

    showBottomPanel = settings.value("showBottomPanel", true).toBool();

    // 更新文件树
    updateFileTree(currentProjectPath);

    // 启动 Agent
    agentManager = new AgentManager(currentProjectPath, apiKey, this);
    connectAgentSignals();  // ★ 使用统一信号连接

    if (!apiKey.isEmpty())
        agentManager->start();
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("projectPath", currentProjectPath);
    settings.setValue("apiKey", apiKey);
    settings.setValue("showBottomPanel", showBottomPanel);
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

bool MainWindow::saveAllModifiedFilesForSelfUpdate()
{
    for (int i = 0; i < editorTabs->count(); ++i)
    {
        CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->widget(i));
        if (!editor || !editor->isModified())
            continue;

        QString filePath = editor->filePath();
        if (filePath.isEmpty())
        {
            appendToChat(QString("YCode 自更新已取消：文件 \"%1\" 尚未保存，请先保存或关闭该标签页。").arg(editorTabs->tabText(i)), false);
            statusMessage->setText("YCode 自更新已取消");
            return false;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            appendToChat("YCode 自更新已取消：无法保存文件 " + filePath, false);
            statusMessage->setText("YCode 自更新已取消");
            return false;
        }

        file.write(editor->toPlainText().toUtf8());
        file.close();
        editor->setModified(false);
    }

    return true;
}

QString MainWindow::runGitCommand(const QStringList &arguments, int timeoutMs, bool *ok)
{
    QProcess process;
    process.setProgram("git");
    process.setArguments(arguments);
    process.setWorkingDirectory(currentProjectPath);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start();

    if (!process.waitForStarted(3000))
    {
        if (ok)
            *ok = false;
        return "git failed to start";
    }

    if (!process.waitForFinished(timeoutMs))
    {
        process.kill();
        process.waitForFinished(1000);
        if (ok)
            *ok = false;
        return "git command timed out";
    }

    QString output = QString::fromUtf8(process.readAll()).trimmed();
    bool success = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    if (ok)
        *ok = success;
    return output;
}

void MainWindow::updateFileTree(const QString &path)
{
    if (fileSystemModel)
    {
        fileSystemModel->setRootPath(path);
        fileTreeView->setRootIndex(fileSystemModel->index(path));
    }
}

// ============================================================
// 关闭事件
// ============================================================

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!selfUpdateInProgress)
    {
        // 保存所有未保存的文件
        for (int i = 0; i < editorTabs->count(); ++i)
        {
            CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->widget(i));
            if (editor && editor->isModified())
            {
                QMessageBox::StandardButton result = QMessageBox::question(
                    this, "保存文件",
                    QString("文件 \"%1\" 已修改，是否保存？").arg(editorTabs->tabText(i)),
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

                if (result == QMessageBox::Save)
                    saveFile();
                else if (result == QMessageBox::Cancel)
                {
                    event->ignore();
                    return;
                }
            }
        }
    }

    if (agentManager)
        agentManager->stop();

    saveSettings();
    event->accept();
}

// ============================================================
// 事件过滤器 (监听全局快捷键和编辑器变化)
// ============================================================

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    return QMainWindow::eventFilter(obj, event);
}
