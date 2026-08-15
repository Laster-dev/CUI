#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/core/Value.h"
#include "framework/controls/CanvasControl.h"
#include <cmath>
#include <format>
#include <random>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::shared_ptr<Canvas> MakeStage(float minHeight) {
    auto stage = CanvasWidget()
        .MinHeight(minHeight);
    stage->ClipToBounds = true;
    stage->CornerRadius = 4.0f;
    stage->Background = D2D1::ColorF(0.97f, 0.97f, 0.99f, 0.5f);
    stage->BorderBrush = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.08f);
    stage->BorderThickness = 1.0f;
    return stage;
}

// ---------------------------------------------------------------------
// 高性能演示：重力物理引擎（即时绘制模式 CanvasControl）
// 物理模拟全部在回调闭包中运行，不产生任何 UIElement DOM 节点，
// 可同时承载上百个小球做实时碰撞。
// ---------------------------------------------------------------------

constexpr float kGravity = 1100.0f;   // 重力加速度 px/s^2
constexpr float kRestitution = 0.82f; // 碰撞能量保留系数
constexpr float kLaunchPower = 4.6f;  // 拖拽距离 -> 初速度 放大倍数
constexpr size_t kMaxBalls = 150;

const D2D1_COLOR_F kPalette[] = {
    Rgb(0xFF6B6B), Rgb(0xFFA94D), Rgb(0xFFD93D), Rgb(0x51CF66),
    Rgb(0x339AF0), Rgb(0x845EF7), Rgb(0xF783AC), Rgb(0x22B8CF),
};

struct Ball {
    float x = 0.0f, y = 0.0f;
    float vx = 0.0f, vy = 0.0f;
    float radius = 8.0f;
    D2D1_COLOR_F color = kPalette[0];
};

class PhysicsWorld {
public:
    float width = 0.0f;
    float height = 0.0f;
    std::vector<Ball> balls;

    void SetViewport(float viewportWidth, float viewportHeight) {
        width = (std::max)(0.0f, viewportWidth);
        height = (std::max)(0.0f, viewportHeight);
        for (auto& ball : balls) {
            if (width <= ball.radius * 2.0f || height <= ball.radius * 2.0f) {
                continue;
            }
            ball.x = (std::max)(ball.radius, (std::min)(width - ball.radius, ball.x));
            ball.y = (std::max)(ball.radius, (std::min)(height - ball.radius, ball.y));
        }
    }

    void SpawnBall(float x, float y, float vx, float vy) {
        if (balls.size() >= kMaxBalls) {
            return;
        }
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_real_distribution<float> r(5.0f, 11.0f);
        static std::uniform_int_distribution<int> c(0, (int)(sizeof(kPalette) / sizeof(kPalette[0])) - 1);

        Ball b;
        b.radius = r(rng);
        if (width <= b.radius * 2.0f || height <= b.radius * 2.0f) {
            return;
        }
        b.x = (std::max)(b.radius, (std::min)(width - b.radius, x));
        b.y = (std::max)(b.radius, (std::min)(height - b.radius, y));
        b.vx = vx;
        b.vy = vy;
        b.color = kPalette[c(rng)];
        balls.push_back(b);
    }

    void SpawnRandom(unsigned int count) {
        if (width <= 16.0f || height <= 20.0f) {
            return;
        }
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> px(8.0f, width - 8.0f);
        std::uniform_real_distribution<float> py(8.0f, (std::max)(8.0f, height * 0.4f));
        std::uniform_real_distribution<float> v(-260.0f, 260.0f);
        for (unsigned int i = 0; i < count; ++i) {
            SpawnBall(px(rng), py(rng), v(rng), v(rng) * 0.4f);
        }
    }

    void Clear() {
        balls.clear();
    }

    // 固定步长子步推进，防止高速小球穿墙
    void Step(float dt) {
        dt = (std::min)(dt, 1.0f / 30.0f);
        const float subDt = 1.0f / 240.0f;
        int steps = (std::max)(1, (int)std::ceil(dt / subDt));
        const float h = dt / (float)steps;
        for (int s = 0; s < steps; ++s) {
            Integrate(h);
        }
        ResolveCollisions();
    }

