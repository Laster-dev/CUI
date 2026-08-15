import os
import re

def update_file(path, transformer):
    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    new_content = transformer(content)
    if new_content != content:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Updated {path}")
        return True
    return False

# 1. Property.h: Add ArrowProxy to ReadOnlyProperty and Property
def fix_property_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\core\Property.h'
    def trans(content):
        if 'struct ArrowProxy' not in content:
            target = '''    operator T() const { return Get(); }
    T Get() const { return m_getter ? m_getter() : T{}; }'''
            replacement = '''    operator T() const { return Get(); }
    T Get() const { return m_getter ? m_getter() : T{}; }

    struct ArrowProxy {
        T value;
        const T* operator->() const { return &value; }
    };
    ArrowProxy operator->() const { return ArrowProxy{ Get() }; }'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

# 2. Shape.h: Fill, Stroke, StrokeThickness
def fix_shape_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\shapes\Shape.h'
    def trans(content):
        if 'struct ShapeFillProperty' not in content:
            target = '    // -------------------------------------------------------------------------'
            replacement = '''    struct ShapeFillProperty {
        Shape* owner;
        ShapeFillProperty& operator=(D2D1_COLOR_F fill) { owner->SetFill(fill); return *this; }
        operator D2D1_COLOR_F() const { return owner->GetFill(); }
        D2D1_COLOR_F Get() const { return owner->GetFill(); }
    } Fill{this};

    struct ShapeStrokeProperty {
        Shape* owner;
        ShapeStrokeProperty& operator=(D2D1_COLOR_F stroke) { owner->SetStroke(stroke); return *this; }
        operator D2D1_COLOR_F() const { return owner->GetStroke(); }
        D2D1_COLOR_F Get() const { return owner->GetStroke(); }
    } Stroke{this};

    struct ShapeStrokeThicknessProperty {
        Shape* owner;
        ShapeStrokeThicknessProperty& operator=(float t) { owner->SetStrokeThickness(t); return *this; }
        operator float() const { return owner->GetStrokeThickness(); }
        float Get() const { return owner->GetStrokeThickness(); }
    } StrokeThickness{this};

    // -------------------------------------------------------------------------'''
            content = content.replace(target, replacement, 1)
        return content
    update_file(path, trans)

# 3. CanvasControl.h: Viewport
def fix_canvas_control_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CanvasControl.h'
    def trans(content):
        if 'struct CanvasViewportProperty' not in content:
            target = '    // Callbacks'
            replacement = '''    struct CanvasViewportProperty {
        CanvasControl* owner;
        CanvasViewportProperty& operator=(const Rect& vp) { owner->SetViewport(vp); return *this; }
        operator Rect() const { return owner->GetViewport(); }
        Rect Get() const { return owner->GetViewport(); }
    } Viewport{this};

    // Callbacks'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

# 4. TeachingTip.h: IsModal, PreferredPlacement
def fix_teaching_tip_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TeachingTip.h'
    def trans(content):
        if 'struct TeachingTipIsModalProperty' not in content:
            target = '    void SetTitle(const std::string& title);'
            replacement = '''    struct TeachingTipIsModalProperty {
        TeachingTip* owner;
        TeachingTipIsModalProperty& operator=(bool m) { owner->SetIsModal(m); return *this; }
        operator bool() const { return owner->GetIsModal(); }
        bool Get() const { return owner->GetIsModal(); }
    } IsModal{this};

    struct TeachingTipPreferredPlacementProperty {
        TeachingTip* owner;
        TeachingTipPreferredPlacementProperty& operator=(BubblePlacement p) { owner->SetPreferredPlacement(p); return *this; }
        operator BubblePlacement() const { return owner->GetPreferredPlacement(); }
        BubblePlacement Get() const { return owner->GetPreferredPlacement(); }
    } PreferredPlacement{this};

    void SetTitle(const std::string& title);'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

