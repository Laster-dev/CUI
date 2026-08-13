#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ProgressBar.h"
#include "ProgressBarDiag.h"
#include "../animation/AnimationManager.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace CUI {

namespace {
float WinUI3EaseInOut(float t) {
    return t < 0.5f
        ? 4.0f * t * t * t
        : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

// Max relative chunk width used by the WinUI3 primary indicator formula.
constexpr float kMaxChunkFrac = 0.12f + 0.38f;
} // namespace

ProgressBar::ProgressBar() {
    SetFillColorToken(ThemeTokenId::AccentColor);
    SetTrackColorToken(ThemeTokenId::CardBorder);
    SetWidth(200.0f);
    SetHeight(3.0f);
    SetCornerRadius(1.5f);
    m_displayValue = GetValue();
    SetHoverBackgroundToken(ThemeTokenId::Unset);
    SetHoverBackground(D2D1::ColorF(0, 0, 0, 0));
    SetBackground(D2D1::ColorF(0, 0, 0, 0));
    ProgressBarDiag::Log("[PB] ctor this=%p", (void*)this);
}

ProgressBar::~ProgressBar() {
    ProgressBarDiag::Log("[PB] dtor this=%p indeterminate=%d", (void*)this, m_isIndeterminate ? 1 : 0);
    m_hostVisual.Reset();
    m_hostClip.Reset();
    m_pill1 = {};
    m_pill2 = {};
    m_overlayAttached = false;
}

void ProgressBar::SetIsIndeterminate(bool ind) {
    if (m_isIndeterminate == ind) {
        return;
    }
    ProgressBarDiag::Log(
        "[PB] SetIsIndeterminate this=%p ind=%d -> %d boundsEmpty=%d",
        (void*)this,
        m_isIndeterminate ? 1 : 0,
        ind ? 1 : 0,
        m_bounds.IsEmpty() ? 1 : 0);
    m_isIndeterminate = ind;
    NotifyFieldChanged(PropertyId::IsIndeterminate, Value(ind));
    MarkRenderRectDirty(m_bounds);
    if (ind) {
        RequestAnimationTicks();
        ProgressBarDiag::Log(
            "[PB] SetIsIndeterminate arm ticksReg=%d (0=rejected until live-tree / OnNavigatedTo)",
            IsAnimationTicksRegistered() ? 1 : 0);
    } else {
        m_releaseOverlay = true;
    }
}

void ProgressBar::OnNavigatedFrom() {
    ProgressBarDiag::Log("[PB] OnNavigatedFrom this=%p ind=%d", (void*)this, m_isIndeterminate ? 1 : 0);
    m_releaseOverlay = true;
    UIElement::OnNavigatedFrom();
}

void ProgressBar::ReleaseComposeOverlay(GraphicsContext* ctx) {
    if (m_overlayAttached && m_hostVisual && ctx) {
        ctx->DetachCompositionOverlay(m_hostVisual.Get());
    }
    m_hostVisual.Reset();
    m_hostClip.Reset();
    m_pill1 = {};
    m_pill2 = {};
    m_pillMaxWDips = 0.0f;
    m_hostWidthPx = 0;
    m_hostHeightPx = 0;
    m_overlayAttached = false;
    m_pillsParented = false;
    m_useDcompOverlay = false;
    m_releaseOverlay = false;
}

std::vector<PropertyMeta> ProgressBar::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "value", "进度数值 (Value)", "进度配置", "number" });
    metas.push_back({ "minimum", "最小值 (Minimum)", "进度配置", "number" });
    metas.push_back({ "maximum", "最大值 (Maximum)", "进度配置", "number" });
    metas.push_back({ "isIndeterminate", "不确定模式 (IsIndeterminate)", "进度配置", "bool" });
    return metas;
}

Size ProgressBar::Measure(Size availableSize) {
    (void)availableSize;
    float expW = GetWidth(); if (expW < 0) expW = 200.0f;
    float expH = GetHeight(); if (expH < 0) expH = 3.0f;
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

bool ProgressBar::IsComposeOnlyAnimation() const {
    // DComp overlay + Commit without a real Present is not reliably visible on
    // CreateSwapChainForComposition, and a DXGI_PRESENT_DO_NOT_SEQUENCE kick
    // stalled the flip queue (~1 FPS, dead NavigationView). Use scene strip Present.
    return false;
}

void ProgressBar::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    if (m_bounds.IsEmpty()) {
        return;
    }
    if (!IsIndeterminate()
        && !(UIElement::AreAnimationsEnabled() && std::abs(GetValue() - m_displayValue) > 0.01f)) {
        return;
    }
    const Rect area = m_bounds.Inflate(2.0f);
    dirtyRect = hasDirty ? dirtyRect.Union(area) : area;
    hasDirty = true;
}

