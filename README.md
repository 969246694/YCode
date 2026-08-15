# YCode

[![CI](https://github.com/969246694/YCode/actions/workflows/ci.yml/badge.svg)](https://github.com/969246694/YCode/actions/workflows/ci.yml)

YCode 是一个 Windows 桌面 AI 编程助手项目，包含：

- `agent.cpp`: 基于 DeepSeek API 的本地命令行 Agent（23 个工具：文件/命令/搜索/Git/联网/任务/记忆/自更新，流式输出，会话自动恢复）。
- `YZCodex/`: 使用 Qt 6 和 C++17 编写的图形客户端（编辑器、文件树、终端、聊天、游戏开发工作区、实时预览）。
- `YCodeEngine/`: YCode 内置 C++17 游戏引擎内核，提供场景、2D 物理（盒/圆/胶囊碰撞、接触与命中事件、射线检测）、贴图绘制、音频、窗口绘制、事件总线、插件 ABI、插件加载器和游戏项目模板。
- `build.bat`、`run_ycode.bat`、`manage_api_key.ps1`: Windows 下的构建、启动和 API Key 管理脚本。

## Agent 工具（23 个）

| 类别 | 工具 |
|---|---|
| 文件 | `read_file` `write_file` `list_directory` `search_files` `search_content` `create_directory` `delete_file` `move_file` `get_file_info` `download_file` |
| 命令 | `execute_command`（危险命令拦截 + 10 分钟超时） |
| 工程 | `git_status` `git_diff` `git_commit` `git_push`（提交/推送需授权） |
| 网络 | `web_search` `fetch_url` |
| 协作 | `tasks`（任务清单） `memory`（长期记忆） `think`（显式思考） |
| 自更新 | `restart_agent` `rebuild_and_restart_ycode` `apply_self_changes` |

## 依赖

- Windows
- Visual Studio 2022 C++ 工具链
- CMake 3.20+
- Qt 6.8+，MSVC 2022 64-bit
- vcpkg 安装的 `libcurl`

`nlohmann/json` 已 vendor 到 `YCodeEngine/third_party/nlohmann/`，用于 Agent JSON 处理和 YCodeEngine 场景加载。`Box2D` 已 vendor 到 `YCodeEngine/third_party/box2d/`，用于 YCodeEngine 2D 物理。

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

## 测试

YCodeEngine 内置了针对 EventBus、Scene、SceneLoader、ResourceManager、PhysicsWorld2D 的单元测试，通过 CTest 运行：

```bat
cd YCodeEngine
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Agent 的安全与解析逻辑测试（危险命令识别 / 注入防护 / SSE 流式解析，需 Windows + vcpkg libcurl）：

```bat
set VCPKG_INSTALLED=C:\vcpkg\installed\x64-windows
cl /EHsc /utf-8 tests\agent_logic_tests.cpp /I YCodeEngine\third_party /I %VCPKG_INSTALLED%\include /link /LIBPATH:%VCPKG_INSTALLED%\lib libcurl.lib shell32.lib /OUT:agent_tests.exe
agent_tests.exe
```

GitHub Actions 会在每次 push / pull request 时自动：在 Ubuntu 与 Windows 上构建 YCodeEngine 并运行测试；在 Windows 上构建 `agent.exe` 并运行 agent 逻辑测试；安装 Qt 6.8 后构建 YZCodex Qt 客户端（见 `.github/workflows/ci.yml`）。

## 配置

运行前需要设置 DeepSeek API Key：

```bat
set DEEPSEEK_API_KEY=your-api-key-here
```

可选环境变量（不设置则用默认值）：

- `YCODE_API_URL`：API 端点地址，默认 `https://api.deepseek.com/v1/chat/completions`（兼容 OpenAI 格式的端点均可）。
- `YCODE_MODEL`：模型名，默认 `deepseek-v4-pro`。

```bat
set YCODE_MODEL=deepseek-chat
```

也可以使用：

```powershell
.\manage_api_key.ps1
```

客户端设置窗口中输入的 API Key 只在当前运行会话中使用，不会写入 Qt 设置文件；重启后建议从 `DEEPSEEK_API_KEY` 环境变量读取。不要把真实 API Key、会话文件或本地构建产物提交到仓库。

## 安全护栏

Agent 的 `execute_command` 与 `delete_file` 工具内置了破坏性操作拦截：

- `execute_command` 执行 `del` / `erase` / `rmdir` / `rd` / `rm` / `format` / `diskpart` / `shutdown` / `taskkill` / `reg` / `setx` / `bcdedit` / `takeown` / `icacls` / `cacls` 以及 PowerShell 的 `Remove-Item` 等命令前会被拦截。
- `delete_file` 删除文件或目录前需要确认。

授权方式：

- 独立模式（终端直接运行 `agent.exe`）：拦截时交互输入 `y` 确认。
- 托管模式（通过 YCode 客户端）：在聊天中发送 `/allow-dangerous` 授权，`/deny-dangerous` 撤销授权。

`search_files` / `search_content` / `list_directory` 等工具会拒绝含 shell 元字符的参数，避免命令注入。

更多安全细节见 [SECURITY.md](SECURITY.md)。

## 游戏开发

YCode 已合并原 `YiyangzaiEngine` 方向，以后游戏开发能力归入同一个 YCode 项目。

Qt 客户端菜单 `游戏开发` 提供：

- 新建 YCode 游戏项目：生成 CMake 项目并链接内置 `YCodeEngine`。
- 打开 YCode 游戏项目：把文件树和终端切换到独立游戏工作区。
- 构建当前游戏项目：运行 CMake configure/build。
- 运行当前游戏项目：启动已构建的游戏可执行文件。
- **实时预览**：一键构建并运行游戏，修改 `src/`、`scenes/` 或 `CMakeLists.txt` 后自动重建并重启（热重载循环）。
- 构建 YCode Engine：在底部终端面板运行 `YCodeEngine/build.bat`。
- 打开引擎源码目录：进入内置引擎内核源码。
- 启动 AI 游戏开发模式：把 Agent 切换到围绕 YCodeEngine 的游戏开发上下文。

YCode 内部区分三个路径：

- `YCode root`: YCode 自身源码根目录，用于 Agent、自更新和 Git 操作。
- `YCodeEngine root`: 内置游戏引擎源码目录。
- `workspace root`: 当前打开的用户游戏项目目录，用于文件树、终端、构建和运行。

`YCodeEngine` 当前包含：

- `EventBus`: 发布/订阅事件总线。
- `Scene` / `Entity` / `Transform2D`: 轻量场景和游戏对象基础层，支持按属性检索实体（`findEntitiesByProperty`）。
- `PhysicsWorld2D` / `BoxCollider2D` / `CircleCollider2D` / `CapsuleCollider2D`: 基于 Box2D 的 2D 刚体物理封装，支持盒/圆/胶囊碰撞体、接触与命中事件回调、射线检测。
- `ResourceManager` / `SceneLoader`: 读取项目资源，并从 JSON 场景文件生成实体与 `physics2D` 刚体声明（`box` / `circle` / `capsule`）。
- `Texture2D` / `AudioPlayer`: 贴图加载绘制（GDI+，PNG/BMP/JPG）与 WAV 音频播放（可循环）。
- `PluginLoader`: 跨平台动态插件加载器。
- `plugin.h`: 稳定 C ABI 插件接口。
- `Engine`: 初始化、tick、shutdown 生命周期。
- `Window` / `Key` / `Canvas2D`: 最小窗口、键盘输入和 2D 绘制封装（矩形与贴图）；Windows 下由 Win32/GDI 实现。

## 自更新

YCode Agent 修改自身源码、Qt 客户端源码、`YCodeEngine`、启动脚本或快捷方式配置后，可以调用 `apply_self_changes` 工具按变更位置自动选择热加载、重建或重启。客户端会退出时，`ycode_self_update.bat` 会依次重建 `agent.exe`、重建 YCode Engine、重建 Qt 客户端、更新桌面快捷方式，然后重新启动 YCode。

只需要重启 Agent 进程时，调用 `restart_agent` 即可。

源码部署者也可以在 YCode 菜单中选择 `帮助 -> 检查更新...`。YCode 会对比本地 Git 版本和 `origin/main`，发现新版本后执行 `git pull --ff-only origin main`，然后走同一套自更新流程。请用 `git clone` 部署项目；直接下载 ZIP 的目录没有 Git 元数据，无法自动拉取最新版。

## 许可证

本项目使用 MIT License，详见 [LICENSE](LICENSE)。
