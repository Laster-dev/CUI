# CUI（showcase 主应用）—— 代码审查报告

- 审查日期：2026-09-19
- 工程路径：`CUI/`
- 规模：58 源文件 / 6 头文件（69 文件）
- 方式：只读静态审查

---

## 1. 范围与架构

| 文件 | 职责 |
|---|---|
| `showcase/app/main.cpp` | 应用入口；`GalleryNavigation::build`（368 行）构建示例导航；页面缓存与 LRU |
| `showcase/.../VSCodeControls.cpp` | VS Code 风格自绘控件集（TitleBar / SideBar 等） |
| `showcase/.../Streaming.cpp` | 屏幕流/图像采集与渲染示例 |
| 其余 `.cpp` | 各类控件与效果演示 |

---

## 2. P0 — 生命周期与并发

| # | 问题 | 位置 | 后果 | 建议 |
|---|---|---|---|---|
| 1 | `navigateToTag` 闭包捕获 `shared_ptr` 形成循环引用 | `showcase/app/main.cpp:331, 368` | 页面与导航器互相持有 → **永不释放** | 改 `weak_ptr`，回调内 `lock()` 判空 |
| 2 | `g_streamImage` 被采集线程与渲染线程并发读写，无同步 | `showcase/.../Streaming.cpp` | **数据竞争**、图像撕裂、偶发崩溃 | 双缓冲 + `atomic` 交换，或互斥锁保护 |
| 3 | GDI `SelectObject` 后未还原原对象 | `showcase/.../Streaming.cpp` | **GDI 句柄泄漏**，长时间运行后资源耗尽 | RAII 封装 `SelectObject`/`DeleteObject` |

---

## 3. P1 — 正确性

| 问题 | 位置 | 说明与建议 |
|---|---|---|
| LRU 淘汰判定 `evict != tag` 与缓存键不一致 | `showcase/app/main.cpp:205` | 淘汰错误页面，缓存命中率下降 | 统一键类型，加断言与单测 |
| `SideBar` 绝对坐标与相对坐标混用 | `VSCodeControls.cpp:553, 561` | 命中测试与绘制区域错位，点击偏移 | 统一到父坐标系后再比较 |
| 使用默认构造的 `GraphicsContext` 做文本测量 | `VSCodeControls.cpp` | 无有效渲染目标，测量结果不可信 | 传入真实上下文或专门的测量上下文 |

---

## 4. P1 — 性能

| 问题 | 位置 | 说明与建议 |
|---|---|---|
| 100Hz 全屏抓取 | `Streaming.cpp` | CPU/GPU 占用过高 | 降频至 30Hz 或改为脏矩形/差分捕获 |
| `TitleBar::OnRender` 155 行 | `VSCodeControls.cpp` | 绘制逻辑过长且每次全量重绘 | 拆分子绘制函数；静态部分走缓存位图 |
| `GalleryNavigation::build` 368 行 | `main.cpp` | 可维护性差 | 改为表驱动（元数据 + 工厂） |
| 逐帧创建 DirectWrite 对象 | 全局 | 同 CUI.Core：按属性元组缓存 |

---

## 5. P2 — 工程质量

- 示例页面之间大量重复的面板/列表构建代码 → 抽取共享构建器。
- 确认 `CUI.vcxproj` 全配置带 `/utf-8`。
- 示例页建议加"复杂度/耗时"标注，便于演示性能特性。

---

## 6. 优先修复建议

**P0（立即）**
1. `navigateToTag` 闭包改 `weak_ptr`（#1）
2. `g_streamImage` 加锁或双缓冲（#2）
3. GDI 对象 RAII 还原（#3）

**P1（本迭代）**
4. LRU 键统一（#4）
5. SideBar 坐标系统一（#5）
6. 测量使用有效 `GraphicsContext`（#6）
7. 抓取降频、`OnRender` 拆分、`build` 表驱动化

**P2**
8. 抽取共享示例组件
9. 构建配置核对
