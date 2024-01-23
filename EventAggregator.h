#ifndef __EVENTAGGREGATOR_H__
#define __EVENTAGGREGATOR_H__

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>

#include "SfaLogger.h"

class MsiSendEventContent;
class DarSendEventContent;
class SarSendEventContent;
class TestEventContent1;

template <typename T>
std::string type_name()
{
  typedef typename std::remove_reference<T>::type TR;

  std::string r = typeid(TR).name();
  if (std::is_const<TR>::value)
    r += " const";
  if (std::is_volatile<TR>::value)
    r += " volatile";
  if (std::is_lvalue_reference<T>::value)
    r += "&";
  else if (std::is_rvalue_reference<T>::value)
    r += "&&";
  return r;
}

template <typename TPayload>
class EventBase {
  private:
  std::vector<std::function<void(std::shared_ptr<TPayload>)>> subscriptions;
  std::mutex m;

  public:
  void InternalPublish(std::shared_ptr<TPayload> payload)
  {
    using namespace std;
    lock_guard<mutex> lock(m);

    if (subscriptions.size() == 0) {
      //DEBUG_LOG("No Subscriptions (%s)", typeid(TPayload).name());
    }
    for (auto &var : subscriptions) {
      var(payload);
    }
  }

  void InternalSubscribe(std::function<void(std::shared_ptr<TPayload>)> action)
  {
    using namespace std;

    lock_guard<mutex> lock(m);
    subscriptions.emplace_back(action);
  }

  void InternalUnSubscribe(std::function<void(std::shared_ptr<TPayload>)> action)
  {
    using namespace std;
    lock_guard<mutex> lock(m);

    function<void(std::shared_ptr<TPayload>)> ttt = nullptr;
    auto target = ttt.template target<void (*)(std::shared_ptr<TPayload>)>();
    // std::function<void(std::shared_ptr<TPayload>)> ttt = subscriptions[0];
    // ttt.target<void(*)(std::shared_ptr<TPayload>)>();

    std::cout << "t : " << action.target_type().name() << std::endl;

    auto result = std::find_if(
            subscriptions.begin(),
            subscriptions.end(),
            [&](std::function<void(std::shared_ptr<TPayload>)> subscription) {
              // Find subscribed function same as [action]'s name.
              auto ptr = subscription.template target<void (*)(std::shared_ptr<TPayload>)>();
              auto ptr2 = action.template target<void (*)(std::shared_ptr<TPayload>)>();

              std::cout << "s : " << subscription.target_type().name();
              if (ptr && *ptr && ptr2 && *ptr2) {
                if (*ptr == *ptr2) {
                  std::cout << " 0x" << std::hex << *ptr << " 0x" << std::hex << *ptr2 << std::endl;
                  return true;
                }
              }
              std::cout << std::endl;
              return false;
            });

    if (result != subscriptions.end()) {
      SFA_DEBUG("Remove subscription");
      subscriptions.erase(result);
    }
    else {
      SFA_DEBUG("Fail Remove subscription");
      for (size_t i = 0; i < subscriptions.size(); i++) {
        auto ptr = subscriptions[i].template target<void (*)(std::shared_ptr<TPayload>)>();

        /* code */
      }
    }
  }
};

/// Test
template <typename TPayload>
class PubSubEvent : private EventBase<TPayload> {
  public:
  // void Publish(std::shared_ptr<TPayload> payload) {
  //     EventBase<TPayload>::InternalPublish(payload);
  // }
  void Publish(std::shared_ptr<void> payload)
  {
    EventBase<TPayload>::InternalPublish(std::static_pointer_cast<TPayload>(payload));
  }

  void Subscribe(std::function<void(std::shared_ptr<TPayload>)> action)
  {
    EventBase<TPayload>::InternalSubscribe(std::move(action));
  }

  void UnSubscribe(std::function<void(std::shared_ptr<TPayload>)> action)
  {
    EventBase<TPayload>::InternalUnSubscribe(action);
  }
};

class EventAggregator {
  private:
  /// <summary>
  /// [TypeName, Event]
  /// </summary>
  std::unordered_map<std::string, std::shared_ptr<void>> events;
  std::mutex mutex;

  template <typename T>
  std::shared_ptr<T> InternalGetEvent()
  {
    std::lock_guard<std::mutex> lock(mutex);

    std::string name = type_name<T>();

    auto finded = events.find(name);

    if (finded == events.end()) {
      events[name] = std::make_shared<T>();

      return std::static_pointer_cast<T>(events[name]);
    }
    else {
      auto item = std::static_pointer_cast<T>(finded->second);
      std::string name(typeid(item).name());
      std::string typeName(typeid(std::shared_ptr<T>).name());

      if (name != typeName || item == nullptr) {
        events.erase(name);
        return nullptr;
      }

      return item;
    }
  }

  public:
  EventAggregator() = default;
  static std::shared_ptr<EventAggregator> GetInstance()
  {
    static std::shared_ptr<EventAggregator> instance = std::make_shared<EventAggregator>();
    return instance;
  }

  template <typename T>
  static std::shared_ptr<T> GetEvent()
  {
    return GetInstance()->InternalGetEvent<T>();
  }
};

#endif