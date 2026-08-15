#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/topology/TopologyView.h"
#include <format>
#include <memory>
#include <random>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

/**
 * @brief 构建结构清晰、分层明确的微服务与云原生架构拓扑数据。
 */
std::pair<std::vector<std::shared_ptr<TopologyNode>>, std::vector<TopologyEdge>> BuildCloudArchitectureData() {
    std::vector<std::shared_ptr<TopologyNode>> nodes;
    std::vector<TopologyEdge> edges;

    // 1. API 网关根节点 (Level 0)
    auto gateway = std::make_shared<TopologyNode>("gateway", "API Gateway", "统一流量路由", "🌐");
    gateway->status = TopologyNodeStatus::Success;
    gateway->badge = "1.8k req/s";
    nodes.push_back(gateway);

    // 2. 认证鉴权集群 (Level 1, Subtree A - 可折叠)
    auto authCluster = std::make_shared<TopologyNode>("auth_cluster", "Auth Cluster", "安全认证中枢", "🛡️");
    authCluster->status = TopologyNodeStatus::Success;
    authCluster->badge = "2 Pods";
    gateway->AddChild(authCluster);
    nodes.push_back(authCluster);

    auto oauthService = std::make_shared<TopologyNode>("oauth_svc", "OAuth2 Service", "JWT 令牌校验", "🔑");
    oauthService->status = TopologyNodeStatus::Success;
    oauthService->badge = "12ms";
    authCluster->AddChild(oauthService);
    nodes.push_back(oauthService);

    auto tokenVault = std::make_shared<TopologyNode>("token_vault", "Secret Vault", "凭据加密金库", "🔒");
    tokenVault->status = TopologyNodeStatus::Success;
    tokenVault->badge = "99.99%";
    authCluster->AddChild(tokenVault);
    nodes.push_back(tokenVault);

    // 3. 订单业务集群 (Level 1, Subtree B - 可折叠)
    auto orderCluster = std::make_shared<TopologyNode>("order_cluster", "Order Cluster", "订单微服务中台", "📦");
    orderCluster->status = TopologyNodeStatus::Processing;
    orderCluster->badge = "3 Pods";
    gateway->AddChild(orderCluster);
    nodes.push_back(orderCluster);

    auto orderCore = std::make_shared<TopologyNode>("order_core", "Order Core Svc", "主订单流转引擎", "⚙️");
    orderCore->status = TopologyNodeStatus::Processing;
    orderCore->badge = "620 rps";
    orderCluster->AddChild(orderCore);
    nodes.push_back(orderCore);

    auto paymentBridge = std::make_shared<TopologyNode>("payment_bridge", "Payment Gateway", "第三方支付结算", "💳");
    paymentBridge->status = TopologyNodeStatus::Success;
    paymentBridge->badge = "99.95%";
    orderCluster->AddChild(paymentBridge);
    nodes.push_back(paymentBridge);

    auto inventorySvc = std::make_shared<TopologyNode>("inventory_svc", "Inventory Svc", "库存实时扣减", "🏬");
    inventorySvc->status = TopologyNodeStatus::Success;
    inventorySvc->badge = "18ms";
    orderCluster->AddChild(inventorySvc);
    nodes.push_back(inventorySvc);

    // 4. 数据基础设施集群 (Level 1, Subtree C - 可折叠)
    auto dataCluster = std::make_shared<TopologyNode>("data_cluster", "Storage & DB", "分布式存储底座", "🗄️");
    dataCluster->status = TopologyNodeStatus::Warning;
    dataCluster->badge = "3 Nodes";
    gateway->AddChild(dataCluster);
    nodes.push_back(dataCluster);

    auto pgDb = std::make_shared<TopologyNode>("pg_db", "PostgreSQL Main", "高可用主从库", "🐘");
    pgDb->status = TopologyNodeStatus::Success;
    pgDb->badge = "Master";
    dataCluster->AddChild(pgDb);
    nodes.push_back(pgDb);

    auto redisCache = std::make_shared<TopologyNode>("redis_cache", "Redis Cluster", "热点缓存分片", "⚡");
    redisCache->status = TopologyNodeStatus::Success;
    redisCache->badge = "0.6ms";
    dataCluster->AddChild(redisCache);
    nodes.push_back(redisCache);

    auto kafkaBroker = std::make_shared<TopologyNode>("kafka_broker", "Kafka MQ", "异步事件管道", "📨");
    kafkaBroker->status = TopologyNodeStatus::Warning;
    kafkaBroker->badge = "92% Load";
    dataCluster->AddChild(kafkaBroker);
    nodes.push_back(kafkaBroker);

    // 拓扑连线建立
    edges.push_back(TopologyEdge("gateway", "auth_cluster", "gRPC"));
    edges.push_back(TopologyEdge("gateway", "order_cluster", "HTTPS"));
    edges.push_back(TopologyEdge("gateway", "data_cluster", "Pool"));
    edges.push_back(TopologyEdge("auth_cluster", "oauth_svc", "RPC"));
    edges.push_back(TopologyEdge("auth_cluster", "token_vault", "TLS"));
    edges.push_back(TopologyEdge("order_cluster", "order_core", "TCP"));
    edges.push_back(TopologyEdge("order_cluster", "payment_bridge", "HTTPS"));
    edges.push_back(TopologyEdge("order_cluster", "inventory_svc", "gRPC"));
    edges.push_back(TopologyEdge("data_cluster", "pg_db", "SQL"));
    edges.push_back(TopologyEdge("data_cluster", "redis_cache", "RESP"));
    edges.push_back(TopologyEdge("data_cluster", "kafka_broker", "Stream"));

    return { nodes, edges };
}

} // anonymous namespace

