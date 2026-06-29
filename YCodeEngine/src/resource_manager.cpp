#include "ycode/resource_manager.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace ycode {

ResourceManager::ResourceManager(std::string rootPath)
    : rootPath_(std::move(rootPath))
{
    if (rootPath_.empty())
        rootPath_ = ".";
}

void ResourceManager::setRootPath(std::string rootPath)
{
    rootPath_ = std::move(rootPath);
    if (rootPath_.empty())
        rootPath_ = ".";
}

const std::string& ResourceManager::rootPath() const
{
    return rootPath_;
}

std::string ResourceManager::resolvePath(const std::string& path) const
{
    std::filesystem::path input(path);
    if (input.is_absolute())
        return input.lexically_normal().string();

    return (std::filesystem::path(rootPath_) / input).lexically_normal().string();
}

bool ResourceManager::exists(const std::string& path) const
{
    return std::filesystem::exists(resolvePath(path));
}

bool ResourceManager::readText(const std::string& path, std::string& out, std::string* error) const
{
    std::string resolved = resolvePath(path);
    std::ifstream file(resolved, std::ios::in | std::ios::binary);
    if (!file)
    {
        if (error)
            *error = "Failed to open text resource: " + resolved;
        return false;
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    out = stream.str();
    return true;
}

} // namespace ycode
