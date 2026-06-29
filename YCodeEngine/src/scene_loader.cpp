#include "ycode/scene_loader.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>

namespace ycode {
namespace {

struct JsonValue {
    enum class Type {
        Null,
        Bool,
        Number,
        String,
        Object,
        Array
    };

    Type type = Type::Null;
    bool boolValue = false;
    double numberValue = 0.0;
    std::string stringValue;
    std::map<std::string, JsonValue> objectValue;
    std::vector<JsonValue> arrayValue;
};

class JsonParser {
public:
    explicit JsonParser(const std::string& text)
        : text_(text)
    {
    }

    bool parse(JsonValue& value, std::string* error)
    {
        skipUtf8Bom();
        skipWhitespace();
        if (!parseValue(value, error))
            return false;

        skipWhitespace();
        if (position_ != text_.size())
            return fail("Unexpected trailing content", error);
        return true;
    }

private:
    bool parseValue(JsonValue& value, std::string* error)
    {
        skipWhitespace();
        if (position_ >= text_.size())
            return fail("Unexpected end of JSON", error);

        char ch = text_[position_];
        if (ch == '{')
            return parseObject(value, error);
        if (ch == '[')
            return parseArray(value, error);
        if (ch == '"')
            return parseStringValue(value, error);
        if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch)))
            return parseNumber(value, error);
        if (matchLiteral("true"))
        {
            value.type = JsonValue::Type::Bool;
            value.boolValue = true;
            return true;
        }
        if (matchLiteral("false"))
        {
            value.type = JsonValue::Type::Bool;
            value.boolValue = false;
            return true;
        }
        if (matchLiteral("null"))
        {
            value.type = JsonValue::Type::Null;
            return true;
        }

        return fail("Unexpected JSON token", error);
    }

    bool parseObject(JsonValue& value, std::string* error)
    {
        value = JsonValue();
        value.type = JsonValue::Type::Object;
        ++position_;
        skipWhitespace();

        if (consume('}'))
            return true;

        while (position_ < text_.size())
        {
            std::string key;
            if (!parseString(key, error))
                return false;

            skipWhitespace();
            if (!consume(':'))
                return fail("Expected ':' after object key", error);

            JsonValue child;
            if (!parseValue(child, error))
                return false;
            value.objectValue[std::move(key)] = std::move(child);

            skipWhitespace();
            if (consume('}'))
                return true;
            if (!consume(','))
                return fail("Expected ',' or '}' in object", error);
            skipWhitespace();
        }

        return fail("Unterminated object", error);
    }

    bool parseArray(JsonValue& value, std::string* error)
    {
        value = JsonValue();
        value.type = JsonValue::Type::Array;
        ++position_;
        skipWhitespace();

        if (consume(']'))
            return true;

        while (position_ < text_.size())
        {
            JsonValue child;
            if (!parseValue(child, error))
                return false;
            value.arrayValue.push_back(std::move(child));

            skipWhitespace();
            if (consume(']'))
                return true;
            if (!consume(','))
                return fail("Expected ',' or ']' in array", error);
            skipWhitespace();
        }

        return fail("Unterminated array", error);
    }

    bool parseStringValue(JsonValue& value, std::string* error)
    {
        value = JsonValue();
        value.type = JsonValue::Type::String;
        return parseString(value.stringValue, error);
    }

    bool parseString(std::string& out, std::string* error)
    {
        skipWhitespace();
        if (!consume('"'))
            return fail("Expected string", error);

        out.clear();
        while (position_ < text_.size())
        {
            char ch = text_[position_++];
            if (ch == '"')
                return true;

            if (ch != '\\')
            {
                out.push_back(ch);
                continue;
            }

            if (position_ >= text_.size())
                return fail("Unterminated escape sequence", error);

            char escaped = text_[position_++];
            switch (escaped)
            {
            case '"':
            case '\\':
            case '/':
                out.push_back(escaped);
                break;
            case 'b':
                out.push_back('\b');
                break;
            case 'f':
                out.push_back('\f');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            default:
                return fail("Unsupported string escape", error);
            }
        }

        return fail("Unterminated string", error);
    }

    bool parseNumber(JsonValue& value, std::string* error)
    {
        const char* begin = text_.c_str() + position_;
        char* end = nullptr;
        double number = std::strtod(begin, &end);
        if (end == begin)
            return fail("Expected number", error);

        position_ += static_cast<std::size_t>(end - begin);
        value = JsonValue();
        value.type = JsonValue::Type::Number;
        value.numberValue = number;
        return true;
    }

    bool consume(char expected)
    {
        if (position_ < text_.size() && text_[position_] == expected)
        {
            ++position_;
            return true;
        }
        return false;
    }

    bool matchLiteral(const char* literal)
    {
        std::size_t start = position_;
        while (*literal)
        {
            if (position_ >= text_.size() || text_[position_] != *literal)
            {
                position_ = start;
                return false;
            }
            ++position_;
            ++literal;
        }
        return true;
    }

    void skipWhitespace()
    {
        while (position_ < text_.size() &&
               std::isspace(static_cast<unsigned char>(text_[position_])))
        {
            ++position_;
        }
    }

    void skipUtf8Bom()
    {
        if (text_.size() >= 3 &&
            static_cast<unsigned char>(text_[0]) == 0xEF &&
            static_cast<unsigned char>(text_[1]) == 0xBB &&
            static_cast<unsigned char>(text_[2]) == 0xBF)
        {
            position_ = 3;
        }
    }

    bool fail(const std::string& message, std::string* error) const
    {
        if (error)
            *error = message + " at byte " + std::to_string(position_);
        return false;
    }

    const std::string& text_;
    std::size_t position_ = 0;
};

