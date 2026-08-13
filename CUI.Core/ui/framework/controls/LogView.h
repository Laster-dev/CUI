#pragma once
#include "Control.h"
#include "ScrollbarAutoHide.h"
#include "ChromiumScrollAnimator.h"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace CUI {

class Button;
class TextBox;

enum class LogLevel : uint8_t {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5
};

class LogView : public Control {
public:
    static constexpr uint32_t kDefaultCapacity = 8192;
    static constexpr uint8_t kAllLevels = 0x3F;

    LogView();
    virtual ~LogView() override;

    virtual const char* GetClassName() const override { return "LogView"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    virtual HCURSOR GetCursor() const override;
    bool AcceptsTabFocus() const override { return true; }

    virtual Size Measure(Size availableSize) override;
    virtual void Arrange(Rect finalRect) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnMouseDblClick(Point pt) override;
    virtual void OnMouseWheel(float delta) override;
    virtual void OnKeyDown(int vkCode) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override;
    virtual void OnThemeChanged() override;

    void Append(LogLevel level, std::string message);
    void Append(LogLevel level, std::string category, std::string message);
    void Clear();

    size_t GetCount() const { return m_size; }
    size_t GetVisibleCount() const;
    uint32_t GetLevelCount(LogLevel level) const;

    void SetExpanded(bool expanded);
    bool IsExpanded() const { return m_expanded; }

    void SetMaxEntries(uint32_t capacity);
    uint32_t GetMaxEntries() const { return m_cap; }

    void SetPersistEnabled(bool enabled);
    bool GetPersistEnabled() const { return m_persistEnabled; }
    void SetPersistPath(std::string path);
    const std::string& GetPersistPath() const { return m_persistPath; }
    void FlushPersist();

    void SetLevelEnabled(LogLevel level, bool enabled);
    bool GetLevelEnabled(LogLevel level) const;
    void SetLevelMask(uint8_t mask);
    uint8_t GetLevelMask() const { return m_levelMask; }

    void SetFilterText(const std::string& text);
    std::string GetFilterText() const;

    void SetFollowTail(bool follow);
    bool GetFollowTail() const { return m_follow; }

    bool CopySelection() const;
    bool CopyVisible() const;
    void SelectAllVisible();
    void ClearSelection();

    Event<LogView*>& OnExpandedChanged() { return m_onExpandedChanged; }

private:
    struct Record {
        uint32_t seq = 0;
        LogLevel level = LogLevel::Info;
        char time[13]{};
        std::string category;
        std::string message;
    };

    enum class Part : uint8_t { None, Header, Chip, Row, Scrollbar };

    void BuildChrome();
    void ApplyChromeVisibility();
    void LayoutChrome();
    void StyleIconButton(Button& btn, const char* svg, const char* tooltip, ThemeTokenId color);
    void SyncActionButtons();
    void SyncChipAnimTargets(bool snap);
    float ExpandProgress() const;
    float ExpandHeight() const;
    void ApplyExpandLayout();
    bool BodyInteractive() const;

    const Record& AtLogical(uint32_t logical) const;
    const Record* RecordBySeq(uint32_t seq) const;
    uint32_t VisibleSeqAt(size_t i) const;
    const Record* VisibleRecord(size_t i) const;
    bool Matches(const Record& rec) const;
    void RebuildVisible();
    void DropOldestIfFull();
    void ScrollToTail();
    void ClampScroll();
    void SetScrollTarget(float y, bool animate);
    void StopSmoothScroll();
    bool AdvanceSmoothScroll();
    void EnsurePersistPath();
    void QueuePersist(const Record& rec);

    Rect HeaderRect() const;
    Rect ToolbarRect() const;
    Rect ChipGroupRect() const;
    Rect ChipRect(int i) const;
    Rect SearchRect() const;
    int ViewRowCount() const;
    Rect ToolBtnRect(int slot) const;
    Rect BodyRect() const;
    Rect ScrollbarTrack() const;
    Rect ScrollbarThumb() const;
    Part HitPart(Point pt, int* index) const;
    int RowIndexFromY(float y) const;
    void SetSelection(int a, int b);
    std::string FormatRecord(const Record& rec) const;
    std::string CollectRows(bool selectedOnly) const;
    void DirtyHeader();
    void DirtyBody();
    D2D1_COLOR_F LevelColor(LogLevel level) const;
    const char* LevelTag(LogLevel level) const;

    void PaintHeader(GraphicsContext& ctx);
    void PaintChips(GraphicsContext& ctx);
    void PaintRows(GraphicsContext& ctx);
    void PaintScrollbar(GraphicsContext& ctx);

    static constexpr float kHeaderH = 32.0f;
    static constexpr float kToolH = 34.0f;
    static constexpr float kRowH = 22.0f;
    static constexpr float kSbW = 8.0f;
    static constexpr float kChipW = 18.0f;
    static constexpr float kChipH = 22.0f;
    static constexpr float kChipGap = 1.0f;
    static constexpr float kTimeCol = 86.0f;
    static constexpr float kLevelCol = 32.0f;
    static constexpr float kCatCol = 40.0f;
    static constexpr float kIconBtn = 26.0f;
    static constexpr float kIconGap = 4.0f;
    static constexpr int kIconCount = 3;

    std::vector<Record> m_buf;
    uint32_t m_cap = kDefaultCapacity;
    uint32_t m_begin = 0;
    uint32_t m_size = 0;
    uint32_t m_nextSeq = 1;
    uint32_t m_firstSeq = 1;
    uint32_t m_levelCounts[6]{};

    std::vector<uint32_t> m_visibleSeq;
    uint32_t m_visibleHead = 0;
    bool m_filterActive = false;
    uint8_t m_levelMask = kAllLevels;
    std::string m_filterFold;

    bool m_expanded = false;
    float m_expandedHeight = 280.0f;
    AnimatedScalar m_expandAnim{ 0.0f };
    AnimatedScalar m_chipAnim[6];
    bool m_follow = true;
    bool m_persistEnabled = false;
    bool m_syncingUi = false;
    std::string m_persistPath;
    std::string m_persistBuf;

    float m_scrollY = 0.0f;
    float m_maxScrollY = 0.0f;
    ChromiumScrollAnimator m_scrollAnimator;
    bool m_dragScrollbar = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScroll = 0.0f;
    ScrollbarAutoHide m_sbHide;

    int m_selA = -1;
    int m_selB = -1;
    int m_hoverRow = -1;
    int m_hoverChip = -1;
    Part m_hoverPart = Part::None;
    bool m_selecting = false;

    std::shared_ptr<TextBox> m_search;
    std::shared_ptr<Button> m_btnCopy;
    std::shared_ptr<Button> m_btnClear;
    std::shared_ptr<Button> m_btnFollow;

    Event<LogView*> m_onExpandedChanged;
};

} // namespace CUI
