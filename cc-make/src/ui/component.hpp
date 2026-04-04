#pragma once
#include "ui/layout.hpp"
#include "terminal/color.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <variant>

namespace ccmake {

// DOM node types
enum class NodeType { Box, Text, Root };

// A node in the component tree
class Component {
public:
    virtual ~Component() = default;

    // Type
    virtual NodeType node_type() const = 0;
    bool is_box() const { return node_type() == NodeType::Box; }
    bool is_text() const { return node_type() == NodeType::Text; }
    bool is_root() const { return node_type() == NodeType::Root; }

    // Tree structure
    Component* parent() const { return parent_; }
    const std::vector<std::unique_ptr<Component>>& children() const { return children_; }
    int child_count() const { return static_cast<int>(children_.size()); }

    void add_child(std::unique_ptr<Component> child);
    void remove_child(Component* child);
    Component* child_at(int index) const;

    // Style
    LayoutStyle& style() { return style_; }
    const LayoutStyle& style() const { return style_; }
    void set_style(const LayoutStyle& s) { style_ = s; }

    // Text styles (for Text and Box components)
    TextStyle& text_style() { return text_style_; }
    const TextStyle& text_style() const { return text_style_; }

    // Layout
    LayoutNode* layout_node() const { return layout_node_.get(); }
    const LayoutRect& computed_layout() const { return layout_node_->layout(); }

    // Lifecycle
    virtual void on_mount() {}
    virtual void on_unmount() {}

    // Dirty tracking
    bool is_dirty() const { return dirty_; }
    void mark_dirty();

    // Text content (for Text components)
    virtual std::string text_content() const { return ""; }
    virtual void set_text(const std::string&) {}

protected:
    Component(NodeType type, std::unique_ptr<LayoutNode> layout_node);
    void set_layout_node(std::unique_ptr<LayoutNode> node) { layout_node_ = std::move(node); }

private:
    NodeType node_type_;
    LayoutStyle style_;
    TextStyle text_style_;
    std::vector<std::unique_ptr<Component>> children_;
    Component* parent_ = nullptr;
    std::unique_ptr<LayoutNode> layout_node_;
    bool dirty_ = true;

    void propagate_dirty();
};

// Root component -- top-level container representing the terminal screen
class RootComponent : public Component {
public:
    RootComponent();
    NodeType node_type() const override { return NodeType::Root; }
};

// Box component -- container with flexbox layout
class BoxComponent : public Component {
public:
    BoxComponent();
    NodeType node_type() const override { return NodeType::Box; }
};

// Text component -- renders text with styles
class TextComponent : public Component {
public:
    explicit TextComponent(const std::string& text = "");
    NodeType node_type() const override { return NodeType::Text; }
    std::string text_content() const override { return text_; }
    void set_text(const std::string& text) override;

private:
    std::string text_;
};

}  // namespace ccmake
