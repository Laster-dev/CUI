#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/topology/TopologyView.h"
#include <format>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

/**
 * @brief 构建微服务与云原生架构拓扑图数据。
 */
std::pair<std::vector<std::shared_ptr<TopologyNode>>, std::vector<TopologyEdge>> BuildCloudArchitectureData() {
    std::vector<std::shared_ptr<TopologyNode>> nodes;
    std::vector<TopologyEdge> edges;

    // 1. API 网关根节点
    auto gateway = std::make_shared<TopologyNode>("gateway", "API Gateway", "入口流量路由", "🌐");
    gateway->status = TopologyNodeStatus::Success;
    gateway->badge = "1.8k req/s";
    nodes.push_back(gateway);

    // 2. 认证鉴权集群 (可折叠)
    auto authCluster = std::make_shared<TopologyNode>("auth_cluster", "Auth Cluster", "统一认证中枢", "🛡️");
    authCluster->status = TopologyNodeStatus::Success;
    authCluster->badge = "2 Pods";
    nodes.push_back(authCluster);

    auto oauthService = std::make_shared<TopologyNode>("oauth_svc", "OAuth2 Service", "JWT 令牌校验", "🔑");
    oauthService->status = TopologyNodeStatus::Success;
    oauthService->badge = "12ms";
    authCluster->AddChild(oauthService);
    nodes.push_back(oauthService);

    auto tokenVault = std::make_shared<TopologyNode>("token_vault", "Secret Vault", "密钥与凭据保险箱", "🔒");
    tokenVault->status = TopologyNodeStatus::Success;
    tokenVault->badge = "99.99%";
    authCluster->AddChild(tokenVault);
    nodes.push_back(tokenVault);

    // 3. 订单业务中台集群 (可折叠)
    auto orderCluster = std::make_shared<TopologyNode>("order_cluster", "Order Cluster", "订单微服务中台", "📦");
    orderCluster->status = TopologyNodeStatus::Processing;
    orderCluster->badge = "4 Pods";
    nodes.push_back(orderCluster);

    auto orderCore = std::make_shared<TopologyNode>("order_core", "Order Core Svc", "主订单处理流水线", "⚙️");
    orderCore->status = TopologyNodeStatus::Processing;
    orderCore->badge = "620 rps";
    orderCluster->AddChild(orderCore);
    nodes.push_back(orderCore);

    auto paymentBridge = std::make_shared<TopologyNode>("payment_bridge", "Payment Gateway", "第三方支付清结算", "💳");
    paymentBridge->status = TopologyNodeStatus::Success;
    paymentBridge->badge = "99.95%";
    orderCluster->AddChild(paymentBridge);
    nodes.push_back(paymentBridge);

    auto inventorySvc = std::make_shared<TopologyNode>("inventory_svc", "Inventory Svc", "实时库存分仓扣减", "🏬");
    inventorySvc->status = TopologyNodeStatus::Success;
    inventorySvc->badge = "18ms";
    orderCluster->AddChild(inventorySvc);
    nodes.push_back(inventorySvc);

    // 4. 数据基础设施集群 (可折叠)
    auto dataCluster = std::make_shared<TopologyNode>("data_cluster", "Storage & DB", "分布式存储底座", "🗄️");
    dataCluster->status = TopologyNodeStatus::Warning;
    dataCluster->badge = "3 Nodes";
    nodes.push_back(dataCluster);

    auto pgDb = std::make_shared<TopologyNode>("pg_db", "PostgreSQL Main", "高可用主从集群", "🐘");
    pgDb->status = TopologyNodeStatus::Success;
    pgDb->badge = "Master";
    dataCluster->AddChild(pgDb);
    nodes.push_back(pgDb);

    auto redisCache = std::make_shared<TopologyNode>("redis_cache", "Redis Cluster", "热点缓存分片", "⚡");
    redisCache->status = TopologyNodeStatus::Success;
    redisCache->badge = "0.6ms";
    dataCluster->AddChild(redisCache);
    nodes.push_back(redisCache);

    auto kafkaBroker = std::make_shared<TopologyNode>("kafka_broker", "Kafka MQ", "日志与事件异步管道", "📨");
    kafkaBroker->status = TopologyNodeStatus::Warning;
    kafkaBroker->badge = "92% Load";
    dataCluster->AddChild(kafkaBroker);
    nodes.push_back(kafkaBroker);

    // 拓扑连线拓扑图
    edges.push_back(TopologyEdge("gateway", "auth_cluster", "gRPC"));
    edges.push_back(TopologyEdge("gateway", "order_cluster", "HTTPS"));
    edges.push_back(TopologyEdge("auth_cluster", "oauth_svc", "RPC"));
    edges.push_back(TopologyEdge("oauth_svc", "token_vault", "TLS"));
    edges.push_back(TopologyEdge("order_cluster", "order_core", "TCP"));
    edges.push_back(TopologyEdge("order_core", "payment_bridge", "HTTPS"));
    edges.push_back(TopologyEdge("order_core", "inventory_svc", "gRPC"));
    edges.push_back(TopologyEdge("order_cluster", "data_cluster", "Pool"));
    edges.push_back(TopologyEdge("data_cluster", "pg_db", "SQL"));
    edges.push_back(TopologyEdge("data_cluster", "redis_cache", "RESP"));
    edges.push_back(TopologyEdge("data_cluster", "kafka_broker", "Stream"));

    return { nodes, edges };
}

} // anonymous namespace

