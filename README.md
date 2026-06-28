# YCode

YCode 是一个 Windows 桌面 AI 编程助手项目，包含：

- `agent.cpp`: 基于 DeepSeek API 的本地命令行 Agent。
- `YZCodex/`: 使用 Qt 6 和 C++17 编写的图形客户端。
- `YCodeEngine/`: YCode 内置 C++17 游戏引擎内核，提供事件总线、插件 ABI、插件加载器和游戏项目模板。
- `build.bat`、`run_ycode.bat`、`manage_api_key.ps1`: Windows 下的构建、启动和 API Key 管理脚本。

## 依赖

- Windows
- Visual Studio 2022 C++ 工具链
- CMake 3.20+
- Qt 6.8+，MSVC 2022 64-bit
- vcpkg 安装的 `libcurl`
- `nlohmann/json`

## 构建

构建脚本会自动尝试通过 `vswhere` 查找 Visual Studio。若你的安装路径不是默认位置，可先设置这些环境变量：

```bat
set VS_VCVARS64=C:\Path\To\VC\Auxiliary\Build\vcvars64.bat
set VCPKG_ROOT=C:\vcpkg
set VCPKG_TRIPLET=x64-windows
set QT_DIR=C:\Qt\6.8.0\msvc2022_64
```

先在仓库根目录构建 Agent：

```bat
build.bat
```

构建内置游戏引擎：

```bat
cd YCodeEngine
build.bat
cd ..
```

再构建 Qt 客户端：

```bat
cd YZCodex
build.bat
```

启动时客户端会自动从可执行文件位置向上查找仓库根目录。需要覆盖时可设置：

```bat
set YCODE_PROJECT_ROOT=D:\path\to\YCode
```

## 配置

运行前需要设置 DeepSeek API Key：

```bat
set DEEPSEEK_API_KEY=your-api-key-here
```

也可以使用：

```powershell
.\manage_api_key.ps1
```

客户端设置窗口中输入的 API Key 只在当前运行会话中使用，不会写入 Qt 设置文件；重启后建议从 `DEEPSEEK_API_KEY` 环境变量读取。不要把真实 API Key、会话文件或本地构建产物提交到仓库。

## 游戏开发

YCode 已合并原 `YiyangzaiEngine` 方向，以后游戏开发能力归入同一个 YCode 项目。

Qt 客户端菜单 `游戏开发` 提供：

- 新建 YCode 游戏项目：生成 CMake 项目并链接内置 `YCodeEngine`。
- 打开 YCode 游戏项目：把文件树和终端切换到独立游戏工作区。
- 构建当前游戏项目：运行 CMake configure/build。
- 运行当前游戏项目：启动已构建的游戏可执行文件。
- 构建 YCode Engine：在底部终端面板运行 `YCodeEngine/build.bat`。
- 打开引擎源码目录：进入内置引擎内核源码。
- 启动 AI 游戏开发模式：把 Agent 切换到围绕 YCodeEngine 的游戏开发上下文。

YCode 内部区分三个路径：

- `YCode root`: YCode 自身源码根目录，用于 Agent、自更新和 Git 操作。
- `YCodeEngine root`: 内置游戏引擎源码目录。
- `workspace root`: 当前打开的用户游戏项目目录，用于文件树、终端、构建和运行。

`YCodeEngine` 当前包含：

- `EventBus`: 发布/订阅事件总线。
- `Scene` / `Entity` / `Transform2D`: 轻量场景和游戏对象基础层。
- `PluginLoader`: 跨平台动态插件加载器。
- `plugin.h`: 稳定 C ABI 插件接口。
- `Engine`: 初始化、tick、shutdown 生命周期。
- `Window`: 最小原生窗口层；Windows 下使用 Win32 创建可见游戏窗口。

## 自更新

YCode Agent 修改自身源码、Qt 客户端源码、`YCodeEngine`、启动脚本或快捷方式配置后，可以调用 `apply_self_changes` 工具按变更位置自动选择热加载、重建或重启。客户端会退出时，`ycode_self_update.bat` 会依次重建 `agent.exe`、重建 YCode Engine、重建 Qt 客户端、更新桌面快捷方式，然后重新启动 YCode。

只需要重启 Agent 进程时，调用 `restart_agent` 即可。

源码部署者也可以在 YCode 菜单中选择 `帮助 -> 检查更新...`。YCode 会对比本地 Git 版本和 `origin/main`，发现新版本后执行 `git pull --ff-only origin main`，然后走同一套自更新流程。请用 `git clone` 部署项目；直接下载 ZIP 的目录没有 Git 元数据，无法自动拉取最新版。

## 许可证

本项目使用 MIT License，详见 [LICENSE](LICENSE)。
