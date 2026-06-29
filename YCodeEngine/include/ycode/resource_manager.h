#ifndef YCODE_RESOURCE_MANAGER_H
#define YCODE_RESOURCE_MANAGER_H

#include <string>

namespace ycode {

class ResourceManager {
public:
    explicit ResourceManager(std::string rootPath = ".");

    void setRootPath(std::string rootPath);
    const std::string& rootPath() const;

    std::string resolvePath(const std::string& path) const;
    bool exists(const std::string& path) const;
    bool readText(const std::string& path, std::string& out, std::string* error = nullptr) const;

private:
    std::string rootPath_;
};

} // namespace ycode

#endif // YCODE_RESOURCE_MANAGER_H
