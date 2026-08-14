#include "ycode/scene_loader.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace ycode {
namespace {

using Json = nlohmann::json;

std::string stripUtf8Bom(std::string text)
{
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF)
    {
        text.erase(0, 3);
    }
    return text;
}

const Json* findField(const Json& object, const char* key)
{
    if (!object.is_object())
        return nullptr;

    auto it = object.find(key);
    return it == object.end() ? nullptr : &(*it);
}

bool readStringField(const Json& object, const char* key, std::string& out, std::string* error)
{
    const Json* value = findField(object, key);
    if (!value)
        return true;
    if (!value->is_string())
    {
        if (error)
            *error = std::string("Expected '") + key + "' to be a string";
        return false;
    }

    out = value->get<std::string>();
    return true;
}

bool readBoolField(const Json& object, const char* key, bool& out, std::string* error)
{
    const Json* value = findField(object, key);
    if (!value)
        return true;
    if (!value->is_boolean())
    {
        if (error)
            *error = std::string("Expected '") + key + "' to be a boolean";
        return false;
    }

    out = value->get<bool>();
    return true;
}

bool readNumberField(const Json& object, const char* key, float& out, std::string* error)
{
    const Json* value = findField(object, key);
    if (!value)
        return true;
    if (!value->is_number())
    {
        if (error)
            *error = std::string("Expected '") + key + "' to be a number";
        return false;
    }

    out = value->get<float>();
    return true;
}

bool readVec2Field(const Json& object, const char* key, Vec2& out, std::string* error)
{
    const Json* value = findField(object, key);
    if (!value)
        return true;

    if (!value->is_array() || value->size() != 2 ||
        !(*value)[0].is_number() || !(*value)[1].is_number())
    {
        if (error)
            *error = std::string("Expected '") + key + "' to be a two-number array";
        return false;
    }

    out.x = (*value)[0].get<float>();
    out.y = (*value)[1].get<float>();
    return true;
}

std::string toLowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

bool readBodyTypeField(const Json& object, const char* key, BodyType2D& out, std::string* error)
{
    const Json* value = findField(object, key);
    if (!value)
        return true;
    if (!value->is_string())
    {
        if (error)
            *error = std::string("Expected '") + key + "' to be a string";
        return false;
    }

    std::string bodyType = toLowerAscii(value->get<std::string>());
    if (bodyType == "static")
        out = BodyType2D::Static;
    else if (bodyType == "kinematic")
        out = BodyType2D::Kinematic;
    else if (bodyType == "dynamic")
        out = BodyType2D::Dynamic;
    else
    {
        if (error)
            *error = "Expected '" + std::string(key) + "' to be static, kinematic, or dynamic";
        return false;
    }

    return true;
}

bool applyBoxColliderFields(const Json& object, BoxCollider2D& box, std::string* error)
{
    std::string shape;
    if (!readStringField(object, "shape", shape, error))
        return false;
    if (!shape.empty() && toLowerAscii(shape) != "box")
    {
        if (error)
            *error = "Only box physics2D colliders are supported";
        return false;
    }

    return readVec2Field(object, "halfSizeMeters", box.halfSizeMeters, error) &&
           readNumberField(object, "density", box.density, error) &&
           readNumberField(object, "friction", box.friction, error) &&
           readNumberField(object, "restitution", box.restitution, error) &&
           readBoolField(object, "fixedRotation", box.fixedRotation, error);
}

bool applyCircleColliderFields(const Json& object, CircleCollider2D& circle, std::string* error)
{
    return readNumberField(object, "radiusMeters", circle.radiusMeters, error) &&
           readNumberField(object, "density", circle.density, error) &&
           readNumberField(object, "friction", circle.friction, error) &&
           readNumberField(object, "restitution", circle.restitution, error) &&
           readBoolField(object, "fixedRotation", circle.fixedRotation, error);
}