bool ProgressBar::OnAnimationTick() {
    if (AnimationManager* mgr = AnimationManager::Current()) {
        if (!mgr->IsInLiveTree(this)) {
            return false;
        }
    }

    const float deltaSeconds = UIElement::GetAnimationDeltaSeconds();
    m_lastTickTime = std::chrono::steady_clock::now();

    if (IsIndeterminate()) {
        if (m_visibility != Visibility::Visible || m_bounds.IsEmpty()) {
            ProgressBarDiag::Log(
                "[PB] tick abort: visible=%d emptyBounds=%d",
                m_visibility == Visibility::Visible ? 1 : 0,
                m_bounds.IsEmpty() ? 1 : 0);
            return false;
        }
        m_animOffset += 1.0f * deltaSeconds;
        if (m_animOffset > 10000.0f) {
            m_animOffset = std::fmod(m_animOffset, 2.0f);
        }
        // Local strip dirty — Window Present1 should cover only this band.
        MarkRenderRectDirty(m_bounds);
        const unsigned n = ++ProgressBarDiag::TickCount();
        if (ProgressBarDiag::ShouldLogDetail(n)) {
            ProgressBarDiag::Log(
                "[PB] tick#%u dt=%.4f anim=%.3f useDcomp=0 fallbackDirty=1 bounds=(%.1f,%.1f,%.1fx%.1f)",
                n,
                deltaSeconds,
                m_animOffset,
                m_bounds.x, m_bounds.y, m_bounds.width, m_bounds.height);
        }
        RequestAnimationTicks();
        return true;
    }

    if (!UIElement::AreAnimationsEnabled()) {
        m_displayValue = GetValue();
        return false;
    }

    float target = GetValue();
    float delta = target - m_displayValue;
    if (std::abs(delta) <= 0.01f) {
        m_displayValue = target;
        return false;
    }
    float smoothing = 1.0f - std::exp(-12.0f * deltaSeconds);
    m_displayValue += delta * smoothing;
    MarkRenderRectDirty(m_bounds);
    RequestAnimationTicks();
    return true;
}

bool ProgressBar::HasSelfAnimation() const {
    if (m_visibility != Visibility::Visible || m_bounds.IsEmpty()) {
        return false;
    }
    return IsIndeterminate()
        || (UIElement::AreAnimationsEnabled() && std::abs(GetValue() - m_displayValue) > 0.01f);
}

void ProgressBar::UpdatePillTransform(
    ComposePill& pill, float localXDips, float chunkWDips, float dpi, bool visible) {
    if (!pill.transform || !pill.visual) {
        return;
    }
    const float sx = (!visible || m_pillMaxWDips <= 0.001f)
        ? 0.0f
        : std::clamp(chunkWDips / m_pillMaxWDips, 0.0f, 1.0f);
    const float tx = localXDips * dpi;
    D2D_MATRIX_3X2_F m = {
        sx, 0.0f,
        0.0f, visible ? 1.0f : 0.0f,
        tx, 0.0f
    };
    pill.transform->SetMatrix(m);
}

