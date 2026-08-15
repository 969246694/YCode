#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>
#include "curl/curl.h"
#include "nlohmann/json.hpp"
#include <windows.h>
#include <shellapi.h>

using json = nlohmann::json;

const int MAX_TOOL_ITERATIONS = 10;
const int API_MAX_RETRIES = 3;
const double DEFAULT_TEMPERATURE = 0.8;
const std::string SESSION_FILE = "agent_session.json";
const std::string SIGNAL_RESTART_AGENT = "SIGNAL:RESTART_AGENT";
const std::string SIGNAL_REBUILD_RESTART_YCODE = "SIGNAL:REBUILD_RESTART_YCODE";
const std::string SIGNAL_RELOAD_STYLE = "SIGNAL:RELOAD_STYLE";
const std::string SIGNAL_ASSISTANT_START = "SIGNAL:ASSISTANT_START";
const std::string SIGNAL_ASSISTANT_END = "SIGNAL:ASSISTANT_END";

// 整轮回复（可能跨多个工具迭代）只发射一次 START，避免 GUI 拆出多个气泡
static bool g_assistantStartEmitted = false;

// ============================================================
// HTTP 回调
// ============================================================
size_t WriteToFileCallback(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return fwrite(ptr, size, nmemb, stream);
}

// ============================================================
// 工具函数
// ============================================================
std::string executeShellCommand(const std::string &command)
{
    std::string result;
    FILE *pipe = _popen(command.c_str(), "r");
    if (!pipe) return "Tool Error: 无法打开命令管道。";
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) result += buffer;
    _pclose(pipe);
    if (result.empty()) return "(无输出)";
    if (result.length() > 8000) result = result.substr(0, 8000) + "\n... (截断)";
    return result;
}

// 判断字符串是否包含会破坏 cmd 命令拼接的元字符。
// 注意：* 与 ? 是 dir/findstr 的合法通配符，放行；这里只拦截能逃逸引号或执行命令的字符。
static bool hasShellMetacharacter(const std::string &value)
{
    for (char c : value)
    {
        if (c == '"' || c == '&' || c == '|' || c == ';' || c == '<' ||
            c == '>' || c == '^' || c == '%' || c == '!' ||
            c == '\n' || c == '\r')
            return true;
    }
    return false;
}

std::string readFile(const std::string &filepath)
{
    struct _stat64 st;
    if (_stat64(filepath.c_str(), &st) != 0) return "Tool Error: 无法访问文件 " + filepath;
    if (st.st_size > 10 * 1024 * 1024) return "Tool Error: 文件过大";
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file.is_open()) return "Tool Error: 无法打开文件 " + filepath;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    if (content.empty()) return "(文件为空)";
    if (content.length() > 50000) content = content.substr(0, 50000) + "\n... (截断)";
    return content;
}

bool writeFile(const std::string &filepath, const std::string &content)
{
    std::ofstream file(filepath, std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;
    file << content;
    file.close();
    return true;
}

std::string searchFiles(const std::string &pattern, const std::string &directory)
{
    if (hasShellMetacharacter(pattern) || hasShellMetacharacter(directory))
        return "Tool Error: pattern 或 directory 含非法字符，已拒绝执行（避免命令注入）。";
    std::string cmd = "dir /s /b \"" + directory + "\\" + pattern + "\" 2>nul";
    std::string result = executeShellCommand(cmd);
    if (result == "(无输出)" || result.empty()) return "未找到: " + pattern;
    return result;
}

std::string searchContent(const std::string &text, const std::string &filePattern, const std::string &directory)
{
    if (hasShellMetacharacter(text) || hasShellMetacharacter(filePattern) || hasShellMetacharacter(directory))
        return "Tool Error: text、filePattern 或 directory 含非法字符，已拒绝执行（避免命令注入）。";
    std::string cmd = "findstr /s /n /i /c:\"" + text + "\" \"" + directory + "\\" + filePattern + "\" 2>nul";
    std::string result = executeShellCommand(cmd);
    if (result == "(无输出)" || result.empty()) return "未找到包含 \"" + text + "\" 的内容";
    return result;
}

std::string createDirectory(const std::string &path)
{
    if (CreateDirectoryA(path.c_str(), NULL)) return "OK 目录创建: " + path;
    if (GetLastError() == ERROR_ALREADY_EXISTS) return "目录已存在: " + path;
    return "FAIL 目录创建: " + path;
}

std::string deleteFile(const std::string &path)
{
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return "FAIL: 不存在 " + path;
    if (attrs & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (hasShellMetacharacter(path))
            return "FAIL 目录删除: 路径含非法字符，已拒绝执行（避免命令注入）";
        std::string cmd = "rmdir /s /q \"" + path + "\" 2>nul";
        system(cmd.c_str());
        if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) return "OK 目录删除: " + path;
        return "FAIL 目录删除: " + path;
    }
    if (DeleteFileA(path.c_str())) return "OK 文件删除: " + path;
    return "FAIL 文件删除: " + path;
}

std::string moveFile(const std::string &source, const std::string &destination)
{
    if (MoveFileA(source.c_str(), destination.c_str())) return "OK 移动: " + source + " -> " + destination;
    return "FAIL 移动: " + source;
}

std::string getFileInfo(const std::string &path)
{
    struct _stat64 st;
    if (_stat64(path.c_str(), &st) != 0) return "FAIL 获取信息: " + path;
    std::ostringstream info;
    info << "文件: " << path << "\n";
    info << "大小: " << st.st_size << " 字节";
    double size = (double)st.st_size;
    const char *units[] = {"B", "KB", "MB", "GB"};
    int unitIdx = 0;
    while (size >= 1024.0 && unitIdx < 3) { size /= 1024.0; unitIdx++; }
    info << " (" << std::fixed << std::setprecision(2) << size << " " << units[unitIdx] << ")\n";
    char timeBuf[64];
    struct tm timeinfo;
    localtime_s(&timeinfo, &st.st_mtime);
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    info << "修改: " << timeBuf << "\n";
    info << "类型: " << ((st.st_mode & _S_IFDIR) ? "目录" : "文件");
    return info.str();
}

std::string downloadFile(const std::string &url, const std::string &savePath)
{
    // 防止静默覆盖已存在的文件；如需覆盖请先删除或换用其它路径。
    struct _stat64 st;
    if (_stat64(savePath.c_str(), &st) == 0)
        return "FAIL 下载: 目标文件已存在，已拒绝覆盖: " + savePath;

    CURL *curl = curl_easy_init();
    if (!curl) return "FAIL 下载: 无法初始化 CURL";
    FILE *file = fopen(savePath.c_str(), "wb");
    if (!file) { curl_easy_cleanup(curl); return "FAIL 下载: 无法创建文件"; }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    CURLcode res = curl_easy_perform(curl);
    fclose(file);
    curl_easy_cleanup(curl);
    if (res == CURLE_OK) return "OK 下载: " + savePath;
    remove(savePath.c_str());
    return "FAIL 下载: " + std::string(curl_easy_strerror(res));
}

