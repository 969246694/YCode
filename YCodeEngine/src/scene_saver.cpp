#include "ycode/scene_saver.h"

#include <nlohmann/json.hpp>

#include <fstream>

namespace ycode {
namespace {

using Json = nlohmann::json;

const char* bodyTypeName(BodyType2D type)
{
    switch (type)
    {
    case BodyType2D::Static: return "static";
    case BodyType2D::Kinematic: return "kinematic";
    case BodyType2D::Dynamic:
    default: return "dynamic";
    }
}

Json vec2ToJson(const Vec2& value)
{
    return Json::array({value.x, value.y});
}

Json boxToJson(const BoxCollider2D& box)
{
    Json out;
    out["halfSizeMeters"] = vec2ToJson(box.halfSizeMeters);
    out["density"] = box.density;
    out["friction"] = box.friction;
    out["restitution"] = box.restitution;
    out["fixedRotation"] = box.fixedRotation;
    return out;
}

Json circleToJson(const CircleCollider2D& circle)
{
    Json out;
    out["radiusMeters"] = circle.radiusMeters;
    out["density"] = circle.density;
    out["friction"] = circle.friction;
    out["restitution"] = circle.restitution;
    out["fixedRotation"] = circle.fixedRotation;
    return out;
}

Json capsuleToJson(const CapsuleCollider2D& capsule)
{
    Json out;
    out["center1"] = vec2ToJson(capsule.center1);
    out["center2"] = vec2ToJson(capsule.center2);
    out["radiusMeters"] = capsule.radiusMeters;
    out["density"] = capsule.density;
    out["friction"] = capsule.friction;
    out["restitution"] = capsule.restitution;
    out["fixedRotation"] = capsule.fixedRotation;
    return out;
}

} // namespace

std::string SceneSaver::saveToText(const Scene& scene)
{
    Json root;
    root["name"] = scene.name();

    Json entities = Json::array();
    for (const Entity& entity : scene.entities())
    {
        Json e;
        e["name"] = entity.name;
        e["active"] = entity.active;

        Json transform;
        transform["position"] = vec2ToJson(entity.transform.position);
        transform["rotationDegrees"] = entity.transform.rotationDegrees;
        transform["scale"] = vec2ToJson(entity.transform.scale);
        e["transform"] = transform;

        if (entity.physics2D.enabled)
        {
            Json physics;
            physics["enabled"] = true;
            physics["bodyType"] = bodyTypeName(entity.physics2D.bodyType);
            if (entity.physics2D.useCapsule)
                physics["capsule"] = capsuleToJson(entity.physics2D.capsule);
            else if (entity.physics2D.useCircle)
                physics["circle"] = circleToJson(entity.physics2D.circle);
            else
                physics["box"] = boxToJson(entity.physics2D.box);
            e["physics2D"] = physics;
        }

        if (!entity.properties.empty())
            e["properties"] = Json(entity.properties);

        entities.push_back(e);
    }
    root["entities"] = entities;
    return root.dump(2);
}

bool SceneSaver::saveToFile(const std::string& path, const Scene& scene, std::string* error)
{
    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file)
    {
        if (error)
            *error = "Failed to open scene file for writing: " + path;
        return false;
    }
    file << saveToText(scene);
    return true;
}

} // namespace ycode
