# YZCodex - AI 编程助手

基于 DeepSeek V4-Pro 的 Qt 客户端应用程序。

## 功能特性

- 🎨 现代深色主题界面
- 📝 多标签页代码编辑器（带语法高亮和行号）
- 💬 实时对话界面
- 🤖 集成 DeepSeek Agent
- 📁 项目文件浏览器
- 🔧 本地工具调用支持

## 安装依赖

### 1. 安装 Qt 6.8.x

本项目已检测到本地 Qt 安装：

- `C:\Qt\6.8.0\msvc2022_64`

如果你还没有安装 Qt，请使用 Qt 官方安装器安装 **Qt 6.8.x** 和 **MSVC 2022 64-bit**。

### 2. 编译项目

#### 方法一：使用编译脚本（推荐）

```bash
cd F:\YiyangzaiCode\YZCodex
build.bat
```

#### 方法二：手动编译

```bash
cd F:\YiyangzaiCode\YZCodex
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64/lib/cmake" -DCMAKE_BUILD_TYPE=Release -A x64
cmake --build . --config Release
```

### 3. 测试 Agent

#### 方法一：使用编译脚本（推荐）

```bash
cd F:\YiyangzaiCode\YZCodex
build.bat
```

#### 方法二：手动编译

```bash
cd F:\YiyangzaiCode\YZCodex
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### 3. 测试 Agent

```bash
cd F:\YiyangzaiCode\YZCodex
test_agent.bat
```

### 3. 运行程序

```bash
./Release/YZCodex.exe
```

## 配置

### API Key 设置

在运行程序前，请设置 DeepSeek API Key：

```bash
# 方法1：环境变量
set DEEPSEEK_API_KEY=your-api-key-here

# 方法2：在程序设置中配置
```

### 项目结构

```
F:\YiyangzaiCode\YZCodex\
├── CMakeLists.txt          # CMake 配置
├── main.cpp               # 程序入口
├── MainWindow.h/.cpp      # 主窗口
├── ChatWidget.h/.cpp      # 对话组件
├── CodeEditor.h/.cpp      # 代码编辑器
├── AgentManager.h/.cpp    # Agent 管理器
└── resources/
    ├── style.qss          # 样式表
    └── icon.ico           # 应用程序图标
```

## 快捷键

- `Ctrl+N`: 新建文件
- `Ctrl+O`: 打开文件
- `Ctrl+S`: 保存文件
- `Ctrl+Shift+S`: 另存为
- `Ctrl+Enter`: 发送消息

## 工具调用

在对话中使用以下命令调用本地工具：

- `/tool help` - 显示帮助
- `/tool shell <命令>` - 执行 shell 命令
- `/tool ls <目录>` - 列出目录内容
- `/tool cat <文件>` - 查看文件内容
- `/tool env` - 显示环境变量

## 开发说明

- 使用 Qt 6.8+ 和 C++17
- 支持 Windows 平台
- 集成 DeepSeek API
- 深色主题设计

## 许可证

MIT License
