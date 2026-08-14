# Changelog

本文档记录 YCode 项目的重要变更。版本号遵循 [Keep a Changelog](https://keepachangelog.com/) 风格。

## [Unreleased]

### 安全

- 恢复 `agent.cpp` 中两处 `CURLOPT_SSL_VERIFYPEER` 证书校验（下载与 API 调用均启用 TLS 校验）。
- 新增 `execute_command` 危险命令识别：拦截 `del` / `erase` / `rmdir` / `rd` / `rm` / `format` / `diskpart` / `shutdown` / `taskkill` / `reg` / `setx` / `bcdedit` / `takeown` / `icacls` / `cacls` 以及 PowerShell 的 `Remove-Item`，并能穿透 `cmd /c`、`powershell -command` 等包装前缀。
- `execute_command` 与 `delete_file` 执行破坏性操作前需授权：独立模式交互 `y/n` 确认；托管模式通过 `/allow-dangerous` 授权、`/deny-dangerous` 撤销。
- `search_files` / `search_content` / `list_directory` / `delete_file` 拒绝含 shell 元字符的参数，避免命令注入。
- `download_file` 拒绝覆盖已存在的文件，避免静默覆盖本地文件。
- 移除 `agent.exe` 通过 `argv[1]` 传递 API Key 的方式（密钥仅从环境变量或交互输入读取）。

### 健壮性

- 工具参数解析失败不再中断整轮对话，改为返回可恢复的 `Tool Error` 给模型。
- 模型返回空 `content`、缺失 `arguments` 或缺失 `tool_call_id` 时不再抛异常。
- 非 200 HTTP 响应现在附带响应体片段，便于排查错误。
- 每次 Agent 响应后自动保存会话，避免进程崩溃丢失对话历史。
- 统一历史裁剪阈值（`trimHistory` 与调用处一致保留最近 20 条）。
- 修正系统提示词字符串拼接缺空格的问题，改用 `\n` 分隔。

### YZCodex（Qt 客户端）

- 修复关闭窗口时多标签页场景下保存错文件的问题：`closeEvent` 现在对每个已修改的标签页直接调用 `saveEditorToFile` 保存，而非误存当前标签页；「另存为」被取消时会中止关闭，避免误丢改动。

### YCodeEngine

- `EventBus::publish` 派发前拷贝订阅快照，处理函数在派发过程中订阅/退订不再导致迭代器失效。
- 清理 `SceneLoader` 对 `physics2D` 对象的冗余碰撞体解析，仅从 `box`/`collider` 子对象读取。
- 引擎统一添加 `/utf-8` 编译选项（MSVC），避免中文注释在 GBK 代码页下被误读。

### 测试与 CI

- 新增 `YCodeEngine/tests/engine_tests.cpp`：覆盖 EventBus（含重入退订）、Scene、SceneLoader、ResourceManager、PhysicsWorld2D（含重力下落）、Engine 生命周期与文件读写路径。
- `YCodeEngine/CMakeLists.txt` 接入 CTest，`ctest` 一键运行全部单元测试。
- 新增 `.github/workflows/ci.yml`：每次 push / pull request 自动在 Ubuntu 与 Windows 上构建 YCodeEngine 并运行测试。

### 文档

- 根 `README.md` 增加「测试」「安全护栏」章节。
- `YCodeEngine/README.md` 增加「Tests」章节。
