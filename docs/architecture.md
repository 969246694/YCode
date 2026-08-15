# YCode 架构

YCode 是「一个能自我进化的 AI 编程助手」+「一个内置游戏引擎」。三层结构：

```
┌─────────────────────────────────────────────┐
│  YZCodex（Qt 客户端，Windows）               │
│  编辑器 / 文件树 / 终端 / 聊天 / 游戏开发工作区 │
│  实时预览 / 流式气泡 / 主题                    │
└───────────────┬───────────────┬─────────────┘
                │ QProcess + 管道 │
                ▼               ▼
┌───────────────────────┐  ┌────────────────────────┐
│  agent.exe（DeepSeek）│  │  用户游戏项目（C++17）   │
│  23 个工具 / 流式 SSE  │  │  src/ + scenes/ + assets│
│  安全护栏 / 崩溃诊断    │  │  构建时链接 YCodeEngine │
│  会话持久化 / 自更新    │  │                        │
└───────────┬───────────┘  └───────────┬────────────┘
            │                          │
            ▼                          ▼
     DeepSeek API (stream)     YCodeEngine（静态库）
```

## 组件

### agent.exe（`agent.cpp`，单文件命令行程序）

- 通过 stdin/stdout 与客户端对话（托管模式 `YCODE_MANAGED=1` 下不打印 REPL 提示符，输出 `SIGNAL:*` 信号供客户端识别）。
- 工具调用循环：`stream: true` 流式响应，SSE 解析，正文实时打印，`SIGNAL:ASSISTANT_START/END` 包裹一轮回复。
- 23 个工具：文件 / 命令 / 搜索 / Git / 网络 / 任务 / 记忆 / 自更新。
- 安全：危险命令与 `git_commit/git_push` 需授权（`/allow-dangerous`）；shell 元字符注入防护；命令 10 分钟超时；TLS 校验。
- 崩溃诊断：`set_terminate` + 向量化异常处理，写 `agent_crash.log`（配合 PDB 解析符号）。
- 自更新：`apply_self_changes` 按路径热加载/重建/重启；会话每次响应后自动保存、启动时自动恢复。

### YZCodex（Qt 6 客户端，`YZCodex/`）

- `MainWindow.{cpp,Ui,Theme,Game}.cpp`：主窗口按职责拆分（核心逻辑 / 界面搭建 / 主题 / 游戏开发）。
- `AgentManager`：管理 `agent.exe` 子进程，识别 `SIGNAL:*` 信号，流式内容追加到同一气泡。
- `ChatWidget`：聊天气泡，支持流式追加。
- 游戏开发：新建/打开/构建/运行项目、实时预览（`QFileSystemWatcher` 监听源码/场景变更 → 自动重建重启）。

### YCodeEngine（C++17 静态库，`YCodeEngine/`）

- `Scene/Entity/Transform2D` + `properties`（支持按属性检索）。
- `PhysicsWorld2D`：Box2D 封装，盒/圆/胶囊碰撞体、接触与命中事件、射线检测。
- `SceneLoader` / `SceneSaver`：JSON 场景加载与保存（往返兼容）。
- `Texture2D`（GDI+）/ `AudioPlayer`（PlaySound）/ `Canvas2D`（矩形、贴图、文字）。
- `EventBus`、`PluginLoader`（C ABI 插件）、`Window`（Win32/GDI，含 null 实现）。
- 跨平台：Windows 用 GDI+/GDI/winmm，其它平台提供 null 实现。

## 数据流

- **对话**：用户消息 → 客户端写 `agent.exe` stdin → Agent 调 API（流式）→ 逐块写 stdout → 客户端渲染气泡。
- **工具执行**：模型请求工具 → Agent 执行（文件/命令/Git 等）→ 结果写回对话历史 → 模型总结。
- **自我修改**：Agent 改源码 → `apply_self_changes` 判定热加载/重建/重启 → 客户端信号接管。
- **游戏**：客户端生成项目 → CMake 构建链接 YCodeEngine → 运行；预览模式下文件变更触发重建重启。

## 关键约定

- `.bat` 文件用 ASCII（cmd 按 GBK 读取，中文注释会解析崩）。
- 引擎/客户端 CMake 均启用 `/utf-8`（引擎为 PUBLIC，传播给游戏项目）。
- 密钥只从环境变量读取；`agent_session.json` / `agent_tasks.json` / `agent_memory.json` / `agent_crash.log` 不入库。
- CI（`.github/workflows/ci.yml`）：引擎（Ubuntu+Windows）、Agent（Windows）、Qt 客户端（Windows）全部自动构建与测试。