    bool AnyMoving() const {
        for (const auto& b : balls) {
            if (std::abs(b.vx) > 2.0f || std::abs(b.vy) > 2.0f) {
                return true;
            }
        }
        return false;
    }

private:
    void Integrate(float dt) {
        for (auto& b : balls) {
            b.vy += kGravity * dt;
            b.x += b.vx * dt;
            b.y += b.vy * dt;

            // 墙壁反弹
            if (b.x - b.radius < 0.0f) {
                b.x = b.radius;
                b.vx = -b.vx * kRestitution;
            } else if (b.x + b.radius > width) {
                b.x = width - b.radius;
                b.vx = -b.vx * kRestitution;
            }
            if (b.y - b.radius < 0.0f) {
                b.y = b.radius;
                b.vy = -b.vy * kRestitution;
            } else if (b.y + b.radius > height) {
                b.y = height - b.radius;
                b.vy = -b.vy * kRestitution;
                b.vx *= 0.985f; // 地面摩擦
                if (std::abs(b.vy) < 18.0f) {
                    b.vy = 0.0f; // 静止阈值
                }
            }
        }
    }

    void ResolveCollisions() {
        for (size_t i = 0; i < balls.size(); ++i) {
            for (size_t j = i + 1; j < balls.size(); ++j) {
                Ball& a = balls[i];
                Ball& b = balls[j];
                float dx = b.x - a.x;
                float dy = b.y - a.y;
                float distSq = dx * dx + dy * dy;
                float minDist = a.radius + b.radius;
                if (distSq >= minDist * minDist || distSq < 0.0001f) {
                    continue;
                }
                float dist = std::sqrt(distSq);
                float nx = dx / dist;
                float ny = dy / dist;

                // 位置分离
                float overlap = minDist - dist;
                a.x -= nx * overlap * 0.5f;
                a.y -= ny * overlap * 0.5f;
                b.x += nx * overlap * 0.5f;
                b.y += ny * overlap * 0.5f;

                // 等质量弹性冲量 + 恢复系数
                float dvn = (b.vx - a.vx) * nx + (b.vy - a.vy) * ny;
                if (dvn < 0.0f) {
                    float impulse = -(1.0f + kRestitution) * dvn * 0.5f;
                    a.vx -= impulse * nx;
                    a.vy -= impulse * ny;
                    b.vx += impulse * nx;
                    b.vy += impulse * ny;
                }
            }
        }
    }
};

struct AimState {
    bool dragging = false;
    Point start{ 0.0f, 0.0f };
    Point current{ 0.0f, 0.0f };
};

