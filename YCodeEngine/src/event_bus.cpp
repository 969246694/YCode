#include "ycode/event_bus.h"

#include <algorithm>
#include <utility>

namespace ycode {

EventBus::SubscriptionId EventBus::subscribe(const std::string& eventType, Handler handler)
{
    if (!handler)
        return 0;

    SubscriptionId id = nextId_++;
    subscriptions_.push_back({id, eventType, std::move(handler)});
    return id;
}

bool EventBus::unsubscribe(SubscriptionId id)
{
    auto oldSize = subscriptions_.size();
    subscriptions_.erase(
        std::remove_if(subscriptions_.begin(), subscriptions_.end(),
                       [id](const Subscription& sub) { return sub.id == id; }),
        subscriptions_.end());
    return subscriptions_.size() != oldSize;
}

void EventBus::publish(const Event& event) const
{
    for (const auto& sub : subscriptions_)
    {
        if (sub.eventType == event.type || sub.eventType == "*")
            sub.handler(event);
    }
}

void EventBus::clear()
{
    subscriptions_.clear();
}

} // namespace ycode
