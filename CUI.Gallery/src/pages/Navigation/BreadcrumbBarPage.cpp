#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/BreadcrumbBar.h"
#include "framework/controls/Button.h"
#include <format>
#include <memory>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

/**
 * @brief 构建 BreadcrumbBar (面包屑导航) 展示页面。
 * 遵循 Fluent 规范，全弹性流式布局，节点点击即时截断跳转。
 */
Element BuildBreadcrumbBarPage() {
    // ==========================================
    // 1. 常规用法 — 路径预设与交互式跳转
    // ==========================================
    auto bread1 = std::make_shared<BreadcrumbBar>();
    auto path1 = std::make_shared<std::vector<std::string>>(
        std::vector<std::string>{ "CUI", "Gallery", "src", "pages", "Media", "ImagePage.cpp" });
    bread1->SetPath(*path1);

    auto status1 = MakeStatus("点击路径中任意祖先节点，路径将自动截断并跳转至该层级。");

    // 点击节点实现实际跳转
    bread1->OnItemClicked().Connect([bread1, path1, status1](BreadcrumbBar*, int index, const std::string& node) {
        if (index >= 0 && index < static_cast<int>(path1->size())) {
            path1->resize(static_cast<size_t>(index + 1));
            bread1->SetPath(*path1);
            status1->Text = std::format("已成功跳转并截断至第 {} 级节点: [{}]", index + 1, node);
        }
    });

    auto btnShort = Button("项目源码路径")
        .OnClick([bread1, path1, status1](UIElement*) {
            *path1 = { "CUI", "Gallery", "src", "pages", "Media", "ImagePage.cpp" };
            bread1->SetPath(*path1);
            status1->Text = "已重载路径: 项目源码路径 (6 级)";
        });

    auto btnSys = Button("系统文件路径")
        .OnClick([bread1, path1, status1](UIElement*) {
            *path1 = { "C:", "Windows", "System32", "drivers", "etc", "hosts" };
            bread1->SetPath(*path1);
            status1->Text = "已重载路径: 系统文件路径 (6 级)";
        });

    auto btnWeb = Button("网站导航路径")
        .OnClick([bread1, path1, status1](UIElement*) {
            *path1 = { "首页", "产品中心", "云服务", "容器实例", "控制台" };
            bread1->SetPath(*path1);
            status1->Text = "已重载路径: 网站导航路径 (5 级)";
        });

    // ==========================================
    // 2. 溢出折叠 — 超长深层路径与下拉菜单跳转
    // ==========================================
    auto bread2 = std::make_shared<BreadcrumbBar>();
    auto path2 = std::make_shared<std::vector<std::string>>(
        std::vector<std::string>{ "项目", "平台", "微服务", "订单域", "结算", "网关", "v3", "develop",
                                  "deploy", "k8s", "production", "helm", "values.yaml" });
    bread2->SetPath(*path2);

    auto status2 = MakeStatus("超长路径在窄屏下前方自动折叠为【…】；点击【…】可在弹出菜单中选取祖先节点直接跳转。");

    bread2->OnItemClicked().Connect([bread2, path2, status2](BreadcrumbBar*, int index, const std::string& node) {
        if (index >= 0 && index < static_cast<int>(path2->size())) {
            path2->resize(static_cast<size_t>(index + 1));
            bread2->SetPath(*path2);
            status2->Text = std::format("已从折叠菜单/导航条跳转至第 {} 级: [{}]", index + 1, node);
        }
    });

    auto btnLong = Button("加载 15 级极深路径")
        .OnClick([bread2, path2, status2](UIElement*) {
            *path2 = { "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O" };
            bread2->SetPath(*path2);
            status2->Text = "已加载 15 级深层路径，前方已自动折叠为 […]。";
        });

    // ==========================================
    // 3. 动态前进与后退 (Push / Pop)
    // ==========================================
    auto bread3 = std::make_shared<BreadcrumbBar>();
    auto path3 = std::make_shared<std::vector<std::string>>(
        std::vector<std::string>{ "根目录", "工作区" });
    bread3->SetPath(*path3);

    auto status3 = MakeStatus("通过 PushNode 进入子级目录，或 PopNode 返回上级目录。");

    bread3->OnItemClicked().Connect([bread3, path3, status3](BreadcrumbBar*, int index, const std::string& node) {
        if (index >= 0 && index < static_cast<int>(path3->size())) {
            path3->resize(static_cast<size_t>(index + 1));
            bread3->SetPath(*path3);
            status3->Text = std::format("直接点击跳转至: [{}] (深度: {})", node, path3->size());
        }
    });

    auto btnPush = Button("➡️ PushNode 进入子目录")
        .OnClick([bread3, path3, status3](UIElement*) {
            static int s_seq = 1;
            path3->push_back(std::format("子目录_{}", s_seq++));
            bread3->SetPath(*path3);
            status3->Text = std::format("已深入至 [{}]，当前路径深度: {}", path3->back(), path3->size());
        });

    auto btnPop = Button("⬅️ PopNode 返回上级")
        .OnClick([bread3, path3, status3](UIElement*) {
            if (path3->size() <= 1) {
                status3->Text = "已处于根目录，无法继续后退。";
                return;
            }
            path3->pop_back();
            bread3->SetPath(*path3);
            status3->Text = std::format("已回退至 [{}]，当前路径深度: {}", path3->back(), path3->size());
        });

    auto btnReset = Button("🔄 重置路径")
        .OnClick([bread3, path3, status3](UIElement*) {
            *path3 = { "根目录", "工作区" };
            bread3->SetPath(*path3);
            status3->Text = "已重置为初始两级路径。";
        });

    SamplePageSpec spec;
    spec.title = "BreadcrumbBar (面包屑导航)";
    spec.subtitle = "显示当前位置的层级路径：节点直接点击跳转并截断、过长时前方折叠为 … 弹层选择，支持动态 Push / Pop。";
    spec.sections = {
        {
            "常规用法 — 路径预设与交互式截断跳转",
            "1. 点击任意祖先节点，路径将自动截断并触发跳转；\n"
            "2. OnItemClicked 回调携带节点真实索引与文本；\n"
            "3. 点击下方预设按钮可随时重载不同业务场景的路径数据。",
            Column(12, {
                Row(8, { btnShort, btnSys, btnWeb }),
                bread1,
                status1,
            }),
        },
        {
            "溢出折叠 — 深层路径与弹层菜单",
            "路径超过可用宽度时，前方的节点自动折叠进一个【…】按钮，点击弹出菜单可选取任意被折叠的祖先节点直接跳转。",
            Column(12, {
                Row(8, { btnLong }),
                bread2,
                status2,
            }),
        },
        {
            "动态操作 — 进出目录 (Push / Pop)",
            "1. PushNode 深入下一级子目录，PopNode 退出返回上一级；\n"
            "2. 同样支持点击任意节点直接回跳截断。",
            Column(12, {
                Row(8, { btnPush, btnPop, btnReset }),
                bread3,
                status3,
            }),
        },
    };

    spec.source = R"cpp(// 1. 初始化路径并挂载
auto bread = std::make_shared<BreadcrumbBar>();
std::vector<std::string> path = { "CUI", "Gallery", "src", "pages" };
bread->SetPath(path);

// 2. 节点点击交互：截断路径并实现真实跳转
bread->OnItemClicked().Connect([bread, &path](BreadcrumbBar*, int index, const std::string& node) {
    if (index >= 0 && index < path.size()) {
        path.resize(index + 1);
        bread->SetPath(path); // 路径同步更新
    }
});

// 3. 动态前进与后退
path.push_back("SubDir");
bread->SetPath(path);
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