std::shared_ptr<CanvasControl> BuildPhysicsCanvas(
    std::shared_ptr<PhysicsWorld> world,
    std::shared_ptr<AimState> aim,
    State<int> ballCount) {
    auto canvas = CanvasControlWidget();
    canvas->MinHeight = 340.0f;
    canvas->ClipToBounds = true;

    canvas->SetOnDraw([world, aim](GraphicsContext& ctx, Size size) {
        world->SetViewport(size.width, size.height);
        // 深色舞台背景 + 边框
        ctx.FillRoundedRect(Rect(0, 0, size.width, size.height), 6.0f, Rgb(0x16161E));
        ctx.DrawRoundedRect(Rect(0.5f, 0.5f, size.width - 1.0f, size.height - 1.0f), 6.0f, Rgb(0x3A3A4A), 1.0f);

        // 小球：实心圆盘用粗描边弧线一次调用绘制，无裁剪开销，适合大批量
        for (const auto& b : world->balls) {
            ctx.DrawSmoothArc(Point(b.x, b.y), b.radius * 0.5f, 0.0f, 6.28318530718f, b.color, b.radius);
            ctx.DrawSmoothArc(Point(b.x, b.y), b.radius - 1.0f, 0.0f, 6.28318530718f,
                              D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.28f), 1.5f);
        }

        // 拖拽瞄准：瞄准线 + 出射方向预览 + 幽灵球
        if (aim->dragging) {
            float dx = aim->current.x - aim->start.x;
            float dy = aim->current.y - aim->start.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            ctx.DrawSmoothLine(aim->start, aim->current, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.4f), 1.5f);

            if (dist > 2.0f) {
                // 出射方向与拖拽方向相反，速度正比于拖拽距离
                float n = (std::min)(dist, 260.0f);
                float ux = -dx / dist;
                float uy = -dy / dist;
                Point tip{ aim->start.x + ux * n * 0.9f, aim->start.y + uy * n * 0.9f };
                ctx.DrawSmoothLine(aim->start, tip, Rgb(0x51CF66), 2.0f);
                ctx.DrawSmoothArc(tip, 3.5f, 0.0f, 6.28318530718f, Rgb(0x51CF66), 3.0f);
            }
            ctx.DrawSmoothArc(aim->start, 9.0f, 0.0f, 6.28318530718f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.35f), 1.5f);
        }
    });

    // 回调闭包存储于 canvas 自身，捕获裸指针避免 shared_ptr 循环引用
    CanvasControl* canvasRaw = canvas.get();

    canvas->SetOnCanvasMouseDown([aim, canvasRaw](Point pt) {
        aim->dragging = true;
        aim->start = pt;
        aim->current = pt;
        canvasRaw->RequestAnimationTicks(); // 拖拽期间也需要逐帧刷新
    });

    canvas->SetOnCanvasMouseMove([aim, canvasRaw](Point pt) {
        if (aim->dragging) {
            aim->current = pt;
            canvasRaw->MarkRenderRectDirty(canvasRaw->GetBounds());
        }
    });

    canvas->SetOnCanvasMouseUp([aim, canvasRaw, world, ballCount](Point pt) {
        if (!aim->dragging) {
            return;
        }
        aim->dragging = false;
        aim->current = pt;
        float dx = pt.x - aim->start.x;
        float dy = pt.y - aim->start.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 3.0f) {
            // 弹弓发射：速度 = 反向拖拽向量 * 倍率
            world->SpawnBall(aim->start.x, aim->start.y, -dx * kLaunchPower, -dy * kLaunchPower);
            ballCount = (int)world->balls.size();
            canvasRaw->RequestAnimationTicks();
        }
        canvasRaw->MarkRenderRectDirty(canvasRaw->GetBounds());
    });

    // 每帧物理步进：全部静止且未在拖拽时返回 false 自动停止 Tick
    canvas->SetOnTick([world, aim, canvasRaw](float dt) {
        world->Step(dt);
        canvasRaw->MarkRenderRectDirty(canvasRaw->GetBounds());
        return aim->dragging || world->AnyMoving();
    });

    return canvas;
}

} // namespace

