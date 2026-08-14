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
)").arg(safeName, enginePath);

    QString mainCpp = QString(
R"(#include <ycode/core.h>

#include <chrono>
#include <iostream>
#include <thread>

namespace {

void drawBox(ycode::Canvas2D& canvas,
             const ycode::Entity& entity,
             int width,
             int height,
             float boxWidth,
             float boxHeight,
             ycode::Color fill)
{
    float centerX = static_cast<float>(width) * 0.5f + entity.transform.position.x;
    float centerY = static_cast<float>(height) * 0.5f - entity.transform.position.y;
    float left = centerX - boxWidth * 0.5f;
    float top = centerY - boxHeight * 0.5f;

    canvas.fillRect(left, top, boxWidth, boxHeight, fill);
    canvas.strokeRect(left, top, boxWidth, boxHeight, ycode::Color{235, 245, 255, 255}, 2);
}

void drawScene(ycode::Engine& engine, ycode::EntityId playerId, ycode::EntityId groundId, void* nativeDc, int width, int height)
{
    ycode::Canvas2D canvas(nativeDc, width, height);

    if (auto* ground = engine.scene().findEntity(groundId))
        drawBox(canvas, *ground, width, height, 768.0f, 32.0f, ycode::Color{72, 92, 112, 255});

    if (auto* player = engine.scene().findEntity(playerId))
    {
        float size = 48.0f * player->transform.scale.x;
        if (size < 16.0f)
            size = 16.0f;
        drawBox(canvas, *player, width, height, size, size, ycode::Color{54, 162, 235, 255});
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
    engine.events().subscribe("*", [](const ycode::Event& event) {
        if (event.type == "engine.tick")
            return;
        std::cout << "[event] " << event.type << std::endl;
    });

    std::string error;
    if (!engine.initialize(&error))
    {
        std::cerr << "Failed to initialize YCode Engine: " << error << std::endl;
        return 1;
    }

    auto* player = engine.scene().findEntityByName("Player");
    if (!player)
    {
        std::cerr << "Startup scene does not contain 'Player'" << std::endl;
        return 1;
    }

    ycode::EntityId playerId = player->id;
    auto* ground = engine.scene().findEntityByName("Ground");
    if (!ground)
    {
        std::cerr << "Startup scene does not contain 'Ground'" << std::endl;
        return 1;
    }

    ycode::EntityId groundId = ground->id;
    if (!engine.physics().hasBody(playerId) || !engine.physics().hasBody(groundId))
    {
        std::cerr << "Startup scene is missing physics2D bodies" << std::endl;
        return 1;
    }

    engine.scene().setUpdateHandler([&engine, playerId](ycode::Scene& scene, float deltaSeconds) {
        auto* entity = scene.findEntity(playerId);
        if (!entity || !entity->active)
            return;

        float horizontal = 0.0f;
        float vertical = 0.0f;
        if (engine.window().isKeyDown(ycode::Key::Left) || engine.window().isKeyDown(ycode::Key::A))
            horizontal -= 1.0f;
        if (engine.window().isKeyDown(ycode::Key::Right) || engine.window().isKeyDown(ycode::Key::D))
            horizontal += 1.0f;
        if (engine.window().isKeyDown(ycode::Key::Down) || engine.window().isKeyDown(ycode::Key::S))
            vertical -= 1.0f;
        if (engine.window().isKeyDown(ycode::Key::Up) || engine.window().isKeyDown(ycode::Key::W))
            vertical += 1.0f;

        ycode::Vec2 velocity = engine.physics().linearVelocity(playerId);
        velocity.x = horizontal * 4.0f;
        if (vertical != 0.0f)
            velocity.y = vertical * 4.0f;
        engine.physics().setLinearVelocity(playerId, velocity);
    });
    engine.window().setPaintHandler([&engine, playerId, groundId](void* nativeDc, int width, int height) {
        drawScene(engine, playerId, groundId, nativeDc, width, height);
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

- Arrow keys or WASD: drive the loaded `Player` physics body.

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
      "name": "Player",
      "transform": {
        "position": [0.0, 80.0],
        "rotationDegrees": 0.0,
        "scale": [1.0, 1.0]
      },
      "physics2D": {
        "bodyType": "dynamic",
        "box": {
          "halfSizeMeters": [0.375, 0.375],
          "fixedRotation": true
        }
      },
      "properties": {
        "kind": "prototype"
      }
    },
    {
      "name": "Ground",
      "transform": {
        "position": [0.0, -160.0],
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
// Agent 回调
// ============================================================