bool ProgressBar::EnsureComposeOverlay(GraphicsContext& ctx, float dpi) {
    auto* device = ctx.GetDCompDevice();
    if (!device || !ctx.GetDCompRootVisual()) {
        ProgressBarDiag::Log(
            "[PB] EnsureCompose FAIL: device=%p root=%p usesCompSC=%d",
            (void*)device,
            (void*)ctx.GetDCompRootVisual(),
            ctx.UsesCompositionSwapChain() ? 1 : 0);
        return false;
    }

    const UINT widthPx = (std::max)(1u, static_cast<UINT>(std::ceil(m_bounds.width * dpi)));
    const UINT heightPx = (std::max)(1u, static_cast<UINT>(std::ceil(m_bounds.height * dpi)));
    const float pillMaxW = (std::max)(2.0f, m_bounds.width * kMaxChunkFrac);
    const UINT pillWPx = (std::max)(1u, static_cast<UINT>(std::ceil(pillMaxW * dpi)));

    const bool sizeChanged =
        widthPx != m_hostWidthPx
        || heightPx != m_hostHeightPx
        || std::abs(pillMaxW - m_pillMaxWDips) > 0.5f;

    if (sizeChanged) {
        ProgressBarDiag::Log(
            "[PB] EnsureCompose sizeChange host=%ux%u pillMaxW=%.2f pillWPx=%u",
            widthPx, heightPx, pillMaxW, pillWPx);
        if (m_overlayAttached && m_hostVisual) {
            ctx.DetachCompositionOverlay(m_hostVisual.Get());
            m_overlayAttached = false;
        }
        m_hostVisual.Reset();
        m_hostClip.Reset();
        m_pill1 = {};
        m_pill2 = {};
        m_hostWidthPx = widthPx;
        m_hostHeightPx = heightPx;
        m_pillMaxWDips = pillMaxW;
        m_pillsParented = false;
    }

    if (!m_hostVisual) {
        if (FAILED(device->CreateVisual(&m_hostVisual)) || !m_hostVisual) {
            ProgressBarDiag::Log("[PB] EnsureCompose FAIL: CreateVisual host");
            return false;
        }
        if (FAILED(device->CreateRectangleClip(&m_hostClip)) || !m_hostClip) {
            ProgressBarDiag::Log("[PB] EnsureCompose FAIL: CreateRectangleClip");
            return false;
        }
        m_hostClip->SetLeft(0.0f);
        m_hostClip->SetTop(0.0f);
        m_hostClip->SetRight(static_cast<float>(widthPx));
        m_hostClip->SetBottom(static_cast<float>(heightPx));
        m_hostVisual->SetClip(m_hostClip.Get());
    }

    auto ensurePill = [&](ComposePill& pill, const char* name) -> bool {
        if (!pill.visual) {
            if (FAILED(device->CreateVisual(&pill.visual)) || !pill.visual) {
                ProgressBarDiag::Log("[PB] EnsureCompose FAIL: CreateVisual %s", name);
                return false;
            }
        }
        if (!pill.transform) {
            if (FAILED(device->CreateMatrixTransform(&pill.transform)) || !pill.transform) {
                ProgressBarDiag::Log("[PB] EnsureCompose FAIL: CreateMatrixTransform %s", name);
                return false;
            }
            pill.visual->SetTransform(pill.transform.Get());
        }
        if (!pill.surface) {
            if (!ctx.EnsureCompositionSurface(pill.surface, pillWPx, heightPx) || !pill.surface) {
                ProgressBarDiag::Log("[PB] EnsureCompose FAIL: CreateSurface %s %ux%u", name, pillWPx, heightPx);
                return false;
            }
            float radius = GetCornerRadius();
            if (radius < 0.0f) radius = m_bounds.height * 0.5f;
            D2D1_COLOR_F fillBg = ResolveThemeColor(GetFillColorToken(), ThemeTokenId::AccentColor);
            const bool drawn = ctx.DrawCompositionSurface(
                pill.surface.Get(),
                pillMaxW,
                m_bounds.height,
                [&]() {
                    ctx.FillRoundedRect(Rect(0.0f, 0.0f, pillMaxW, m_bounds.height), radius, fillBg);
                });
            if (!drawn) {
                ProgressBarDiag::Log("[PB] EnsureCompose FAIL: DrawCompositionSurface %s", name);
                pill.surface.Reset();
                return false;
            }
            pill.visual->SetContent(pill.surface.Get());
            ProgressBarDiag::Log("[PB] EnsureCompose pill %s rasterized once %.1fx%.1f dips", name, pillMaxW, m_bounds.height);
        }
        return true;
    };

    if (!ensurePill(m_pill1, "pill1") || !ensurePill(m_pill2, "pill2")) {
        return false;
    }

    if (!m_pillsParented) {
        HRESULT hr1 = m_hostVisual->AddVisual(m_pill1.visual.Get(), TRUE, nullptr);
        HRESULT hr2 = m_hostVisual->AddVisual(m_pill2.visual.Get(), TRUE, nullptr);
        ProgressBarDiag::Log("[PB] EnsureCompose AddVisual pills hr=0x%08X 0x%08X", (unsigned)hr1, (unsigned)hr2);
        m_pillsParented = true;
    }

    if (!m_overlayAttached) {
        if (!ctx.AttachCompositionOverlay(m_hostVisual.Get())) {
            ProgressBarDiag::Log("[PB] EnsureCompose FAIL: AttachCompositionOverlay");
            return false;
        }
        m_overlayAttached = true;
        ProgressBarDiag::Log("[PB] EnsureCompose OK attached host %ux%u dpi=%.2f", widthPx, heightPx, dpi);
    }
    return true;
}

