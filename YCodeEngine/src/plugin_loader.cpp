#include "ycode/plugin_loader.h"
#include "ycode/version.h"

#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace ycode {

namespace {

#ifdef _WIN32
using NativeLibraryHandle = HMODULE;

NativeLibraryHandle openLibrary(const std::string& path)
{
    return LoadLibraryA(path.c_str());
}

void* resolveSymbol(NativeLibraryHandle handle, const char* name)
{
    return reinterpret_cast<void*>(GetProcAddress(handle, name));
}

void closeLibrary(NativeLibraryHandle handle)
{
    if (handle)
        FreeLibrary(handle);
}
#else
using NativeLibraryHandle = void*;

NativeLibraryHandle openLibrary(const std::string& path)
{
    return dlopen(path.c_str(), RTLD_NOW);
}

void* resolveSymbol(NativeLibraryHandle handle, const char* name)
{
    return dlsym(handle, name);
}

void closeLibrary(NativeLibraryHandle handle)
{
    if (handle)
        dlclose(handle);
}
#endif

} // namespace

struct PluginLoader::LoadedPlugin {
    std::string path;
    NativeLibraryHandle handle = nullptr;
    ycode_plugin_t instance = nullptr;
    ycode_destroy_plugin_func destroy = nullptr;
    ycode_shutdown_plugin_func shutdown = nullptr;
    LoadedPluginInfo info;

    ~LoadedPlugin()
    {
        if (shutdown && instance)
            shutdown(instance);
        if (destroy && instance)
            destroy(instance);
        closeLibrary(handle);
    }
};

PluginLoader::PluginLoader() = default;

PluginLoader::~PluginLoader()
{
    unloadAll();
}

bool PluginLoader::load(const std::string& path, std::string* error)
{
    NativeLibraryHandle handle = openLibrary(path);
    if (!handle)
    {
        if (error)
            *error = "Failed to load plugin library: " + path;
        return false;
    }

    auto create = reinterpret_cast<ycode_create_plugin_func>(
        resolveSymbol(handle, YCODE_CREATE_PLUGIN_FUNC_NAME));
    auto destroy = reinterpret_cast<ycode_destroy_plugin_func>(
        resolveSymbol(handle, YCODE_DESTROY_PLUGIN_FUNC_NAME));
    auto init = reinterpret_cast<ycode_init_plugin_func>(
        resolveSymbol(handle, YCODE_INIT_PLUGIN_FUNC_NAME));
    auto shutdown = reinterpret_cast<ycode_shutdown_plugin_func>(
        resolveSymbol(handle, YCODE_SHUTDOWN_PLUGIN_FUNC_NAME));
    auto getInfo = reinterpret_cast<ycode_get_plugin_info_func>(
        resolveSymbol(handle, YCODE_GET_PLUGIN_INFO_FUNC_NAME));

    if (!create || !destroy || !init || !shutdown || !getInfo)
    {
        closeLibrary(handle);
        if (error)
            *error = "Plugin is missing one or more required YCode exports: " + path;
        return false;
    }

    ycode_plugin_t instance = create();
    if (!instance)
    {
        closeLibrary(handle);
        if (error)
            *error = "Plugin create function returned null: " + path;
        return false;
    }

    if (init(instance) != 0)
    {
        destroy(instance);
        closeLibrary(handle);
        if (error)
            *error = "Plugin initialization failed: " + path;
        return false;
    }

    const ycode_plugin_info_t* rawInfo = getInfo();
    auto loaded = std::make_unique<LoadedPlugin>();
    loaded->path = path;
    loaded->handle = handle;
    loaded->instance = instance;
    loaded->destroy = destroy;
    loaded->shutdown = shutdown;
    loaded->info.path = path;
    loaded->info.name = rawInfo && rawInfo->name ? rawInfo->name : "Unnamed Plugin";
    loaded->info.version = rawInfo
        ? Version{rawInfo->version_major, rawInfo->version_minor, rawInfo->version_patch}
        : Version{0, 0, 0};
    loaded->info.description = rawInfo && rawInfo->description ? rawInfo->description : "";

    pluginInfo_.push_back(loaded->info);
    loaded_.push_back(std::move(loaded));
    return true;
}

void PluginLoader::unloadAll()
{
    loaded_.clear();
    pluginInfo_.clear();
}

const std::vector<LoadedPluginInfo>& PluginLoader::plugins() const
{
    return pluginInfo_;
}

} // namespace ycode
