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

// ============================================================
// HTTP 回调
// ============================================================
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
{
    size_t totalSize = size * nmemb;
    output->append((char *)contents, totalSize);
    return totalSize;
}

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
    std::string cmd = "dir /s /b \"" + directory + "\\" + pattern + "\" 2>nul";
    std::string result = executeShellCommand(cmd);
    if (result == "(无输出)" || result.empty()) return "未找到: " + pattern;
    return result;
}

std::string searchContent(const std::string &text, const std::string &filePattern, const std::string &directory)
{
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
    CURL *curl = curl_easy_init();
    if (!curl) return "FAIL 下载: 无法初始化 CURL";
    FILE *file = fopen(savePath.c_str(), "wb");
    if (!file) { curl_easy_cleanup(curl); return "FAIL 下载: 无法创建文件"; }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    CURLcode res = curl_easy_perform(curl);
    fclose(file);
    curl_easy_cleanup(curl);
    if (res == CURLE_OK) return "OK 下载: " + savePath;
    remove(savePath.c_str());
    return "FAIL 下载: " + std::string(curl_easy_strerror(res));
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

        tools.push_back(makeTool("execute_command", "执行系统命令（仅限查看类命令，如 dir、echo）",
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

        return tools;
    }

    std::string executeTool(const std::string &toolName, const json &args)
    {
        if (toolName == "read_file")
            return readFile(args["filepath"]);
        if (toolName == "write_file")
        {
            if (writeFile(args["filepath"], args["content"]))
                return "OK 写入: " + std::string(args["filepath"]);
            return "FAIL 写入: " + std::string(args["filepath"]);
        }
        if (toolName == "list_directory")
        {
            std::string path = args.contains("path") ? args["path"].get<std::string>() : ".";
            return executeShellCommand("dir \"" + path + "\"");
        }
        if (toolName == "execute_command")
        {
            std::string cmd = args["command"];
            std::cout << "\n  [执行: " << cmd << "]" << std::endl;
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
            return createDirectory(args["path"]);
        if (toolName == "delete_file")
            return deleteFile(args["path"]);
        if (toolName == "move_file")
            return moveFile(args["source"], args["destination"]);
        if (toolName == "get_file_info")
            return getFileInfo(args["path"]);
        if (toolName == "download_file")
        {
            std::cout << "\n  [下载: " << args["url"].get<std::string>() << "]" << std::endl;
            return downloadFile(args["url"], args["savePath"]);
        }
        if (toolName == "restart_agent")
        {
            return restartAgent(false);  // 托管模式：发送信号给 YCode
        }
        if (toolName == "rebuild_and_restart_ycode")
        {
            return requestYCodeRebuildAndRestart();
        }
        return "未知工具: " + toolName;
    }

    std::string callAPI(const json &messages, int retryCount = 0)
    {
        CURL *curl = curl_easy_init();
        if (!curl) return "Error: CURL init failed";

        json requestJson;
        requestJson["model"] = modelName;
        requestJson["messages"] = messages;
        requestJson["tools"] = getTools();
        requestJson["tool_choice"] = "auto";
        requestJson["stream"] = false;
        requestJson["temperature"] = temperature;
        requestJson["max_tokens"] = 8192;

        std::string postData = requestJson.dump();
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey).c_str());

        std::string responseBuffer;
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
        {
            if (retryCount < API_MAX_RETRIES)
            {
                std::cerr << "  重试 (" << (retryCount + 1) << "/" << API_MAX_RETRIES << ")..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 * (retryCount + 1)));
                return callAPI(messages, retryCount + 1);
            }
            return "CURL Error: " + std::string(curl_easy_strerror(res));
        }

        if (httpCode == 429 || httpCode >= 500)
        {
            if (retryCount < API_MAX_RETRIES)
            {
                std::cerr << "  HTTP " << httpCode << " 重试..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(2000 * (retryCount + 1)));
                return callAPI(messages, retryCount + 1);
            }
        }

        if (httpCode != 200)
            return "HTTP Error: " + std::to_string(httpCode);

        return responseBuffer;
    }

public:
    DeepSeekAgent(const std::string &key, const std::string &projectDir = ".")
        : apiKey(key), projectPath(projectDir) {}

    void setTemperature(double t) { temperature = t; }
    void setModel(const std::string &m) { modelName = m; }

    std::string chat(const std::string &userMessage, std::vector<json> &conversationHistory)
    {
        if (!userMessage.empty())
            conversationHistory.push_back({{"role", "user"}, {"content", userMessage}});

        for (int iteration = 0; iteration < MAX_TOOL_ITERATIONS; iteration++)
        {
            json messages = json::array();
            messages.push_back({{"role", "system"}, {"content", getSystemPrompt()}});
            for (const auto &msg : conversationHistory)
                messages.push_back(msg);

            std::string responseBuffer = callAPI(messages);

            try
            {
                json responseJson = json::parse(responseBuffer);
                if (responseJson.contains("choices") && !responseJson["choices"].empty())
                {
                    json assistantMsg = responseJson["choices"][0]["message"];
                    conversationHistory.push_back(assistantMsg);

                    if (assistantMsg.contains("tool_calls") && !assistantMsg["tool_calls"].empty())
                    {
                        if (iteration == 0 && !userMessage.empty())
                            std::cout << "\n  [Agent 正在使用工具...]" << std::endl;

                        for (const auto &toolCall : assistantMsg["tool_calls"])
                        {
                            std::string toolName = toolCall["function"]["name"];
                            std::string argsStr = toolCall["function"]["arguments"].get<std::string>();
                            std::cout << "  调用: " << toolName << std::endl;
                            json toolArgs = json::parse(argsStr);
                            std::string toolResult = executeTool(toolName, toolArgs);
                            conversationHistory.push_back({{"role", "tool"},
                                                           {"tool_call_id", toolCall["id"]},
                                                           {"content", toolResult}});
                        }
                        continue;
                    }
                    else
                    {
                        std::string content = assistantMsg["content"].get<std::string>();
                        if (conversationHistory.size() > 30)
                            trimHistory(conversationHistory);
                        return content;
                    }
                }
                else if (responseJson.contains("error"))
                    return "API Error: " + responseJson["error"]["message"].get<std::string>();
                return "Unknown response format";
            }
            catch (const json::exception &e)
            {
                return "JSON Parse Error: " + std::string(e.what());
            }
        }
        return "达到最大工具调用次数限制";
    }

    std::string getSystemPrompt()
    {
        return std::string("你是 YCode Agent v2.0，运行在 Yiyangzai 自制的编程工具中。") +
               "你有13个工具: read_file, write_file, list_directory, execute_command, " +
               "search_files, search_content, create_directory, delete_file, move_file, get_file_info, download_file, restart_agent, rebuild_and_restart_ycode。 " +
               "修改代码前先读取原文件，用write_file写入完整内容。用中文回答，自信幽默。" +
               "只需要重启 Agent 时调用 restart_agent；修改 agent.cpp、YZCodex 客户端、启动脚本或快捷方式后，调用 rebuild_and_restart_ycode 让整套 YCode 重建并重启。";
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
    std::cout << "  12个工具 | API重试 | 会话持久化 | 自重启" << std::endl;
    std::cout << "========================================" << std::endl;

    std::string apiKey;
    if (argc > 1)
        apiKey = argv[1];
    else
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
    if (argc > 2) projectDir = argv[2];

    DeepSeekAgent agent(apiKey, projectDir);
    std::string input;
    std::vector<json> conversationHistory;

    std::cout << "\n命令: /exit /clear /save /load /restart /self-update /temp 0.5 /model name /help" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    while (true)
    {
        std::cout << "\n你: ";
        std::getline(std::cin, input);

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

        if (input == "/save") { agent.saveSession(conversationHistory); std::cout << "已保存" << std::endl; continue; }
        if (input == "/load") { conversationHistory = agent.loadSession(); continue; }

        if (input.substr(0, 6) == "/temp ")
        {
            try { double t = std::stod(input.substr(6)); if (t >= 0.0 && t <= 1.5) agent.setTemperature(t); }
            catch (...) {}
            continue;
        }

        if (input.substr(0, 7) == "/model ") { agent.setModel(input.substr(7)); continue; }
        if (input == "/help") { std::cout << "YCode Agent v2.0 - 13个工具的全能编程助手 | /restart 重启 Agent | /self-update 重建并重启 YCode" << std::endl; continue; }
        if (input.empty()) continue;

        std::cout << "Agent: " << std::flush;
        std::string response = agent.chat(input, conversationHistory);
        std::cout << response << std::endl;
    }

    return 0;
}