bool ProgressBar::ComposePresent(GraphicsContext& ctx) {
    auto fail = [&](const char* why) -> bool {
        ++ProgressBarDiag::ComposeFail();
        m_useDcompOverlay = false;
        MarkRenderRectDirty(m_bounds);
        ProgressBarDiag::Log(
            "[PB] ComposePresent FAIL#%u: %s usesSC=%d device=%p",
            ProgressBarDiag::ComposeFail(),
            why,
            ctx.UsesCompositionSwapChain() ? 1 : 0,
            (void*)ctx.GetDCompDevice());
        return false;
    };

    if (m_releaseOverlay || !IsIndeterminate()) {
        ReleaseComposeOverlay(&ctx);
        ProgressBarDiag::Log("[PB] ComposePresent skip: %s", m_releaseOverlay ? "releaseOverlay" : "notIndeterminate");
        return false;
    }
    if (m_visibility != Visibility::Visible || m_bounds.IsEmpty()) {
        ReleaseComposeOverlay(&ctx);
        return fail("notVisibleOrEmpty");
    }
    if (!ctx.UsesCompositionSwapChain() || !ctx.GetDCompDevice()) {
        return fail("noCompositionSwapchainOrDevice");
    }

    const float dpi = ctx.GetDpiScale() > 0.001f ? ctx.GetDpiScale() : 1.0f;
    if (!EnsureComposeOverlay(ctx, dpi)) {
        return fail("EnsureComposeOverlay");
    }

    m_hostVisual->SetOffsetX(m_bounds.x * dpi);
    m_hostVisual->SetOffsetY(m_bounds.y * dpi);

    const float W = m_bounds.width;
    constexpr float cycleDur = 2.0f;
    float animTime = m_animOffset;
    if (!UIElement::AreAnimationsEnabled()) {
        animTime = std::floor(m_animOffset * 8.0f) / 8.0f;
    }
    float tNorm = std::fmod(animTime, cycleDur) / cycleDur;
    if (tNorm < 0.0f) {
        tNorm += 1.0f;
    }

    if (tNorm >= 0.0f && tNorm <= 0.75f) {
        const float p1 = tNorm / 0.75f;
        const float e1 = WinUI3EaseInOut(p1);
        const float startX = -0.35f * W;
        const float endX = 1.2f * W;
        const float curX = startX + e1 * (endX - startX);
        const float chunkW = W * (0.12f + 0.38f * std::sin(p1 * 3.14159265f));
        UpdatePillTransform(m_pill1, curX, chunkW, dpi, true);
    } else {
        UpdatePillTransform(m_pill1, 0.0f, 0.0f, dpi, false);
    }

    if (tNorm >= 0.35f && tNorm <= 1.0f) {
        const float p2 = (tNorm - 0.35f) / 0.65f;
        const float e2 = WinUI3EaseInOut(p2);
        const float startX = -0.25f * W;
        const float endX = 1.15f * W;
        const float curX = startX + e2 * (endX - startX);
        const float chunkW = W * (0.08f + 0.28f * std::sin(p2 * 3.14159265f));
        UpdatePillTransform(m_pill2, curX, chunkW, dpi, true);
    } else {
        UpdatePillTransform(m_pill2, 0.0f, 0.0f, dpi, false);
    }

    m_useDcompOverlay = true;
    const unsigned ok = ++ProgressBarDiag::ComposeOk();
    if (ProgressBarDiag::ShouldLogDetail(ok)) {
        ProgressBarDiag::Log(
            "[PB] ComposePresent OK#%u tNorm=%.3f hostOffset=(%.1f,%.1f) attached=%d",
            ok, tNorm, m_bounds.x * dpi, m_bounds.y * dpi, m_overlayAttached ? 1 : 0);
    }
    return true;
}