# 5. Expander.h: Content, ExpandDirection
def fix_expander_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Expander.h'
    def trans(content):
        if 'struct ExpanderContentProperty' not in content:
            target = '    const std::string& GetHeader() const'
            replacement = '''    struct ExpanderContentProperty {
        Expander* owner;
        ExpanderContentProperty& operator=(std::shared_ptr<UIElement> c) { owner->SetContent(std::move(c)); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner->GetContent(); }
        std::shared_ptr<UIElement> Get() const { return owner->GetContent(); }
    } Content{this};

    struct ExpanderExpandDirectionProperty {
        Expander* owner;
        ExpanderExpandDirectionProperty& operator=(ExpandDirection d) { owner->SetExpandDirection(d); return *this; }
        operator ExpandDirection() const { return owner->GetExpandDirection(); }
        ExpandDirection Get() const { return owner->GetExpandDirection(); }
    } ExpandDirection{this};

    const std::string& GetHeader() const'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

# 6. NumberBox.h: Minimum, Maximum, Step, Value
def fix_number_box_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NumberBox.h'
    def trans(content):
        if 'struct NumberBoxMinimumProperty' not in content:
            target = '    float GetValue() const'
            replacement = '''    struct NumberBoxValueProperty {
        NumberBox* owner;
        NumberBoxValueProperty& operator=(float v) { owner->SetValue(v); return *this; }
        operator float() const { return owner->GetValue(); }
        float Get() const { return owner->GetValue(); }
    } Value{this};

    struct NumberBoxMinimumProperty {
        NumberBox* owner;
        NumberBoxMinimumProperty& operator=(float v) { owner->SetMinimum(v); return *this; }
        operator float() const { return owner->GetMinimum(); }
        float Get() const { return owner->GetMinimum(); }
    } Minimum{this};

    struct NumberBoxMaximumProperty {
        NumberBox* owner;
        NumberBoxMaximumProperty& operator=(float v) { owner->SetMaximum(v); return *this; }
        operator float() const { return owner->GetMaximum(); }
        float Get() const { return owner->GetMaximum(); }
    } Maximum{this};

    struct NumberBoxStepProperty {
        NumberBox* owner;
        NumberBoxStepProperty& operator=(float v) { owner->SetStep(v); return *this; }
        operator float() const { return owner->GetStep(); }
        float Get() const { return owner->GetStep(); }
    } Step{this};

    float GetValue() const'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

# 7. TextBlock.h: TextAlign, LineSpacing
def fix_text_block_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TextBlock.h'
    def trans(content):
        if 'struct TextBlockTextAlignProperty' not in content:
            target = '    TextAlign GetTextAlign() const'
            replacement = '''    struct TextBlockTextAlignProperty {
        TextBlock* owner;
        TextBlockTextAlignProperty& operator=(TextAlign a) { owner->SetTextAlign(a); return *this; }
        operator TextAlign() const { return owner->GetTextAlign(); }
        TextAlign Get() const { return owner->GetTextAlign(); }
    } TextAlign{this};

    struct TextBlockLineSpacingProperty {
        TextBlock* owner;
        TextBlockLineSpacingProperty& operator=(float s) { owner->SetLineSpacing(s); return *this; }
        operator float() const { return owner->GetLineSpacing(); }
        float Get() const { return owner->GetLineSpacing(); }
    } LineSpacing{this};

    TextAlign GetTextAlign() const'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

# 8. PasswordBox.h: Password
def fix_password_box_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\PasswordBox.h'
    def trans(content):
        if 'struct PasswordBoxPasswordProperty' not in content:
            target = '    void SetPassword(const std::string& pwd);'
            replacement = '''    struct PasswordBoxPasswordProperty {
        PasswordBox* owner;
        PasswordBoxPasswordProperty& operator=(const std::string& p) { owner->SetPassword(p); return *this; }
        operator std::string() const { return owner->GetPassword(); }
        std::string Get() const { return owner->GetPassword(); }
    } Password{this};

    void SetPassword(const std::string& pwd);'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

fix_property_h()
fix_shape_h()
fix_canvas_control_h()
fix_teaching_tip_h()
fix_expander_h()
fix_number_box_h()
fix_text_block_h()
fix_password_box_h()
print("Fixed control properties.")
