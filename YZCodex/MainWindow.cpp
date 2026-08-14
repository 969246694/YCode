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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      agentManager(nullptr),
      sidebarTitle(nullptr),
      searchGlobalInput(nullptr),
      chatTitle(nullptr),
      chatInputArea(nullptr),
      outputLog(nullptr),
      showBottomPanel(true),
      selfUpdateInProgress(false),
      assistantStreaming(false),
      currentActivity(0)
{
    QString iconPath = defaultIconPath();
    if (!iconPath.isEmpty())
        setWindowIcon(QIcon(iconPath));

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
    reloadStyleSheet(false);

    setWindowTitle("YCode - AI 编程助手");
    setMinimumSize(1200, 800);
    resize(1400, 900);

    if (apiKey.isEmpty())
    {
        QMessageBox::information(this, "欢迎使用 YCode",
                                 "请先设置 DeepSeek API Key。\n\n"
                                 "推荐设置环境变量 DEEPSEEK_API_KEY；也可以在设置中输入临时 API Key。");
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
    connect(agentManager, &AgentManager::reloadStyleRequested, this, &MainWindow::onReloadStyleRequested);
    connect(agentManager, &AgentManager::assistantStreamStarted, this, &MainWindow::onAssistantStreamStarted);
    connect(agentManager, &AgentManager::assistantStreamEnded, this, &MainWindow::onAssistantStreamEnded);
}

// ============================================================
// 主界面布局
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
        applyPanelThemeToEditor(editor);

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
        applyPanelTheme();
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
    QString filePath = QFileDialog::getOpenFileName(this, "打开文件", activeWorkspacePath(),
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
    applyPanelThemeToEditor(editor);

    QString fileName = QFileInfo(filePath).fileName();
    int index = editorTabs->addTab(editor, fileName);
    editorTabs->setCurrentIndex(index);

    statusMessage->setText("已打开: " + fileName);
    updateStatusBar();
}

bool MainWindow::saveEditorToFile(CodeEditor *editor)
{
    if (!editor)
        return false;

    QString filePath = editor->filePath();
    if (filePath.isEmpty())
    {
        QString chosen = QFileDialog::getSaveFileName(this, "保存文件", activeWorkspacePath(),
                                                      "C++文件 (*.cpp);;头文件 (*.h);;Python (*.py);;文本文件 (*.txt)");
        if (chosen.isEmpty())
            return false;
        filePath = chosen;
        editor->setFilePath(filePath);
        int idx = editorTabs->indexOf(editor);
        if (idx >= 0)
            editorTabs->setTabText(idx, QFileInfo(filePath).fileName());
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "错误", "无法保存文件: " + filePath);
        return false;
    }

    file.write(editor->toPlainText().toUtf8());
    file.close();
    editor->setModified(false);
    return true;
}

void MainWindow::saveFile()
{
    CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->currentWidget());
    if (!editor)
        return;

    if (saveEditorToFile(editor))
        statusMessage->setText("已保存: " + QFileInfo(editor->filePath()).fileName());
}

