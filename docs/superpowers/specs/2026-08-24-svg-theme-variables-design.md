# SVG 主题变量设计

日期：2026-08-24

## 目标

让内嵌 SVG 的 `fill` 与 `stroke` 支持主题变量，而非将十六进制颜色固化在 SVG 文本中。

目标写法：

```xml
<path d="..." fill="var(--foreground)"/>
<path d="..." stroke="var(--accent)"/>
```

## 范围

- 支持完整 SVG 标记及由 `SvgPath` 包装生成的 SVG。
- 支持 `fill`、`stroke`、`color` 属性中的 `var(--token-name)`。
- 保留 `#RRGGBB`、`#RRGGBBAA`、`rgb()`、`none`、`url(...)` 的既有行为。
- 保留现有 `DrawSvg(..., tint)` 强制 Tint 行为；显式 Tint 的优先级高于变量解析。

不在本次范围：

- CSS `<style>`、class 选择器、复杂 `var(--name, fallback)` 语法。
- 继承外部 CSS 文件。

## 变量语义

| SVG 变量 | 解析规则 |
| --- | --- |
| `--foreground` | 调用方传入的前景色；无调用方颜色时使用主题 `TextPrimary`。 |
| `--background` | 主题 `WindowBackground`。 |
| `--accent` | 主题 `AccentColor`。 |
| `--text-primary` | 主题 `TextPrimary`。 |
| `--text-secondary` | 主题 `TextSecondary`。 |
| `--border` | 主题 `BorderColor`。 |
| `--disabled` | 主题 `TextDisabled`。 |

`currentColor` 作为 `var(--foreground)` 的兼容别名。

## 渲染与缓存

1. `GraphicsContext::GetOrCreateSvg` 在创建 Direct2D SVG Document 前扫描并替换主题变量。
2. 缓存键由原始 SVG、解析后的主题颜色、Tint 颜色以及主题版本组成。
3. `ThemeManager` 主题变化时版本递增；下一次绘制使用新键创建文档，旧缓存条目不再命中。
4. 不修改传入的 SVG 源字符串，也不要求调用方手动刷新图标。

## API 设计

保留现有接口：

```cpp
ctx.DrawSvg(source, bounds, tint, opacity);
```

新增可选主题上下文参数（默认由当前 `ThemeManager` 提供）：

```cpp
ctx.DrawSvg(source, bounds, tint, opacity, SvgThemeContext{
    .foreground = elementForeground,
});
```

普通控件、`SvgIcon` 和标题栏图标传入所属元素的实际前景色；其他调用不传时由 `TextPrimary` 兜底。

## 兼容与优先级

1. `tint` 非空：对形状执行既有强制 Tint，忽略其 `fill` / `stroke` 变量语义。
2. `tint` 为空：`var(--...)` 解析为主题颜色。
3. 普通固定色继续按原 SVG 绘制。
4. `fill="none"`、`fill-opacity="0"` 和 `url(...)` 不做变量替换。

## 验证

- 在亮/暗主题中渲染 `var(--foreground)`、`var(--accent)`、固定十六进制色混合 SVG。
- 对 `fill`、`stroke` 与 `currentColor` 分别验证。
- 切换主题后确认图标变色、固定色不变、缓存不会复用旧主题文档。
- 保持 Demo、Gallery 和 Showcase 编译通过。
