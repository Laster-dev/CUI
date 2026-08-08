#pragma once
#include "Control.h"
#include <chrono>
#include <dcomp.h>
#include <d2d1_1.h>
#include <wrl/client.h>

namespace CUI {

class ProgressBar : public Control {
public:
    ProgressBar();
    virtual ~ProgressBar();

    virtual const char* GetClassName() const override { return "ProgressBar"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual bool IsComposeOnlyAnimation() const override;
    virtual bool ComposePresent(GraphicsContext& ctx) override;
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const override;
    virtual void OnNavigatedFrom() override;

    float GetValue() const { return m_value; }
    void SetValue(float val) {
        m_value = val;
        NotifyFieldChanged(PropertyId::ControlValue, Value(val));
        if (!UIElement::AreAnimationsEnabled() || m_bounds.IsEmpty()) {
            m_displayValue = val;
        }
        MarkRenderRectDirty(m_bounds);
        RequestAnimationTicks();
    }

    float GetMinimum() const { return m_minimum; }
    void SetMinimum(float minVal) {
        m_minimum = minVal;
        NotifyFieldChanged(PropertyId::Minimum, Value(minVal));
        MarkRenderRectDirty(m_bounds);
    }

    float GetMaximum() const { return m_maximum; }
    void SetMaximum(float maxVal) {
        m_maximum = maxVal;
        NotifyFieldChanged(PropertyId::Maximum, Value(maxVal));
        MarkRenderRectDirty(m_bounds);
    }

    bool IsIndeterminate() const { return m_isIndeterminate; }
    void SetIsIndeterminate(bool ind);

private:
    struct ComposePill {
        Microsoft::WRL::ComPtr<IDCompositionVisual> visual;
        Microsoft::WRL::ComPtr<IDCompositionSurface> surface;
        Microsoft::WRL::ComPtr<IDCompositionMatrixTransform> transform;
    };

    void ReleaseComposeOverlay(GraphicsContext* ctx);
    bool EnsureComposeOverlay(GraphicsContext& ctx, float dpi);
    void UpdatePillTransform(ComposePill& pill, float localXDips, float chunkWDips, float dpi, bool visible);
    void DrawIndeterminateFallback(GraphicsContext& ctx) const;

    float m_value = 0.0f;
    float m_minimum = 0.0f;
    float m_maximum = 100.0f;
    bool m_isIndeterminate = false;
    float m_animOffset = 0.0f;
    float m_displayValue = 0.0f;
    std::chrono::steady_clock::time_point m_lastTickTime{};

    Microsoft::WRL::ComPtr<IDCompositionVisual> m_hostVisual;
    Microsoft::WRL::ComPtr<IDCompositionRectangleClip> m_hostClip;
    ComposePill m_pill1;
    ComposePill m_pill2;
    float m_pillMaxWDips = 0.0f;
    UINT m_hostWidthPx = 0;
    UINT m_hostHeightPx = 0;
    bool m_overlayAttached = false;
    bool m_pillsParented = false;
    bool m_useDcompOverlay = false;
    bool m_releaseOverlay = false;
};

} // namespace CUI
