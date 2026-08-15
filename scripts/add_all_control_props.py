import os
import re

# 1. Shape.h & Shape.cpp
def fix_shape():
    shape_h = '''#pragma once
#include "../UIElement.h"
#include "../Image.h" // 引用 Stretch 枚举类型

namespace CUI {

/**
 * @brief 所有 2D 声明式矢量图形 DOM 节点的抽象基类（Shape）。
 */
class Shape : public UIElement {
public:
    Shape();
    virtual ~Shape() = default;

    virtual const char* GetClassName() const override { return "Shape"; }

    struct ShapeFillProperty {
        Shape* owner = nullptr;
        ShapeFillProperty() = default;
        explicit ShapeFillProperty(Shape* o) : owner(o) {}
        ShapeFillProperty& operator=(D2D1_COLOR_F fill) { if (owner) owner->SetFill(fill); return *this; }
        operator D2D1_COLOR_F() const { return owner ? owner->GetFill() : D2D1_COLOR_F{}; }
        D2D1_COLOR_F Get() const { return owner ? owner->GetFill() : D2D1_COLOR_F{}; }
    } Fill;

    struct ShapeStrokeProperty {
        Shape* owner = nullptr;
        ShapeStrokeProperty() = default;
        explicit ShapeStrokeProperty(Shape* o) : owner(o) {}
        ShapeStrokeProperty& operator=(D2D1_COLOR_F stroke) { if (owner) owner->SetStroke(stroke); return *this; }
        operator D2D1_COLOR_F() const { return owner ? owner->GetStroke() : D2D1_COLOR_F{}; }
        D2D1_COLOR_F Get() const { return owner ? owner->GetStroke() : D2D1_COLOR_F{}; }
    } Stroke;

    struct ShapeStrokeThicknessProperty {
        Shape* owner = nullptr;
        ShapeStrokeThicknessProperty() = default;
        explicit ShapeStrokeThicknessProperty(Shape* o) : owner(o) {}
        ShapeStrokeThicknessProperty& operator=(float t) { if (owner) owner->SetStrokeThickness(t); return *this; }
        operator float() const { return owner ? owner->GetStrokeThickness() : 1.0f; }
        float Get() const { return owner ? owner->GetStrokeThickness() : 1.0f; }
    } StrokeThickness;

    D2D1_COLOR_F GetFill() const { return m_fill; }
    void SetFill(D2D1_COLOR_F fill) { m_fill = fill; Invalidate(); }

    D2D1_COLOR_F GetStroke() const { return m_stroke; }
    void SetStroke(D2D1_COLOR_F stroke) { m_stroke = stroke; Invalidate(); }

    float GetStrokeThickness() const { return m_strokeThickness; }
    void SetStrokeThickness(float thickness) { m_strokeThickness = thickness; Invalidate(); }

    Stretch GetStretch() const { return m_stretch; }
    void SetStretch(Stretch stretch) { m_stretch = stretch; Invalidate(); }

    void Invalidate() { InvalidateMeasure(); InvalidateArrange(); }
    virtual void OnRender(GraphicsContext& ctx) override = 0;

protected:
    D2D1_COLOR_F m_fill = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f);
    D2D1_COLOR_F m_stroke = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f);
    float m_strokeThickness = 1.0f;
    Stretch m_stretch = Stretch::None;
};

} // namespace CUI
'''
    with open(r'E:\C++project\CUI\CUI.Core\ui\framework\controls\shapes\Shape.h', 'w', encoding='utf-8') as f:
        f.write(shape_h)

    shape_cpp = '''#include "Shape.h"

namespace CUI {

Shape::Shape() : Fill(this), Stroke(this), StrokeThickness(this) {
}

} // namespace CUI
'''
    with open(r'E:\C++project\CUI\CUI.Core\ui\framework\controls\shapes\Shape.cpp', 'w', encoding='utf-8') as f:
        f.write(shape_cpp)

