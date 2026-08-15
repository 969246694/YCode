#ifndef YCODE_SCENE_SAVER_H
#define YCODE_SCENE_SAVER_H

#include "ycode/scene.h"

#include <string>

namespace ycode {

/// 把场景序列化为 JSON（与 SceneLoader 的格式兼容，可往返加载/保存）。
class SceneSaver {
public:
    static bool saveToFile(const std::string& path, const Scene& scene, std::string* error = nullptr);
    static std::string saveToText(const Scene& scene);
};

} // namespace ycode

#endif // YCODE_SCENE_SAVER_H
