// Agent 纯逻辑单元测试：危险命令识别、shell 元字符防护、SSE 流式解析。
// 通过 include 真实的 agent.cpp 直接测试其函数（Windows 专用，需 libcurl）。
#include <cstdio>
#include <cstring>
#include <string>
#include "nlohmann/json.hpp"
using json = nlohmann::json;

#define main ycode_agent_real_main
#include "../agent.cpp"
#undef main

static int g_ok = 0;
static int g_fail = 0;
#define CHECK(c) do { if (c) ++g_ok; else { ++g_fail; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static void feed(StreamContext &ctx, const std::string &payload)
{
    std::string data = "data: " + payload + "\n\n";
    StreamWriteCallback((void *)data.c_str(), 1, data.size(), &ctx);
}

// ------------------------------------------------------------
static void testDangerousCommand()
{
    // 危险命令应被拦截
    CHECK(isDangerousCommand("del file.txt"));
    CHECK(isDangerousCommand("erase c:\\tmp\\a"));
    CHECK(isDangerousCommand("rmdir /s /q build"));
    CHECK(isDangerousCommand("rd /s /q build"));
    CHECK(isDangerousCommand("rm -rf src"));
    CHECK(isDangerousCommand("format c:"));
    CHECK(isDangerousCommand("diskpart"));
    CHECK(isDangerousCommand("shutdown /s /t 0"));
    CHECK(isDangerousCommand("taskkill /f /im notepad.exe"));
    CHECK(isDangerousCommand("reg delete HKLM\\x"));
    CHECK(isDangerousCommand("setx FOO bar"));
    CHECK(isDangerousCommand("bcdedit /set testsigning on"));
    CHECK(isDangerousCommand("takeown /f C:\\x"));
    CHECK(isDangerousCommand("icacls C:\\x /grant u:f"));
    CHECK(isDangerousCommand("cacls C:\\x /g u:f"));

    // 包装前缀不应绕过检测
    CHECK(isDangerousCommand("cmd /c del x"));
    CHECK(isDangerousCommand("powershell -Command \"Remove-Item x\""));
    CHECK(isDangerousCommand("start cmd /k rmdir /s /q build"));

    // 分隔符拼接后的危险命令
    CHECK(isDangerousCommand("echo hi & format c:"));
    CHECK(isDangerousCommand("dir && del x"));

    // 安全命令应放行
    CHECK(!isDangerousCommand("dir"));
    CHECK(!isDangerousCommand("echo hello world"));
    CHECK(!isDangerousCommand("cd /d F:\\temp"));
    CHECK(!isDangerousCommand("git status"));
    CHECK(!isDangerousCommand("cl /EHsc main.cpp"));
    CHECK(!isDangerousCommand("echo del")); // del 作为参数文本，不应误伤
}

// ------------------------------------------------------------
static void testShellMetacharacter()
{
    CHECK(hasShellMetacharacter("a&b"));
    CHECK(hasShellMetacharacter("a|b"));
    CHECK(hasShellMetacharacter("a\"b"));
    CHECK(hasShellMetacharacter("a;b"));
    CHECK(hasShellMetacharacter("a<b"));
    CHECK(hasShellMetacharacter("a>b"));
    CHECK(hasShellMetacharacter("a^b"));
    CHECK(hasShellMetacharacter("a%PATH%b"));
    CHECK(hasShellMetacharacter("a!b"));

    // 通配符与普通路径放行
    CHECK(!hasShellMetacharacter("*.cpp"));
    CHECK(!hasShellMetacharacter("C:/temp/file.txt"));
    CHECK(!hasShellMetacharacter("hello world"));
    CHECK(!hasShellMetacharacter(""));
}

// ------------------------------------------------------------
static void testSseParsing()
{
    // 正文跨多个 chunk + 跨块断行
    {
        StreamContext ctx;
        json c1; c1["choices"][0]["delta"]["content"] = "Hel";
        json c2; c2["choices"][0]["delta"]["content"] = "lo world";
        feed(ctx, c1.dump());
        feed(ctx, c2.dump());
        feed(ctx, "[DONE]");
        CHECK(ctx.content == "Hello world");
        CHECK(ctx.toolCallAcc.empty());
    }

    // 工具调用：arguments 跨 chunk 累积
    {
        StreamContext ctx;
        json t1;
        t1["choices"][0]["delta"]["tool_calls"][0]["index"] = 0;
        t1["choices"][0]["delta"]["tool_calls"][0]["id"] = "c1";
        t1["choices"][0]["delta"]["tool_calls"][0]["function"]["name"] = "read_file";
        t1["choices"][0]["delta"]["tool_calls"][0]["function"]["arguments"] = "{\"filepath\":\"";
        feed(ctx, t1.dump());

        json t2;
        t2["choices"][0]["delta"]["tool_calls"][0]["index"] = 0;
        t2["choices"][0]["delta"]["tool_calls"][0]["function"]["arguments"] = "a.cpp\"}";
        feed(ctx, t2.dump());
        feed(ctx, "[DONE]");

        CHECK(ctx.toolCallAcc.size() == 1);
        if (ctx.toolCallAcc.size() == 1)
        {
            CHECK(ctx.toolCallAcc[0].id == "c1");
            CHECK(ctx.toolCallAcc[0].name == "read_file");
            CHECK(ctx.toolCallAcc[0].arguments == "{\"filepath\":\"a.cpp\"}");
        }
    }
}

// ------------------------------------------------------------
int main()
{
    testDangerousCommand();
    testShellMetacharacter();
    testSseParsing();

    std::printf("\n%d checks, %d failures\n", g_ok + g_fail, g_fail);
    if (g_fail == 0)
        std::printf("ALL AGENT LOGIC TESTS PASSED\n");
    else
        std::printf("SOME TESTS FAILED\n");
    return g_fail == 0 ? 0 : 1;
}
