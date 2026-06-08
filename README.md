# YCode

YCode 是一个 Windows 桌面 AI 编程助手项目，包含：

- `agent.cpp`: 基于 DeepSeek API 的本地命令行 Agent。
- `YZCodex/`: 使用 Qt 6 和 C++17 编写的图形客户端。
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

## 自更新

YCode Agent 修改自身源码、Qt 客户端源码、启动脚本或快捷方式配置后，可以调用 `rebuild_and_restart_ycode` 工具触发完整自更新。客户端会退出，`ycode_self_update.bat` 会依次重建 `agent.exe`、重建 Qt 客户端、更新桌面快捷方式，然后重新启动 YCode。

只需要重启 Agent 进程时，调用 `restart_agent` 即可。

源码部署者也可以在 YCode 菜单中选择 `帮助 -> 检查更新...`。YCode 会对比本地 Git 版本和 `origin/main`，发现新版本后执行 `git pull --ff-only origin main`，然后走同一套自更新流程。请用 `git clone` 部署项目；直接下载 ZIP 的目录没有 Git 元数据，无法自动拉取最新版。

## 许可证

本项目使用 MIT License，详见 [LICENSE](LICENSE)。
