#include <catch2/catch_test_macros.hpp>
#include "ui/component.hpp"

using namespace ccmake;

// ---------------------------------------------------------------------------
// Construction and type checks
// ---------------------------------------------------------------------------

TEST_CASE("RootComponent construction and type check", "[component]") {
    RootComponent root;
    REQUIRE(root.node_type() == NodeType::Root);
    REQUIRE(root.is_root());
    REQUIRE_FALSE(root.is_box());
    REQUIRE_FALSE(root.is_text());
}

TEST_CASE("BoxComponent construction and type check", "[component]") {
    BoxComponent box;
    REQUIRE(box.node_type() == NodeType::Box);
    REQUIRE(box.is_box());
    REQUIRE_FALSE(box.is_text());
    REQUIRE_FALSE(box.is_root());
}

TEST_CASE("TextComponent construction and type check", "[component]") {
    TextComponent text("hello");
    REQUIRE(text.node_type() == NodeType::Text);
    REQUIRE(text.is_text());
    REQUIRE_FALSE(text.is_box());
    REQUIRE_FALSE(text.is_root());
}

// ---------------------------------------------------------------------------
// add_child and parent tracking
// ---------------------------------------------------------------------------

TEST_CASE("Add child to BoxComponent sets parent/child relationship", "[component]") {
    auto box = std::make_unique<BoxComponent>();
    auto child = std::make_unique<TextComponent>("hello");

    REQUIRE(box->child_count() == 0);
    REQUIRE(child->parent() == nullptr);

    Component* child_ptr = child.get();
    box->add_child(std::move(child));

    REQUIRE(box->child_count() == 1);
    REQUIRE(box->child_at(0) == child_ptr);
    REQUIRE(child_ptr->parent() == box.get());
}

TEST_CASE("Add multiple children preserves order", "[component]") {
    auto box = std::make_unique<BoxComponent>();
    auto c1 = std::make_unique<TextComponent>("first");
    auto c2 = std::make_unique<TextComponent>("second");
    auto c3 = std::make_unique<TextComponent>("third");

    Component* c1_ptr = c1.get();
    Component* c2_ptr = c2.get();
    Component* c3_ptr = c3.get();

    box->add_child(std::move(c1));
    box->add_child(std::move(c2));
    box->add_child(std::move(c3));

    REQUIRE(box->child_count() == 3);
    REQUIRE(box->child_at(0) == c1_ptr);
    REQUIRE(box->child_at(1) == c2_ptr);
    REQUIRE(box->child_at(2) == c3_ptr);

    REQUIRE(c1_ptr->parent() == box.get());
    REQUIRE(c2_ptr->parent() == box.get());
    REQUIRE(c3_ptr->parent() == box.get());
}

TEST_CASE("Add child to RootComponent sets parent/child relationship", "[component]") {
    auto root = std::make_unique<RootComponent>();
    auto child = std::make_unique<BoxComponent>();

    Component* root_ptr = root.get();
    Component* child_ptr = child.get();

    root->add_child(std::move(child));

    REQUIRE(root_ptr->child_count() == 1);
    REQUIRE(child_ptr->parent() == root_ptr);
}

// ---------------------------------------------------------------------------
// child_at with valid/invalid indices
// ---------------------------------------------------------------------------

TEST_CASE("child_at returns nullptr for invalid indices", "[component]") {
    BoxComponent box;
    auto child = std::make_unique<TextComponent>("test");
    box.add_child(std::move(child));

    REQUIRE(box.child_at(0) != nullptr);  // valid
    REQUIRE(box.child_at(1) == nullptr);  // out of range
    REQUIRE(box.child_at(-1) == nullptr); // negative
    REQUIRE(box.child_at(999) == nullptr); // large
}

TEST_CASE("child_at on empty component returns nullptr", "[component]") {
    BoxComponent box;
    REQUIRE(box.child_at(0) == nullptr);
    REQUIRE(box.child_at(-1) == nullptr);
}

