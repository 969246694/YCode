#ifndef YCODE_PLUGIN_H
#define YCODE_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* ycode_plugin_t;

typedef struct {
    const char* name;
    int version_major;
    int version_minor;
    int version_patch;
    const char* description;
} ycode_plugin_info_t;

typedef ycode_plugin_t (*ycode_create_plugin_func)();
typedef void (*ycode_destroy_plugin_func)(ycode_plugin_t plugin);
typedef int (*ycode_init_plugin_func)(ycode_plugin_t plugin);
typedef void (*ycode_shutdown_plugin_func)(ycode_plugin_t plugin);
typedef const ycode_plugin_info_t* (*ycode_get_plugin_info_func)();

#define YCODE_CREATE_PLUGIN_FUNC_NAME "ycode_create_plugin"
#define YCODE_DESTROY_PLUGIN_FUNC_NAME "ycode_destroy_plugin"
#define YCODE_INIT_PLUGIN_FUNC_NAME "ycode_init_plugin"
#define YCODE_SHUTDOWN_PLUGIN_FUNC_NAME "ycode_shutdown_plugin"
#define YCODE_GET_PLUGIN_INFO_FUNC_NAME "ycode_get_plugin_info"

#ifdef __cplusplus
}
#endif

#endif // YCODE_PLUGIN_H

