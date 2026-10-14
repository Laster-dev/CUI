#pragma once

#include "RecyclePool.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace CUI {

/**
 * @brief 虚拟化数据项提供者抽象接口（IVirtualizingItemProvider）。
 * 为 VirtualizingStackPanel、VirtualList 等虚拟化面板提供数据项元信息、
 * 视口实例化绑定以及回收策略。
 */
class IVirtualizingItemProvider {
public:
    virtual ~IVirtualizingItemProvider() = default;

    /// 数据项总数（可达到百万/千万级别）
    virtual size_t GetItemCount() const = 0;

    /// 获取特定索引项的高度（像素）。如果全列表等高，直接返回固定值
    virtual float GetItemHeight(size_t index) const = 0;

    /// 获取特定索引项的类型标识（用于在 RecyclePool 中分类复用不同模板样式的容器）
    virtual std::string GetItemTypeKey(size_t index) const {
        (void)index;
        return "default";
    }

    /**
     * @brief 为指定索引实例化或重新绑定 UIElement。
     * @param index 当前数据项的全局索引
     * @param pool 控件回收复用池，可通过 pool.Acquire 获取复用实例
     */
    virtual std::shared_ptr<UIElement> RealizeItem(size_t index, RecyclePool& pool) = 0;

    /**
     * @brief 当某项滚出可视区域及缓冲区时触发回收通知。
     */
    virtual void RecycleItem(size_t index, std::shared_ptr<UIElement> element, RecyclePool& pool) {
        if (element) {
            pool.Release(GetItemTypeKey(index), std::move(element));
        }
    }
};

/**
 * @brief 基于函数回调的轻量虚拟化提供者（FunctionalVirtualizingItemProvider）。
 * 便于通过现代 Lambda 快速配置虚拟化数据。
 */
class FunctionalVirtualizingItemProvider : public IVirtualizingItemProvider {
public:
    using CountGetter = std::function<size_t()>;
    using HeightGetter = std::function<float(size_t)>;
    using TypeKeyGetter = std::function<std::string(size_t)>;
    using RealizeCallback = std::function<std::shared_ptr<UIElement>(size_t, RecyclePool&)>;
    using RecycleCallback = std::function<void(size_t, std::shared_ptr<UIElement>, RecyclePool&)>;

    FunctionalVirtualizingItemProvider(
        CountGetter countGetter,
        float fixedItemHeight,
        RealizeCallback realizeCallback)
        : m_countGetter(std::move(countGetter))
        , m_fixedItemHeight(fixedItemHeight)
        , m_realizeCallback(std::move(realizeCallback)) {}

    FunctionalVirtualizingItemProvider(
        CountGetter countGetter,
        HeightGetter heightGetter,
        RealizeCallback realizeCallback)
        : m_countGetter(std::move(countGetter))
        , m_fixedItemHeight(0.0f)
        , m_heightGetter(std::move(heightGetter))
        , m_realizeCallback(std::move(realizeCallback)) {}

    void SetTypeKeyGetter(TypeKeyGetter getter) { m_typeKeyGetter = std::move(getter); }
    void SetRecycleCallback(RecycleCallback callback) { m_recycleCallback = std::move(callback); }

    virtual size_t GetItemCount() const override {
        return m_countGetter ? m_countGetter() : 0;
    }

    virtual float GetItemHeight(size_t index) const override {
        if (m_fixedItemHeight > 0.0f) return m_fixedItemHeight;
        return m_heightGetter ? m_heightGetter(index) : 32.0f;
    }

    virtual std::string GetItemTypeKey(size_t index) const override {
        return m_typeKeyGetter ? m_typeKeyGetter(index) : "default";
    }

    virtual std::shared_ptr<UIElement> RealizeItem(size_t index, RecyclePool& pool) override {
        return m_realizeCallback ? m_realizeCallback(index, pool) : nullptr;
    }

    virtual void RecycleItem(size_t index, std::shared_ptr<UIElement> element, RecyclePool& pool) override {
        if (m_recycleCallback) {
            m_recycleCallback(index, element, pool);
        } else {
            IVirtualizingItemProvider::RecycleItem(index, element, pool);
        }
    }

private:
    CountGetter m_countGetter;
    float m_fixedItemHeight;
    HeightGetter m_heightGetter;
    TypeKeyGetter m_typeKeyGetter;
    RealizeCallback m_realizeCallback;
    RecycleCallback m_recycleCallback;
};

/**
 * @brief 基于 std::vector<T> 强类型集合的便捷提供者模板。
 */
template<typename T, typename BinderFn>
class VectorVirtualizingItemProvider : public IVirtualizingItemProvider {
public:
    VectorVirtualizingItemProvider(
        const std::vector<T>& data,
        float itemHeight,
        BinderFn binder)
        : m_data(data)
        , m_itemHeight(itemHeight)
        , m_binder(std::move(binder)) {}

    virtual size_t GetItemCount() const override {
        return m_data.size();
    }

    virtual float GetItemHeight(size_t /*index*/) const override {
        return m_itemHeight;
    }

    virtual std::shared_ptr<UIElement> RealizeItem(size_t index, RecyclePool& pool) override {
        if (index >= m_data.size()) return nullptr;
        return m_binder(index, m_data[index], pool);
    }

private:
    const std::vector<T>& m_data;
    float m_itemHeight;
    BinderFn m_binder;
};

} // namespace CUI