# 2. DatePicker.h & DatePicker.cpp
def fix_datepicker():
    path_h = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\DatePicker.h'
    with open(path_h, 'r', encoding='utf-8') as f:
        h = f.read()
    if 'struct DatePickerFormattedDateProperty' not in h:
        prop = '''    struct DatePickerFormattedDateProperty {
        DatePicker* owner = nullptr;
        DatePickerFormattedDateProperty() = default;
        explicit DatePickerFormattedDateProperty(DatePicker* o) : owner(o) {}
        operator std::string() const { return owner ? owner->GetFormattedDate() : ""; }
        std::string Get() const { return owner ? owner->GetFormattedDate() : ""; }
    } FormattedDate;

'''
        h = h.replace('    std::string GetFormattedDate() const;', prop + '    std::string GetFormattedDate() const;')
        with open(path_h, 'w', encoding='utf-8') as f:
            f.write(h)

    path_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\DatePicker.cpp'
    with open(path_cpp, 'r', encoding='utf-8') as f:
        cpp = f.read()
    if 'FormattedDate(this)' not in cpp:
        cpp = cpp.replace('DatePicker::DatePicker() {', 'DatePicker::DatePicker() : FormattedDate(this) {')
        with open(path_cpp, 'w', encoding='utf-8') as f:
            f.write(cpp)

# 3. TimePicker.h & TimePicker.cpp
def fix_timepicker():
    path_h = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TimePicker.h'
    with open(path_h, 'r', encoding='utf-8') as f:
        h = f.read()
    if 'struct TimePickerFormattedTimeProperty' not in h:
        prop = '''    struct TimePickerFormattedTimeProperty {
        TimePicker* owner = nullptr;
        TimePickerFormattedTimeProperty() = default;
        explicit TimePickerFormattedTimeProperty(TimePicker* o) : owner(o) {}
        operator std::string() const { return owner ? owner->GetFormattedTime() : ""; }
        std::string Get() const { return owner ? owner->GetFormattedTime() : ""; }
    } FormattedTime;

'''
        h = h.replace('    std::string GetFormattedTime() const;', prop + '    std::string GetFormattedTime() const;')
        with open(path_h, 'w', encoding='utf-8') as f:
            f.write(h)

    path_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TimePicker.cpp'
    with open(path_cpp, 'r', encoding='utf-8') as f:
        cpp = f.read()
    if 'FormattedTime(this)' not in cpp:
        cpp = cpp.replace('TimePicker::TimePicker() {', 'TimePicker::TimePicker() : FormattedTime(this) {')
        with open(path_cpp, 'w', encoding='utf-8') as f:
            f.write(cpp)

# 4. ContentDialog.h & ContentDialog.cpp (or inline)
def fix_contentdialog():
    path_h = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\MessageBox.h'
    with open(path_h, 'r', encoding='utf-8') as f:
        h = f.read()
    if 'struct ContentDialogPrimaryButtonTextProperty' not in h:
        props = '''    struct ContentDialogPrimaryButtonTextProperty {
        ContentDialog* owner = nullptr;
        ContentDialogPrimaryButtonTextProperty() = default;
        explicit ContentDialogPrimaryButtonTextProperty(ContentDialog* o) : owner(o) {}
        ContentDialogPrimaryButtonTextProperty& operator=(const std::string& t) { if (owner) owner->SetPrimaryButtonText(t); return *this; }
        operator std::string() const { return owner ? owner->GetPrimaryButtonText() : ""; }
        std::string Get() const { return owner ? owner->GetPrimaryButtonText() : ""; }
    } PrimaryButtonText;

    struct ContentDialogSecondaryButtonTextProperty {
        ContentDialog* owner = nullptr;
        ContentDialogSecondaryButtonTextProperty() = default;
        explicit ContentDialogSecondaryButtonTextProperty(ContentDialog* o) : owner(o) {}
        ContentDialogSecondaryButtonTextProperty& operator=(const std::string& t) { if (owner) owner->SetSecondaryButtonText(t); return *this; }
        operator std::string() const { return owner ? owner->GetSecondaryButtonText() : ""; }
        std::string Get() const { return owner ? owner->GetSecondaryButtonText() : ""; }
    } SecondaryButtonText;

    struct ContentDialogCloseButtonTextProperty {
        ContentDialog* owner = nullptr;
        ContentDialogCloseButtonTextProperty() = default;
        explicit ContentDialogCloseButtonTextProperty(ContentDialog* o) : owner(o) {}
        ContentDialogCloseButtonTextProperty& operator=(const std::string& t) { if (owner) owner->SetCloseButtonText(t); return *this; }
        operator std::string() const { return owner ? owner->GetCloseButtonText() : ""; }
        std::string Get() const { return owner ? owner->GetCloseButtonText() : ""; }
    } CloseButtonText;

    struct ContentDialogInputEnabledProperty {
        ContentDialog* owner = nullptr;
        ContentDialogInputEnabledProperty() = default;
        explicit ContentDialogInputEnabledProperty(ContentDialog* o) : owner(o) {}
        ContentDialogInputEnabledProperty& operator=(bool e) { if (owner) owner->SetInputEnabled(e); return *this; }
        operator bool() const { return owner ? owner->GetInputEnabled() : false; }
        bool Get() const { return owner ? owner->GetInputEnabled() : false; }
    } InputEnabled;

    struct ContentDialogInputTextProperty {
        ContentDialog* owner = nullptr;
        ContentDialogInputTextProperty() = default;
        explicit ContentDialogInputTextProperty(ContentDialog* o) : owner(o) {}
        ContentDialogInputTextProperty& operator=(const std::string& t) { if (owner) owner->SetInputText(t); return *this; }
        operator std::string() const { return owner ? owner->GetInputText() : ""; }
        std::string Get() const { return owner ? owner->GetInputText() : ""; }
    } InputText;

'''
        h = h.replace('    void SetPrimaryButtonText(const std::string& text);', props + '    void SetPrimaryButtonText(const std::string& text);')
        with open(path_h, 'w', encoding='utf-8') as f:
            f.write(h)

    path_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\MessageBox.cpp'
    with open(path_cpp, 'r', encoding='utf-8') as f:
        cpp = f.read()
    if 'PrimaryButtonText(this)' not in cpp:
        cpp = cpp.replace('ContentDialog::ContentDialog(const std::string& title, const std::string& message)',
                          'ContentDialog::ContentDialog(const std::string& title, const std::string& message) : PrimaryButtonText(this), SecondaryButtonText(this), CloseButtonText(this), InputEnabled(this), InputText(this)')
        with open(path_cpp, 'w', encoding='utf-8') as f:
            f.write(cpp)

