#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/BreadcrumbBar.h"
#include <format>
#include <memory>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildBreadcrumbBarPage() {
    // ---------- 1. 常规用法 ----------
    auto bread1 = std::make_shared<BreadcrumbBar>();
    bread1->SetPath({ "CUI", "Gallery", "src", "pages", "Media", "ImagePage.cpp" });

    auto status1 = MakeStatus("点击任意节点查看点击事件；路径过长时前方自动折叠为 …");
    bread1->OnItemClicked().Connect([status1](BreadcrumbBar*, int index, const std::string& node) {
        status1->Text = std::format("已点击第 {} 级节点: [{}]", index + 1, node);
    });

    auto btnShort = ElevatedButton("项目源码路径", [bread1, status1](UIElement*) {
        bread1->SetPath({ "CUI", "Gallery", "src", "pages", "Media", "ImagePage.cpp" });
        status1->Text = "已切换到: 项目源码路径 (6 级)";
    }).Build();
    auto btnSys = ElevatedButton("系统文件路径", [bread1, status1](UIElement*) {
        bread1->SetPath({ "C:", "Windows", "System32", "drivers", "etc", "hosts" });
        status1->Text = "已切换到: 系统文件路径";
    }).Build();
    auto btnWeb = ElevatedButton("网站导航路径", [bread1, status1](UIElement*) {
        bread1->SetPath({ "首页", "产品中心", "云服务", "容器实例", "控制台" });
        status1->Text = "已切换到: 网站导航路径";
    }).Build();

    // ---------- 2. 溢出折叠 ----------
    auto bread2 = std::make_shared<BreadcrumbBar>();
    bread2->SetPath({ "项目", "平台", "微服务", "订单域", "结算", "网关", "v3", "develop",
                      "deploy", "k8s", "production", "helm", "values.yaml" });

    auto status2 = MakeStatus("路径过长时前方折叠为 …，点击 … 可在弹出菜单中选取被折叠的祖先节点。");
    bread2->OnItemClicked().Connect([status2](BreadcrumbBar*, int index, const std::string& node) {
        status2->Text = std::format("已点击第 {} 级节点: [{}]", index + 1, node);
    });

    auto btnLong = ElevatedButton("更深路径", [bread2, status2](UIElement*) {
        bread2->SetPath({ "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O" });
        status2->Text = "已切换到 15 级深度路径，观察折叠行为";
    }).Build();

    // ---------- 3. 动态操作 ----------
    auto bread3 = std::make_shared<BreadcrumbBar>();
    auto path = std::make_shared<std::vector<std::string>>(
        std::vector<std::string>{ "根目录", "工作区" });

    auto status3 = MakeStatus("");

    auto btnPush = ElevatedButton("PushNode 进入子目录", [bread3, path, status3](UIElement*) {
        static int s_seq = 1;
        path->push_back(std::format("子目录 {}", s_seq++));
        bread3->SetPath(*path);
        status3->Text = std::format("已进入 [{}]，当前深度 {}", path->back(), path->size());
    }).Build();
    auto btnPop = ElevatedButton("PopNode 返回上级", [bread3, path, status3](UIElement*) {
        if (path->size() <= 1) {
            status3->Text = "已到达根目录，无法继续返回。";
            return;
        }
        path->pop_back();
        bread3->SetPath(*path);
        status3->Text = std::format("已返回 [{}]，当前深度 {}", path->back(), path->size());
    }).Build();
    auto btnReset = ElevatedButton("重置", [bread3, path, status3](UIElement*) {
        path->assign({ "根目录", "工作区" });
        bread3->SetPath(*path);
        status3->Text = "已重置为两级路径。";
    }).Build();
    bread3->SetPath(*path);
    status3->Text = "通过 PushNode / PopNode 动态维护导航深度。";

    SamplePageSpec spec;
    spec.title = "BreadcrumbBar (面包屑导航)";
    spec.subtitle = "显示当前位置的层级路径：节点可直接点击跳转，过长时前方折叠为 … 弹层选择，支持动态 Push / Pop。";
    spec.sections = {
        {
            "常规用法 — 路径预设",
            "1. SetPath 一次性设置完整路径，每个节点渲染为可点击的跳转链接；\n"
            "2. OnItemClicked 回调携带节点索引与文本，用于跳转/回显；\n"
            "3. 三种不同场景的路径预设一键切换。",
            Column(12, {
                Row(8, { btnShort, btnSys, btnWeb }),
                bread1,
                status1,
            }),
        },
        {
            "溢出折叠 — 深层路径",
            "路径超过可用宽度时，前方的节点自动折叠进一个 … 按钮，点击弹出菜单可选取任意被折叠的祖先节点。",
            Column(12, {
                Row(8, { btnLong }),
                bread2,
                status2,
            }),
        },
        {
            "动态操作 — Push / Pop",
            "1. PushNode 进入下一级目录，PopNode 返回上一级；\n"
            "2. 深度不足时 Pop 会被安全拦截。",
            Column(12, {
                Row(8, { btnPush, btnPop, btnReset }),
                bread3,
                status3,
            }),
        },
    };

    spec.source = R"(
// 1) 设置路径
auto bread = std::make_shared<BreadcrumbBar>();
bread->SetPath({ "CUI", "Gallery", "src", "pages" });

// 2) 点击事件（含被折叠进 … 的祖先节点）
bread->OnItemClicked().Connect([](BreadcrumbBar*, int index, const std::string& node) {
    // index 为节点层级，node 为节点文本
});

// 3) 动态维护路径
bread->PushNode("Media");
bread->PopNode();
)";

    return BuildSamplePage(spec);
}

} // namespace Gallery
