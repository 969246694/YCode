#include <iostream>
#include <string>
#include <curl/curl.h>

int main()
{
    std::cout << "程序开始运行..." << std::endl;
    std::cout << "正在测试 CURL 库..." << std::endl;

    CURL *curl = curl_easy_init();
    if (curl)
    {
        std::cout << "CURL 初始化成功！" << std::endl;
        curl_easy_cleanup(curl);
    }
    else
    {
        std::cout << "CURL 初始化失败！" << std::endl;
    }

    std::cout << "测试完成，按回车键退出..." << std::endl;
    std::cin.get();
    return 0;
}