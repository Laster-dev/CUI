#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/docking/DockManager.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/Button.h"
#include <format>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

/**
 * @brief 构建 DockManager (高级停靠窗口管理器) 展示页面。
 * 遵循 Fluent 现代规范，全弹性流式布局，不硬编码固定宽度。
 */
Element BuildDockManagerPage() {
    auto statusLabel = MakeStatus("提示：拖拽面板标题/页签可触发九宫格吸附罗盘；点击右上角 📌 图钉可折叠到边缘；拖拽分割线可调节大小。");

    // ==========================================
    // 1. 初始化 DockManager 实例
    // ==========================================
    auto dock = DockManagerWidget()
        .Height(480.0f)
        .Border(D2D1::ColorF(0x3F3F46, 0.6f), 1.0f)
        .CornerRadius(6.0f)
        .Build();

    // 默认分栏初始物理尺寸
    dock->SetSideSize(DockSide::Left, 190.0f);
    dock->SetSideSize(DockSide::Right, 190.0f);
    dock->SetSideSize(DockSide::Bottom, 120.0f);

    // ==========================================
    // 2. 构造左侧工具栏面板 (Left Panes)
    // ==========================================
    // 2.1 解决方案资源管理器
    auto solutionContent = Column(4, {
        Text("📁 CUI.Solution (项目解决方案)").FontWeight(FontWeight::SemiBold).FontSize(12.0f),
        Text("  ├─ 📁 CUI.Core (核心框架)").FontSize(12.0f).Foreground(D2D1::ColorF(0x94A3B8, 1.0f)),
        Text("  │   ├─ 📄 DockManager.cpp").FontSize(12.0f).Foreground(D2D1::ColorF(0x38BDF8, 1.0f)),
        Text("  │   └─ 📄 DockFloatWindow.cpp").FontSize(12.0f).Foreground(D2D1::ColorF(0x38BDF8, 1.0f)),
        Text("  ├─ 📁 CUI.Gallery (示例画廊)").FontSize(12.0f).Foreground(D2D1::ColorF(0x94A3B8, 1.0f)),
        Text("  │   └─ 📄 DockManagerPage.cpp").FontSize(12.0f).Foreground(D2D1::ColorF(0x38BDF8, 1.0f)),
        Text("  └─ 📄 README.md").FontSize(12.0f).Foreground(D2D1::ColorF(0xA78BFA, 1.0f)),
    }).Padding(8.0f).Background(D2D1::ColorF(0x18181B, 0.4f)).Build();

    // 2.2 大纲结构视图
    auto outlineContent = Column(4, {
        Text("class DockManager : public UIElement").FontWeight(FontWeight::SemiBold).FontSize(12.0f).Foreground(D2D1::ColorF(0x4ADE80, 1.0f)),
        Text("  + AddToolPane(title, content, side)").FontSize(11.0f),
        Text("  + AddDocument(title, content)").FontSize(11.0f),
        Text("  + FloatPane(paneIndex, pt)").FontSize(11.0f),
        Text("  + SetPaneAutoHide(paneIndex, bool)").FontSize(11.0f),
        Text("  + SaveLayout(path) / LoadLayout(path)").FontSize(11.0f),
    }).Padding(8.0f).Background(D2D1::ColorF(0x18181B, 0.4f)).Build();

    int paneSolution = dock->AddToolPane("解决方案资源管理器", solutionContent, DockSide::Left);
    dock->AddToolPane("结构大纲", outlineContent, DockSide::Left);

    // ==========================================
    // 3. 构造中央文档编辑面板 (Center Documents)
    // ==========================================
    // 3.1 C++ 源码文件文档
    auto codeEditor = TextField()
        .Text("#include <CUI/CUIDsl.h>\n"
              "#include <CUI/DockManager.h>\n\n"
              "int main() {\n"
              "    auto dock = Make<DockManager>();\n"
              "    dock->AddToolPane(\"资源树\", tree, DockSide::Left);\n"
              "    dock->AddDocument(\"Main.cpp\", editor);\n"
              "    return 0;\n"
              "}")
        .Height(200.0f);
    codeEditor->SetAcceptsReturn(true);
    codeEditor->SetTextWrapping(true);

    // 3.2 架构说明文档
    auto archDoc = TextField()
        .Text("# CUI Docking Management Architecture\n\n"
              "1. **五大核心区域**：Left / Top / Right / Bottom / Center 文档区。\n"
              "2. **多模式支持**：\n"
              "   - Tab 页签成组与平滑切换下划线；\n"
              "   - 独立多子窗口原生浮动 (FloatWindow)；\n"
              "   - 九宫格吸附罗盘 (Compass Guides)；\n"
              "   - 边缘窄条折叠抽屉 (AutoHide Strips)。")
        .Height(200.0f);
    archDoc->SetAcceptsReturn(true);
    archDoc->SetTextWrapping(true);

    dock->AddDocument("Main.cpp", codeEditor.Build());
    dock->AddDocument("Architecture.md", archDoc.Build());

    // ==========================================
    // 4. 构造右侧属性检查器面板 (Right Panes)
    // ==========================================
    auto propsContent = Column(6, {
        Text("🔧 属性检查器 (Inspector)").FontWeight(FontWeight::SemiBold).FontSize(12.0f),
        Row(4, { Text("控件名称: ").FontSize(11.0f).Foreground(D2D1::ColorF(0x94A3B8, 1.0f)), Text("MainDockView").FontSize(11.0f) }),
        Row(4, { Text("停靠模式: ").FontSize(11.0f).Foreground(D2D1::ColorF(0x94A3B8, 1.0f)), Text("DockSide::Left").FontSize(11.0f).Foreground(D2D1::ColorF(0x38BDF8, 1.0f)) }),
        Row(4, { Text("分栏尺寸: ").FontSize(11.0f).Foreground(D2D1::ColorF(0x94A3B8, 1.0f)), Text("190.0 px").FontSize(11.0f) }),
        Row(4, { Text("自动收拢: ").FontSize(11.0f).Foreground(D2D1::ColorF(0x94A3B8, 1.0f)), Text("False (可钉选)").FontSize(11.0f).Foreground(D2D1::ColorF(0x4ADE80, 1.0f)) }),
    }).Padding(8.0f).Background(D2D1::ColorF(0x18181B, 0.4f)).Build();

    auto diagContent = Column(4, {
        Text("📊 性能与依赖监控").FontWeight(FontWeight::SemiBold).FontSize(12.0f),
        Text("• 帧率: 60 FPS (Direct2D 硬件加速)").FontSize(11.0f).Foreground(D2D1::ColorF(0x4ADE80, 1.0f)),
        Text("• 内存占用: 24.2 MB").FontSize(11.0f),
        Text("• 活动面板: 7 个").FontSize(11.0f),
    }).Padding(8.0f).Background(D2D1::ColorF(0x18181B, 0.4f)).Build();

    dock->AddToolPane("属性检查器", propsContent, DockSide::Right);
    dock->AddToolPane("性能与诊断", diagContent, DockSide::Right);

    // ==========================================
    // 5. 构造底部输出与控制台面板 (Bottom Panes)
    // ==========================================
    auto outputContent = Column(4, {
        Text("[18:50:12] [生成] 正在启动 x64-Debug 增量构建...").FontSize(11.0f).Foreground(D2D1::ColorF(0x94A3B8, 1.0f)),
        Text("[18:50:14] [生成] CUI.Core.lib -> 已完成 0 错误，0 警告。").FontSize(11.0f).Foreground(D2D1::ColorF(0x4ADE80, 1.0f)),
        Text("[18:50:15] [生成] 停靠管理器布局树初始化完成，视口就绪。").FontSize(11.0f).Foreground(D2D1::ColorF(0x38BDF8, 1.0f)),
    }).Padding(8.0f).Background(D2D1::ColorF(0x141416, 0.8f)).Build();

    auto termContent = Column(4, {
        Text("PS E:\\C++project\\CUI> git status").FontSize(11.0f).Foreground(D2D1::ColorF(0x38BDF8, 1.0f)),
        Text("On branch master - working tree clean").FontSize(11.0f).Foreground(D2D1::ColorF(0x94A3B8, 1.0f)),
        Text("PS E:\\C++project\\CUI> _").FontSize(11.0f).Foreground(D2D1::ColorF(0x4ADE80, 1.0f)),
    }).Padding(8.0f).Background(D2D1::ColorF(0x141416, 0.8f)).Build();

    dock->AddToolPane("输出控制台", outputContent, DockSide::Bottom);
    dock->AddToolPane("集成终端", termContent, DockSide::Bottom);

    // ==========================================
    // 6. 交互操作控制栏 (Interactive Actions)
    // ==========================================
    int dynamicCount = 1;

    auto btnAddToolLeft = Button("添加左侧工具面板")
        .OnClick([dock, statusLabel, &dynamicCount](UIElement*) {
            std::string title = std::format("自定义工具 {}", dynamicCount++);
            auto card = Column(4, {
                Text(std::format("这是动态添加的侧边面板：{}", title)).FontSize(12.0f),
                Text("支持随时拖拽重排、悬浮或关闭。").FontSize(11.0f).Foreground(D2D1::ColorF(0x94A3B8, 1.0f)),
            }).Padding(8.0f).Build();

            dock->AddToolPane(title, card, DockSide::Left);
            statusLabel->Text = std::format("已成功向左侧停靠区注入：【{}】", title);
        });

    auto btnAddDoc = Button("添加中央文档")
        .OnClick([dock, statusLabel, &dynamicCount](UIElement*) {
            std::string title = std::format("Document_{}.txt", dynamicCount++);
            auto docBox = TextField()
                .Text(std::format("// 这是动态新建的文档：{}\n// 支持在中央文档区成组并排或拖出为悬浮窗口。", title))
                .Height(160.0f);
            docBox->SetAcceptsReturn(true);
            docBox->SetTextWrapping(true);

            dock->AddDocument(title, docBox.Build());
            statusLabel->Text = std::format("已向中央文档区添加新标签页：【{}】", title);
        });

    auto btnToggleAutoHide = Button("折叠左侧面板 (AutoHide)")
        .OnClick([dock, paneSolution, statusLabel](UIElement*) {
            static bool autoHidden = false;
            autoHidden = !autoHidden;
            dock->SetPaneAutoHide(paneSolution, autoHidden);
            statusLabel->Text = autoHidden
                ? "已将【解决方案资源管理器】收拢折叠至左侧窄条，鼠标悬停窄条即可抽屉式滑出展示。"
                : "已恢复【解决方案资源管理器】的常驻固定停靠状态。";
        });

    SamplePageSpec spec;
    spec.title = "DockManager (停靠管理器)";
    spec.subtitle = "企业级 IDE 窗口与面板停靠管理系统，支持多方位停靠、九宫格吸附罗盘、选项卡成组、独立悬浮窗口与边缘自动折叠收纳。";
    spec.sections = {
        {
            "高级 IDE 停靠管理系统 (全弹性响应式工作区)",
            "内置左侧资源管理器/大纲、中央多文档区、右侧属性检查器以及底部控制台/终端。所有分割线均可直接拖动调节大小，页签支持点击切换与拖拽吸附。",
            Column(12, {
                dock,
            }),
        },
        {
            "动态面板注入与状态控制",
            "支持在运行时动态注册工具侧栏面板或工作区文档，并演示边缘 AutoHide 折叠收纳效果。",
            Column(12, {
                Row(8, { btnAddToolLeft, btnAddDoc, btnToggleAutoHide }),
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 创建 DockManager 实例
auto dock = DockManagerWidget()
    .Height(480.0f)
    .Build();

// 2. 注入各方位工具面板与中央文档
dock->AddToolPane("解决方案资源管理器", solutionContent, DockSide::Left);
dock->AddToolPane("结构大纲", outlineContent, DockSide::Left);
dock->AddDocument("Main.cpp", codeEditor);
dock->AddToolPane("属性检查器", propsContent, DockSide::Right);
dock->AddToolPane("输出控制台", outputContent, DockSide::Bottom);

// 3. 动态控制 AutoHide 折叠或独立悬浮
dock->SetPaneAutoHide(paneIndex, true); // 折叠为边缘窄条
dock->FloatPane(paneIndex);             // 剥离为原生独立悬浮子窗口
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
