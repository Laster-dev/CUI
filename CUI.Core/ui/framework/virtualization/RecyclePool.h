#pragma once

#include "../controls/UIElement.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace CUI {

/**
 * @brief 通用组件回收复用池（RecyclePool）。
 * 针对虚拟化容器（如 VirtualizingStackPanel、ListView、DataGrid），
 * 负责缓存滚出可视区域的控件实例，并在新数据滚入视口时进行复用（Re-bind），
 * 消除高频滚动时反复 allocate/deallocate UIElement 导致的 GC/堆内存颠簸与卡顿。
 */
class RecyclePool {
public:
    explicit RecyclePool(size_t defaultMaxCapacity = 64)
        : m_defaultMaxCapacity(defaultMaxCapacity) {}

    ~RecyclePool() {
        Clear();
    }

    // 禁用拷贝，允许移动
    RecyclePool(const RecyclePool&) = delete;
    RecyclePool& operator=(const RecyclePool&) = delete;
    RecyclePool(RecyclePool&&) noexcept = default;
    RecyclePool& operator=(RecyclePool&&) noexcept = default;

    /**
     * @brief 从复用池获取一个指定类型键的空闲控件实例，若池中无空闲实例则调用 factory 新建。
     * @param typeKey 控件类型或模板标识（如 "default", "header", "image_row"）
     * @param factory 当池为空时的创建工厂函数
     * @return 准备复用或全新创建的 UIElement 实例
     */
    std::shared_ptr<UIElement> Acquire(
        const std::string& typeKey,
        const std::function<std::shared_ptr<UIElement>()>& factory)
    {
        auto it = m_pools.find(typeKey);
        if (it != m_pools.end() && !it->second.empty()) {
            auto element = std::move(it->second.back());
            it->second.pop_back();
            ++m_totalReused;
            return element;
        }

        ++m_totalCreated;
        if (factory) {
            return factory();
        }
        return nullptr;
    }

    /**
     * @brief 释放一个控件实例回收到复用池中。
     * @param typeKey 对应的控件类型或模板标识
     * @param element 需要回收的控件实例
     */
    void Release(const std::string& typeKey, std::shared_ptr<UIElement> element) {
        if (!element) {
            return;
        }

        // 解除视觉树关系
        if (element->GetParent()) {
            element->ApplyParent(nullptr);
        }

        size_t capacity = GetCapacityFor(typeKey);
        auto& pool = m_pools[typeKey];
        if (pool.size() < capacity) {
            pool.push_back(std::move(element));
            ++m_totalRecycled;
        } else {
            // 超出缓存容量，自然析构销毁
            ++m_totalDiscarded;
        }
    }

    /**
     * @brief 设定特定类型槽位的最大缓存数量。
     */
    void SetCapacity(const std::string& typeKey, size_t maxCapacity) {
        m_capacities[typeKey] = maxCapacity;
        auto it = m_pools.find(typeKey);
        if (it != m_pools.end() && it->second.size() > maxCapacity) {
            it->second.resize(maxCapacity);
        }
    }

    /**
     * @brief 获取指定类型槽位当前已缓存的空闲实例数。
     */
    size_t GetPoolCount(const std::string& typeKey) const {
        auto it = m_pools.find(typeKey);
        return (it != m_pools.end()) ? it->second.size() : 0;
    }

    /**
     * @brief 获取所有槽位的空闲实例总数。
     */
    size_t GetTotalPoolCount() const {
        size_t sum = 0;
        for (const auto& [_, pool] : m_pools) {
            sum += pool.size();
        }
        return sum;
    }

    /**
     * @brief 清空复用池中所有缓存的控件。
     */
    void Clear() {
        m_pools.clear();
    }

    // 统计指标（供 DevTools / Visual Inspector 诊断）
    uint64_t GetTotalCreated() const { return m_totalCreated; }
    uint64_t GetTotalReused() const { return m_totalReused; }
    uint64_t GetTotalRecycled() const { return m_totalRecycled; }
    uint64_t GetTotalDiscarded() const { return m_totalDiscarded; }

private:
    size_t GetCapacityFor(const std::string& typeKey) const {
        auto it = m_capacities.find(typeKey);
        return (it != m_capacities.end()) ? it->second : m_defaultMaxCapacity;
    }

    size_t m_defaultMaxCapacity;
    std::unordered_map<std::string, std::vector<std::shared_ptr<UIElement>>> m_pools;
    std::unordered_map<std::string, size_t> m_capacities;

    uint64_t m_totalCreated = 0;
    uint64_t m_totalReused = 0;
    uint64_t m_totalRecycled = 0;
    uint64_t m_totalDiscarded = 0;
};

} // namespace CUI
