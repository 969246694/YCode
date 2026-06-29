#ifndef YCODE_SCENE_LOADER_H
#define YCODE_SCENE_LOADER_H

#include "ycode/scene.h"

#include <string>

namespace ycode {

class SceneLoader {
public:
    static bool loadFromFile(const std::string& path, Scene& scene, std::string* error = nullptr);
    static bool loadFromText(const std::string& text, Scene& scene, std::string* error = nullptr);
};

} // namespace ycode

#endif // YCODE_SCENE_LOADER_H