// ============================================================
// 危险命令识别 — execute_command 的安全护栏
// ============================================================
static bool containsIgnoreCase(const std::string &value, const std::string &needle)
{
    if (needle.empty()) return true;
    if (value.size() < needle.size()) return false;
    for (size_t i = 0; i + needle.size() <= value.size(); ++i)
    {
        bool match = true;
        for (size_t j = 0; j < needle.size(); ++j)
        {
            if (std::tolower((unsigned char)value[i + j]) != std::tolower((unsigned char)needle[j]))
            {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

static bool equalsIgnoreCase(const std::string &a, const std::string &b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
            return false;
    return true;
}

static bool isDangerousCommand(const std::string &command)
{
    // PowerShell 删除 cmdlet，出现在命令任何位置都视为危险
    if (containsIgnoreCase(command, "remove-item"))
        return true;

    static const std::vector<std::string> dangerousTokens = {
        "del", "erase", "rmdir", "rd", "rm", "format", "diskpart",
        "shutdown", "taskkill", "reg", "setx", "bcdedit", "takeown",
        "icacls", "cacls"
    };
    static const std::vector<std::string> wrapperTokens = {
        "cmd", "cmd.exe", "powershell", "powershell.exe", "pwsh",
        "call", "start", "/c", "/k", "-c", "-command", "/command"
    };

    // 按语句分隔符拆分，逐条定位第一个真正的命令 token
    std::string statement;
    for (size_t i = 0; i <= command.size(); ++i)
    {
        char c = (i < command.size()) ? command[i] : '&';
        bool isSep = (c == '&' || c == '|' || c == ';' || c == '\n' || c == '\r' || c == '(' || c == ')');
        if (!isSep)
        {
            statement.push_back(c);
            continue;
        }

        std::istringstream tok(statement);
        std::string token;
        while (tok >> token)
        {
            bool wrapper = false;
            for (const std::string &w : wrapperTokens)
            {
                if (equalsIgnoreCase(token, w)) { wrapper = true; break; }
            }
            if (wrapper)
                continue;

            for (const std::string &d : dangerousTokens)
                if (equalsIgnoreCase(token, d)) return true;
            break;
        }
        statement.clear();
    }
    return false;
}

// ============================================================
// 流式响应（SSE）解析 — 实时打印正文并累积 tool_calls
// ============================================================
struct ToolCallAcc
{
    std::string id;
    std::string name;
    std::string arguments;
};

struct StreamContext
{
    std::string raw;                       // 原始响应体（错误报告用）
    std::string lineBuffer;                // SSE 行缓冲
    std::string content;                   // 累积的正文
    std::vector<ToolCallAcc> toolCallAcc;  // 按 index 累积的 tool_calls
    bool contentStarted = false;           // 是否已开始输出正文（用于包裹流式起止信号）
};

size_t StreamWriteCallback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    StreamContext *ctx = static_cast<StreamContext *>(userdata);
    size_t total = size * nmemb;
    const char *data = static_cast<const char *>(ptr);
    ctx->raw.append(data, total);
    ctx->lineBuffer.append(data, total);

    size_t pos;
    while ((pos = ctx->lineBuffer.find('\n')) != std::string::npos)
    {
        std::string line = ctx->lineBuffer.substr(0, pos);
        ctx->lineBuffer.erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.rfind("data:", 0) != 0)
            continue;

        std::string payload = line.substr(5);
        size_t first = payload.find_first_not_of(' ');
        if (first == std::string::npos)
            continue;
        payload = payload.substr(first);
        if (payload == "[DONE]")
            continue;

        try
        {
            json chunk = json::parse(payload);
            if (!chunk.contains("choices") || chunk["choices"].empty())
                continue;
            const json &delta = chunk["choices"][0]["delta"];

            if (delta.contains("content") && delta["content"].is_string())
            {
                std::string text = delta["content"].get<std::string>();
                if (!ctx->contentStarted)
                {
                    ctx->contentStarted = true;
                    if (!g_assistantStartEmitted)
                    {
                        g_assistantStartEmitted = true;
                        std::cout << SIGNAL_ASSISTANT_START << std::endl;
                    }
                }
                ctx->content += text;
                std::cout << text << std::flush; // 流式打印正文
            }

            if (delta.contains("tool_calls") && delta["tool_calls"].is_array())
            {
                for (const auto &tc : delta["tool_calls"])
                {
                    int index = tc.value("index", 0);
                    if (index < 0 || index > 512)
                        continue; // 防御畸形 index，避免 resize 抛异常
                    if (index >= static_cast<int>(ctx->toolCallAcc.size()))
                        ctx->toolCallAcc.resize(index + 1);
                    ToolCallAcc &acc = ctx->toolCallAcc[index];
                    if (tc.contains("id") && tc["id"].is_string())
                        acc.id = tc["id"].get<std::string>();
                    if (tc.contains("function"))
                    {
                        const json &fn = tc["function"];
                        if (fn.contains("name") && fn["name"].is_string())
                            acc.name = fn["name"].get<std::string>();
                        if (fn.contains("arguments") && fn["arguments"].is_string())
                            acc.arguments += fn["arguments"].get<std::string>();
                    }
                }
            }
        }
        catch (...) { /* 忽略任何异常，避免异常逃逸出 curl 的 C 回调导致进程崩溃 */ }
    }
    return total;
}

// ============================================================
// 网络搜索与文本处理辅助
// ============================================================
static std::string urlEncode(const std::string &value)
{
    std::ostringstream out;
    for (unsigned char c : value)
    {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            out << c;
        else
            out << '%' << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c;
    }
    return out.str();
}

static std::string stripHtmlTags(std::string text)
{
    std::string out;
    out.reserve(text.size());
    bool inTag = false;
    for (char c : text)
    {
        if (c == '<') { inTag = true; continue; }
        if (c == '>') { inTag = false; continue; }
        if (!inTag) out.push_back(c);
    }
    size_t pos;
    while ((pos = out.find("&amp;")) != std::string::npos) out.replace(pos, 5, "&");
    while ((pos = out.find("&lt;")) != std::string::npos) out.replace(pos, 4, "<");
    while ((pos = out.find("&gt;")) != std::string::npos) out.replace(pos, 4, ">");
    while ((pos = out.find("&quot;")) != std::string::npos) out.replace(pos, 6, "\"");
    while ((pos = out.find("&#39;")) != std::string::npos) out.replace(pos, 5, "'");
    while ((pos = out.find("&nbsp;")) != std::string::npos) out.replace(pos, 6, " ");
    return out;
}

static std::string extractSearchResults(const std::string &html)
{
    std::string text = stripHtmlTags(html);
    std::string compact;
    compact.reserve(text.size());
    bool prevSpace = false;
    for (char c : text)
    {
        if (c == '\n' || c == '\r' || c == '\t')
            c = ' ';
        if (c == ' ' && prevSpace)
            continue;
        compact.push_back(c);
        prevSpace = (c == ' ');
    }
    if (compact.length() > 8000)
        compact = compact.substr(0, 8000) + "\n...(截断)";
    return "搜索结果（尽力提取，可能不完整）：\n" + compact;
}

static size_t StringWriteCallback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    std::string *out = static_cast<std::string *>(userdata);
    size_t total = size * nmemb;
    try
    {
        out->append(static_cast<const char *>(ptr), total);
    }
    catch (...)
    {
        return 0; // 通知 curl 传输失败，避免异常逃逸出 C 回调
    }
    return total;
}

static std::string fetchUrlText(const std::string &url)
{
    std::string result;
    CURL *curl = curl_easy_init();
    if (!curl) return "";
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) YCodeAgent");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StringWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return "";
    return result;
}

