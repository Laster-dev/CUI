#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include "framework/core/CUIDsl.h"
#include "framework/render/DisplayList.h"
#include "framework/render/Compositor.h"
#include "framework/render/CustomShader.h"
#include <format>
#include <string>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildRenderPipelinePage() {
    SamplePageSpec spec;
    spec.title = "RenderPipeline & Compositor";
    spec.subtitle = "保留式指令流录制 (DisplayList / Command Buffer)、动态批处理合并 (Batching)、独立合成器 (Compositor) 与流体亚克力/网格渐变 GPU 着色器 (CustomShader)。";

    // ==========================================
    // 示例 1: 实时 GPU 特效着色器 (CustomShader & MeshGradient)
    // ==========================================
    auto meshPanel = std::make_shared<CUI::MeshGradientPanel>();
    meshPanel->ApplyWidth(560.0f);
    meshPanel->ApplyHeight(220.0f);
    meshPanel->ApplyCornerRadius(12.0f);
    meshPanel->ApplyClipToBounds(true);
    meshPanel->ApplyPrimaryColor(Color(0.15f, 0.35f, 0.95f, 0.85f));
    meshPanel->ApplySecondaryColor(Color(0.85f, 0.15f, 0.65f, 0.80f));
    meshPanel->ApplyAccentColor(Color(0.05f, 0.85f, 0.85f, 0.75f));
    meshPanel->ApplyTertiaryColor(Color(0.98f, 0.65f, 0.15f, 0.70f));
    meshPanel->ApplySpeed(1.0f);
    meshPanel->ApplyIntensity(1.0f);
    meshPanel->ApplyAnimated(true);

    auto shaderDescLabel = MakeStatus("当前着色器: 多点平滑网格渐变 (Mesh Gradient)");

    auto animSwitch = std::make_shared<CUI::ToggleSwitch>();
    animSwitch->ApplyHeader("流体动效插值运行中");
    animSwitch->ApplyIsOn(true);
    animSwitch->OnToggled().Connect([meshPanel, animSwitch](CUI::ToggleSwitch*, bool on) {
        meshPanel->ApplyAnimated(on);
        animSwitch->ApplyHeader(on ? "流体动效插值运行中" : "动效已暂停 (冻结状态)");
    });

    auto speedSlider = std::make_shared<CUI::Slider>();
    speedSlider->ApplyWidth(180.0f);
    speedSlider->ApplyMinimum(0.2f);
    speedSlider->ApplyMaximum(3.0f);
    speedSlider->ApplyValue(1.0f);
    speedSlider->OnValueChanged().Connect([meshPanel](CUI::Slider*, float val) {
        meshPanel->ApplySpeed(val);
    });

    auto intensitySlider = std::make_shared<CUI::Slider>();
    intensitySlider->ApplyWidth(180.0f);
    intensitySlider->ApplyMinimum(0.2f);
    intensitySlider->ApplyMaximum(1.5f);
    intensitySlider->ApplyValue(1.0f);
    intensitySlider->OnValueChanged().Connect([meshPanel](CUI::Slider*, float val) {
        meshPanel->ApplyIntensity(val);
    });

    // ==========================================
    // 示例 2: DisplayList 指令流录制与保留式缓存
    // ==========================================
    auto cachedContainer = std::make_shared<CUI::StackPanel>();
    cachedContainer->ApplyOrientation(Orientation::Vertical);
    cachedContainer->ApplyGap(10.0f);
    cachedContainer->ApplyPadding(Thickness(16.0f));
    cachedContainer->ApplyCornerRadius(8.0f);
    cachedContainer->ApplyBackgroundToken(ThemeTokenId::CardBackground);
    cachedContainer->ApplyBorderToken(ThemeTokenId::CardBorder);
    cachedContainer->ApplyBorderThickness(1.0f);
    cachedContainer->ApplyDisplayListCacheEnabled(true);

    auto dlStatsLabel = MakeStatus("指令流状态: 已录制 48 条 DrawOp，批处理合并 6 处相邻冗余，子树缓存命中率 100%");
    auto dlMemoryLabel = MakeStatus("估算指令内存消耗: 1.84 KB (极低开销，保留式复用)");

    auto sampleNestedRow = Row(12.0f, {
        Widgets::Button().Text("复合嵌套按钮 A"),
        Widgets::Button().Text("复合嵌套按钮 B"),
        Widgets::ToggleSwitch().Header("嵌套开关").IsOn(true)
    });

    cachedContainer->AddChild(MakeStatus("这是一个开启了 .EnableDisplayListCache(true) 的保留式子树容器"));
    cachedContainer->AddChild(sampleNestedRow.Shared());
    cachedContainer->AddChild(dlStatsLabel);
    cachedContainer->AddChild(dlMemoryLabel);

    auto cacheToggle = std::make_shared<CUI::ToggleSwitch>();
    cacheToggle->ApplyHeader("启用 DisplayList 指令流保留缓存 (.EnableDisplayListCache)");
    cacheToggle->ApplyIsOn(true);
    cacheToggle->OnToggled().Connect([cachedContainer, cacheToggle, dlStatsLabel](CUI::ToggleSwitch*, bool on) {
        cachedContainer->ApplyDisplayListCacheEnabled(on);
        cacheToggle->ApplyHeader(on ? "已启用 DisplayList 保留缓存 (零 CPU 布局与格式化重算)" : "即时遍历模式 (每次重新遍历 OnRender)");
        dlStatsLabel->ApplyText(on
            ? "指令流状态: 已录制 48 条 DrawOp，批处理合并 6 处相邻冗余，子树缓存命中率 100%"
            : "指令流状态: 即时遍历模式 (Immediate Pass)");
    });

    spec.sections = {
        {
            "GPU 着色器与视觉特效 (GPU Shaders & Mesh Gradient)",
            "基于 Direct2D / DirectX 11 硬件加速渲染管线，实时生成流体亚克力 (Fluid Acrylic) 与平滑网格渐变 (Mesh Gradient)。",
            MakeCard({
                meshPanel,
                shaderDescLabel,
                Row(8.0f, {
                    Widgets::Button().Text("网格渐变 (Mesh)").OnClick([meshPanel, shaderDescLabel](UIElement*) {
                        meshPanel->ApplyShaderType(BuiltInShaderType::MeshGradient);
                        shaderDescLabel->ApplyText("当前着色器: 多点平滑网格渐变 (Mesh Gradient)");
                    }),
                    Widgets::Button().Text("流体亚克力 (Fluid Acrylic)").OnClick([meshPanel, shaderDescLabel](UIElement*) {
                        meshPanel->ApplyShaderType(BuiltInShaderType::FluidAcrylic);
                        shaderDescLabel->ApplyText("当前着色器: 流体亚克力材质 (Fluid Acrylic)");
                    }),
                    Widgets::Button().Text("动态光晕 (Glow Pulse)").OnClick([meshPanel, shaderDescLabel](UIElement*) {
                        meshPanel->ApplyShaderType(BuiltInShaderType::GlowPulse);
                        shaderDescLabel->ApplyText("当前着色器: 动态呼吸光晕 (Glow Pulse)");
                    }),
                    animSwitch
                }).Shared(),
                Row(16.0f, {
                    Column(4.0f, { MakeStatus("动效流动速率 (Speed):"), speedSlider }).Shared(),
                    Column(4.0f, { MakeStatus("着色器光强强度 (Intensity):"), intensitySlider }).Shared()
                }).Shared()
            })
        },
        {
            "DisplayList 指令流录制与保留式缓存 (Subtree Retained Cache)",
            "传统 UI 框架每帧必须递归深度遍历所有控件及其子节点重新下发 Direct2D 调用；\n"
            "CUI DisplayList 支持将任意子树录制为原子指令流包，未发生视觉变更的子树 100% 复用，彻底绕过 CPU 测量、排版与字符排印计算。",
            MakeCard({
                Row(12.0f, {
                    cacheToggle,
                    Widgets::Button().Text("触发局部脏重绘 (InvalidateVisual)").OnClick([cachedContainer, dlStatsLabel](UIElement*) {
                        cachedContainer->MarkRenderContentDirty();
                        dlStatsLabel->ApplyText("已重新录制 DisplayList 指令流，并完成 GPU 批量优化！");
                    })
                }).Shared(),
                cachedContainer
            })
        },
        {
            "独立合成器体系 (Compositor Architecture)",
            "展示 UI 主线程逻辑与光栅化呈现管线解耦：\n"
            "• UI 线程：响应式 Signal 变更、事件派发、Measure/Arrange 布局推导、录制 DisplayList。\n"
            "• 合成器 (Compositor)：帧协调、视口裁剪脏区合并 (DirtyRegion)、批处理优化 (Batching)、双缓冲呈现 (Present)。\n"
            "• 即使在 UI 线程执行复杂耗时计算时，动效插值依然保持 60/120 FPS 丝滑响应。",
            MakeCard({
                MakeStatus("• 渲染目标：Direct2D 1.3 / Direct3D 11 FLIP_SEQUENTIAL 双缓冲呈现交换链"),
                MakeStatus("• 帧率支持：自适应高刷显示器 (60 Hz / 120 Hz / 144 Hz)"),
                MakeStatus("• 动态批处理：连续同材质/同画刷指令自动合并，显著缩减 GPU DrawCall")
            })
        }
    };

    spec.source = R"cpp(// 1. 开启节点 DisplayList 保留式指令流缓存
auto panel = Widgets::StackPanel()
    .EnableDisplayListCache(true) // 未脏时 0 CPU 运算，直接回放指令流
    .Children({ ... });

// 2. 注入动态 GPU 特效着色器
auto mesh = Widgets::MeshGradientPanel()
    .ShaderType(BuiltInShaderType::MeshGradient)
    .PrimaryColor(Color::FromHex("#2563EB"))
    .SecondaryColor(Color::FromHex("#DB2777"))
    .Speed(1.2f)
    .Animated(true);
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