const JsonValue* objectField(const JsonValue& value, const std::string& key)
{
    if (value.type != JsonValue::Type::Object)
        return nullptr;

    auto it = value.objectValue.find(key);
    return it == value.objectValue.end() ? nullptr : &it->second;
}

bool readStringField(const JsonValue& object, const std::string& key, std::string& out)
{
    const JsonValue* value = objectField(object, key);
    if (!value || value->type != JsonValue::Type::String)
        return false;

    out = value->stringValue;
    return true;
}

bool readBoolField(const JsonValue& object, const std::string& key, bool& out)
{
    const JsonValue* value = objectField(object, key);
    if (!value || value->type != JsonValue::Type::Bool)
        return false;

    out = value->boolValue;
    return true;
}

bool readNumberField(const JsonValue& object, const std::string& key, float& out)
{
    const JsonValue* value = objectField(object, key);
    if (!value || value->type != JsonValue::Type::Number)
        return false;

    out = static_cast<float>(value->numberValue);
    return true;
}

bool readVec2Field(const JsonValue& object, const std::string& key, Vec2& out, std::string* error)
{
    const JsonValue* value = objectField(object, key);
    if (!value)
        return false;

    if (value->type != JsonValue::Type::Array || value->arrayValue.size() != 2 ||
        value->arrayValue[0].type != JsonValue::Type::Number ||
        value->arrayValue[1].type != JsonValue::Type::Number)
    {
        if (error)
            *error = "Expected '" + key + "' to be a two-number array";
        return false;
    }

    out.x = static_cast<float>(value->arrayValue[0].numberValue);
    out.y = static_cast<float>(value->arrayValue[1].numberValue);
    return true;
}

bool applyTransform(const JsonValue& entityObject, Entity& entity, std::string* error)
{
    const JsonValue* transform = objectField(entityObject, "transform");
    if (!transform)
        return true;
    if (transform->type != JsonValue::Type::Object)
    {
        if (error)
            *error = "Expected 'transform' to be an object";
        return false;
    }

    std::string fieldError;
    if (!readVec2Field(*transform, "position", entity.transform.position, &fieldError) && !fieldError.empty())
    {
        if (error)
            *error = fieldError;
        return false;
    }

    readNumberField(*transform, "rotationDegrees", entity.transform.rotationDegrees);

    fieldError.clear();
    if (!readVec2Field(*transform, "scale", entity.transform.scale, &fieldError) && !fieldError.empty())
    {
        if (error)
            *error = fieldError;
        return false;
    }

    return true;
}

bool applyProperties(const JsonValue& entityObject, Entity& entity, std::string* error)
{
    const JsonValue* properties = objectField(entityObject, "properties");
    if (!properties)
        return true;
    if (properties->type != JsonValue::Type::Object)
    {
        if (error)
            *error = "Expected 'properties' to be an object";
        return false;
    }

    for (const auto& entry : properties->objectValue)
    {
        if (entry.second.type == JsonValue::Type::String)
            entity.properties[entry.first] = entry.second.stringValue;
        else if (entry.second.type == JsonValue::Type::Number)
            entity.properties[entry.first] = std::to_string(entry.second.numberValue);
        else if (entry.second.type == JsonValue::Type::Bool)
            entity.properties[entry.first] = entry.second.boolValue ? "true" : "false";
        else if (entry.second.type == JsonValue::Type::Null)
            entity.properties[entry.first] = "";
        else
        {
            if (error)
                *error = "Scene entity property '" + entry.first + "' must be string, number, bool, or null";
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
    JsonValue root;
    JsonParser parser(text);
    if (!parser.parse(root, error))
        return false;

    if (root.type != JsonValue::Type::Object)
    {
        if (error)
            *error = "Scene root must be an object";
        return false;
    }

    const JsonValue* entities = objectField(root, "entities");
    if (!entities || entities->type != JsonValue::Type::Array)
    {
        if (error)
            *error = "Scene must contain an 'entities' array";
        return false;
    }

    scene.clear();

    std::string sceneName;
    if (readStringField(root, "name", sceneName))
        scene.setName(sceneName);

    for (const JsonValue& entityValue : entities->arrayValue)
    {
        if (entityValue.type != JsonValue::Type::Object)
        {
            if (error)
                *error = "Each scene entity must be an object";
            return false;
        }

        std::string entityName;
        readStringField(entityValue, "name", entityName);

        Entity& entity = scene.createEntity(entityName);
        readBoolField(entityValue, "active", entity.active);

        if (!applyTransform(entityValue, entity, error))
            return false;
        if (!applyProperties(entityValue, entity, error))
            return false;
    }

    return true;
}

} // namespace ycode