bool applyPhysics2D(const Json& entityObject, Entity& entity, std::string* error)
{
    const Json* physics = findField(entityObject, "physics2D");
    if (!physics)
        physics = findField(entityObject, "physics");
    if (!physics)
        return true;
    if (!physics->is_object())
    {
        if (error)
            *error = "Expected 'physics2D' to be an object";
        return false;
    }

    PhysicsBody2D physicsBody;
    physicsBody.enabled = true;

    if (!readBoolField(*physics, "enabled", physicsBody.enabled, error))
        return false;
    if (!readBodyTypeField(*physics, "bodyType", physicsBody.bodyType, error))
        return false;
    if (!readBodyTypeField(*physics, "type", physicsBody.bodyType, error))
        return false;

    // 碰撞体字段只从 box/collider 子对象读取（与文档格式一致），
    // 不再对整个 physics 对象重复解析。
    const Json* box = findField(*physics, "box");
    const Json* collider = findField(*physics, "collider");
    const Json* colliderObject = box ? box : collider;
    if (colliderObject)
    {
        if (!colliderObject->is_object())
        {
            if (error)
                *error = box ? "Expected 'box' to be an object" : "Expected 'collider' to be an object";
            return false;
        }

        if (!applyBoxColliderFields(*colliderObject, physicsBody.box, error))
            return false;
    }

    // 圆形碰撞体（与 box 互斥，若同时存在以 circle 为准）
    const Json* circle = findField(*physics, "circle");
    if (circle)
    {
        if (!circle->is_object())
        {
            if (error)
                *error = "Expected 'circle' to be an object";
            return false;
        }

        if (!applyCircleColliderFields(*circle, physicsBody.circle, error))
            return false;
        physicsBody.useCircle = true;
    }

    entity.physics2D = physicsBody;
    return true;
}

bool applyTransform(const Json& entityObject, Entity& entity, std::string* error)
{
    const Json* transform = findField(entityObject, "transform");
    if (!transform)
        return true;
    if (!transform->is_object())
    {
        if (error)
            *error = "Expected 'transform' to be an object";
        return false;
    }

    return readVec2Field(*transform, "position", entity.transform.position, error) &&
           readNumberField(*transform, "rotationDegrees", entity.transform.rotationDegrees, error) &&
           readVec2Field(*transform, "scale", entity.transform.scale, error);
}

bool applyProperties(const Json& entityObject, Entity& entity, std::string* error)
{
    const Json* properties = findField(entityObject, "properties");
    if (!properties)
        return true;
    if (!properties->is_object())
    {
        if (error)
            *error = "Expected 'properties' to be an object";
        return false;
    }

    for (auto it = properties->begin(); it != properties->end(); ++it)
    {
        const Json& value = it.value();
        if (value.is_string())
            entity.properties[it.key()] = value.get<std::string>();
        else if (value.is_number())
            entity.properties[it.key()] = value.dump();
        else if (value.is_boolean())
            entity.properties[it.key()] = value.get<bool>() ? "true" : "false";
        else if (value.is_null())
            entity.properties[it.key()] = "";
        else
        {
            if (error)
                *error = "Scene entity property '" + it.key() + "' must be string, number, bool, or null";
            return false;
        }
    }

    return true;
}

} // namespace

bool SceneLoader::loadFromFile(const std::string& path, Scene& scene, std::string* error)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file)
    {
        if (error)
            *error = "Failed to open scene file: " + path;
        return false;
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    return loadFromText(stream.str(), scene, error);
}

bool SceneLoader::loadFromText(const std::string& text, Scene& scene, std::string* error)
{
    Json root;
    try
    {
        root = Json::parse(stripUtf8Bom(text));
    }
    catch (const Json::parse_error& parseError)
    {
        if (error)
            *error = std::string("Scene JSON parse error: ") + parseError.what();
        return false;
    }

    if (!root.is_object())
    {
        if (error)
            *error = "Scene root must be an object";
        return false;
    }

    const Json* entities = findField(root, "entities");
    if (!entities || !entities->is_array())
    {
        if (error)
            *error = "Scene must contain an 'entities' array";
        return false;
    }

    scene.clear();

    std::string sceneName;
    if (!readStringField(root, "name", sceneName, error))
        return false;
    if (!sceneName.empty())
        scene.setName(sceneName);

    for (const Json& entityValue : *entities)
    {
        if (!entityValue.is_object())
        {
            if (error)
                *error = "Each scene entity must be an object";
            return false;
        }

        std::string entityName;
        if (!readStringField(entityValue, "name", entityName, error))
            return false;

        Entity& entity = scene.createEntity(entityName);
        if (!readBoolField(entityValue, "active", entity.active, error))
            return false;
        if (!applyTransform(entityValue, entity, error))
            return false;
        if (!applyPhysics2D(entityValue, entity, error))
            return false;
        if (!applyProperties(entityValue, entity, error))
            return false;
    }

    return true;
}

} // namespace ycode
