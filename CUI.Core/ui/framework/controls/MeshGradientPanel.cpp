#include "MeshGradientPanel.h"

namespace CUI {

MeshGradientPanel::MeshGradientPanel() {
    m_shader = std::make_unique<CustomShader>(m_shaderType);
    m_startTime = std::chrono::steady_clock::now();
}

MeshGradientPanel::~MeshGradientPanel() = default;

void MeshGradientPanel::ApplyShaderType(BuiltInShaderType type) {
    if (m_shaderType != type) {
        m_shaderType = type;
        m_shader = std::make_unique<CustomShader>(type);
        MarkRenderContentDirty();
    }
}

void MeshGradientPanel::ApplyPrimaryColor(Color c) {
    m_params.primaryColor = c;
    MarkRenderContentDirty();
}

void MeshGradientPanel::ApplySecondaryColor(Color c) {
    m_params.secondaryColor = c;
    MarkRenderContentDirty();
}

void MeshGradientPanel::ApplyAccentColor(Color c) {
    m_params.accentColor = c;
    MarkRenderContentDirty();
}

void MeshGradientPanel::ApplyTertiaryColor(Color c) {
    m_params.tertiaryColor = c;
    MarkRenderContentDirty();
}

void MeshGradientPanel::ApplySpeed(float speed) {
    m_params.speed = speed;
}

void MeshGradientPanel::ApplyIntensity(float intensity) {
    m_params.intensity = intensity;
    MarkRenderContentDirty();
}

void MeshGradientPanel::ApplyAnimated(bool animated) {
    if (m_animated != animated) {
        m_animated = animated;
        MarkRenderContentDirty();
    }
}

void MeshGradientPanel::OnRender(GraphicsContext& ctx) {
    if (m_animated) {
        const auto now = std::chrono::steady_clock::now();
        m_params.time = std::chrono::duration<float>(now - m_startTime).count();
    }

    if (m_shader) {
        m_shader->Render(ctx, m_bounds, m_params);
    }
}

} // namespace CUI
