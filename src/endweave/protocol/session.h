#pragma once

#include <any>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace endweave {

/** Per-connection state that transforms keep across packets. Both directions of a connection
 * share one session.
 * @see ViaVersion UserConnection, UserConnectionImpl. */
class Session {
public:
    Session() = default;
    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;

    /** @see ViaVersion UserConnection#get(Class). */
    template <class T>
    [[nodiscard]] T *get()
    {
        const auto it = storage_.find(std::type_index(typeid(T)));
        return it == storage_.end() ? nullptr : std::any_cast<T>(&it->second);
    }

    /** @see ViaVersion UserConnection#put(StorableObject). */
    template <class T>
    void put(T value)
    {
        storage_[std::type_index(typeid(T))] = std::move(value);
    }

    /** @see ViaVersion UserConnection#has(Class). */
    template <class T>
    [[nodiscard]] bool has() const
    {
        return storage_.contains(std::type_index(typeid(T)));
    }

    /** @see ViaVersion UserConnection#remove(Class). */
    template <class T>
    void remove()
    {
        storage_.erase(std::type_index(typeid(T)));
    }

private:
    std::unordered_map<std::type_index, std::any> storage_;
};

} // namespace endweave