# 5. Flyout.h & Flyout.cpp
def fix_flyout():
    path_h = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Flyout.h'
    with open(path_h, 'r', encoding='utf-8') as f:
        h = f.read()
    if 'struct FlyoutPlacementProperty' not in h:
        props = '''    struct FlyoutPlacementProperty {
        Flyout* owner = nullptr;
        FlyoutPlacementProperty() = default;
        explicit FlyoutPlacementProperty(Flyout* o) : owner(o) {}
        FlyoutPlacementProperty& operator=(BubblePlacement p) { if (owner) owner->SetPlacement(p); return *this; }
        operator BubblePlacement() const { return owner ? owner->GetPlacement() : BubblePlacement::Bottom; }
        BubblePlacement Get() const { return owner ? owner->GetPlacement() : BubblePlacement::Bottom; }
    } Placement;

    struct FlyoutContentProperty {
        Flyout* owner = nullptr;
        FlyoutContentProperty() = default;
        explicit FlyoutContentProperty(Flyout* o) : owner(o) {}
        FlyoutContentProperty& operator=(std::shared_ptr<UIElement> c) { if (owner) owner->SetContent(std::move(c)); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner ? owner->GetContent() : nullptr; }
        std::shared_ptr<UIElement> Get() const { return owner ? owner->GetContent() : nullptr; }
    } Content;

'''
        h = h.replace('    BubblePlacement GetPlacement() const', props + '    BubblePlacement GetPlacement() const')
        with open(path_h, 'w', encoding='utf-8') as f:
            f.write(h)

    path_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Flyout.cpp'
    with open(path_cpp, 'r', encoding='utf-8') as f:
        cpp = f.read()
    if 'Placement(this)' not in cpp:
        cpp = cpp.replace('Flyout::Flyout() {', 'Flyout::Flyout() : Placement(this), Content(this) {')
        with open(path_cpp, 'w', encoding='utf-8') as f:
            f.write(cpp)