void MainWindow::saveAsFile()
{
    int currentIndex = editorTabs->currentIndex();
    if (currentIndex < 0)
        return;

    CodeEditor *editor = qobject_cast<CodeEditor *>(editorTabs->widget(currentIndex));
    if (!editor)
        return;

    QString filePath = QFileDialog::getSaveFileName(this, "保存文件", activeWorkspacePath(),
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
    applyPanelThemeToEditor(editor);
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
    assistantStreaming = false; // 新消息开始，终止上一轮流式状态

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
// 游戏开发功能
// ============================================================

void MainWindow::onAgentOutput(const QString &output)
{
    if (assistantStreaming)
        chatDisplay->appendToLastAssistant(output);
    else
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

void MainWindow::onAssistantStreamStarted()
{
    assistantStreaming = true;
    chatDisplay->beginAssistantMessage();
    statusMessage->setText("Agent 正在回复...");
}

void MainWindow::onAssistantStreamEnded()
{
    assistantStreaming = false;
    statusMessage->setText("就绪");
}

void MainWindow::onYCodeSelfUpdateRequested()
{
    appendToChat("YCode 将退出并执行完整自更新：重建 Agent、重建客户端、更新快捷方式，然后重新启动。", false);
    statusMessage->setText("YCode 自更新准备中...");
    startYCodeSelfUpdate();
}

void MainWindow::onReloadStyleRequested()
{
    reloadStyleSheet(true);
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
    loadPanelTheme(settings);
    QString fallbackProjectPath = defaultProjectPath();
    currentProjectPath = settings.value("ycodeRootPath",
                                        settings.value("projectPath", fallbackProjectPath)).toString();
    if (currentProjectPath.isEmpty() || !QDir(currentProjectPath).exists() ||
        !QFileInfo::exists(QDir(currentProjectPath).filePath("agent.cpp")) ||
        !QFileInfo::exists(QDir(currentProjectPath).filePath("YZCodex")))
    {
        currentProjectPath = fallbackProjectPath;
    }

    workspacePath = settings.value("workspacePath").toString();
    if (!workspacePath.isEmpty() && !isYCodeGameProject(workspacePath))
        workspacePath.clear();

    QByteArray envApiKey = qgetenv("DEEPSEEK_API_KEY");
    if (!envApiKey.isEmpty())
        apiKey = QString::fromUtf8(envApiKey);
    else
        apiKey.clear();
    settings.remove("apiKey");

    showBottomPanel = settings.value("showBottomPanel", true).toBool();
    applyPanelTheme();

    // 更新文件树
    updateFileTree(activeWorkspacePath());

    // 启动 Agent
    agentManager = new AgentManager(currentProjectPath, apiKey, this);
    agentManager->setWorkspacePath(workspacePath);
    connectAgentSignals();  // ★ 使用统一信号连接

    if (!apiKey.isEmpty())
        agentManager->start();
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("ycodeRootPath", currentProjectPath);
    settings.setValue("projectPath", currentProjectPath);
    settings.setValue("workspacePath", workspacePath);
    settings.remove("apiKey");
    settings.setValue("showBottomPanel", showBottomPanel);
    savePanelTheme(settings);
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

QString MainWindow::defaultProjectPath() const
{
    QByteArray envRoot = qgetenv("YCODE_PROJECT_ROOT");
    if (!envRoot.isEmpty())
    {
        QString envPath = QDir::fromNativeSeparators(QString::fromUtf8(envRoot));
        if (QDir(envPath).exists())
            return QDir(envPath).absolutePath();
    }

    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 8; ++i)
    {
        QString candidate = dir.absolutePath();
        if (QFileInfo::exists(QDir(candidate).filePath("agent.cpp")) &&
            QFileInfo::exists(QDir(candidate).filePath("YZCodex")))
        {
            return candidate;
        }

        if (!dir.cdUp())
            break;
    }

    return QDir::currentPath();
}

QString MainWindow::defaultIconPath() const
{
    QDir root(defaultProjectPath());
    QString rootIconPath = root.filePath("YCode.ico");
    if (QFileInfo::exists(rootIconPath))
        return rootIconPath;

    QString resourceIconPath = root.filePath("YZCodex/resources/icon.ico");
    if (QFileInfo::exists(resourceIconPath))
        return resourceIconPath;

    return QString();
}

QString MainWindow::ycodeEnginePath() const
{
    QDir currentRoot(currentProjectPath);
    QString currentCandidate = currentRoot.filePath("YCodeEngine");
    if (QFileInfo::exists(currentCandidate))
        return QDir(currentCandidate).absolutePath();

    QDir defaultRoot(defaultProjectPath());
    QString defaultCandidate = defaultRoot.filePath("YCodeEngine");
    if (QFileInfo::exists(defaultCandidate))
        return QDir(defaultCandidate).absolutePath();

    return QDir(currentProjectPath).filePath("YCodeEngine");
}

bool MainWindow::writeTextFile(const QString &filePath, const QString &content)
{
    QFileInfo info(filePath);
    if (!info.absoluteDir().exists() && !info.absoluteDir().mkpath("."))
        return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    file.write(content.toUtf8());
    file.close();
    return true;
}

bool MainWindow::isYCodeGameProject(const QString &path) const
{
    QDir dir(path);
    return QFileInfo::exists(dir.filePath("CMakeLists.txt")) &&
           QFileInfo::exists(dir.filePath("src/main.cpp"));
}

QString MainWindow::activeWorkspacePath() const
{
    if (!workspacePath.isEmpty() && QDir(workspacePath).exists())
        return workspacePath;

    if (!currentProjectPath.isEmpty() && QDir(currentProjectPath).exists())
        return currentProjectPath;

    return defaultProjectPath();
}

QString MainWindow::gameExecutablePath(const QString &projectPath) const
{
    QDir projectDir(projectPath);
    QString projectName = QFileInfo(projectDir.absolutePath()).fileName();
#ifdef Q_OS_WIN
    return projectDir.filePath("build/msvc2022_64/Release/" + projectName + ".exe");
#else
    return projectDir.filePath("build/msvc2022_64/" + projectName);
#endif
}

void MainWindow::runTerminalProcess(const QString &title, const QString &program,
                                    const QStringList &arguments, const QString &workingDirectory)
{
    bottomPanel->setVisible(true);
    showBottomPanel = true;
    bottomTabs->setCurrentIndex(0);
    terminalOutput->appendPlainText("== " + title + " ==");

    QProcess *process = new QProcess(this);
    process->setWorkingDirectory(workingDirectory);
    process->setProcessChannelMode(QProcess::MergedChannels);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("YCODE_NONINTERACTIVE", "1");
    process->setProcessEnvironment(env);

    connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
        terminalOutput->appendPlainText(QString::fromLocal8Bit(process->readAllStandardOutput()));
        QScrollBar *sb = terminalOutput->verticalScrollBar();
        sb->setValue(sb->maximum());
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, title](int exitCode, QProcess::ExitStatus exitStatus) {
        terminalOutput->appendPlainText(QString("== %1 finished: exit %2 ==")
                                            .arg(title)
                                            .arg(exitCode));
        statusMessage->setText(exitStatus == QProcess::NormalExit && exitCode == 0
                                   ? title + " 成功"
                                   : title + " 失败");
        process->deleteLater();
    });

    process->start(program, arguments);
    if (!process->waitForStarted(3000))
    {
        terminalOutput->appendPlainText("Failed to start: " + program + " " + arguments.join(' '));
        statusMessage->setText(title + " 失败");
        process->deleteLater();
    }
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
                {
                    if (!saveEditorToFile(editor))
                    {
                        // 保存被取消或失败：中止关闭
                        event->ignore();
                        return;
                    }
                }
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
