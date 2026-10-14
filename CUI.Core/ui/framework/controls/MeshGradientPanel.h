#pragma once
#include "UIElement.h"
#include "../render/CustomShader.h"
#include <chrono>

namespace CUI {

/**
 * @brief 网格渐变与流体着色器特效容器面板
 */
class MeshGradientPanel : public UIElement {
public:
    MeshGradientPanel();
    ~MeshGradientPanel() override;

    void ApplyShaderType(BuiltInShaderType type);
    BuiltInShaderType GetShaderType() const { return m_shaderType; }

    void ApplyPrimaryColor(Color c);
    Color GetPrimaryColor() const { return m_params.primaryColor; }

    void ApplySecondaryColor(Color c);
    Color GetSecondaryColor() const { return m_params.secondaryColor; }

    void ApplyAccentColor(Color c);
    Color GetAccentColor() const { return m_params.accentColor; }

    void ApplyTertiaryColor(Color c);
    Color GetTertiaryColor() const { return m_params.tertiaryColor; }

    void ApplySpeed(float speed);
    float GetSpeed() const { return m_params.speed; }

    void ApplyIntensity(float intensity);
    float GetIntensity() const { return m_params.intensity; }

    void ApplyAnimated(bool animated);
    bool IsAnimated() const { return m_animated; }

    void OnRender(GraphicsContext& ctx) override;

private:
    BuiltInShaderType m_shaderType = BuiltInShaderType::MeshGradient;
    ShaderParams m_params;
    std::unique_ptr<CustomShader> m_shader;
    bool m_animated = true;
    std::chrono::steady_clock::time_point m_startTime;
};

} // namespace CUI
