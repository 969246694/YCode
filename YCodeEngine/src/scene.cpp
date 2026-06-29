#include "ycode/scene.h"

#include <algorithm>
#include <utility>

namespace ycode {

Scene::Scene(std::string name)
    : name_(std::move(name))
{
}

Entity& Scene::createEntity(const std::string& name)
{
    Entity entity;
    entity.id = nextEntityId_++;
    entity.name = name.empty() ? "Entity " + std::to_string(entity.id) : name;
    entities_.push_back(std::move(entity));
    return entities_.back();
}

bool Scene::destroyEntity(EntityId id)
{
    auto before = entities_.size();
    entities_.erase(std::remove_if(entities_.begin(), entities_.end(),
                                   [id](const Entity& entity) {
                                       return entity.id == id;
                                   }),
                    entities_.end());
    return entities_.size() != before;
}

Entity* Scene::findEntity(EntityId id)
{
    auto it = std::find_if(entities_.begin(), entities_.end(),
                           [id](const Entity& entity) {
                               return entity.id == id;
                           });
    return it == entities_.end() ? nullptr : &(*it);
}

const Entity* Scene::findEntity(EntityId id) const
{
    auto it = std::find_if(entities_.begin(), entities_.end(),
                           [id](const Entity& entity) {
                               return entity.id == id;
                           });
    return it == entities_.end() ? nullptr : &(*it);
}

Entity* Scene::findEntityByName(const std::string& name)
{
    auto it = std::find_if(entities_.begin(), entities_.end(),
                           [&name](const Entity& entity) {
                               return entity.name == name;
                           });
    return it == entities_.end() ? nullptr : &(*it);
}

const Entity* Scene::findEntityByName(const std::string& name) const
{
    auto it = std::find_if(entities_.begin(), entities_.end(),
                           [&name](const Entity& entity) {
                               return entity.name == name;
                           });
    return it == entities_.end() ? nullptr : &(*it);
}

std::vector<Entity>& Scene::entities()
{
    return entities_;
}

const std::vector<Entity>& Scene::entities() const
{
    return entities_;
}

std::size_t Scene::entityCount() const
{
    return entities_.size();
}

bool Scene::empty() const
{
    return entities_.empty();
}

void Scene::clear()
{
    entities_.clear();
    nextEntityId_ = 1;
}

void Scene::update(float deltaSeconds)
{
    if (updateHandler_)
        updateHandler_(*this, deltaSeconds);
}

void Scene::setUpdateHandler(UpdateHandler handler)
{
    updateHandler_ = std::move(handler);
}

const std::string& Scene::name() const
{
    return name_;
}

void Scene::setName(std::string name)
{
    name_ = std::move(name);
}

} // namespace ycode