/**
 * @brief 创建可折叠带平滑动画、子树隔离清晰排版与可编辑/只读切换的拓扑图展示页面。
 */
Element BuildTopologyPage() {
    auto [nodes, edges] = BuildCloudArchitectureData();

    // 创建拓扑图核心控件 (宽度自适应撑满容器，高度设为 560px)
    auto topoPtr = TopologyWidget()
        .Height(860.0f)
        .Nodes(nodes)
        .Edges(edges)
        .FlowParticles(true)
        .LayoutType(TopologyLayoutType::HierarchicalLeftRight)
        .Build();

    // 状态与属性提示面板
    auto statusLabel = MakeStatus("点击或悬停节点查看链路拓扑；点击节点右下角 [+] / [-] 体验平滑折叠动画。");

    topoPtr->OnNodeClicked().Connect([statusLabel](TopologyView*, std::shared_ptr<TopologyNode> n) {
        if (!n) return;
        std::string statusStr = (n->status == TopologyNodeStatus::Success) ? "健康 (Success)" :
                                (n->status == TopologyNodeStatus::Warning) ? "告警 (Warning)" :
                                (n->status == TopologyNodeStatus::Error) ? "故障 (Error)" :
                                (n->status == TopologyNodeStatus::Processing) ? "高负载 (Processing)" : "正常 (Normal)";
        statusLabel->Text = std::format("已选择节点：{} [{}] | 状态: {} | 指标: {}",
                                        n->label, n->id, statusStr, n->badge.empty() ? "无" : n->badge);
    });

    topoPtr->OnNodeToggleExpanded().Connect([statusLabel](TopologyView*, std::shared_ptr<TopologyNode> n, bool expanded) {
        if (!n) return;
        statusLabel->Text = std::format("集群 [{}] 触发了平滑{}动画 (子树节点有序隔离聚散)。", n->label, expanded ? "展开" : "折叠");
    });

    // --- 1. 视图与视口操作 (支持平滑插值动画) ---
    auto btnExpandAll = Button("全部展开")
        .OnClick([topoPtr](UIElement*) {
            topoPtr->ExpandAll();
        });

    auto btnCollapseAll = Button("全部折叠")
        .OnClick([topoPtr](UIElement*) {
            topoPtr->CollapseAll();
        });

    auto btnZoomIn = Button("放大 (+)")
        .OnClick([topoPtr](UIElement*) {
            topoPtr->ZoomIn(true);
        });

    auto btnZoomOut = Button("缩小 (-)")
        .OnClick([topoPtr](UIElement*) {
            topoPtr->ZoomOut(true);
        });

    auto btnFit = Button("自适应居中 [F]")
        .OnClick([topoPtr](UIElement*) {
            topoPtr->FitToView(true);
        });

    auto btnReset = Button("重置视角 [R]")
        .OnClick([topoPtr](UIElement*) {
            topoPtr->ResetView(true);
        });

    auto btnParticles = ToggleButtonWidget("流向粒子")
        .OnClick([topoPtr](UIElement*) {
            topoPtr->SetFlowParticlesEnabled(!topoPtr->IsFlowParticlesEnabled());
        });

    // --- 2. 排版算法切换 ---
    auto btnLeftRight = Button("水平分层(子树隔离)")
        .OnClick([topoPtr](UIElement*) {
            topoPtr->SetLayoutType(TopologyLayoutType::HierarchicalLeftRight);
        });

    auto btnTopDown = Button("垂直树状")
        .OnClick([topoPtr](UIElement*) {
            topoPtr->SetLayoutType(TopologyLayoutType::HierarchicalTopDown);
        });

    auto btnRadial = Button("径向同心圆")
        .OnClick([topoPtr](UIElement*) {
            topoPtr->SetLayoutType(TopologyLayoutType::Radial);
        });

    auto btnForce = Button("弹性力导向")
        .OnClick([topoPtr](UIElement*) {
            topoPtr->SetLayoutType(TopologyLayoutType::ForceDirected);
        });

    // --- 3. 编辑能力 vs 只读功能切换展示 ---
    static int s_dynamicNodeCounter = 1;
    auto btnReadOnlyToggle = ToggleButtonWidget("只读模式 (Read-Only)")
        .OnClick([topoPtr, statusLabel](UIElement* sender) {
            bool isReadOnly = !topoPtr->IsReadOnly();
            topoPtr->SetIsReadOnly(isReadOnly);
            statusLabel->Text = isReadOnly ? "当前模式：只读预览模式 (已锁定节点拖拽与编辑删除能力)" : "当前模式：可编辑模式 (支持自由拖拽排布、增删节点与连线)";
        });

    auto btnAddNode = Button("新增动态服务")
        .OnClick([topoPtr, statusLabel](UIElement*) {
            if (topoPtr->IsReadOnly()) {
                statusLabel->Text = "当前处于只读模式，无法新增节点。请先切换至编辑模式。";
                return;
            }
            std::string id = std::format("dyn_svc_{}", s_dynamicNodeCounter++);
            auto newNode = std::make_shared<TopologyNode>(id, std::format("Worker Node {}", s_dynamicNodeCounter), "弹性微服务算力", "⚡");
            newNode->status = TopologyNodeStatus::Success;
            newNode->badge = "100 rps";
            newNode->currentPos = Point(200.0f, 200.0f);

            // 挂载到网关
            auto gw = topoPtr->FindNode("gateway");
            if (gw) gw->AddChild(newNode);

            topoPtr->AddNode(newNode);
            topoPtr->AddEdge(TopologyEdge("gateway", id, "gRPC"));
            statusLabel->Text = std::format("已成功添加新服务节点 [{}] 并连接至网关。", id);
        });

    auto btnDeleteNode = Button("删除选中节点 [Del]")
        .OnClick([topoPtr, statusLabel](UIElement*) {
            if (topoPtr->IsReadOnly()) {
                statusLabel->Text = "当前处于只读模式，无法删除节点。";
                return;
            }
            auto selected = topoPtr->GetSelectedItem();
            if (!selected) {
                statusLabel->Text = "请先点击选中一个待删除的拓扑节点。";
                return;
            }
            std::string deletedId = selected->id;
            topoPtr->RemoveNode(deletedId);
            statusLabel->Text = std::format("已删除节点 [{}] 及其关联连线。", deletedId);
        });

    auto btnCycleStatus = Button("切换节点状态")
        .OnClick([topoPtr, statusLabel](UIElement*) {
            auto selected = topoPtr->GetSelectedItem();
            if (!selected) {
                statusLabel->Text = "请先选中一个节点以切换其状态。";
                return;
            }
            int nextStatus = (static_cast<int>(selected->status) + 1) % 6;
            selected->status = static_cast<TopologyNodeStatus>(nextStatus);
            topoPtr->MarkRenderContentDirty();
            statusLabel->Text = std::format("节点 [{}] 状态已更新为枚举值: {}", selected->label, nextStatus);
        });

    SamplePageSpec spec;
    spec.title = "TopologyView (拓扑图)";
    spec.subtitle = "支持平滑折叠/展开动画、子树隔离分明排版、缩放平移动画与可编辑/只读双模式的现代 Fluent 架构拓扑图控件。";
    spec.sections = {
        {
            "云原生微服务架构拓扑 (可折叠带动画 · 子树隔离不交织)",
            "1. 子树隔离：每个父集群展开后，其子节点严格组织在专属垂直区间内，杜绝交叉重叠；\n2. 动画缩放：滚轮缩放与放大/缩小按钮均支持平滑指数插值；\n3. 模式切换：支持在只读预览与自由编辑拖拽排版间无缝切换；\n4. 自适应尺寸：控件自动铺满当前卡片容器宽度。",
            Column(12, {
                Row(8, {
                    btnExpandAll,
                    btnCollapseAll,
                    btnZoomIn,
                    btnZoomOut,
                    btnFit,
                    btnReset,
                    btnParticles,
                }),
                Row(8, {
                    btnLeftRight,
                    btnTopDown,
                    btnRadial,
                    btnForce,
                    btnReadOnlyToggle,
                    btnAddNode,
                    btnDeleteNode,
                    btnCycleStatus,
                }),
                topoPtr,
                statusLabel,
            }),
        },
    };

    spec.source = R"(// 构造全宽自适应拓扑图
auto topology = TopologyWidget()
    .Height(560.0f)
    .Nodes(nodes)
    .Edges(edges)
    .FlowParticles(true)
    .LayoutType(TopologyLayoutType::HierarchicalLeftRight)
    .Build();
)";

    return BuildSamplePage(spec);
}

} // namespace Gallery