void ProgressBar::DrawIndeterminateFallback(GraphicsContext& ctx) const {
    float radius = GetCornerRadius();
    if (radius < 0.0f) radius = m_bounds.height * 0.5f;
    D2D1_COLOR_F fillBg = ResolveThemeColor(GetFillColorToken(), ThemeTokenId::AccentColor);

    ctx.PushClip(m_bounds);
    const float W = m_bounds.width;
    constexpr float cycleDur = 2.0f;
    float animTime = m_animOffset;
    if (!UIElement::AreAnimationsEnabled()) {
        animTime = std::floor(m_animOffset * 8.0f) / 8.0f;
    }
    float tNorm = std::fmod(animTime, cycleDur) / cycleDur;
    if (tNorm < 0.0f) {
        tNorm += 1.0f;
    }

    if (tNorm >= 0.0f && tNorm <= 0.75f) {
        const float p1 = tNorm / 0.75f;
        const float e1 = WinUI3EaseInOut(p1);
        const float startX = m_bounds.x - 0.35f * W;
        const float endX = m_bounds.x + 1.2f * W;
        const float curX = startX + e1 * (endX - startX);
        const float chunkW = W * (0.12f + 0.38f * std::sin(p1 * 3.14159265f));
        ctx.FillRoundedRect(Rect(curX, m_bounds.y, chunkW, m_bounds.height), radius, fillBg);
    }
    if (tNorm >= 0.35f && tNorm <= 1.0f) {
        const float p2 = (tNorm - 0.35f) / 0.65f;
        const float e2 = WinUI3EaseInOut(p2);
        const float startX = m_bounds.x - 0.25f * W;
        const float endX = m_bounds.x + 1.15f * W;
        const float curX = startX + e2 * (endX - startX);
        const float chunkW = W * (0.08f + 0.28f * std::sin(p2 * 3.14159265f));
        ctx.FillRoundedRect(Rect(curX, m_bounds.y, chunkW, m_bounds.height), radius, fillBg);
    }
    ctx.PopClip();
}

void ProgressBar::OnRender(GraphicsContext& ctx) {
    if (IsIndeterminate()) {
        const unsigned n = ++ProgressBarDiag::OnRenderIndeterminate();
        if (ProgressBarDiag::ShouldLogDetail(n)) {
            ProgressBarDiag::Log(
                "[PB] OnRender indeterminate#%u this=%p ticksReg=%d useDcomp=%d "
                "bounds=(%.1f,%.1f,%.1fx%.1f) usesSC=%d",
                n,
                (void*)this,
                IsAnimationTicksRegistered() ? 1 : 0,
                m_useDcompOverlay ? 1 : 0,
                m_bounds.x, m_bounds.y, m_bounds.width, m_bounds.height,
                ctx.UsesCompositionSwapChain() ? 1 : 0);
        }
    }

    if (IsIndeterminate() && !IsAnimationTicksRegistered()) {
        RequestAnimationTicks();
    } else if (!IsIndeterminate()
        && UIElement::AreAnimationsEnabled()
        && std::abs(GetValue() - m_displayValue) > 0.01f
        && !IsAnimationTicksRegistered()) {
        RequestAnimationTicks();
    }

    if (m_releaseOverlay) {
        ReleaseComposeOverlay(&ctx);
    }

    float radius = GetCornerRadius();
    if (radius < 0.0f) radius = m_bounds.height * 0.5f;
    D2D1_COLOR_F trackBg = ResolveThemeColor(GetTrackColorToken(), ThemeTokenId::CardBorder);
    D2D1_COLOR_F fillBg = ResolveThemeColor(GetFillColorToken(), ThemeTokenId::AccentColor);

    ctx.FillRoundedRect(m_bounds, radius, trackBg);

    if (IsIndeterminate()) {
        // Always scene-draw pills; detach any leftover DComp host from earlier experiments.
        if (m_overlayAttached || m_hostVisual) {
            ReleaseComposeOverlay(&ctx);
        }
        DrawIndeterminateFallback(ctx);
        return;
    }

    if (!IsAnimationTicksRegistered()
        && std::abs(GetValue() - m_displayValue) > 0.01f) {
        if (!UIElement::AreAnimationsEnabled()) {
            m_displayValue = GetValue();
        }
    }

    float minVal = GetMinimum();
    float maxVal = GetMaximum();
    float val = std::clamp(m_displayValue, minVal, maxVal);
    float ratio = (maxVal > minVal) ? (val - minVal) / (maxVal - minVal) : 0.0f;
    float fillW = m_bounds.width * ratio;
    if (fillW > 0.0f) {
        ctx.FillRoundedRect(Rect(m_bounds.x, m_bounds.y, fillW, m_bounds.height), radius, fillBg);
    }
}

} // namespace CUI