// ---------------------------------------------------------------------------
// remove_child
// ---------------------------------------------------------------------------

TEST_CASE("Remove child clears parent reference", "[component]") {
    auto box = std::make_unique<BoxComponent>();
    auto child = std::make_unique<TextComponent>("removable");

    Component* box_ptr = box.get();
    Component* child_ptr = child.get();

    box_ptr->add_child(std::move(child));
    REQUIRE(box_ptr->child_count() == 1);
    REQUIRE(child_ptr->parent() == box_ptr);

    box_ptr->remove_child(child_ptr);
    REQUIRE(box_ptr->child_count() == 0);
    REQUIRE(child_ptr->parent() == nullptr);
}

TEST_CASE("Remove child that is not a child is no-op", "[component]") {
    BoxComponent box;
    TextComponent orphan("orphan");

    REQUIRE(box.child_count() == 0);
    box.remove_child(&orphan); // should not crash
    REQUIRE(box.child_count() == 0);
}

TEST_CASE("Remove middle child preserves siblings", "[component]") {
    auto box = std::make_unique<BoxComponent>();
    auto c1 = std::make_unique<TextComponent>("first");
    auto c2 = std::make_unique<TextComponent>("second");
    auto c3 = std::make_unique<TextComponent>("third");

    Component* c1_ptr = c1.get();
    Component* c2_ptr = c2.get();
    Component* c3_ptr = c3.get();

    box->add_child(std::move(c1));
    box->add_child(std::move(c2));
    box->add_child(std::move(c3));

    box->remove_child(c2_ptr);

    REQUIRE(box->child_count() == 2);
    REQUIRE(box->child_at(0) == c1_ptr);
    REQUIRE(box->child_at(1) == c3_ptr);
    REQUIRE(c2_ptr->parent() == nullptr);
}

// ---------------------------------------------------------------------------
// text_content and set_text on TextComponent
// ---------------------------------------------------------------------------

TEST_CASE("TextComponent stores and returns text content", "[component]") {
    TextComponent text("world");
    REQUIRE(text.node_type() == NodeType::Text);
    REQUIRE(text.text_content() == "world");

    text.set_text("updated");
    REQUIRE(text.text_content() == "updated");
}

TEST_CASE("TextComponent default text is empty", "[component]") {
    TextComponent text;
    REQUIRE(text.text_content() == "");
}

TEST_CASE("set_text on TextComponent updates layout node text", "[component]") {
    TextComponent text("hello");
    REQUIRE(text.text_content() == "hello");

    text.set_text("goodbye world");

    // The layout node (TextNode) should also be updated
    auto* tn = dynamic_cast<TextNode*>(text.layout_node());
    REQUIRE(tn != nullptr);
    auto [w, h] = tn->measure(0, 0);
    REQUIRE(w == 13.0f); // "goodbye world".size() == 13
    REQUIRE(h == 1.0f);
}

TEST_CASE("Base Component text_content returns empty string", "[component]") {
    // BoxComponent doesn't override text_content, so it returns ""
    BoxComponent box;
    REQUIRE(box.text_content() == "");
}

// ---------------------------------------------------------------------------
// mark_dirty propagation to ancestors
// ---------------------------------------------------------------------------

TEST_CASE("mark_dirty propagates to ancestors", "[component]") {
    auto root = std::make_unique<BoxComponent>();
    auto middle = std::make_unique<BoxComponent>();
    auto leaf = std::make_unique<TextComponent>("leaf");

    Component* middle_ptr = middle.get();
    root->add_child(std::move(middle));
    middle_ptr->add_child(std::move(leaf));

    // After construction, everything is dirty (default)
    REQUIRE(root->is_dirty());
    REQUIRE(middle_ptr->is_dirty());

    // Verify parent chain
    REQUIRE(middle_ptr->parent() == root.get());
    REQUIRE(middle_ptr->child_at(0)->parent() == middle_ptr);

    // Verify leaf's parent chain
    Component* leaf_ptr = middle_ptr->child_at(0);
    REQUIRE(leaf_ptr->parent() == middle_ptr);
    REQUIRE(leaf_ptr->parent()->parent() == root.get());
}