std::shared_ptr<UIElement> BuildCanvasPage() {
    // —— 常规用法：绝对坐标定位 ——
    auto stage = MakeStage(180);

    auto button = ElevatedButton("绝对定位按钮").Padding(14, 8, 14, 8).Build();
    button->CanvasLeft = 24.0f;
    button->CanvasTop = 24.0f;
    stage->AddChild(button);

    auto label = Text("Canvas.Left / Canvas.Top 自由摆放").FontSize(13).Build();
    label->ColorToken = ThemeTokenId::TextSecondary;
    label->CanvasLeft = 24.0f;
    label->CanvasTop = 84.0f;
    stage->AddChild(label);

    auto box = RectangleWidget(120, 44).Build();
    box->SetFill(Rgb(0x007ACC, 0.35f));
    box->CornerRadius = 6.0f;
    box->CanvasLeft = 190.0f;
    box->CanvasTop = 70.0f;
    stage->AddChild(box);

    State<float> circleX{ 220.0f };
    State<float> circleY{ 32.0f };
    auto circle = EllipseWidget(72, 72).Build();
    circle->SetFill(Rgb(0x2BAD8E, 0.85f));
    circle->SetStroke(Rgb(0x1B7A63));
    circle->SetStrokeThickness(2.0f);
    circle->CanvasLeft = circleX;
    circle->CanvasTop = circleY;
    stage->AddChild(circle);

    auto reshuffle = ElevatedButton("随机重排", [stage, circle, circleX, circleY](UIElement*) {
        static std::mt19937 rng(1337);
        const Rect bounds = stage->GetBounds();
        const float maxX = (std::max)(0.0f, bounds.width - circle->GetBounds().width);
        const float maxY = (std::max)(0.0f, bounds.height - circle->GetBounds().height);
        std::uniform_real_distribution<float> dx(0.0f, maxX);
        std::uniform_real_distribution<float> dy(0.0f, maxY);
        float nextX = circleX.Get();
        float nextY = circleY.Get();
        do {
            nextX = dx(rng);
            nextY = dy(rng);
        } while (std::abs(nextX - circleX.Get()) < 1.0f && std::abs(nextY - circleY.Get()) < 1.0f);
        circleX = nextX;
        circleY = nextY;
        circle->CanvasLeft = nextX;
        circle->CanvasTop = nextY;
    }).Build();

    auto positionValue = MakeComputed<std::string>([](float x, float y) {
        return std::format("圆形当前坐标：({:.0f}, {:.0f})", x, y);
    }, circleX, circleY);
    auto positionStatus = MakeStatus("");
    positionStatus->Text->Bind(positionValue, BindingMode::OneWay);

    // —— 叠加与层级 ——
    auto layerStage = MakeStage(190);

    auto backRect = RectangleWidget(200, 110).Build();
    backRect->SetFill(Rgb(0x4A90D9, 0.9f));
    backRect->CornerRadius = 4.0f;
    backRect->CanvasLeft = 16.0f;
    backRect->CanvasTop = 16.0f;
    backRect->ZIndex = 0;
    layerStage->AddChild(backRect);

    auto frontEllipse = EllipseWidget(150, 90).Build();
    frontEllipse->SetFill(Rgb(0xE8833A, 0.9f));
    frontEllipse->CanvasLeft = 120.0f;
    frontEllipse->SetStroke(Rgb(0xC86A24));
    frontEllipse->SetStrokeThickness(2.0f);
    frontEllipse->CanvasTop = 70.0f;
    frontEllipse->ZIndex = 1;
    layerStage->AddChild(frontEllipse);

    auto slash = LineWidget(16, 16, 300, 150).Width(320).Height(160).Build();
    slash->SetStroke(Rgb(0xE5484D));
    slash->SetStrokeThickness(3.0f);
    slash->ZIndex = 2;
    layerStage->AddChild(slash);

    // —— 与流式布局对比 ——
    auto canvasSide = MakeStage(150);
    auto sideRect = RectangleWidget(110, 46).Build();
    sideRect->SetFill(Rgb(0x007ACC, 0.45f));
    sideRect->CornerRadius = 4.0f;
    sideRect->CanvasLeft = 20.0f;
    sideRect->CanvasTop = 24.0f;
    canvasSide->AddChild(sideRect);

    auto sideCircle = EllipseWidget(52, 52).Build();
    sideCircle->SetFill(Rgb(0x2BAD8E, 0.9f));
    sideCircle->CanvasLeft = 48.0f;
    sideCircle->CanvasTop = 92.0f;
    canvasSide->AddChild(sideCircle);

    auto flowColumn = Column(8).Padding(12)
        .MinHeight(150.0f);
    flowColumn->BackgroundToken = ThemeTokenId::CardBackground;
    auto flowRect = RectangleWidget(110, 46).Build();
    flowRect->SetFill(Rgb(0x007ACC, 0.45f));
    flowRect->CornerRadius = 4.0f;
    flowColumn->AddChild(flowRect);
    auto flowCircle = EllipseWidget(52, 52).Build();
    flowCircle->SetFill(Rgb(0x2BAD8E, 0.9f));
    flowColumn->AddChild(flowCircle);

    // —— 高性能：重力引擎 + 发射小球 ——
    auto world = std::make_shared<PhysicsWorld>();
    auto aim = std::make_shared<AimState>();
    State<int> ballCount{ 0 };

    auto physicsCanvas = BuildPhysicsCanvas(world, aim, ballCount);

    auto clearButton = ElevatedButton("清空", [world, physicsCanvas, ballCount](UIElement*) {
        world->Clear();
        ballCount = 0;
        physicsCanvas->MarkRenderRectDirty(physicsCanvas->GetBounds());
    }).Build();

    auto burstButton = ElevatedButton("随机发射 3", [world, physicsCanvas, ballCount](UIElement*) {
        world->SpawnRandom(3);
        ballCount = (int)world->balls.size();
        physicsCanvas->RequestAnimationTicks();
        physicsCanvas->MarkRenderRectDirty(physicsCanvas->GetBounds());
    }).Build();

    auto physicsStatusValue = MakeComputed<std::string>([](int count) {
        return std::format("画布内小球：{}", count);
    }, ballCount);
    auto physicsStatus = MakeStatus("");
    physicsStatus->Text->Bind(physicsStatusValue, BindingMode::OneWay);

    SamplePageSpec spec;
    spec.title = "Canvas(画布)";
    spec.subtitle = "使用绝对坐标定位子元素的容器，适合自由排布与图层叠加；搭配 CanvasControl 可承载高性能即时绘制场景。";
    spec.sections = {
        {
            "常规用法",
            "子元素通过附加属性 SetCanvasLeft / SetCanvasTop 定位，互不挤压、可自由重叠。点击“随机重排”可实时移动圆形。",
            Column(12, {
                stage,
                Row(12, {reshuffle, positionStatus }),
            }),
        },
        {
            "叠加与层级",
            "使用 Canvas.ZIndex 明确控制图层；同层仍按 AddChild 顺序绘制，命中测试则从最上层开始。",
            Column(12, {
                layerStage,
                MakeStatus("蓝色矩形 (Z=0) → 橙色椭圆 (Z=1) → 红色斜线 (Z=2)。"),
            }),
        },
        {
            "与流式布局对比",
            "Canvas 不以子元素反推自身尺寸，当前示例宽度随父容器可用空间伸缩；StackPanel 则按子元素顺序参与测量。",
            Row(24, {
                Column(6, {MakeLabel("Canvas(150×150)", 13.0f, ThemeTokenId::TextSecondary, true),
                    canvasSide,
                }),
                Column(6, {MakeLabel("StackPanel(自动撑开)", 13.0f, ThemeTokenId::TextSecondary, true),
                    flowColumn,
                }),
            }),
        },
        {
            "高性能：重力引擎 + 发射小球",
            "CanvasControl 即时绘制模式：物理模拟全部在绘图闭包中运行，不产生任何 DOM 节点。"
            "在画布上按住并拖拽瞄准，松开即以弹弓方式发射小球；支持上百个小球实时重力、反弹与互撞。",
            Column(12, {
                physicsCanvas,
                WrapPanelWidget("Horizontal").Gap(12).Children({ burstButton, clearButton, physicsStatus }).Build(),
            }),
        },
    };
    spec.source =
        "auto stage = CanvasWidget();\n"
        "stage->MinHeight = 180.0f;  // 宽度跟随父容器\n"
        "stage->ClipToBounds = true;\n"
        "\n"
        "auto button = ElevatedButton(\"绝对定位按钮\").Build();\n"
        "button->CanvasLeft = 24.0f;\n"
        "button->CanvasTop = 24.0f;\n"
        "stage->AddChild(button);\n"
        "\n"
        "// ---- 高性能：CanvasControl 即时绘制 + 物理 Tick ----\n"
        "auto canvas = CanvasControlWidget();\n"
        "canvas->MinHeight = 340.0f; // 宽度跟随父容器\n"
        "canvas->ClipToBounds = true;\n"
        "\n"
        "canvas->SetOnDraw([](GraphicsContext& ctx, Size size) {\n"
        "    for (const auto& b : world.balls) {\n"
        "        ctx.DrawSmoothArc(Point(b.x, b.y), b.radius * 0.5f,\n"
        "                          0, 6.283f, b.color, b.radius);\n"
        "    }\n"
        "});\n"
        "\n"
        "canvas->SetOnCanvasMouseDown([](Point pt) { aim.dragging = true; });\n"
        "canvas->SetOnCanvasMouseUp([](Point pt) {\n"
        "    world.SpawnBall(aim.start.x, aim.start.y, -dx, -dy);\n"
        "});\n"
        "\n"
        "// 每帧物理步进；全部静止后返回 false 自动停止 Tick\n"
        "canvas->SetOnTick([](float dt) {\n"
        "    world.Step(dt);\n"
        "    canvas->MarkRenderRectDirty(canvas->GetBounds());\n"
        "    return aim.dragging || world.AnyMoving();\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
