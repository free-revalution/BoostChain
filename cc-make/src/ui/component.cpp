#include "ui/component.hpp"
#include <algorithm>

namespace ccmake {

// ---------------------------------------------------------------------------
// Component -- base class
// ---------------------------------------------------------------------------

Component::Component(NodeType type, std::unique_ptr<LayoutNode> layout_node)
    : node_type_(type), layout_node_(std::move(layout_node)) {}

void Component::add_child(std::unique_ptr<Component> child) {
    child->parent_ = this;
    Component* raw = child.get();
    children_.push_back(std::move(child));

    // Sync layout tree: add child's layout node to parent's layout node
    if (layout_node_ && raw->layout_node_) {
        // Copy component style to layout node
        raw->layout_node_->set_style(raw->style_);
        layout_node_->add_child(raw->layout_node_.get());
    }

    propagate_dirty();
}

void Component::remove_child(Component* child) {
    auto it = std::find_if(children_.begin(), children_.end(),
        [child](const std::unique_ptr<Component>& c) { return c.get() == child; });
    if (it != children_.end()) {
        // Sync layout tree: remove child's layout node from parent's layout node
        if (layout_node_ && (*it)->layout_node_) {
            layout_node_->remove_child((*it)->layout_node_.get());
        }

        (*it)->parent_ = nullptr;
        children_.erase(it);
        propagate_dirty();
    }
}

Component* Component::child_at(int index) const {
    if (index < 0 || index >= static_cast<int>(children_.size())) return nullptr;
    return children_[static_cast<size_t>(index)].get();
}

void Component::mark_dirty() {
    dirty_ = true;
    propagate_dirty();
}

void Component::propagate_dirty() {
    Component* p = parent_;
    while (p) {
        p->dirty_ = true;
        p = p->parent_;
    }
}

// ---------------------------------------------------------------------------
// RootComponent -- top-level container
// ---------------------------------------------------------------------------

RootComponent::RootComponent()
    : Component(NodeType::Root, std::make_unique<BoxNode>()) {
    // Root fills available space by default
    style().flex_grow = 1.0f;
}

// ---------------------------------------------------------------------------
// BoxComponent -- flexbox container
// ---------------------------------------------------------------------------

BoxComponent::BoxComponent()
    : Component(NodeType::Box, std::make_unique<BoxNode>()) {
    style().flex_direction = FlexDirection::Column;
    style().flex_grow = 1.0f;
}

// ---------------------------------------------------------------------------
// TextComponent -- text leaf
// ---------------------------------------------------------------------------

TextComponent::TextComponent(const std::string& text)
    : Component(NodeType::Text, std::make_unique<TextNode>(text)),
      text_(text) {}

void TextComponent::set_text(const std::string& text) {
    text_ = text;
    // Update the underlying layout node's text without replacing it,
    // since the parent layout node holds a pointer to this layout node
    if (auto* tn = dynamic_cast<TextNode*>(layout_node())) {
        tn->set_text(text);
    }
    mark_dirty();
}

}  // namespace ccmake