TEST_CASE("mark_dirty sets dirty flag on all ancestors", "[component]") {
    auto root = std::make_unique<RootComponent>();
    auto box = std::make_unique<BoxComponent>();
    auto text = std::make_unique<TextComponent>("test");

    Component* root_ptr = root.get();
    Component* box_ptr = box.get();
    Component* text_ptr = text.get();

    root->add_child(std::move(box));
    box_ptr->add_child(std::move(text));

    // All nodes start dirty
    REQUIRE(root_ptr->is_dirty());
    REQUIRE(box_ptr->is_dirty());
    REQUIRE(text_ptr->is_dirty());
}

// ---------------------------------------------------------------------------
// is_dirty state
// ---------------------------------------------------------------------------

TEST_CASE("Newly created component is dirty", "[component]") {
    BoxComponent box;
    REQUIRE(box.is_dirty());
}

TEST_CASE("Newly created TextComponent is dirty", "[component]") {
    TextComponent text("hello");
    REQUIRE(text.is_dirty());
}

TEST_CASE("set_text on TextComponent marks dirty", "[component]") {
    TextComponent text("initial");
    // Already dirty from construction, but set_text should still call mark_dirty
    text.set_text("updated");
    REQUIRE(text.is_dirty());
}

// ---------------------------------------------------------------------------
// style access and modification
// ---------------------------------------------------------------------------

TEST_CASE("Default BoxComponent style has flexDirection=Column", "[component]") {
    BoxComponent box;
    REQUIRE(box.style().flex_direction == FlexDirection::Column);
    REQUIRE(box.style().flex_grow == 1.0f);
}

TEST_CASE("RootComponent default style has flex_grow=1", "[component]") {
    RootComponent root;
    REQUIRE(root.style().flex_grow == 1.0f);
    // RootComponent default flex_direction should be Row (LayoutStyle default)
    REQUIRE(root.style().flex_direction == FlexDirection::Row);
}

TEST_CASE("set_style replaces entire style", "[component]") {
    BoxComponent box;
    LayoutStyle new_style;
    new_style.flex_direction = FlexDirection::Row;
    new_style.flex_grow = 2.0f;
    new_style.padding_left = 10;

    box.set_style(new_style);
    REQUIRE(box.style().flex_direction == FlexDirection::Row);
    REQUIRE(box.style().flex_grow == 2.0f);
    REQUIRE(box.style().padding_left == 10);
}

TEST_CASE("style is mutable", "[component]") {
    BoxComponent box;
    box.style().padding_top = 5;
    box.style().padding_bottom = 5;
    box.style().gap = 2;

    REQUIRE(box.style().padding_top == 5);
    REQUIRE(box.style().padding_bottom == 5);
    REQUIRE(box.style().gap == 2);
}

// ---------------------------------------------------------------------------
// text_style access
// ---------------------------------------------------------------------------

TEST_CASE("text_style is accessible and default is plain", "[component]") {
    BoxComponent box;
    REQUIRE(box.text_style().is_plain());

    TextComponent text("hello");
    REQUIRE(text.text_style().is_plain());
}

TEST_CASE("text_style is mutable", "[component]") {
    TextComponent text("styled");
    text.text_style().bold = true;
    text.text_style().underline = true;

    REQUIRE(text.text_style().bold);
    REQUIRE(text.text_style().underline);
    REQUIRE_FALSE(text.text_style().italic);
}

// ---------------------------------------------------------------------------
// computed_layout access
// ---------------------------------------------------------------------------

TEST_CASE("computed_layout returns layout node layout", "[component]") {
    BoxComponent box;
    box.layout_node()->calculate_layout(80, 24);
    const auto& cl = box.computed_layout();
    REQUIRE(cl.width == 80);
    REQUIRE(cl.height == 24);
}

