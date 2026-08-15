import os
import re

def enrich_comments():
    # 1. Shape.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\shapes\Shape.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct ShapeFillProperty {', '''    /**
     * @brief 形状填充画刷属性代理，支持通过颜色或画刷直接赋值与隐式转换。
     */
    struct ShapeFillProperty {''')
    c = c.replace('    struct ShapeStrokeProperty {', '''    /**
     * @brief 形状边框描边画刷属性代理，支持通过颜色或画刷直接赋值与隐式转换。
     */
    struct ShapeStrokeProperty {''')
    c = c.replace('    struct ShapeStrokeThicknessProperty {', '''    /**
     * @brief 形状边框描边线条宽度属性代理。
     */
    struct ShapeStrokeThicknessProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 2. DatePicker.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\DatePicker.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    ReadOnlyProperty<std::string> FormattedDate;', '''    /**
     * @brief 格式化日期只读属性代理，返回符合当前格式规范的日期字符串 (如 "YYYY-MM-DD")。
     */
    ReadOnlyProperty<std::string> FormattedDate;''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 3. TimePicker.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TimePicker.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    ReadOnlyProperty<std::string> FormattedTime;', '''    /**
     * @brief 格式化时间只读属性代理，返回符合当前格式规范的时间字符串 (如 "HH:MM")。
     */
    ReadOnlyProperty<std::string> FormattedTime;''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 4. MessageBox.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\MessageBox.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct MessageBoxPrimaryTextProperty {', '''    /**
     * @brief 主确认按钮显示文本属性代理。
     */
    struct MessageBoxPrimaryTextProperty {''')
    c = c.replace('    struct MessageBoxSecondaryTextProperty {', '''    /**
     * @brief 次要按钮显示文本属性代理。
     */
    struct MessageBoxSecondaryTextProperty {''')
    c = c.replace('    struct MessageBoxCloseTextProperty {', '''    /**
     * @brief 关闭/取消按钮显示文本属性代理。
     */
    struct MessageBoxCloseTextProperty {''')
    c = c.replace('    struct MessageBoxInputEnabledProperty {', '''    /**
     * @brief 是否启用单行文本输入框属性代理。
     */
    struct MessageBoxInputEnabledProperty {''')
    c = c.replace('    struct MessageBoxInputTextProperty {', '''    /**
     * @brief 输入框中的文本内容属性代理。
     */
    struct MessageBoxInputTextProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 5. Flyout.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Flyout.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct FlyoutPlacementProperty {', '''    /**
     * @brief 浮出层弹出锚定方位属性代理 (Top/Bottom/Left/Right/Auto 等)。
     */
    struct FlyoutPlacementProperty {''')
    c = c.replace('    struct FlyoutContentProperty {', '''    /**
     * @brief 浮出层内部承载的 UIElement 根内容元素属性代理。
     */
    struct FlyoutContentProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 6. TextBlock.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBlock.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct TextBlockTextAlignProperty {', '''    /**
     * @brief 文本水平对齐方式属性代理 (Left/Center/Right)。
     */
    struct TextBlockTextAlignProperty {''')
    c = c.replace('    struct TextBlockLineSpacingProperty {', '''    /**
     * @brief 文本行间距大小属性代理。
     */
    struct TextBlockLineSpacingProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 7. TextBox.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBox.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct TextBoxIsReadOnlyProperty {', '''    /**
     * @brief 文本框是否处于只读状态属性代理。
     */
    struct TextBoxIsReadOnlyProperty {''')
    c = c.replace('    struct TextBoxIsPasswordModeProperty {', '''    /**
     * @brief 是否启用密码遮罩显示模式属性代理。
     */
    struct TextBoxIsPasswordModeProperty {''')
    c = c.replace('    struct TextBoxShowRevealButtonProperty {', '''    /**
     * @brief 是否显示密码明文查看小眼睛按钮属性代理。
     */
    struct TextBoxShowRevealButtonProperty {''')
    c = c.replace('    struct TextBoxAcceptsReturnProperty {', '''    /**
     * @brief 是否允许回车键插入换行符 (多行模式) 属性代理。
     */
    struct TextBoxAcceptsReturnProperty {''')
    c = c.replace('    struct TextBoxTextWrappingProperty {', '''    /**
     * @brief 文本自动换行模式属性代理 (NoWrap/Wrap)。
     */
    struct TextBoxTextWrappingProperty {''')
    c = c.replace('    struct TextBoxAllowDropProperty {', '''    /**
     * @brief 文本框是否接收文件及文本拖放属性代理。
     */
    struct TextBoxAllowDropProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 8. PasswordBox.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\PasswordBox.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct PasswordBoxPasswordProperty {', '''    /**
     * @brief 密码输入框真实明文密码属性代理。
     */
    struct PasswordBoxPasswordProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 9. NumberBox.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NumberBox.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct NumberBoxValueProperty {', '''    /**
     * @brief 数字框当前数值属性代理。
     */
    struct NumberBoxValueProperty {''')
    c = c.replace('    struct NumberBoxMinimumProperty {', '''    /**
     * @brief 数字框允许输入的下限最小值属性代理。
     */
    struct NumberBoxMinimumProperty {''')
    c = c.replace('    struct NumberBoxMaximumProperty {', '''    /**
     * @brief 数字框允许输入的上限最大值属性代理。
     */
    struct NumberBoxMaximumProperty {''')
    c = c.replace('    struct NumberBoxStepProperty {', '''    /**
     * @brief 微调步长增量属性代理。
     */
    struct NumberBoxStepProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 10. CanvasControl.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CanvasControl.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct CanvasControlViewportProperty {', '''    /**
     * @brief 画布逻辑视口尺寸 (Width x Height) 属性代理。
     */
    struct CanvasControlViewportProperty {''')
    c = c.replace('    struct CanvasControlOnDrawProperty {', '''    /**
     * @brief 画布自定义 Direct2D 渲染回调属性代理。
     */
    struct CanvasControlOnDrawProperty {''')
    c = c.replace('    struct CanvasControlOnMouseDownProperty {', '''    /**
     * @brief 画布鼠标按下事件回调属性代理。
     */
    struct CanvasControlOnMouseDownProperty {''')
    c = c.replace('    struct CanvasControlOnMouseUpProperty {', '''    /**
     * @brief 画布鼠标抬起事件回调属性代理。
     */
    struct CanvasControlOnMouseUpProperty {''')
    c = c.replace('    struct CanvasControlOnMouseMoveProperty {', '''    /**
     * @brief 画布鼠标移动事件回调属性代理。
     */
    struct CanvasControlOnMouseMoveProperty {''')
    c = c.replace('    struct CanvasControlOnTickProperty {', '''    /**
     * @brief 画布定时渲染刷新 Tick 回调属性代理。
     */
    struct CanvasControlOnTickProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 11. Expander.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Expander.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct ExpanderIsExpandedProperty {', '''    /**
     * @brief 折叠面板当前是否处于展开状态属性代理。
     */
    struct ExpanderIsExpandedProperty {''')
    c = c.replace('    struct ExpanderContentProperty {', '''    /**
     * @brief 折叠面板折叠区承载的 UIElement 内容元素属性代理。
     */
    struct ExpanderContentProperty {''')
    c = c.replace('    struct ExpanderExpandDirectionProperty {', '''    /**
     * @brief 折叠展开方向属性代理 (Down/Up 等)。
     */
    struct ExpanderExpandDirectionProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 12. RatingControl.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\RatingControl.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct RatingControlMaxRatingProperty {', '''    /**
     * @brief 评分控件最大星级总数 (1~10) 属性代理。
     */
    struct RatingControlMaxRatingProperty {''')
    c = c.replace('    struct RatingControlStepProperty {', '''    /**
     * @brief 评分调节精度步长 (如 0.5 或 1.0) 属性代理。
     */
    struct RatingControlStepProperty {''')
    c = c.replace('    struct RatingControlValueProperty {', '''    /**
     * @brief 评分控件当前分值属性代理。
     */
    struct RatingControlValueProperty {''')
    c = c.replace('    struct RatingControlIsReadOnlyProperty {', '''    /**
     * @brief 评分控件是否处于只读展示模式属性代理。
     */
    struct RatingControlIsReadOnlyProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 13. ListBox.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListBox.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct ListBoxSelectedIndicesProperty {', '''    /**
     * @brief 列表框多选选中的索引集合属性代理。
     */
    struct ListBoxSelectedIndicesProperty {''')
    c = c.replace('    struct ListBoxSelectedItemProperty {', '''    /**
     * @brief 列表框当前选中的单项文本属性代理。
     */
    struct ListBoxSelectedItemProperty {''')
    c = c.replace('    struct ListBoxAllowDragProperty {', '''    /**
     * @brief 是否允许拖拽列表项属性代理。
     */
    struct ListBoxAllowDragProperty {''')
    c = c.replace('    struct ListBoxAllowDropProperty {', '''    /**
     * @brief 是否允许向列表框拖放放入项属性代理。
     */
    struct ListBoxAllowDropProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 14. ListView.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListView.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct ListViewSelectedIndicesProperty {', '''    /**
     * @brief 多列列表视图多选行索引集合属性代理。
     */
    struct ListViewSelectedIndicesProperty {''')
    c = c.replace('    struct ListViewCaretIndexProperty {', '''    /**
     * @brief 列表视图当前活动光标行索引属性代理。
     */
    struct ListViewCaretIndexProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 15. TreeView.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TreeView.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct TreeViewItemsProperty {', '''    /**
     * @brief 树视图根节点集合属性代理，支持容器级操作。
     */
    struct TreeViewItemsProperty {''')
    c = c.replace('    struct TreeViewSelectedItemProperty {', '''    /**
     * @brief 树视图当前选中的节点对象属性代理。
     */
    struct TreeViewSelectedItemProperty {''')
    c = c.replace('    struct TreeViewIndentWidthProperty {', '''    /**
     * @brief 树视图每级子节点的水平缩进宽度属性代理。
     */
    struct TreeViewIndentWidthProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 16. NavigationView.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NavigationView.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct NavIsPaneOpenProperty {', '''    /**
     * @brief 导航栏侧边面板当前是否处于展开状态属性代理。
     */
    struct NavIsPaneOpenProperty {''')
    c = c.replace('    struct NavIsSettingsVisibleProperty {', '''    /**
     * @brief 底部设置项是否可见属性代理。
     */
    struct NavIsSettingsVisibleProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 17. NavigationViewItem.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NavigationViewItem.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct NavItemContentProperty {', '''    /**
     * @brief 导航项标题文本内容属性代理。
     */
    struct NavItemContentProperty {''')
    c = c.replace('    struct NavItemSelectsOnInvokedProperty {', '''    /**
     * @brief 导航项被点击激活时是否自动选中该项属性代理。
     */
    struct NavItemSelectsOnInvokedProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 18. WindowTitleBar.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\WindowTitleBar.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct WindowTitleBarRightContentProperty {', '''    /**
     * @brief 窗口标题栏右侧自定义操作区内容元素属性代理。
     */
    struct WindowTitleBarRightContentProperty {''')
    c = c.replace('    struct WindowTitleBarMenuBarProperty {', '''    /**
     * @brief 窗口标题栏内嵌的主菜单栏对象属性代理。
     */
    struct WindowTitleBarMenuBarProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 19. Window.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\window\Window.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct WindowThemeModeProperty {', '''    /**
     * @brief 窗口主题模式 (Light / Dark) 属性代理。
     */
    struct WindowThemeModeProperty {''')
    c = c.replace('    struct WindowBackdropTypeProperty {', '''    /**
     * @brief 窗口背景材质 (Mica / Acrylic / None) 属性代理。
     */
    struct WindowBackdropTypeProperty {''')
    c = c.replace('    struct WindowRenderStatsProperty {', '''    /**
     * @brief 渲染性能统计浮层是否显示属性代理。
     */
    struct WindowRenderStatsProperty {''')
    c = c.replace('    struct WindowRootElementProperty {', '''    /**
     * @brief 窗口承载的顶层根 UI 元素属性代理。
     */
    struct WindowRootElementProperty {''')
    c = c.replace('    struct WindowHWNDProperty {', '''    /**
     * @brief Win32 宿主窗口原生句柄只读属性代理。
     */
    struct WindowHWNDProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 20. CheckBox.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CheckBox.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct CheckBoxIsCheckedProperty {', '''    /**
     * @brief 复选框选中状态布尔值属性代理。
     */
    struct CheckBoxIsCheckedProperty {''')
    c = c.replace('    struct CheckBoxIsThreeStateProperty {', '''    /**
     * @brief 是否启用三态模式 (支持未选/半选/全选) 属性代理。
     */
    struct CheckBoxIsThreeStateProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 21. ToggleButton.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ToggleButton.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct ToggleButtonIsCheckedProperty {', '''    /**
     * @brief 切换开关按钮选中状态布尔值属性代理。
     */
    struct ToggleButtonIsCheckedProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 22. Slider.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Slider.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct SliderMinimumProperty {', '''    /**
     * @brief 滑块调节范围最小值属性代理。
     */
    struct SliderMinimumProperty {''')
    c = c.replace('    struct SliderMaximumProperty {', '''    /**
     * @brief 滑块调节范围最大值属性代理。
     */
    struct SliderMaximumProperty {''')
    c = c.replace('    struct SliderStepProperty {', '''    /**
     * @brief 滑块微调步长精度属性代理。
     */
    struct SliderStepProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    # 23. MarkdownView.h
    p = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\MarkdownView.h'
    with open(p, 'r', encoding='utf-8') as f:
        c = f.read()
    c = c.replace('    struct MarkdownViewMarkdownProperty {', '''    /**
     * @brief Markdown 视图原始文档字符串内容属性代理。
     */
    struct MarkdownViewMarkdownProperty {''')
    with open(p, 'w', encoding='utf-8') as f:
        f.write(c)

    print("All header property comments enriched.")

enrich_comments()
