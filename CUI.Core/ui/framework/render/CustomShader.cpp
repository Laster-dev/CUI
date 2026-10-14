#include "CustomShader.h"
#include "GraphicsContext.h"
#include <cmath>

namespace CUI {

CustomShader::CustomShader(BuiltInShaderType type) : m_type(type) {}
CustomShader::~CustomShader() = default;

void CustomShader::Render(GraphicsContext& ctx, const Rect& bounds, const ShaderParams& params) {
    if (bounds.IsEmpty()) return;

    ctx.PushClip(bounds);

    switch (m_type) {
    case BuiltInShaderType::FluidAcrylic:
        RenderFluidAcrylic(ctx, bounds, params);
        break;
    case BuiltInShaderType::MeshGradient:
        RenderMeshGradient(ctx, bounds, params);
        break;
    case BuiltInShaderType::GlowPulse:
        RenderGlowPulse(ctx, bounds, params);
        break;
    }

    ctx.PopClip();
}

void CustomShader::RenderMeshGradient(GraphicsContext& ctx, const Rect& bounds, const ShaderParams& params) {
    ID2D1DeviceContext* d2d = ctx.GetD2DContext();
    if (!d2d) return;

    const float t = params.time * params.speed;

    // 基础底色填充
    ctx.FillRect(bounds, Color(0.08f, 0.09f, 0.12f, 1.0f));

    struct MeshPoint {
        Point pt;
        Color color;
        float radius;
    };

    const float maxDim = (std::max)(bounds.width, bounds.height);
    const float r = maxDim * 0.75f;

    MeshPoint points[4] = {
        {
            Point(bounds.x + bounds.width * (0.25f + 0.18f * std::sin(t * 0.8f)),
                  bounds.y + bounds.height * (0.25f + 0.18f * std::cos(t * 0.6f))),
            params.primaryColor,
            r
        },
        {
            Point(bounds.x + bounds.width * (0.75f + 0.18f * std::cos(t * 0.7f)),
                  bounds.y + bounds.height * (0.30f + 0.18f * std::sin(t * 0.9f))),
            params.secondaryColor,
            r
        },
        {
            Point(bounds.x + bounds.width * (0.30f + 0.18f * std::cos(t * 0.9f)),
                  bounds.y + bounds.height * (0.75f + 0.18f * std::sin(t * 0.7f))),
            params.accentColor,
            r
        },
        {
            Point(bounds.x + bounds.width * (0.70f + 0.18f * std::sin(t * 0.6f)),
                  bounds.y + bounds.height * (0.70f + 0.18f * std::cos(t * 0.8f))),
            params.tertiaryColor,
            r
        }
    };

    D2D1_RECT_F d2dDest = bounds.ToD2D();

    for (int i = 0; i < 4; ++i) {
        D2D1_GRADIENT_STOP stops[2];
        stops[0].position = 0.0f;
        stops[0].color = D2D1::ColorF(points[i].color.r, points[i].color.g, points[i].color.b, points[i].color.a * params.intensity);
        stops[1].position = 1.0f;
        stops[1].color = D2D1::ColorF(points[i].color.r, points[i].color.g, points[i].color.b, 0.0f);

        ComPtr<ID2D1GradientStopCollection> stopCollection;
        HRESULT hr = d2d->CreateGradientStopCollection(
            stops, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &stopCollection
        );
        if (FAILED(hr) || !stopCollection) continue;

        D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES radialProps{};
        radialProps.center = D2D1::Point2F(points[i].pt.x, points[i].pt.y);
        radialProps.gradientOriginOffset = D2D1::Point2F(0.0f, 0.0f);
        radialProps.radiusX = points[i].radius;
        radialProps.radiusY = points[i].radius;

        ComPtr<ID2D1RadialGradientBrush> radialBrush;
        hr = d2d->CreateRadialGradientBrush(radialProps, stopCollection.Get(), &radialBrush);
        if (SUCCEEDED(hr) && radialBrush) {
            d2d->FillRectangle(&d2dDest, radialBrush.Get());
        }
    }
}

void CustomShader::RenderFluidAcrylic(GraphicsContext& ctx, const Rect& bounds, const ShaderParams& params) {
    const float t = params.time * params.speed;

    // 半透明背景基底
    ctx.FillRect(bounds, Color(0.12f, 0.14f, 0.18f, 0.72f * params.intensity));

    // 动态波形光带
    const float waveY = bounds.y + bounds.height * 0.5f + std::sin(t * 1.5f) * (bounds.height * 0.25f);
    const Color waveColor = Color(params.accentColor.r, params.accentColor.g, params.accentColor.b, 0.22f * params.intensity);
    ctx.FillRoundedRect(Rect(bounds.x + 8.0f, waveY - 24.0f, bounds.width - 16.0f, 48.0f), 24.0f, waveColor);

    // 顶部高光反射边线 (Specular Edge Highlight)
    ctx.DrawSmoothLine(
        Point(bounds.x + 2.0f, bounds.y + 1.0f),
        Point(bounds.Right() - 2.0f, bounds.y + 1.0f),
        Color(1.0f, 1.0f, 1.0f, 0.35f * params.intensity),
        1.2f
    );

    // 底部微弱阴影边线
    ctx.DrawSmoothLine(
        Point(bounds.x + 2.0f, bounds.Bottom() - 1.0f),
        Point(bounds.Right() - 2.0f, bounds.Bottom() - 1.0f),
        Color(0.0f, 0.0f, 0.0f, 0.25f * params.intensity),
        1.0f
    );
}

void CustomShader::RenderGlowPulse(GraphicsContext& ctx, const Rect& bounds, const ShaderParams& params) {
    const float t = params.time * params.speed;
    const float pulse = (std::sin(t * 3.0f) + 1.0f) * 0.5f; // 0.0f ~ 1.0f
    const float alpha = (0.25f + 0.45f * pulse) * params.intensity;

    Color glowCol = Color(params.accentColor.r, params.accentColor.g, params.accentColor.b, alpha);

    // 多重外发光光环
    for (int i = 3; i >= 1; --i) {
        float inflate = static_cast<float>(i) * 3.0f;
        float layerAlpha = alpha / static_cast<float>(i);
        Color c = Color(glowCol.r, glowCol.g, glowCol.b, layerAlpha);
        ctx.DrawRoundedRect(bounds.Inflate(inflate), 8.0f + inflate, c, 2.0f);
    }
}

} // namespace CUI