/**
 * @brief 创建可折叠带动画拓扑图控件展示页面。
 */
Element BuildTopologyPage() {
    auto [nodes, edges] = BuildCloudArchitectureData();

    // 创建拓扑图核心控件
    auto topoPtr = TopologyWidget()
        .Width(860.0f)
        .Height(520.0f)
        .Nodes(nodes)
        .Edges(edges)
        .FlowParticles(true)
        .LayoutType(TopologyLayoutType::HierarchicalLeftRight)
        .Build();

    // 选中状态提示面板
    auto statusLabel = MakeStatus("点击或悬停节点查看拓扑详情；点击节点右下角 [+] / [-] 体验平滑折叠动画。");

    topoPtr->OnNodeClicked().Connect([statusLabel](TopologyView*, std::shared_ptr<TopologyNode> n) {
        if (!n) return;
        std::string statusStr = (n->status == TopologyNodeStatus::Success) ? "健康" :
                                (n->status == TopologyNodeStatus::Warning) ? "告警" :
                                (n->status == TopologyNodeStatus::Error) ? "故障" : "运行中";
        statusLabel->Text = std::format("已选择节点：{} [{}] | 状态: {} | 指标: {}",
                                        n->label, n->id, statusStr, n->badge.empty() ? "无" : n->badge);
    });

    topoPtr->OnNodeToggleExpanded().Connect([statusLabel](TopologyView*, std::shared_ptr<TopologyNode> n, bool expanded) {
        if (!n) return;
        statusLabel->Text = std::format("集群 [{}] 已触发动画{}。", n->label, expanded ? "展开" : "折叠");
    });

    // 交互操作工具栏按钮
    auto btnExpandAll = Button("全部展开");
    btnExpandAll->OnClick().Connect([topoPtr](UIElement*) {
        topoPtr->ExpandAll();
    });

    auto btnCollapseAll = Button("全部折叠");
    btnCollapseAll->OnClick().Connect([topoPtr](UIElement*) {
        topoPtr->CollapseAll();
    });

    auto btnFit = Button("自适应居中 [F]");
    btnFit->OnClick().Connect([topoPtr](UIElement*) {
        topoPtr->FitToView();
    });

    auto btnReset = Button("重置视角 [R]");
    btnReset->OnClick().Connect([topoPtr](UIElement*) {
        topoPtr->ResetView();
    });

    auto btnLeftRight = Button("水平分层");
    btnLeftRight->OnClick().Connect([topoPtr](UIElement*) {
        topoPtr->SetLayoutType(TopologyLayoutType::HierarchicalLeftRight);
    });

    auto btnTopDown = Button("垂直树状");
    btnTopDown->OnClick().Connect([topoPtr](UIElement*) {
        topoPtr->SetLayoutType(TopologyLayoutType::HierarchicalTopDown);
    });

    auto btnRadial = Button("径向环形");
    btnRadial->OnClick().Connect([topoPtr](UIElement*) {
        topoPtr->SetLayoutType(TopologyLayoutType::Radial);
    });

    auto btnForce = Button("弹性力导向");
    btnForce->OnClick().Connect([topoPtr](UIElement*) {
        topoPtr->SetLayoutType(TopologyLayoutType::ForceDirected);
    });

    auto btnParticles = ToggleButtonWidget("流向粒子");
    btnParticles->OnClick().Connect([topoPtr](UIElement*) {
        topoPtr->SetFlowParticlesEnabled(!topoPtr->IsFlowParticlesEnabled());
    });

    SamplePageSpec spec;
    spec.title = "TopologyView (拓扑图)";
    spec.subtitle = "支持平滑折叠/展开动画、贝塞尔流动粒子、层次化与力导向排版的现代 Fluent 架构拓扑图可视化控件。";
    spec.sections = {
        {
            "云原生微服务架构拓扑 (可折叠带动画)",
            "点击节点右下角 [+] / [-] 展开或折叠子集群，子节点伴随弹簧缓动与透明度 Fade 平滑聚散；支持滚轮缩放与鼠标拖拽画布/节点。",
            Column(12, {
                Row(8, {
                    btnExpandAll,
                    btnCollapseAll,
                    btnFit,
                    btnReset,
                    btnParticles,
                }),
                Row(8, {
                    btnLeftRight,
                    btnTopDown,
                    btnRadial,
                    btnForce,
                }),
                topoPtr,
                statusLabel,
            }),
        },
    };

    spec.source = R"(// 构造拓扑图
auto topology = TopologyWidget()
    .Width(860.0f)
    .Height(520.0f)
    .Nodes(nodes)
    .Edges(edges)
    .FlowParticles(true)
    .LayoutType(TopologyLayoutType::HierarchicalLeftRight);
)";

    return BuildSamplePage(spec);
}

} // namespace Gallery