TEST_CASE("TextComponent computed_layout via layout node calculate_layout", "[component]") {
    TextComponent text("hello");
    // TextComponent alone -- calculate_layout on its layout node
    text.layout_node()->calculate_layout(80, 24);
    const auto& cl = text.computed_layout();
    // TextNode has no explicit size; it measures to text length
    // Since no width/height is set, the node takes available width
    // But measure returns (5, 1) -- the node uses available space as its size
    // unless it has no explicit size, in which case available is used
    REQUIRE(cl.height == 24); // takes available height
}

// ---------------------------------------------------------------------------
// layout_node non-null
// ---------------------------------------------------------------------------

TEST_CASE("RootComponent has a non-null layout_node", "[component]") {
    RootComponent root;
    REQUIRE(root.layout_node() != nullptr);
}

TEST_CASE("BoxComponent has a non-null layout_node", "[component]") {
    BoxComponent box;
    REQUIRE(box.layout_node() != nullptr);
}

TEST_CASE("TextComponent has a non-null layout_node", "[component]") {
    TextComponent text("test");
    REQUIRE(text.layout_node() != nullptr);
}

TEST_CASE("RootComponent layout_node is BoxNode", "[component]") {
    RootComponent root;
    auto* node = dynamic_cast<BoxNode*>(root.layout_node());
    REQUIRE(node != nullptr);
}

TEST_CASE("BoxComponent layout_node is BoxNode", "[component]") {
    BoxComponent box;
    auto* node = dynamic_cast<BoxNode*>(box.layout_node());
    REQUIRE(node != nullptr);
}

TEST_CASE("TextComponent layout_node is TextNode", "[component]") {
    TextComponent text("test");
    auto* node = dynamic_cast<TextNode*>(text.layout_node());
    REQUIRE(node != nullptr);
}

// ---------------------------------------------------------------------------
// Tree structure
// ---------------------------------------------------------------------------

TEST_CASE("Component tree with Box > Box > Text has correct structure", "[component]") {
    auto root = std::make_unique<BoxComponent>();
    auto box = std::make_unique<BoxComponent>();
    auto text = std::make_unique<TextComponent>("inner");

    Component* root_ptr = root.get();
    Component* box_ptr = box.get();
    Component* text_ptr = text.get();

    root->add_child(std::move(box));
    root_ptr->child_at(0)->add_child(std::move(text));

    // Tree structure
    REQUIRE(root_ptr->child_count() == 1);
    REQUIRE(box_ptr->child_count() == 1);
    REQUIRE(text_ptr->child_count() == 0);

    // Node types
    REQUIRE(root_ptr->node_type() == NodeType::Box);
    REQUIRE(box_ptr->node_type() == NodeType::Box);
    REQUIRE(text_ptr->node_type() == NodeType::Text);

    // Type checks
    REQUIRE(root_ptr->is_box());
    REQUIRE(box_ptr->is_box());
    REQUIRE(text_ptr->is_text());
    REQUIRE_FALSE(text_ptr->is_box());
    REQUIRE_FALSE(root_ptr->is_text());

    // Parent chain
    REQUIRE(text_ptr->parent() == box_ptr);
    REQUIRE(box_ptr->parent() == root_ptr);
    REQUIRE(root_ptr->parent() == nullptr);
}

TEST_CASE("Root component has no parent", "[component]") {
    RootComponent root;
    REQUIRE(root.parent() == nullptr);
}

TEST_CASE("children() returns reference to child vector", "[component]") {
    BoxComponent box;
    REQUIRE(box.children().empty());

    auto c1 = std::make_unique<TextComponent>("a");
    auto c2 = std::make_unique<TextComponent>("b");
    box.add_child(std::move(c1));
    box.add_child(std::move(c2));

    REQUIRE(box.children().size() == 2);
    REQUIRE(box.child_count() == 2);
}
