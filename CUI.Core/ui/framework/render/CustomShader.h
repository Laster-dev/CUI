#pragma once
#include <memory>
#include <vector>
#include <string>
#include <d2d1.h>
#include <d2d1_1.h>
#include <wrl/client.h>
#include "../core/Value.h"

namespace CUI {

using Microsoft::WRL::ComPtr;
class GraphicsContext;

/**
 * @brief 自定义着色器渲染参数
 */
struct ShaderParams {
    float time = 0.0f;
    float intensity = 1.0f;
    float speed = 1.0f;
    Color primaryColor = Color(0.2f, 0.4f, 0.9f, 0.8f);
    Color secondaryColor = Color(0.8f, 0.2f, 0.7f, 0.7f);
    Color accentColor = Color(0.1f, 0.8f, 0.8f, 0.6f);
    Color tertiaryColor = Color(0.95f, 0.6f, 0.1f, 0.5f);
    float custom[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

enum class BuiltInShaderType {
    FluidAcrylic,   // 流体亚克力材质
    MeshGradient,   // 多点平滑网格渐变
    GlowPulse       // 动态边框/表面光效
};

/**
 * @brief 自定义着色器特效类
 * 封装通用 GPU / Direct2D 特效管线。
 */
class CustomShader {
public:
    explicit CustomShader(BuiltInShaderType type);
    ~CustomShader();

    BuiltInShaderType GetType() const { return m_type; }
    void Render(GraphicsContext& ctx, const Rect& bounds, const ShaderParams& params);

private:
    void RenderFluidAcrylic(GraphicsContext& ctx, const Rect& bounds, const ShaderParams& params);
    void RenderMeshGradient(GraphicsContext& ctx, const Rect& bounds, const ShaderParams& params);
    void RenderGlowPulse(GraphicsContext& ctx, const Rect& bounds, const ShaderParams& params);

    BuiltInShaderType m_type;
};

} // namespace CUI
