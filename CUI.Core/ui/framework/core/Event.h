#pragma once
#include <functional>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace CUI {

using EventId = uint64_t;

template<typename... Args>
class Event {
public:
    using Handler = std::function<void(Args...)>;

    struct Connection {
        EventId id;
        Handler handler;
    };

    EventId Connect(Handler handler) {
        EventId id = ++m_nextId;
        m_handlers.push_back({ id, handler });
        return id;
    }

    void Disconnect(EventId id) {
        m_handlers.erase(
            std::remove_if(m_handlers.begin(), m_handlers.end(),
                [id](const Connection& conn) { return conn.id == id; }),
            m_handlers.end());
    }

    void Invoke(Args... args) const {
        for (const auto& conn : m_handlers) {
            if (conn.handler) {
                conn.handler(args...);
            }
        }
    }

    void Clear() {
        m_handlers.clear();
    }

private:
    std::vector<Connection> m_handlers;
    EventId m_nextId = 0;
};

} // namespace CUI
