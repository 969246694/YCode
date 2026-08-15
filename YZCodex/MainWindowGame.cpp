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
#include <QFileSystemWatcher>

void MainWindow::createGameProject()
{
    QString parentDir = QFileDialog::getExistingDirectory(this, "选择游戏项目父目录", activeWorkspacePath());
    if (parentDir.isEmpty())
        return;

    bool ok = false;
    QString projectName = QInputDialog::getText(this, "新建 YCode 游戏项目",
                                                "项目名称:", QLineEdit::Normal,
                                                "YCodeGame", &ok).trimmed();
    if (!ok || projectName.isEmpty())
        return;

    QString safeName = projectName;
    safeName.replace(QRegularExpression("[^A-Za-z0-9_]"), "_");
    if (safeName.at(0).isDigit())
        safeName.prepend("Game_");

    QString projectDir = QDir(parentDir).filePath(safeName);
    if (QDir(projectDir).exists())
    {
        QMessageBox::warning(this, "项目已存在", "目录已存在: " + projectDir);
        return;
    }

    QDir dir;
    if (!dir.mkpath(QDir(projectDir).filePath("src")) ||
        !dir.mkpath(QDir(projectDir).filePath("assets")) ||
        !dir.mkpath(QDir(projectDir).filePath("scenes")) ||
        !dir.mkpath(QDir(projectDir).filePath("plugins")))
    {
        QMessageBox::warning(this, "创建失败", "无法创建项目目录: " + projectDir);
        return;
    }

    QString enginePath = QDir::fromNativeSeparators(ycodeEnginePath());
    QString cmake = QString(
R"(cmake_minimum_required(VERSION 3.20)
project(%1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("%2" ycode_engine_build)

add_executable(%1
    src/main.cpp
)

target_link_libraries(%1 PRIVATE ycode_engine)
# add_subdirectory 内的 project() 会重置外层 C++ 标准，这里显式指定
target_compile_features(%1 PRIVATE cxx_std_17)
)").arg(safeName, enginePath);

    QString mainCpp = QString(
R"(#include <ycode/core.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

namespace {

void drawBox(ycode::Canvas2D& canvas, const ycode::Entity& entity, int width, int height,
             float boxWidth, float boxHeight, ycode::Color fill)
{
    float centerX = static_cast<float>(width) * 0.5f + entity.transform.position.x;
    float centerY = static_cast<float>(height) * 0.5f - entity.transform.position.y;
    canvas.fillRect(centerX - boxWidth * 0.5f, centerY - boxHeight * 0.5f, boxWidth, boxHeight, fill);
}

void drawCircle(ycode::Canvas2D& canvas, const ycode::Entity& entity, int width, int height,
                float radius, ycode::Color fill)
{
    float centerX = static_cast<float>(width) * 0.5f + entity.transform.position.x;
    float centerY = static_cast<float>(height) * 0.5f - entity.transform.position.y;
    const int segments = 24;
    for (int i = 0; i < segments; ++i)
    {
        float angle = static_cast<float>(i) * 6.2831853f / static_cast<float>(segments);
        float x = centerX + std::cos(angle) * radius;
        float y = centerY + std::sin(angle) * radius;
        canvas.fillRect(x - 3.0f, y - 3.0f, 6.0f, 6.0f, fill);
    }
}

} // namespace

int main()
{
    ycode::EngineConfig config;
    config.appName = "%1";
    config.projectRoot = ".";
    config.startupScenePath = "scenes/main.scene.json";
    config.loadStartupScene = true;
    config.window.title = "%1";
    config.window.width = 1280;
    config.window.height = 720;

    ycode::Engine engine(config);
    std::string error;
    if (!engine.initialize(&error))
    {
        std::cerr << "Failed to initialize YCode Engine: " << error << std::endl;
        return 1;
    }

    auto* ball = engine.scene().findEntityByName("Ball");
    auto* paddle = engine.scene().findEntityByName("Paddle");
    auto* ground = engine.scene().findEntityByName("Ground");
    if (!ball || !paddle || !ground)
    {
        std::cerr << "Startup scene is missing Ball/Paddle/Ground" << std::endl;
        return 1;
    }

    ycode::EntityId ballId = ball->id;
    ycode::EntityId paddleId = paddle->id;
    ycode::EntityId groundId = ground->id;

    // 给球一个初始速度，让它飞起来
    engine.physics().setLinearVelocity(ballId, ycode::Vec2{2.0f, 6.0f});

    // 接触事件：球碰到挡板/地面时打印（打砖块/挡球游戏的记分钩子）
    engine.physics().setContactHandler([&](ycode::EntityId a, ycode::EntityId b, bool begin) {
        if (!begin)
            return;
        if (a == paddleId || b == paddleId)
            std::cout << "[接触] 球碰到挡板！" << std::endl;
        else if (a == groundId || b == groundId)
            std::cout << "[接触] 球落地了。" << std::endl;
    });

    // 键盘控制挡板（Kinematic 刚体：直接改 transform 即可驱动）
    engine.scene().setUpdateHandler([&engine, paddleId](ycode::Scene& scene, float) {
        auto* entity = scene.findEntity(paddleId);
        if (!entity || !entity->active)
            return;
        float horizontal = 0.0f;
        if (engine.window().isKeyDown(ycode::Key::Left) || engine.window().isKeyDown(ycode::Key::A))
            horizontal -= 1.0f;
        if (engine.window().isKeyDown(ycode::Key::Right) || engine.window().isKeyDown(ycode::Key::D))
            horizontal += 1.0f;
        entity->transform.position.x += horizontal * 8.0f;
        if (entity->transform.position.x < -550.0f)
            entity->transform.position.x = -550.0f;
        if (entity->transform.position.x > 550.0f)
            entity->transform.position.x = 550.0f;
    });

    engine.window().setPaintHandler([&engine, ballId, paddleId, groundId](void* nativeDc, int width, int height) {
        ycode::Canvas2D canvas(nativeDc, width, height);
        if (auto* g = engine.scene().findEntity(groundId))
            drawBox(canvas, *g, width, height, 768.0f, 32.0f, ycode::Color{72, 92, 112, 255});
        if (auto* p = engine.scene().findEntity(paddleId))
            drawBox(canvas, *p, width, height, 160.0f, 20.0f, ycode::Color{54, 162, 235, 255});
        if (auto* b = engine.scene().findEntity(ballId))
            drawCircle(canvas, *b, width, height, 24.0f, ycode::Color{255, 200, 60, 255});
    });
    engine.setRenderHandler([&engine]() {
        engine.window().invalidate();
    });

    while (engine.isRunning())
    {
        engine.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    engine.shutdown();
    return 0;
}
)").arg(safeName);

    QString readme = QString(
R"(# %1

This is a YCode game project powered by the built-in YCode Engine.

## Structure

- `src/main.cpp`: game entry point and scene update loop.
- `scenes/main.scene.json`: startup scene loaded by YCode Engine.
- `assets/`: game assets.
- `plugins/`: optional native plugins.

## Controls

- Left / Right arrows (or A / D): move the paddle to bounce the ball.
- Watch the terminal for contact events (ball hits paddle / ground).

## Build

```bat
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
)").arg(safeName);

    QString sceneManifest = QString(
R"({
  "name": "%1 Main Scene",
  "entities": [
    {
      "name": "Ball",
      "transform": {
        "position": [0.0, 100.0],
        "rotationDegrees": 0.0,
        "scale": [1.0, 1.0]
      },
      "physics2D": {
        "bodyType": "dynamic",
        "circle": {
          "radiusMeters": 0.375,
          "restitution": 0.9,
          "friction": 0.1
        }
      },
      "properties": {
        "kind": "ball"
      }
    },
    {
      "name": "Paddle",
      "transform": {
        "position": [0.0, -140.0],
        "rotationDegrees": 0.0,
        "scale": [1.0, 1.0]
      },
      "physics2D": {
        "bodyType": "kinematic",
        "box": {
          "halfSizeMeters": [1.25, 0.15],
          "friction": 0.5
        }
      },
      "properties": {
        "kind": "paddle"
      }
    },
    {
      "name": "Ground",
      "transform": {
        "position": [0.0, -200.0],
        "rotationDegrees": 0.0,
        "scale": [1.0, 1.0]
      },
      "physics2D": {
        "bodyType": "static",
        "box": {
          "halfSizeMeters": [6.0, 0.25],
          "density": 0.0,
          "friction": 0.6
        }
      },
      "properties": {
        "kind": "static"
      }
    }
  ]
}
)").arg(safeName);

    if (!writeTextFile(QDir(projectDir).filePath("CMakeLists.txt"), cmake) ||
        !writeTextFile(QDir(projectDir).filePath("src/main.cpp"), mainCpp) ||
        !writeTextFile(QDir(projectDir).filePath("scenes/main.scene.json"), sceneManifest) ||
        !writeTextFile(QDir(projectDir).filePath("README.md"), readme) ||
        !writeTextFile(QDir(projectDir).filePath("assets/.gitkeep"), "") ||
        !writeTextFile(QDir(projectDir).filePath("plugins/.gitkeep"), ""))
    {
        QMessageBox::warning(this, "创建失败", "项目文件写入失败: " + projectDir);
        return;
    }

    QFile mainFile(QDir(projectDir).filePath("src/main.cpp"));
    if (mainFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        CodeEditor *editor = new CodeEditor();
        editor->setPlainText(QString::fromUtf8(mainFile.readAll()));
        editor->setFilePath(mainFile.fileName());
        applyPanelThemeToEditor(editor);
        int index = editorTabs->addTab(editor, "main.cpp");
        editorTabs->setCurrentIndex(index);
    }

    workspacePath = QDir(projectDir).absolutePath();
    if (agentManager)
        agentManager->setWorkspacePath(workspacePath);
    updateFileTree(workspacePath);
    saveSettings();

    appendToChat("已创建 YCode 游戏项目: " + projectDir, false);
    statusMessage->setText("游戏项目已创建");
}

void MainWindow::openGameProject()
{
    QString dirPath = QFileDialog::getExistingDirectory(this, "打开 YCode 游戏项目", activeWorkspacePath());
    if (dirPath.isEmpty())
        return;

    if (!isYCodeGameProject(dirPath))
    {
        QMessageBox::warning(this, "不是 YCode 游戏项目",
                             "所选目录缺少 CMakeLists.txt 或 src/main.cpp:\n" + dirPath);
        return;
    }

    workspacePath = QDir(dirPath).absolutePath();
    if (agentManager)
        agentManager->setWorkspacePath(workspacePath);
    updateFileTree(workspacePath);
    saveSettings();
    appendToChat("已打开 YCode 游戏项目: " + workspacePath, false);
    statusMessage->setText("游戏项目已打开");
}

void MainWindow::buildGameProject()
{
    if (workspacePath.isEmpty() || !isYCodeGameProject(workspacePath))
    {
        QMessageBox::information(this, "没有游戏项目", "请先在 游戏开发 菜单中新建或打开 YCode 游戏项目。");
        return;
    }

    QString command = "cmake -S . -B build\\msvc2022_64 -A x64 && "
                      "cmake --build build\\msvc2022_64 --config Release";
    runTerminalProcess("Building game project", "cmd.exe", QStringList() << "/c" << command, workspacePath);
}

void MainWindow::runGameProject()
{
    if (workspacePath.isEmpty() || !isYCodeGameProject(workspacePath))
    {
        QMessageBox::information(this, "没有游戏项目", "请先在 游戏开发 菜单中新建或打开 YCode 游戏项目。");
        return;
    }

    QString executablePath = gameExecutablePath(workspacePath);
    if (!QFileInfo::exists(executablePath))
    {
        QMessageBox::information(this, "需要先构建",
                                 "没有找到游戏可执行文件，请先构建当前游戏项目。\n\n" + executablePath);
        return;
    }

    runTerminalProcess("Running game project", executablePath, QStringList(), workspacePath);
}

void MainWindow::buildYCodeEngine()
{
    QString engineDir = ycodeEnginePath();
    QString scriptPath = QDir(engineDir).filePath("build.bat");
    if (!QFileInfo::exists(scriptPath))
    {
        QMessageBox::warning(this, "构建失败", "找不到构建脚本: " + scriptPath);
        return;
    }

    runTerminalProcess("Building YCode Engine", "cmd.exe",
                       QStringList() << "/c" << QDir::toNativeSeparators(scriptPath), engineDir);
}

void MainWindow::openYCodeEngineFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(ycodeEnginePath()));
}

void MainWindow::sendGameDevPrompt()
{
    QString prompt = QString("进入 YCode 游戏开发模式。请基于内置 YCodeEngine 协助我设计、实现和调试游戏项目；"
                             "优先使用 YCodeEngine 的 Scene/Entity/Transform2D、PhysicsWorld2D/BoxCollider2D 2D 物理、ResourceManager/SceneLoader JSON 场景加载、Key 输入枚举、Canvas2D 绘制、事件总线、插件 ABI、CMake 游戏项目模板和 C++17 工作流。"
                             "当前游戏工作区是: %1。")
                         .arg(workspacePath.isEmpty() ? QString("未打开") : workspacePath);
    inputField->setText(prompt);
    sendMessage();
}

// ============================================================
// 实时预览：构建并运行游戏，修改源码/场景后自动重建并重启
// ============================================================
void MainWindow::runGamePreview()
{
    if (workspacePath.isEmpty() || !isYCodeGameProject(workspacePath))
    {
        QMessageBox::information(this, "没有游戏项目", "请先在 游戏开发 菜单中新建或打开 YCode 游戏项目。");
        return;
    }

    stopPreviewProcess();
    terminalOutput->appendPlainText("== 实时预览：开始构建 ==");
    startPreviewBuild();
}

void MainWindow::stopGamePreview()
{
    stopPreviewProcess();
    if (previewBuildProc)
    {
        previewBuildProc->kill();
        previewBuildProc->waitForFinished(1000);
        previewBuildProc->deleteLater();
        previewBuildProc = nullptr;
    }
    if (previewWatcher)
        previewWatcher->removePaths(previewWatcher->files() + previewWatcher->directories());
    statusMessage->setText("预览已停止");
}

void MainWindow::startPreviewBuild()
{
    if (previewBuildProc)
    {
        previewBuildProc->kill();
        previewBuildProc->deleteLater();
        previewBuildProc = nullptr;
    }

    previewBuildProc = new QProcess(this);
    previewBuildProc->setWorkingDirectory(workspacePath);
    previewBuildProc->setProcessChannelMode(QProcess::MergedChannels);

    connect(previewBuildProc, &QProcess::readyReadStandardOutput, this, [this]() {
        terminalOutput->appendPlainText(QString::fromLocal8Bit(previewBuildProc->readAll()));
    });
    connect(previewBuildProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0)
        {
            terminalOutput->appendPlainText("== 构建成功，启动游戏 ==");
            launchPreviewProcess();
            setupPreviewWatcher();
            statusMessage->setText("实时预览运行中：修改源码/场景后自动重建并重启");
        }
        else
        {
            appendToChat("⚠️ 预览构建失败，请查看终端输出", false);
            statusMessage->setText("预览构建失败");
        }
        previewBuildProc->deleteLater();
        previewBuildProc = nullptr;
    });

    QString command = "cmake -S . -B build\\msvc2022_64 -A x64 && "
                      "cmake --build build\\msvc2022_64 --config Release";
    terminalOutput->appendPlainText("> " + command);
    previewBuildProc->start("cmd.exe", QStringList() << "/c" << command);
}

void MainWindow::launchPreviewProcess()
{
    stopPreviewProcess();
    QString exe = gameExecutablePath(workspacePath);
    if (!QFileInfo::exists(exe))
    {
        appendToChat("未找到游戏可执行文件: " + exe, false);
        return;
    }

    previewProcess = new QProcess(this);
    previewProcess->setWorkingDirectory(workspacePath);
    previewProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(previewProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        terminalOutput->appendPlainText(QString::fromLocal8Bit(previewProcess->readAll()));
    });
    connect(previewProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
        terminalOutput->appendPlainText(QString("== 游戏进程退出: %1 ==").arg(exitCode));
    });

    terminalOutput->appendPlainText("== 游戏进程已启动: " + exe + " ==");
    previewProcess->start(exe, QStringList());
}

void MainWindow::stopPreviewProcess()
{
    if (previewProcess)
    {
        if (previewProcess->state() != QProcess::NotRunning)
        {
            previewProcess->kill();
            previewProcess->waitForFinished(2000);
        }
        previewProcess->deleteLater();
        previewProcess = nullptr;
    }
}

void MainWindow::setupPreviewWatcher()
{
    if (!previewWatcher)
    {
        previewWatcher = new QFileSystemWatcher(this);
        connect(previewWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::onGameSourceChanged);
        connect(previewWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::onGameSourceChanged);
    }

    QStringList old = previewWatcher->files() + previewWatcher->directories();
    if (!old.isEmpty())
        previewWatcher->removePaths(old);

    QStringList paths;
    for (const QString &sub : {QString("src"), QString("scenes")})
    {
        QDir dir(QDir(workspacePath).filePath(sub));
        if (!dir.exists())
            continue;
        paths << dir.absolutePath(); // 目录级：新增/删除文件
        for (const QString &f : dir.entryList({"*.cpp", "*.h", "*.hpp", "*.c", "*.json"}, QDir::Files))
            paths << dir.filePath(f); // 文件级：内容修改
    }
    paths << QDir(workspacePath).filePath("CMakeLists.txt");
    previewWatcher->addPaths(paths);
}

void MainWindow::onGameSourceChanged()
{
    if (!previewDebounceTimer)
    {
        previewDebounceTimer = new QTimer(this);
        previewDebounceTimer->setSingleShot(true);
        connect(previewDebounceTimer, &QTimer::timeout, this, &MainWindow::reloadPreviewDebounced);
    }
    previewDebounceTimer->start(800); // 防抖：连续保存只触发一次
}

void MainWindow::reloadPreviewDebounced()
{
    if (workspacePath.isEmpty() || !isYCodeGameProject(workspacePath))
        return;

    terminalOutput->appendPlainText("== 检测到源码/场景变更，重新构建并重启预览 ==");
    startPreviewBuild();
}
