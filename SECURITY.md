# Security Policy

YCode 是一个本地运行的 AI 编程助手，其 Agent 拥有读写文件、执行命令、下载文件等能力。这意味着：**请只在可信环境中运行它，并审慎授权破坏性操作。**

## 内置护栏

- **TLS 证书校验**：Agent 调用 DeepSeek API 与下载文件均启用 `CURLOPT_SSL_VERIFYPEER`，防止中间人攻击。
- **危险命令拦截**：`execute_command` 会拦截删除 / 格式化 / 注册表 / 环境变量 / 关机等命令；`delete_file` 执行前需授权。
- **授权机制**：独立模式（终端）交互输入 `y/n`；托管模式（YCode 客户端）在聊天中发送 `/allow-dangerous` 授权、`/deny-dangerous` 撤销。
- **命令注入防护**：`search_files` / `search_content` / `list_directory` / `delete_file` 会拒绝含 shell 元字符（`" & | ; < > ^ % !` 等）的参数。
- **防止静默覆盖**：`download_file` 拒绝覆盖已存在的文件。
- **密钥保护**：API Key 仅从环境变量或交互输入读取，不写入配置文件，也不通过命令行参数传递。

## 使用建议

- 不要在不可信目录下运行 Agent；Agent 会在其工作目录内执行命令与读写文件。
- 不要用生产环境或包含敏感数据的目录作为工作区。
- 妥善保管 `DEEPSEEK_API_KEY`，不要提交到仓库或分享。
- 谨慎使用 `/allow-dangerous`，用完后可用 `/deny-dangerous` 撤销授权。

## 报告漏洞

如发现安全问题，请通过 GitHub Issues 报告，并避免公开敏感细节。
