#ifndef YCODE_PLUGIN_LOADER_H
#define YCODE_PLUGIN_LOADER_H

#include "ycode/plugin.h"
#include "ycode/version.h"

#include <memory>
#include <string>
#include <vector>

namespace ycode {

struct LoadedPluginInfo {
    std::string path;
    std::string name;
    Version version;
    std::string description;
};

class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();

    PluginLoader(const PluginLoader&) = delete;
    PluginLoader& operator=(const PluginLoader&) = delete;

    bool load(const std::string& path, std::string* error = nullptr);
    void unloadAll();
    const std::vector<LoadedPluginInfo>& plugins() const;

private:
    struct LoadedPlugin;
    std::vector<std::unique_ptr<LoadedPlugin>> loaded_;
    std::vector<LoadedPluginInfo> pluginInfo_;
};

} // namespace ycode

#endif // YCODE_PLUGIN_LOADER_H
