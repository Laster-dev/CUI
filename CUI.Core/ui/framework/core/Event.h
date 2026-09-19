#pragma once
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace CUI {

using EventId = uint64_t;

template<typename... Args>
class Event {
public:
    using Handler = std::function<void(Args...)>;

    struct Connection {
        EventId id = 0;
        Handler handler;
        // 共享的“存活”标志：Disconnect 后，即使其它调用方仍持有旧快照，也不会再触发该 handler。
        std::shared_ptr<std::atomic<bool>> alive;
    };

    EventId Connect(Handler handler) {
        const EventId id = ++m_nextId;
        Connection conn;
        conn.id = id;
        conn.handler = std::move(handler);
        conn.alive = std::make_shared<std::atomic<bool>>(true);
        EnsureUniqueStorage();
        m_handlers->push_back(std::move(conn));
        return id;
    }

    EventId Subscribe(Handler handler) {
        return Connect(std::move(handler));
    }

    void Disconnect(EventId id) {
        if (!m_handlers) {
            return;
        }
        // 先让所有旧快照失效，保证正在进行的 Invoke 不会再调用该 handler。
        for (const Connection& conn : *m_handlers) {
            if (conn.id == id && conn.alive) {
                conn.alive->store(false, std::memory_order_release);
            }
        }
        EnsureUniqueStorage();
        auto& handlers = *m_handlers;
        handlers.erase(
            std::remove_if(handlers.begin(), handlers.end(),
                [id](const Connection& conn) { return conn.id == id; }),
            handlers.end());
    }

    // 触发事件：仅做一次引用计数递增（写时复制），不再整体拷贝 handler 列表，
    // 也无需对每个 handler 做 O(n) 查找——整体复杂度由 O(n^2) 降为 O(n)。
    void Invoke(Args... args) const {
        std::shared_ptr<const std::vector<Connection>> snapshot = m_handlers;
        if (!snapshot) {
            return;
        }
        for (const Connection& conn : *snapshot) {
            if (!conn.handler) {
                continue;
            }
            if (conn.alive && !conn.alive->load(std::memory_order_acquire)) {
                continue;
            }
            conn.handler(args...);
        }
    }

    void Clear() {
        if (!m_handlers) {
            return;
        }
        for (const Connection& conn : *m_handlers) {
            if (conn.alive) {
                conn.alive->store(false, std::memory_order_release);
            }
        }
        EnsureUniqueStorage();
        m_handlers->clear();
    }

    bool Empty() const { return !m_handlers || m_handlers->empty(); }
    std::size_t Size() const { return m_handlers ? m_handlers->size() : 0u; }

private:
    // 写前复制：若当前存储正被某次 Invoke 的快照持有，则先复制一份再修改，
    // 避免遍历过程中容器失效（use-after-free）。
    void EnsureUniqueStorage() {
        if (!m_handlers) {
            m_handlers = std::make_shared<std::vector<Connection>>();
            return;
        }
        if (m_handlers.use_count() > 1) {
            m_handlers = std::make_shared<std::vector<Connection>>(*m_handlers);
        }
    }

    std::shared_ptr<std::vector<Connection>> m_handlers;
    EventId m_nextId = 0;
};

} // namespace CUI