// ============================================================
// 重启 Agent — 发送信号给 YCode 客户端来接管重启
// ============================================================
// 新的重启策略：
//   agent.exe 不再自己创建批处理脚本重启，而是向 YCode 客户端
//   发送特殊信号 "SIGNAL:RESTART_AGENT"，然后干净退出。
//   YCode 的 AgentManager 检测到该信号后，会杀掉旧进程并启动新的 agent.exe，
//   这样新的 agent.exe 仍然由 YCode 管理，聊天功能不会断连。
//   如果在独立模式运行（无 YCode），则回退到旧的批处理自重启方式。
// ============================================================
std::string restartAgent(bool standalone = false)
{
    const char *managed = std::getenv("YCODE_MANAGED");
    bool managedByYCode = !standalone && managed && std::string(managed) == "1";

    if (managedByYCode)
    {
        // 由 YCode 客户端托管：发送重启信号，让 YCode 来重启 agent
        std::cout << "\n  [Agent 请求 YCode 客户端执行重启...]" << std::endl;
        std::cout << SIGNAL_RESTART_AGENT << std::endl;
        // 保存会话后再退出
        exit(0);
        return "OK 重启信号已发送";  // 不会执行到这里
    }

    // 独立模式：回退到批处理自重启
    char exePath[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        return "FAIL: 无法获取当前程序路径";

    std::string fullPath(exePath);
    size_t lastSlash = fullPath.find_last_of("\\/");
    std::string exeDir = (lastSlash != std::string::npos) ? fullPath.substr(0, lastSlash) : ".";
    std::string exeName = (lastSlash != std::string::npos) ? fullPath.substr(lastSlash + 1) : fullPath;

    std::string batPath = exeDir + "\\_agent_restart.bat";
    std::ofstream bat(batPath);
    if (!bat.is_open())
        return "FAIL: 无法创建重启脚本 " + batPath;

    bat << "@echo off\n";
    bat << "chcp 65001 >nul\n";
    bat << "echo 正在重启 YCode Agent...\n";
    bat << "ping 127.0.0.1 -n 2 >nul\n";
    bat << "cd /d \"" << exeDir << "\"\n";
    bat << "start \"YCode Agent v2.0\" cmd /k \"cd /d " << exeDir << " && " << exeName << "\"\n";
    bat << "ping 127.0.0.1 -n 1 >nul\n";
    bat << "del \"%~f0\" >nul 2>&1\n";
    bat.close();

    std::cout << "\n  [Agent 即将重启... 请稍候]" << std::endl;
    ShellExecuteA(NULL, "open", batPath.c_str(), NULL, exeDir.c_str(), SW_HIDE);
    exit(0);
    return "OK 重启中...";  // 实际上不会执行到这里
}

std::string requestYCodeRebuildAndRestart()
{
    const char *managed = std::getenv("YCODE_MANAGED");
    if (managed && std::string(managed) == "1")
    {
        std::cout << "\n  [Agent 请求 YCode 执行完整自更新：重建 Agent、客户端并更新快捷方式...]" << std::endl;
        std::cout << SIGNAL_REBUILD_RESTART_YCODE << std::endl;
        exit(0);
        return "OK YCode 自更新信号已发送";
    }

    char exePath[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        return "FAIL: 无法获取当前程序路径";

    std::string fullPath(exePath);
    size_t lastSlash = fullPath.find_last_of("\\/");
    std::string exeDir = (lastSlash != std::string::npos) ? fullPath.substr(0, lastSlash) : ".";
    std::string scriptPath = exeDir + "\\ycode_self_update.bat";

    if (GetFileAttributesA(scriptPath.c_str()) == INVALID_FILE_ATTRIBUTES)
        return "FAIL: 找不到自更新脚本 " + scriptPath;

    ShellExecuteA(NULL, "open", scriptPath.c_str(), NULL, exeDir.c_str(), SW_HIDE);
    exit(0);
    return "OK YCode 自更新脚本已启动";
}

// ============================================================
// 构建单个工具定义的辅助函数
// ============================================================
json makeTool(const std::string &name, const std::string &desc, const json &properties, const json &required)
{
    json tool;
    tool["type"] = "function";
    tool["function"]["name"] = name;
    tool["function"]["description"] = desc;
    tool["function"]["parameters"]["type"] = "object";
    tool["function"]["parameters"]["properties"] = properties;
    tool["function"]["parameters"]["required"] = required;
    return tool;
}

json makeProp(const std::string &name, const std::string &type, const std::string &desc)
{
    json prop;
    prop["type"] = type;
    prop["description"] = desc;
    return prop;
}

// ============================================================
// DeepSeek Agent 类
// ============================================================
class DeepSeekAgent
{
private:
    std::string apiKey;
    std::string apiUrl = "https://api.deepseek.com/v1/chat/completions";
    std::string modelName = "deepseek-v4-pro";
    double temperature = DEFAULT_TEMPERATURE;
    std::string projectPath;
    std::vector<std::string> changedPaths;
    bool allowDangerousCommands = false;

    static std::string toLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    static bool startsWith(const std::string &value, const std::string &prefix)
    {
        return value.rfind(prefix, 0) == 0;
    }

    static bool endsWith(const std::string &value, const std::string &suffix)
    {
        return value.size() >= suffix.size() &&
               value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    static bool hasAnySuffix(const std::string &value, const std::vector<std::string> &suffixes)
    {
        for (const std::string &suffix : suffixes)
        {
            if (endsWith(value, suffix))
                return true;
        }
        return false;
    }

    static std::string absoluteWindowsPath(const std::string &path)
    {
        char buffer[MAX_PATH];
        DWORD len = GetFullPathNameA(path.c_str(), MAX_PATH, buffer, nullptr);
        if (len == 0 || len >= MAX_PATH)
            return path;
        return std::string(buffer);
    }

    std::string normalizeChangedPath(const std::string &path) const
    {
        std::string normalized = absoluteWindowsPath(path);
        std::replace(normalized.begin(), normalized.end(), '\\', '/');

        std::string root = absoluteWindowsPath(projectPath.empty() ? "." : projectPath);
        std::replace(root.begin(), root.end(), '\\', '/');

        std::string normalizedLower = toLower(normalized);
        std::string rootLower = toLower(root);
        if (!rootLower.empty() && rootLower != "." && startsWith(normalizedLower, rootLower + "/"))
            normalized = normalized.substr(root.size() + 1);

        while (startsWith(normalized, "./"))
            normalized = normalized.substr(2);

        return normalized;
    }

    void recordChangedPath(const std::string &path)
    {
        std::string normalized = normalizeChangedPath(path);
        if (normalized.empty())
            return;

        if (std::find(changedPaths.begin(), changedPaths.end(), normalized) == changedPaths.end())
            changedPaths.push_back(normalized);
    }

    bool isStyleReloadPath(const std::string &path) const
    {
        std::string lower = toLower(normalizeChangedPath(path));
        return lower == "yzcodex/resources/style.qss" ||
               lower == "resources/style.qss" ||
               endsWith(lower, "/yzcodex/resources/style.qss");
    }

    bool requiresFullYCodeRebuild(const std::string &path) const
    {
        std::string lower = toLower(normalizeChangedPath(path));

        if (lower == "agent.cpp" || lower == "build.bat" ||
            lower == "run_ycode.bat" || lower == "ycode_self_update.bat" ||
            lower == "update_shortcut.ps1" || lower == "manage_api_key.ps1" ||
            lower == "set_api_key_system.bat" || lower == "upgrade_agent.bat" ||
            lower == "ycode.ico")
        {
            return true;
        }

        if (startsWith(lower, "yzcodex/"))
        {
            if (lower == "yzcodex/cmakelists.txt" ||
                lower == "yzcodex/cmakepresets.json" ||
                lower == "yzcodex/ycode.rc" ||
                lower == "yzcodex/ycode.rc.in" ||
                lower == "yzcodex/resources/icon.ico")
            {
                return true;
            }

            return hasAnySuffix(lower, {".cpp", ".h", ".hpp", ".c", ".cc", ".cxx"});
        }

        if (startsWith(lower, "ycodeengine/"))
        {
            if (lower == "ycodeengine/cmakelists.txt" ||
                lower == "ycodeengine/build.bat")
            {
                return true;
            }

            return hasAnySuffix(lower, {".cpp", ".h", ".hpp", ".c", ".cc", ".cxx"});
        }

        return false;
    }

    std::string joinPaths(const std::vector<std::string> &paths) const
    {
        std::ostringstream out;
        for (size_t i = 0; i < paths.size(); ++i)
        {
            if (i > 0)
                out << ", ";
            out << paths[i];
        }
        return out.str();
    }

    std::string applySelfChanges(const json &args)
    {
        if (args.contains("paths") && args["paths"].is_array())
        {
            for (const auto &path : args["paths"])
            {
                if (path.is_string())
                    recordChangedPath(path.get<std::string>());
            }
        }

        if (changedPaths.empty())
        {
            return "没有记录到需要应用的自修改路径。若你通过 execute_command 修改了文件，请把路径传给 apply_self_changes 的 paths 参数。";
        }

        bool needsFullRebuild = false;
        bool needsStyleReload = false;
        for (const std::string &path : changedPaths)
        {
            if (requiresFullYCodeRebuild(path))
                needsFullRebuild = true;
            if (isStyleReloadPath(path))
                needsStyleReload = true;
        }

        std::string summary = joinPaths(changedPaths);
        if (needsFullRebuild)
        {
            std::cout << "\n  [检测到 YCode 自身源码/构建链路变更: " << summary << "]" << std::endl;
            std::cout << "  [将请求客户端执行完整自重建并重启。]" << std::endl;
            return requestYCodeRebuildAndRestart();
        }

        if (needsStyleReload)
        {
            std::cout << "\n  [检测到样式变更，正在请求客户端热加载样式...]" << std::endl;
            std::cout << SIGNAL_RELOAD_STYLE << std::endl;
            changedPaths.clear();
            return "OK 已请求 YCode 热加载样式: " + summary;
        }

        changedPaths.clear();
        return "变更已记录，但不影响当前运行程序，无需重建或热加载: " + summary;
    }

    bool confirmDangerousOperation(const std::string &description)
    {
        if (allowDangerousCommands)
            return true;

        const char *managed = std::getenv("YCODE_MANAGED");
        bool managedByYCode = managed && std::string(managed) == "1";
        if (managedByYCode)
            return false; // 托管模式下无法交互确认，由调用方提示用户 /allow-dangerous

        std::cout << "\n  ⚠ 检测到危险操作: " << description << std::endl;
        std::cout << "  是否允许执行? (y/n): " << std::flush;
        std::string answer;
        std::getline(std::cin, answer);
        return answer == "y" || answer == "Y" || answer == "yes" || answer == "YES" || answer == "是";
    }

    std::string toolNamesList()
    {
        std::string names;
        for (const auto &t : getTools())
        {
            if (!names.empty()) names += ", ";
            names += t["function"].value("name", std::string("?"));
        }
        return names;
    }

    // ---- Git 工具 ----
    std::string gitStatusTool()
    {
        return executeShellCommand("git -C \"" + projectPath + "\" status --short");
    }

    std::string gitDiffTool()
    {
        return executeShellCommand("git -C \"" + projectPath + "\" diff --stat -- .");
    }

    std::string gitCommitTool(const std::string &message)
    {
        if (message.empty())
            return "Tool Error: commit message 不能为空。";
        if (!confirmDangerousOperation("git_commit: " + message))
            return "已拦截 git_commit（会修改仓库状态）。若用户确实需要，请 /allow-dangerous 授权后重试。";
        std::string msgFile = projectPath + "\\.git_commit_msg.tmp";
        if (!writeFile(msgFile, message))
            return "FAIL: 无法写入提交消息文件";
        std::string cmd = "git -C \"" + projectPath + "\" add -A && git -C \"" + projectPath + "\" commit -F \"" + msgFile + "\"";
        std::string result = executeShellCommand(cmd);
        DeleteFileA(msgFile.c_str());
        return result;
    }

    std::string gitPushTool()
    {
        if (!confirmDangerousOperation("git_push（推送到远程仓库）"))
            return "已拦截 git_push。若用户确实需要，请 /allow-dangerous 授权后重试。";
        std::string branch = executeShellCommand("git -C \"" + projectPath + "\" rev-parse --abbrev-ref HEAD");
        while (!branch.empty() && (branch.back() == '\n' || branch.back() == '\r'))
            branch.pop_back();
        if (branch.empty() || branch == "(无输出)")
            return "FAIL: 无法获取当前分支";
        return executeShellCommand("git -C \"" + projectPath + "\" push origin \"" + branch + "\"");
    }

    // ---- 联网搜索 ----
    std::string webSearchTool(const std::string &query)
    {
        std::string url = "https://www.baidu.com/s?wd=" + urlEncode(query);
        std::string html = fetchUrlText(url);
        if (html.empty())
            return "web_search: 无法访问搜索服务（可能网络受限或超时）。";
        return extractSearchResults(html);
    }

    // ---- 任务清单 ----
    std::string tasksTool(const json &args)
    {
        std::string action = args.value("action", std::string("list"));
        std::string text = args.value("text", std::string(""));
        int index = args.value("index", 0);

        std::string file = projectPath + "\\agent_tasks.json";
        json data = json::array();
        std::string content = readFile(file);
        if (content.find("Tool Error") == std::string::npos && !content.empty())
        {
            try { data = json::parse(content); }
            catch (...) { data = json::array(); }
        }

        if (action == "add")
        {
            if (text.empty()) return "Tool Error: add 需要 text 参数。";
            json task;
            task["text"] = text;
            task["done"] = false;
            data.push_back(task);
            writeFile(file, data.dump(2));
            return "已添加任务: " + text + "（共 " + std::to_string(data.size()) + " 项）";
        }
        if (action == "done")
        {
            if (index < 0 || index >= static_cast<int>(data.size())) return "Tool Error: index 越界。";
            data[index]["done"] = true;
            writeFile(file, data.dump(2));
            return "已完成任务 " + std::to_string(index) + ": " + data[index].value("text", std::string(""));
        }
        if (data.empty()) return "任务列表为空。";
        std::string out = "任务列表：\n";
        for (size_t i = 0; i < data.size(); ++i)
        {
            bool done = data[i].value("done", false);
            out += std::to_string(i + 1) + ". [" + (done ? "x" : " ") + "] " +
                   data[i].value("text", std::string("")) + "\n";
        }
        return out;
    }

    // ---- 长期记忆 ----
    std::string memoryTool(const json &args)
    {
        std::string action = args.value("action", std::string("recall"));
        std::string key = args.value("key", std::string("note"));
        std::string text = args.value("text", std::string(""));

        std::string file = projectPath + "\\agent_memory.json";
        json data = json::object();
        std::string content = readFile(file);
        if (content.find("Tool Error") == std::string::npos && !content.empty())
        {
            try { data = json::parse(content); }
            catch (...) { data = json::object(); }
        }

        if (action == "save")
        {
            if (text.empty()) return "Tool Error: save 需要 text 参数。";
            data["notes"][key] = text;
            writeFile(file, data.dump(2));
            return "已保存记忆 [" + key + "]。";
        }
        if (data.contains("notes") && data["notes"].is_object() && !data["notes"].empty())
        {
            std::string out = "记忆内容：\n";
            for (auto it = data["notes"].begin(); it != data["notes"].end(); ++it)
                out += "[" + it.key() + "] " + it.value().get<std::string>() + "\n";
            return out;
        }
        return "暂无记忆。";
    }

    json getTools()
    {
        json tools = json::array();

        tools.push_back(makeTool("read_file", "读取指定文件的内容",
            json::object({{"filepath", makeProp("filepath", "string", "要读取的文件路径，例如 agent.cpp")}}),
            json::array({"filepath"})));

        tools.push_back(makeTool("write_file", "写入内容到指定文件（会覆盖原文件）",
            json::object({
                {"filepath", makeProp("filepath", "string", "要写入的文件路径")},
                {"content", makeProp("content", "string", "要写入的文件内容")}
            }),
            json::array({"filepath", "content"})));

        tools.push_back(makeTool("list_directory", "列出目录中的文件和子文件夹",
            json::object({
                {"path", makeProp("path", "string", "要列出的目录路径，默认当前目录")}
            }),
            json::array()));

        tools.push_back(makeTool("execute_command", "执行系统命令。删除/格式化/注册表/环境变量/关机等危险命令会被拦截并要求用户授权（用户可发 /allow-dangerous）。",
            json::object({
                {"command", makeProp("command", "string", "要执行的命令")}
            }),
            json::array({"command"})));

        tools.push_back(makeTool("search_files", "递归搜索匹配文件名模式的文件",
            json::object({
                {"pattern", makeProp("pattern", "string", "文件名模式，如 *.cpp")},
                {"directory", makeProp("directory", "string", "搜索的起始目录")}
            }),
            json::array({"pattern"})));

        tools.push_back(makeTool("search_content", "在文件中搜索指定文本内容",
            json::object({
                {"text", makeProp("text", "string", "要搜索的文本")},
                {"filePattern", makeProp("filePattern", "string", "文件名模式")},
                {"directory", makeProp("directory", "string", "搜索的起始目录")}
            }),
            json::array({"text", "filePattern"})));

        tools.push_back(makeTool("create_directory", "创建新目录",
            json::object({
                {"path", makeProp("path", "string", "要创建的目录路径")}
            }),
            json::array({"path"})));

        tools.push_back(makeTool("delete_file", "删除文件或目录（谨慎使用！）",
            json::object({
                {"path", makeProp("path", "string", "要删除的文件或目录路径")}
            }),
            json::array({"path"})));

        tools.push_back(makeTool("move_file", "移动或重命名文件/目录",
            json::object({
                {"source", makeProp("source", "string", "源文件路径")},
                {"destination", makeProp("destination", "string", "目标路径")}
            }),
            json::array({"source", "destination"})));

        tools.push_back(makeTool("get_file_info", "获取文件详细信息（大小、修改时间等）",
            json::object({
                {"path", makeProp("path", "string", "文件或目录路径")}
            }),
            json::array({"path"})));

        tools.push_back(makeTool("download_file", "从URL下载文件到本地",
            json::object({
                {"url", makeProp("url", "string", "下载链接")},
                {"savePath", makeProp("savePath", "string", "本地保存路径")}
            }),
            json::array({"url", "savePath"})));

        // ★ 重启工具 — 通过信号让 YCode 客户端来重启 agent
        tools.push_back(makeTool("restart_agent", "重启 YCode Agent 自身。发送信号给 YCode 客户端，由客户端负责重启 agent 进程，保持连接不断。",
            json::object({}),
            json::array()));

        tools.push_back(makeTool("rebuild_and_restart_ycode", "修改 YCode 自身文件后执行完整自更新：重建 agent.exe、重建 Qt 客户端、更新桌面快捷方式，并重启整个 YCode。",
            json::object({}),
            json::array()));

        json changedPathsProp;
        changedPathsProp["type"] = "array";
        changedPathsProp["description"] = "可选。通过 execute_command 等非文件工具改动过的路径列表。write_file、move_file、delete_file、download_file 会自动记录。";
        changedPathsProp["items"]["type"] = "string";
        tools.push_back(makeTool("apply_self_changes", "根据本轮已修改路径自动应用 YCode 自身变化：样式文件热加载；C++/CMake/脚本/图标等自身代码变化触发完整重建并重启。",
            json::object({{"paths", changedPathsProp}}),
            json::array()));

        // ---- Git 工具 ----
        tools.push_back(makeTool("git_status", "查看 YCode 仓库的 git 状态（未提交的改动）。",
            json::object(), json::array()));
        tools.push_back(makeTool("git_diff", "查看未提交改动的统计摘要（git diff --stat）。",
            json::object(), json::array()));
        tools.push_back(makeTool("git_commit", "把所有改动暂存并提交到 git。会修改仓库状态，需用户授权。",
            json::object({
                {"message", makeProp("message", "string", "提交说明（subject）")}
            }),
            json::array({"message"})));
        tools.push_back(makeTool("git_push", "把本地提交推送到远程仓库 origin。会修改远程状态，需用户授权。",
            json::object(), json::array()));

        // ---- 联网搜索 ----
        tools.push_back(makeTool("web_search", "在网络上搜索关键词（尽力提取结果，可能不完整）。",
            json::object({
                {"query", makeProp("query", "string", "搜索关键词")}
            }),
            json::array({"query"})));

        // ---- 任务清单 ----
        tools.push_back(makeTool("tasks", "管理任务清单：action 为 add（新增，需 text）/ done（标记完成，需 index）/ list（默认）。",
            json::object({
                {"action", makeProp("action", "string", "add / done / list")},
                {"text", makeProp("text", "string", "任务内容（action=add 时必填）")},
                {"index", makeProp("index", "integer", "任务序号，从 0 开始（action=done 时必填）")}
            }),
            json::array()));

        // ---- 长期记忆 ----
        tools.push_back(makeTool("memory", "管理长期记忆：action 为 save（保存，需 key+text）/ recall（默认，查看全部）。",
            json::object({
                {"action", makeProp("action", "string", "save / recall")},
                {"key", makeProp("key", "string", "记忆键名")},
                {"text", makeProp("text", "string", "记忆内容（action=save 时必填）")}
            }),
            json::array()));

        return tools;
    }

    std::string executeTool(const std::string &toolName, const json &args)
    {
        if (toolName == "read_file")
            return readFile(args["filepath"]);
        if (toolName == "write_file")
        {
            std::string filepath = args["filepath"].get<std::string>();
            if (writeFile(filepath, args["content"].get<std::string>()))
            {
                recordChangedPath(filepath);
                return "OK 写入: " + filepath;
            }
            return "FAIL 写入: " + filepath;
        }
        if (toolName == "list_directory")
        {
            std::string path = args.contains("path") ? args["path"].get<std::string>() : ".";
            if (hasShellMetacharacter(path))
                return "Tool Error: path 含非法字符，已拒绝执行（避免命令注入）。";
            return executeShellCommand("dir \"" + path + "\"");
        }
        if (toolName == "execute_command")
        {
            std::string cmd = args["command"];
            std::cout << "\n  [执行: " << cmd << "]" << std::endl;
            if (isDangerousCommand(cmd) && !confirmDangerousOperation("execute_command: " + cmd))
                return "已拦截危险命令: " + cmd +
                       "\n该命令属于删除/格式化/注册表/环境变量/关机等破坏性操作。"
                       "若用户确实要求执行，请先告知用户将要执行的操作，并请用户在聊天中输入 /allow-dangerous 授权后再重试。";
            return executeShellCommand(cmd);
        }
        if (toolName == "search_files")
        {
            std::string dir = args.contains("directory") ? args["directory"].get<std::string>() : ".";
            return searchFiles(args["pattern"], dir);
        }
        if (toolName == "search_content")
        {
            std::string dir = args.contains("directory") ? args["directory"].get<std::string>() : ".";
            return searchContent(args["text"], args["filePattern"], dir);
        }
        if (toolName == "create_directory")
        {
            std::string path = args["path"].get<std::string>();
            std::string result = createDirectory(path);
            if (startsWith(result, "OK"))
                recordChangedPath(path);
            return result;
        }
        if (toolName == "delete_file")
        {
            std::string path = args["path"].get<std::string>();
            if (!confirmDangerousOperation("delete_file: " + path))
                return "已拦截删除操作: " + path +
                       "\n删除文件或目录属于破坏性操作。若用户确实要求删除，请先告知用户将要删除的内容，并请用户在聊天中输入 /allow-dangerous 授权后再重试。";
            std::string result = deleteFile(path);
            if (startsWith(result, "OK"))
                recordChangedPath(path);
            return result;
        }
        if (toolName == "move_file")
        {
            std::string source = args["source"].get<std::string>();
            std::string destination = args["destination"].get<std::string>();
            std::string result = moveFile(source, destination);
            if (startsWith(result, "OK"))
            {
                recordChangedPath(source);
                recordChangedPath(destination);
            }
            return result;
        }
        if (toolName == "get_file_info")
            return getFileInfo(args["path"]);
        if (toolName == "download_file")
        {
            std::cout << "\n  [下载: " << args["url"].get<std::string>() << "]" << std::endl;
            std::string savePath = args["savePath"].get<std::string>();
            std::string result = downloadFile(args["url"].get<std::string>(), savePath);
            if (startsWith(result, "OK"))
                recordChangedPath(savePath);
            return result;
        }
        if (toolName == "restart_agent")
        {
            return restartAgent(false);  // 托管模式：发送信号给 YCode
        }
        if (toolName == "rebuild_and_restart_ycode")
        {
            return requestYCodeRebuildAndRestart();
        }
        if (toolName == "apply_self_changes")
        {
            return applySelfChanges(args);
        }
        if (toolName == "git_status")
        {
            return gitStatusTool();
        }
        if (toolName == "git_diff")
        {
            return gitDiffTool();
        }
        if (toolName == "git_commit")
        {
            return gitCommitTool(args.value("message", std::string("")));
        }
        if (toolName == "git_push")
        {
            return gitPushTool();
        }
        if (toolName == "web_search")
        {
            return webSearchTool(args.value("query", std::string("")));
        }
        if (toolName == "tasks")
        {
            return tasksTool(args);
        }
        if (toolName == "memory")
        {
            return memoryTool(args);
        }
        return "未知工具: " + toolName;
    }

    std::string callAPIStream(const json &messages, StreamContext &ctx, int retryCount = 0)
    {
        CURL *curl = curl_easy_init();
        if (!curl) return "Error: CURL init failed";

        json requestJson;
        requestJson["model"] = modelName;
        requestJson["messages"] = messages;
        requestJson["tools"] = getTools();
        requestJson["tool_choice"] = "auto";
        requestJson["stream"] = true;
        requestJson["temperature"] = temperature;
        requestJson["max_tokens"] = 8192;

        std::string postData = requestJson.dump();
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StreamWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        bool nothingReceived = ctx.content.empty() && ctx.toolCallAcc.empty();

        if (res != CURLE_OK)
        {
            if (retryCount < API_MAX_RETRIES && nothingReceived)
            {
                ctx.raw.clear();
                ctx.lineBuffer.clear();
                std::cerr << "  重试 (" << (retryCount + 1) << "/" << API_MAX_RETRIES << ")..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (retryCount + 1)));
                return callAPIStream(messages, ctx, retryCount + 1);
            }
            return "CURL Error: " + std::string(curl_easy_strerror(res));
        }

        if (httpCode == 429 || httpCode >= 500)
        {
            if (retryCount < API_MAX_RETRIES && nothingReceived)
            {
                ctx.raw.clear();
                ctx.lineBuffer.clear();
                std::cerr << "  HTTP " << httpCode << " 重试..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(2000 * (retryCount + 1)));
                return callAPIStream(messages, ctx, retryCount + 1);
            }
        }

        if (httpCode != 200)
        {
            std::string body = ctx.raw;
            if (body.length() > 500)
                body = body.substr(0, 500) + "...";
            return "HTTP Error: " + std::to_string(httpCode) +
                   (body.empty() ? "" : " - " + body);
        }

        return "";
    }

public:
    DeepSeekAgent(const std::string &key, const std::string &projectDir = ".")
        : apiKey(key), projectPath(projectDir)
    {
        // 支持通过环境变量覆盖 API 地址与模型（便于切换到兼容端点或其它模型）
        const char *envUrl = std::getenv("YCODE_API_URL");
        if (envUrl && *envUrl)
            apiUrl = envUrl;
        const char *envModel = std::getenv("YCODE_MODEL");
        if (envModel && *envModel)
            modelName = envModel;
    }

    void setTemperature(double t) { temperature = t; }
    void setModel(const std::string &m) { modelName = m; }
    void setAllowDangerousCommands(bool allow) { allowDangerousCommands = allow; }
    bool dangerousCommandsAllowed() const { return allowDangerousCommands; }
    size_t toolCount() { return getTools().size(); }

    std::string chat(const std::string &userMessage, std::vector<json> &conversationHistory)
    {
        try
        {
        if (!userMessage.empty())
            conversationHistory.push_back({{"role", "user"}, {"content", userMessage}});

        g_assistantStartEmitted = false; // 新一轮回复：允许发射一次 START

        for (int iteration = 0; iteration < MAX_TOOL_ITERATIONS; iteration++)
        {
            json messages = json::array();
            messages.push_back({{"role", "system"}, {"content", getSystemPrompt()}});
            for (const auto &msg : conversationHistory)
                messages.push_back(msg);

            StreamContext ctx;
            std::string apiError = callAPIStream(messages, ctx);
            if (!apiError.empty())
                return apiError;

            // 依据流式累积结果重建 assistant 消息
            json assistantMsg;
            assistantMsg["role"] = "assistant";
            assistantMsg["content"] = ctx.content.empty() ? json(nullptr) : json(ctx.content);

            json toolCalls = json::array();
            for (const auto &acc : ctx.toolCallAcc)
            {
                json tc;
                tc["id"] = acc.id;
                tc["type"] = "function";
                tc["function"]["name"] = acc.name;
                tc["function"]["arguments"] = acc.arguments.empty() ? std::string("{}") : acc.arguments;
                toolCalls.push_back(tc);
            }
            if (!toolCalls.empty())
                assistantMsg["tool_calls"] = toolCalls;

            conversationHistory.push_back(assistantMsg);

            if (!toolCalls.empty())
            {
                if (iteration == 0 && !userMessage.empty())
                    std::cout << "\n  [Agent 正在使用工具...]" << std::endl;

                for (const auto &tc : toolCalls)
                {
                    const json &function = tc["function"];
                    std::string toolName = function.value("name", std::string("unknown_tool"));
                    std::cout << "  调用: " << toolName << std::endl;

                    std::string toolResult;
                    try
                    {
                        std::string argsStr = function.value("arguments", std::string("{}"));
                        json toolArgs = json::parse(argsStr);
                        toolResult = executeTool(toolName, toolArgs);
                    }
                    catch (const std::exception &e)
                    {
                        toolResult = "Tool Error: " + std::string(e.what());
                    }
                    catch (...)
                    {
                        toolResult = "Tool Error: 未知异常";
                    }

                    conversationHistory.push_back({{"role", "tool"},
                                                   {"tool_call_id", tc.value("id", std::string("unknown_id"))},
                                                   {"content", toolResult}});
                }
                continue;
            }

            // 正文已在流式回调中打印，返回空串避免二次输出
            if (ctx.contentStarted)
                std::cout << SIGNAL_ASSISTANT_END << std::endl;
            if (conversationHistory.size() > 20)
                trimHistory(conversationHistory);
            return "";
        }
        return "达到最大工具调用次数限制";
        }
        catch (const std::exception &e)
        {
            return "Agent Error: " + std::string(e.what());
        }
        catch (...)
        {
            return "Agent Error: 未知异常";
        }
    }

    std::string getSystemPrompt()
    {
        const char *workspaceRoot = std::getenv("YCODE_WORKSPACE_ROOT");
        std::string workspaceInfo = workspaceRoot && *workspaceRoot
            ? "当前游戏工作区是: " + std::string(workspaceRoot) + "。"
            : "当前没有打开独立游戏工作区。";

        return std::string("你是 YCode Agent v2.0，运行在 Yiyangzai 自制的编程工具中。\n") +
               "你有" + std::to_string(getTools().size()) + "个工具: " + toolNamesList() + "。\n" +
               "YCode 已内置 YCodeEngine，具备 C++17 游戏引擎、Scene/Entity/Transform2D 场景层、ResourceManager/SceneLoader JSON 场景加载、原生窗口层、Key 输入枚举、Canvas2D 绘制、事件总线、插件 ABI、游戏项目模板和构建工作流。\n" +
               "YCODE_PROJECT_ROOT 是 YCode 自身源码根目录；YCODE_WORKSPACE_ROOT 是用户游戏项目目录。\n" +
               workspaceInfo + "\n" +
               "修改代码前先读取原文件，用write_file写入完整内容。用中文回答，自信幽默。\n" +
               "增强能力：git_status/git_diff 查看仓库状态；git_commit/git_push 提交并推送（会修改仓库，需用户授权）；web_search 联网搜索；tasks 维护任务清单；memory 保存/读取长期记忆。\n" +
               "凡是修改了 YCode 自身文件，改完后必须调用 apply_self_changes；它会根据路径自动选择热加载、重建或重启。\n" +
               "只有用户明确要求立即重启 Agent 时才直接调用 restart_agent；不要在改完源码后只回复完成。\n" +
               "安全护栏：execute_command 执行删除/格式化/注册表/环境变量/关机等危险命令，delete_file 删除文件/目录，git_commit/git_push 修改仓库时，会被拦截并要求用户授权。被拦截时不要擅自重复尝试，应明确告知用户将要执行的操作，并请用户在聊天中发送 /allow-dangerous 授权（/deny-dangerous 可撤销授权）。";
    }

    void trimHistory(std::vector<json> &history)
    {
        if (history.size() <= 20) return;
        std::vector<json> trimmed;
        for (size_t i = history.size() - 20; i < history.size(); i++)
            trimmed.push_back(history[i]);
        history = trimmed;
    }

    bool saveSession(const std::vector<json> &history, const std::string &filepath = SESSION_FILE)
    {
        json sessionData;
        sessionData["version"] = "2.0";
        sessionData["timestamp"] = std::time(nullptr);
        sessionData["model"] = modelName;
        sessionData["history"] = history;
        return writeFile(filepath, sessionData.dump(2));
    }

    std::vector<json> loadSession(const std::string &filepath = SESSION_FILE)
    {
        std::vector<json> history;
        std::string content = readFile(filepath);
        if (content.find("Tool Error") != std::string::npos) return history;
        try
        {
            json sessionData = json::parse(content);
            if (sessionData.contains("history"))
            {
                for (const auto &msg : sessionData["history"])
                    history.push_back(msg);
                std::cout << "已加载会话 (" << history.size() << " 条消息)" << std::endl;
            }
        }
        catch (...) { std::cerr << "会话文件损坏" << std::endl; }
        return history;
    }
};

// ============================================================
// 主函数
// ============================================================
int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleTitleA("YCode Agent v2.0 - World Domination Edition");
#endif

    std::cout << "========================================" << std::endl;
    std::cout << "  YCode Agent v2.0 - 世界征服版" << std::endl;
    std::cout << "  多工具 | API重试 | 会话持久化 | 自进化分派" << std::endl;
    std::cout << "========================================" << std::endl;

    std::string apiKey;
    {
        const char *envKey = std::getenv("DEEPSEEK_API_KEY");
        if (envKey && *envKey) apiKey = envKey;
    }

    if (apiKey.empty())
    {
        std::cout << "请输入 DeepSeek API Key: ";
        std::getline(std::cin, apiKey);
    }

    if (apiKey.empty())
    {
        std::cerr << "Error: API Key 不能为空" << std::endl;
        return 1;
    }

    std::string projectDir = ".";
    if (argc > 1) projectDir = argv[1];

    DeepSeekAgent agent(apiKey, projectDir);
    std::string input;
    std::vector<json> conversationHistory;

    // 托管模式下由 Qt 客户端负责展示用户/助手角色，跳过 REPL 提示符
    const char *managedEnv = std::getenv("YCODE_MANAGED");
    bool managed = managedEnv && std::string(managedEnv) == "1";

    std::cout << "\n命令: /exit /clear /save /load /restart /self-update /apply-self-changes /temp 0.5 /model name /allow-dangerous /deny-dangerous /help" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    while (true)
    {
        if (!managed)
            std::cout << "\n你: ";
        std::getline(std::cin, input);
        if (std::cin.eof())
            break; // 标准输入关闭（如客户端退出），干净退出避免空转

        if (input == "/exit" || input == "/quit")
        {
            if (!conversationHistory.empty()) agent.saveSession(conversationHistory);
            std::cout << "再见！" << std::endl;
            break;
        }

        if (input == "/clear")
        {
            system("cls");
            conversationHistory.clear();
            std::cout << "对话已清空" << std::endl;
            continue;
        }

        // ★ /restart — 发送信号给 YCode 客户端（如果是托管模式）
        if (input == "/restart")
        {
            if (!conversationHistory.empty())
            {
                agent.saveSession(conversationHistory);
                std::cout << "会话已保存。" << std::endl;
            }
            std::cout << "正在重启 Agent..." << std::endl;
            restartAgent(false);  // 托管模式：打印 SIGNAL:RESTART_AGENT 并 exit(0)
            continue;
        }

        if (input == "/self-update" || input == "/rebuild-restart")
        {
            if (!conversationHistory.empty())
            {
                agent.saveSession(conversationHistory);
                std::cout << "会话已保存。" << std::endl;
            }
            std::cout << "正在请求 YCode 完整自更新..." << std::endl;
            requestYCodeRebuildAndRestart();
            continue;
        }

        if (input == "/apply-self-changes")
        {
            std::cout << agent.chat("请调用 apply_self_changes 应用已记录的 YCode 自身变更。", conversationHistory) << std::endl;
            continue;
        }

        if (input == "/save") { agent.saveSession(conversationHistory); std::cout << "已保存" << std::endl; continue; }
        if (input == "/load") { conversationHistory = agent.loadSession(); continue; }

        if (input.substr(0, 6) == "/temp ")
        {
            try
            {
                double t = std::stod(input.substr(6));
                if (t >= 0.0 && t <= 1.5) { agent.setTemperature(t); std::cout << "温度已设为 " << t << std::endl; }
                else { std::cout << "温度需在 0.0-1.5 之间" << std::endl; }
            }
            catch (...) { std::cout << "无效的温度值" << std::endl; }
            continue;
        }

        if (input.substr(0, 7) == "/model ")
        {
            std::string m = input.substr(7);
            agent.setModel(m);
            std::cout << "模型已设为 " << m << std::endl;
            continue;
        }

        if (input == "/allow-dangerous")
        {
            agent.setAllowDangerousCommands(true);
            std::cout << "已授权执行危险命令（用 /deny-dangerous 撤销）。请谨慎使用。" << std::endl;
            continue;
        }
        if (input == "/deny-dangerous")
        {
            agent.setAllowDangerousCommands(false);
            std::cout << "已关闭危险命令授权。" << std::endl;
            continue;
        }

        if (input == "/help") { std::cout << "YCode Agent v2.0 - " << agent.toolCount() << "个工具的全能编程助手 | apply_self_changes 按路径热加载/重建/重启 | git_commit/git_push 提交推送 | /restart 重启 Agent | /self-update 重建并重启 YCode | /allow-dangerous 授权危险命令 | /deny-dangerous 撤销授权" << std::endl; continue; }
        if (input.empty()) continue;

        if (!managed)
            std::cout << "Agent: " << std::flush;
        std::string response = agent.chat(input, conversationHistory);
        std::cout << response << std::endl;
        agent.saveSession(conversationHistory); // 每次响应后自动保存，避免崩溃丢失历史
    }

    return 0;
}