# 6. TextBlock.h & TextBlock.cpp
def fix_textblock():
    path_h = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBlock.h'
    with open(path_h, 'r', encoding='utf-8') as f:
        h = f.read()
    if 'struct TextBlockTextAlignProperty' not in h:
        props = '''    struct TextBlockTextAlignProperty {
        TextBlock* owner = nullptr;
        TextBlockTextAlignProperty() = default;
        explicit TextBlockTextAlignProperty(TextBlock* o) : owner(o) {}
        TextBlockTextAlignProperty& operator=(TextAlign a) { if (owner) owner->SetTextAlign(a); return *this; }
        operator TextAlign() const { return owner ? owner->GetTextAlign() : TextAlign::Left; }
        TextAlign Get() const { return owner ? owner->GetTextAlign() : TextAlign::Left; }
    } TextAlign;

    struct TextBlockLineSpacingProperty {
        TextBlock* owner = nullptr;
        TextBlockLineSpacingProperty() = default;
        explicit TextBlockLineSpacingProperty(TextBlock* o) : owner(o) {}
        TextBlockLineSpacingProperty& operator=(float s) { if (owner) owner->SetLineSpacing(s); return *this; }
        operator float() const { return owner ? owner->GetLineSpacing() : 0.0f; }
        float Get() const { return owner ? owner->GetLineSpacing() : 0.0f; }
    } LineSpacing;

'''
        h = h.replace('    TextAlign GetTextAlign() const', props + '    TextAlign GetTextAlign() const')
        with open(path_h, 'w', encoding='utf-8') as f:
            f.write(h)

    path_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBlock.cpp'
    with open(path_cpp, 'r', encoding='utf-8') as f:
        cpp = f.read()
    if 'TextAlign(this)' not in cpp:
        cpp = cpp.replace('TextBlock::TextBlock(const std::string& text)\n    : m_text(text) {',
                          'TextBlock::TextBlock(const std::string& text)\n    : m_text(text), TextAlign(this), LineSpacing(this) {')
        with open(path_cpp, 'w', encoding='utf-8') as f:
            f.write(cpp)

# 7. PasswordBox.h & PasswordBox.cpp
def fix_passwordbox():
    path_h = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\PasswordBox.h'
    with open(path_h, 'r', encoding='utf-8') as f:
        h = f.read()
    if 'struct PasswordBoxPasswordProperty' not in h:
        props = '''    struct PasswordBoxPasswordProperty {
        PasswordBox* owner = nullptr;
        PasswordBoxPasswordProperty() = default;
        explicit PasswordBoxPasswordProperty(PasswordBox* o) : owner(o) {}
        PasswordBoxPasswordProperty& operator=(const std::string& p) { if (owner) owner->SetPassword(p); return *this; }
        operator std::string() const { return owner ? owner->GetPassword() : ""; }
        std::string Get() const { return owner ? owner->GetPassword() : ""; }
    } Password;

'''
        h = h.replace('    void SetPassword(const std::string& pwd);', props + '    void SetPassword(const std::string& pwd);')
        with open(path_h, 'w', encoding='utf-8') as f:
            f.write(h)

    path_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\PasswordBox.cpp'
    with open(path_cpp, 'r', encoding='utf-8') as f:
        cpp = f.read()
    if 'Password(this)' not in cpp:
        cpp = cpp.replace('PasswordBox::PasswordBox() : TextBox("请输入密码") {', 'PasswordBox::PasswordBox() : TextBox("请输入密码"), Password(this) {')
        cpp = cpp.replace('PasswordBox::PasswordBox(const std::string& placeholder) : TextBox(placeholder) {', 'PasswordBox::PasswordBox(const std::string& placeholder) : TextBox(placeholder), Password(this) {')
        with open(path_cpp, 'w', encoding='utf-8') as f:
            f.write(cpp)

# 8. CanvasControl.h & CanvasControl.cpp
def fix_canvascontrol():
    path_h = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CanvasControl.h'
    with open(path_h, 'r', encoding='utf-8') as f:
        h = f.read()
    if 'struct CanvasViewportProperty' not in h:
        props = '''    struct CanvasViewportProperty {
        CanvasControl* owner = nullptr;
        CanvasViewportProperty() = default;
        explicit CanvasViewportProperty(CanvasControl* o) : owner(o) {}
        CanvasViewportProperty& operator=(const Rect& vp) { if (owner) owner->SetViewport(vp); return *this; }
        operator Rect() const { return owner ? owner->GetViewport() : Rect{}; }
        Rect Get() const { return owner ? owner->GetViewport() : Rect{}; }
    } Viewport;

'''
        h = h.replace('    // Callbacks', props + '    // Callbacks')
        with open(path_h, 'w', encoding='utf-8') as f:
            f.write(h)

    path_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CanvasControl.cpp'
    with open(path_cpp, 'r', encoding='utf-8') as f:
        cpp = f.read()
    if 'Viewport(this)' not in cpp:
        cpp = cpp.replace('CanvasControl::CanvasControl() {', 'CanvasControl::CanvasControl() : Viewport(this) {')
        with open(path_cpp, 'w', encoding='utf-8') as f:
            f.write(cpp)

fix_shape()
fix_datepicker()
fix_timepicker()
fix_contentdialog()
fix_flyout()
fix_textblock()
fix_passwordbox()
fix_canvascontrol()
print("All control properties added cleanly.")
