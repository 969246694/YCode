#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QProcess>
#include <QTreeView>
#include <QFileSystemModel>
#include <QListWidget>
#include <QStatusBar>
#include <QLabel>
#include <QCloseEvent>
#include <QToolBar>
#include <QToolButton>
#include <QPlainTextEdit>
#include <QMenu>
#include <QStringList>

class ChatWidget;
class CodeEditor;
class AgentManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // 文件操作
    void openFile();
    void saveFile();
    void saveAsFile();
    void newFile();
    void onFileTreeDoubleClicked(const QModelIndex &index);

    // 对话操作
    void sendMessage();
    void clearChat();
    void onAgentOutput(const QString &output);
    void onAgentError(const QString &error);
    void onAgentStatusChanged(const QString &status);
    void onAgentRestarting();            // ★ 新增：agent 正在重启
    void onYCodeSelfUpdateRequested();
    void onReloadStyleRequested();
    void checkForUpdates();
    void createGameProject();
    void buildYCodeEngine();
    void openYCodeEngineFolder();
    void sendGameDevPrompt();

    // 视图切换
    void showFileExplorer();
    void showSearchPanel();
    void showChatPanel();
    void toggleBottomPanel();
    void toggleTerminalTab();
    void toggleProblemsTab();

    // 搜索替换
    void showSearchBar();
    void hideSearchBar();
    void searchNext();
    void searchPrevious();
    void replaceCurrent();
    void replaceAll();

    // 编辑器光标变化
    void onEditorCursorChanged();

private:
    void setupUI();
    void setupActivityBar();
    void setupFileExplorer();
    void setupEditorArea();
    void setupChatPanel();
    void setupBottomPanel();
    void setupStatusBar();
    void setupMenuBar();
    void setupToolBar();
    void setupSearchBar();
    void setupCommandPalette();

    void loadSettings();
    void saveSettings();
    void appendToChat(const QString &message, bool isUser = true);
    void updateFileTree(const QString &path);
    void updateStatusBar();
    void connectAgentSignals();          // ★ 抽取：连接 AgentManager 信号
    bool saveAllModifiedFilesForSelfUpdate();
    bool startYCodeSelfUpdate(const QStringList &arguments = QStringList());
    bool reloadStyleSheet(bool notifyUser = true);
    bool writeTextFile(const QString &filePath, const QString &content);
    QString runGitCommand(const QStringList &arguments, int timeoutMs, bool *ok = nullptr);
    QString defaultProjectPath() const;
    QString defaultIconPath() const;
    QString ycodeEnginePath() const;

    // ============ 布局组件 ============
    // 活动栏 (最左侧)
    QToolBar *activityBar;
    QToolButton *btnExplorer;
    QToolButton *btnSearch;
    QToolButton *btnChat;
    QToolButton *btnSettings;

    // 主水平分割器: 左侧面板 | 编辑区 | 聊天面板
    QSplitter *mainSplitter;

    // 左侧面板 (文件浏览器 / 搜索)
    QWidget *leftSidebar;
    QTabWidget *leftSidebarTabs;
    QTreeView *fileTreeView;
    QFileSystemModel *fileSystemModel;
    QWidget *searchPage;
    QLineEdit *searchGlobalInput;

    // 垂直分割器: 编辑区 + 底部面板
    QSplitter *editorSplitter;

    // 编辑器区域
    QWidget *editorArea;
    QTabWidget *editorTabs;

    // 搜索栏 (编辑器内嵌)
    QWidget *searchBar;
    QLineEdit *searchInput;
    QLineEdit *replaceInput;
    QPushButton *btnSearchNext;
    QPushButton *btnSearchPrev;
    QPushButton *btnReplace;
    QPushButton *btnReplaceAll;
    QPushButton *btnCloseSearch;

    // 聊天面板 (右侧)
    QWidget *chatPanel;
    ChatWidget *chatDisplay;
    QLineEdit *inputField;
    QPushButton *sendButton;

    // 底部面板 (终端 / 问题)
    QWidget *bottomPanel;
    QTabWidget *bottomTabs;
    QPlainTextEdit *terminalOutput;
    QLineEdit *terminalInput;
    QListWidget *problemsList;

    // 状态栏组件
    QStatusBar *statusBar;
    QLabel *statusMessage;
    QLabel *lineColLabel;
    QLabel *languageLabel;
    QLabel *encodingLabel;

    // Agent 管理
    AgentManager *agentManager;

    // 设置
    QString currentProjectPath;
    QString apiKey;
    bool showBottomPanel;
    bool selfUpdateInProgress;
    int currentActivity;  // 0=Explorer, 1=Search, 2=Chat
};

#endif
