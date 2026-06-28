#include "ycode/core.h"

#include <iostream>
#include <string>

int main()
{
    auto version = ycode::getVersion();
    std::cout << "YCode Engine v" << version.major << "." << version.minor << "." << version.patch << std::endl;
    std::cout << "================================" << std::endl;

    ycode::Engine engine({ "YCode Engine Sandbox", "plugins", 60 });
    engine.events().subscribe("*", [](const ycode::Event& event) {
        std::cout << "[event] " << event.type << std::endl;
    });

    std::string error;
    if (!engine.initialize(&error))
    {
        std::cerr << "Engine initialization failed: " << error << std::endl;
        return 1;
    }

    engine.tick();
    engine.shutdown();
    return 0;
}

