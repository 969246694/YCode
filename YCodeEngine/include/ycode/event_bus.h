#ifndef YCODE_EVENT_BUS_H
#define YCODE_EVENT_BUS_H

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ycode {

struct Event {
    std::string type;
    std::unordered_map<std::string, std::string> fields;
};

class EventBus {
public:
    using Handler = std::function<void(const Event&)>;
    using SubscriptionId = std::uint64_t;

    SubscriptionId subscribe(const std::string& eventType, Handler handler);
    bool unsubscribe(SubscriptionId id);
    void publish(const Event& event) const;
    void clear();

private:
    struct Subscription {
        SubscriptionId id;
        std::string eventType;
        Handler handler;
    };

    SubscriptionId nextId_ = 1;
    std::vector<Subscription> subscriptions_;
};

} // namespace ycode

#endif // YCODE_EVENT_BUS_H

