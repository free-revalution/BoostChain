#pragma once
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <cmath>

namespace ccmake {

// Layout enums
enum class FlexDirection { Row, Column };
enum class AlignItems { Start, Center, End, Stretch };
enum class JustifyContent { Start, Center, End, SpaceBetween, SpaceAround };
enum class FlexWrap { NoWrap, Wrap };
enum class Overflow { Visible, Hidden, Scroll };
enum class PositionType { Relative, Absolute };

// Computed layout result for a node
struct LayoutRect {
    float x = 0, y = 0;       // Position relative to parent
    float width = 0, height = 0;  // Computed size
};

// Layout style properties
struct LayoutStyle {
    // Flex
    FlexDirection flex_direction = FlexDirection::Row;
    float flex_grow = 0.0f;
    float flex_shrink = 1.0f;
    float flex_basis = 0.0f;      // 0 = auto
    FlexWrap flex_wrap = FlexWrap::NoWrap;
    float gap = 0.0f;

    // Alignment
    AlignItems align_items = AlignItems::Start;
    JustifyContent justify_content = JustifyContent::Start;
    std::optional<AlignItems> align_self;  // If set, overrides align_items

    // Size
    std::optional<float> width;
    std::optional<float> height;
    std::optional<float> min_width;
    std::optional<float> max_width;
    std::optional<float> min_height;
    std::optional<float> max_height;

    // Spacing
    float margin_top = 0, margin_right = 0, margin_bottom = 0, margin_left = 0;
    float padding_top = 0, padding_right = 0, padding_bottom = 0, padding_left = 0;

    // Position
    PositionType position = PositionType::Relative;
    std::optional<float> top, bottom, left, right;

    // Display
    bool display = true;  // false = hidden

    // Overflow
    Overflow overflow = Overflow::Visible;
};

// Abstract layout node -- the base class for the component tree
class LayoutNode {
public:
    virtual ~LayoutNode() = default;

    // Tree structure
    virtual int child_count() const = 0;
    virtual LayoutNode* child_at(int index) = 0;
    virtual void add_child(LayoutNode* child) = 0;
    virtual void remove_child(LayoutNode* child) = 0;

    // Style access
    const LayoutStyle& style() const { return style_; }
    LayoutStyle& style() { return style_; }
    void set_style(const LayoutStyle& s) { style_ = s; }

    // Computed layout (after calculate_layout)
    const LayoutRect& layout() const { return layout_; }

    // Parent
    LayoutNode* parent() const { return parent_; }
    void set_parent(LayoutNode* p) { parent_ = p; }

    // Layout computation
    void calculate_layout(float available_width, float available_height);

    // Measure function override for text nodes
    virtual std::pair<float, float> measure(float width, float height) {
        return {0, 0};  // Default: no intrinsic size
    }

private:
    LayoutStyle style_;
    LayoutRect layout_;
    LayoutNode* parent_ = nullptr;

    void compute_layout(float available_width, float available_height);
    void layout_flex(float available_width, float available_height);
    void layout_absolute();
};

// Convenience: create a simple container node
class BoxNode : public LayoutNode {
public:
    int child_count() const override;
    LayoutNode* child_at(int index) override;
    void add_child(LayoutNode* child) override;
    void remove_child(LayoutNode* child) override;

private:
    std::vector<LayoutNode*> children_;
};

// A text leaf node with intrinsic size
class TextNode : public LayoutNode {
public:
    explicit TextNode(const std::string& text) : text_(text) {}
    std::pair<float, float> measure(float width, float height) override;

private:
    std::string text_;
};

}  // namespace ccmake
