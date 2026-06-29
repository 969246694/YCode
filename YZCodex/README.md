# YZCodex - YCode Qt 客户端

YZCodex 是 YCode 的 Windows Qt 图形客户端，提供多标签代码编辑器、文件树、终端面板、搜索替换、DeepSeek Agent 对话界面和 YCode Engine 游戏开发入口。

## 依赖

- Visual Studio 2022 C++ 工具链
- CMake 3.20+
- Qt 6.8+，MSVC 2022 64-bit
- 仓库根目录已构建出的 `agent.exe`

## 构建

如果 Visual Studio 或 Qt 不在默认位置，可先设置：

```bat
set VS_VCVARS64=C:\Path\To\VC\Auxiliary\Build\vcvars64.bat
set QT_DIR=C:\Qt\6.8.0\msvc2022_64
```

先在仓库根目录构建 Agent：

```bat
cd ..
build.bat
```

再构建客户端：

```bat
cd YZCodex
build.bat
```

构建产物：

```text
build\msvc2022_64\Release\YCode.exe
```

## 游戏开发入口

菜单 `游戏开发` 提供：

- 新建 YCode 游戏项目
- 打开 YCode 游戏项目
- 构建当前游戏项目
- 运行当前游戏项目
- 构建 YCode Engine
- 打开引擎源码目录
- 启动 AI 游戏开发模式

YCode 客户端会把 YCode 自身根目录和游戏工作区分开：Agent、自更新和检查更新固定使用 YCode 根目录；文件树、终端、游戏构建和运行使用当前游戏工作区。

新建游戏项目会生成 CMake 入口、`src/main.cpp`、`scenes/main.scene.json`、`assets/` 和 `plugins/`。示例代码会让 YCodeEngine 从 `scenes/main.scene.json` 加载 `Player` 实体，用 `Canvas2D` 在窗口中绘制它，并通过方向键或 WASD 更新它的 `Transform2D`。

## 配置

运行前建议设置 DeepSeek API Key：

```bat
set DEEPSEEK_API_KEY=your-api-key-here
```

也可以在客户端设置窗口中输入临时 API Key。临时 API Key 只在当前运行会话中使用，不会写入 Qt 设置文件。

如果客户端无法自动识别仓库根目录，可设置：

```bat
set YCODE_PROJECT_ROOT=D:\path\to\YCode
```

## 快捷键

- `Ctrl+N`: 新建文件
- `Ctrl+O`: 打开文件
- `Ctrl+S`: 保存文件
- `Ctrl+Shift+S`: 另存为
- `Ctrl+Enter`: 发送消息
- `Ctrl+Shift+P`: 命令面板
- `Ctrl+Shift+F`: 全局搜索

## 自更新

使用 `git clone` 部署时，客户端菜单 `帮助 -> 检查更新...` 会对比本地版本和 `origin/main`。发现新版本后会执行 `git pull --ff-only origin main`，然后重建 Agent 和客户端并重启 YCode。
