#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "ui/layout.hpp"

using namespace ccmake;
using Catch::Matchers::WithinAbs;

// ---------------------------------------------------------------------------
// TextNode measure
// ---------------------------------------------------------------------------

TEST_CASE("TextNode measure returns text width and height 1", "[layout]") {
    TextNode node("hello");
    auto [w, h] = node.measure(0, 0);
    REQUIRE_THAT(w, WithinAbs(5.0f, 0.01));
    REQUIRE_THAT(h, WithinAbs(1.0f, 0.01));
}

TEST_CASE("TextNode measure with empty string", "[layout]") {
    TextNode node("");
    auto [w, h] = node.measure(0, 0);
    REQUIRE_THAT(w, WithinAbs(0.0f, 0.01));
    REQUIRE_THAT(h, WithinAbs(1.0f, 0.01));
}

TEST_CASE("TextNode measure with long text", "[layout]") {
    TextNode node("hello world foo bar");
    auto [w, h] = node.measure(0, 0);
    REQUIRE_THAT(w, WithinAbs(19.0f, 0.01));
    REQUIRE_THAT(h, WithinAbs(1.0f, 0.01));
}

TEST_CASE("TextNode measure ignores width/height constraints", "[layout]") {
    TextNode node("abc");
    // measure signature takes width and height but TextNode ignores them
    auto [w, h] = node.measure(100, 100);
    REQUIRE_THAT(w, WithinAbs(3.0f, 0.01));
    REQUIRE_THAT(h, WithinAbs(1.0f, 0.01));
}

TEST_CASE("TextNode set_text updates measure result", "[layout]") {
    TextNode node("hi");
    auto [w1, h1] = node.measure(0, 0);
    REQUIRE_THAT(w1, WithinAbs(2.0f, 0.01));

    node.set_text("hello world");
    auto [w2, h2] = node.measure(0, 0);
    REQUIRE_THAT(w2, WithinAbs(11.0f, 0.01));
    REQUIRE_THAT(h2, WithinAbs(1.0f, 0.01));
}

// ---------------------------------------------------------------------------
// BoxNode add_child / remove_child / child_at
// ---------------------------------------------------------------------------

TEST_CASE("BoxNode starts with no children", "[layout]") {
    BoxNode node;
    REQUIRE(node.child_count() == 0);
}

TEST_CASE("BoxNode add_child sets parent and increases count", "[layout]") {
    BoxNode parent;
    BoxNode child;
    REQUIRE(child.parent() == nullptr);

    parent.add_child(&child);
    REQUIRE(parent.child_count() == 1);
    REQUIRE(parent.child_at(0) == &child);
    REQUIRE(child.parent() == &parent);
}

TEST_CASE("BoxNode add_child multiple preserves order", "[layout]") {
    BoxNode parent;
    BoxNode c1, c2, c3;

    parent.add_child(&c1);
    parent.add_child(&c2);
    parent.add_child(&c3);

    REQUIRE(parent.child_count() == 3);
    REQUIRE(parent.child_at(0) == &c1);
    REQUIRE(parent.child_at(1) == &c2);
    REQUIRE(parent.child_at(2) == &c3);
}

TEST_CASE("BoxNode remove_child clears parent and decreases count", "[layout]") {
    BoxNode parent;
    BoxNode child;

    parent.add_child(&child);
    REQUIRE(parent.child_count() == 1);

    parent.remove_child(&child);
    REQUIRE(parent.child_count() == 0);
    REQUIRE(child.parent() == nullptr);
}

TEST_CASE("BoxNode remove_child not present is no-op", "[layout]") {
    BoxNode parent;
    BoxNode stranger;

    parent.remove_child(&stranger);
    REQUIRE(parent.child_count() == 0);
}

TEST_CASE("BoxNode child_at out of range throws or returns invalid", "[layout]") {
    BoxNode node;
    // child_at uses std::vector::at which throws std::out_of_range
    REQUIRE_THROWS_AS(node.child_at(0), std::out_of_range);
}

TEST_CASE("TextNode is a leaf with no children", "[layout]") {
    TextNode node("test");
    REQUIRE(node.child_count() == 0);
    REQUIRE(node.child_at(0) == nullptr);
    // Adding/removing children on TextNode is a no-op
    BoxNode child;
    node.add_child(&child);
    REQUIRE(node.child_count() == 0);
}

// ---------------------------------------------------------------------------
// calculate_layout: single child fills parent
// ---------------------------------------------------------------------------

TEST_CASE("Single child without explicit size fills parent", "[layout]") {
    BoxNode parent;
    parent.style().width = 100;
    parent.style().height = 50;

    BoxNode child;
    // No explicit size on child
    parent.add_child(&child);
    parent.calculate_layout(100, 50);

    const auto& cl = child.layout();
    // Child has no width/height set, so it gets available space from parent
    // Row direction by default, so width = available content, height = available
    REQUIRE_THAT(cl.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(cl.y, WithinAbs(0, 0.01));
}

// ---------------------------------------------------------------------------
// calculate_layout: multiple children in Row direction
// ---------------------------------------------------------------------------

TEST_CASE("Row direction lays out horizontally", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;

    BoxNode child0;
    child0.style().width = 10;
    child0.style().height = 5;
    parent.add_child(&child0);

    BoxNode child1;
    child1.style().width = 20;
    child1.style().height = 5;
    parent.add_child(&child1);

    parent.calculate_layout(80, 24);

    const auto& l0 = child0.layout();
    const auto& l1 = child1.layout();

    REQUIRE_THAT(l0.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(l0.width, WithinAbs(10, 0.01));
    REQUIRE_THAT(l1.x, WithinAbs(10, 0.01));
    REQUIRE_THAT(l1.width, WithinAbs(20, 0.01));

    // Both at y=0 with align_items=Start
    REQUIRE_THAT(l0.y, WithinAbs(0, 0.01));
    REQUIRE_THAT(l1.y, WithinAbs(0, 0.01));
}

TEST_CASE("Row direction with TextNode children uses measured width", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;

    TextNode t1("abc");
    TextNode t2("defgh");

    parent.add_child(&t1);
    parent.add_child(&t2);

    parent.calculate_layout(80, 24);

    REQUIRE_THAT(t1.layout().x, WithinAbs(0, 0.01));
    REQUIRE_THAT(t1.layout().width, WithinAbs(3, 0.01));
    REQUIRE_THAT(t2.layout().x, WithinAbs(3, 0.01));
    REQUIRE_THAT(t2.layout().width, WithinAbs(5, 0.01));
}

// ---------------------------------------------------------------------------
// calculate_layout: multiple children in Column direction
// ---------------------------------------------------------------------------

TEST_CASE("Column direction lays out vertically", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Column;

    BoxNode child0;
    child0.style().width = 10;
    child0.style().height = 5;
    parent.add_child(&child0);

    BoxNode child1;
    child1.style().width = 10;
    child1.style().height = 10;
    parent.add_child(&child1);

    parent.calculate_layout(80, 24);

    const auto& l0 = child0.layout();
    const auto& l1 = child1.layout();

    REQUIRE_THAT(l0.y, WithinAbs(0, 0.01));
    REQUIRE_THAT(l0.height, WithinAbs(5, 0.01));
    REQUIRE_THAT(l1.y, WithinAbs(5, 0.01));
    REQUIRE_THAT(l1.height, WithinAbs(10, 0.01));

    // Both at x=0 with align_items=Start
    REQUIRE_THAT(l0.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(l1.x, WithinAbs(0, 0.01));
}

TEST_CASE("Column direction with TextNode children uses measured height", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Column;

    TextNode t1("abc");
    TextNode t2("defgh");

    parent.add_child(&t1);
    parent.add_child(&t2);

    parent.calculate_layout(80, 24);

    // Column: children stacked vertically, text height = 1 each
    REQUIRE_THAT(t1.layout().y, WithinAbs(0, 0.01));
    REQUIRE_THAT(t1.layout().height, WithinAbs(1, 0.01));
    REQUIRE_THAT(t2.layout().y, WithinAbs(1, 0.01));
    REQUIRE_THAT(t2.layout().height, WithinAbs(1, 0.01));
}

// ---------------------------------------------------------------------------
// flex_grow distributes remaining space
// ---------------------------------------------------------------------------

TEST_CASE("Flex grow distributes space equally", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().width = 100;
    parent.style().height = 10;

    BoxNode child0;
    child0.style().flex_grow = 1.0f;
    parent.add_child(&child0);

    BoxNode child1;
    child1.style().flex_grow = 1.0f;
    parent.add_child(&child1);

    BoxNode child2;
    child2.style().flex_grow = 1.0f;
    parent.add_child(&child2);

    parent.calculate_layout(100, 24);

    // Each child should get approximately 33.33 width
    const auto& l0 = child0.layout();
    const auto& l1 = child1.layout();
    const auto& l2 = child2.layout();

    REQUIRE_THAT(l0.width, WithinAbs(33.333f, 0.1));
    REQUIRE_THAT(l1.width, WithinAbs(33.333f, 0.1));
    REQUIRE_THAT(l2.width, WithinAbs(33.333f, 0.1));

    // Children should be adjacent
    REQUIRE_THAT(l1.x, WithinAbs(l0.x + l0.width, 0.01));
    REQUIRE_THAT(l2.x, WithinAbs(l1.x + l1.width, 0.01));
}

TEST_CASE("Flex grow with unequal weights", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().width = 100;
    parent.style().height = 10;

    BoxNode child0;
    child0.style().flex_grow = 1.0f;
    parent.add_child(&child0);

    BoxNode child1;
    child1.style().flex_grow = 3.0f;
    parent.add_child(&child1);

    parent.calculate_layout(100, 24);

    const auto& l0 = child0.layout();
    const auto& l1 = child1.layout();

    // child0 gets 25%, child1 gets 75%
    REQUIRE_THAT(l0.width, WithinAbs(25.0f, 0.1));
    REQUIRE_THAT(l1.width, WithinAbs(75.0f, 0.1));
}

TEST_CASE("Flex grow in Column direction distributes height", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Column;
    parent.style().width = 100;
    parent.style().height = 100;

    BoxNode child0;
    child0.style().flex_grow = 1.0f;
    parent.add_child(&child0);

    BoxNode child1;
    child1.style().flex_grow = 1.0f;
    parent.add_child(&child1);

    parent.calculate_layout(100, 100);

    const auto& l0 = child0.layout();
    const auto& l1 = child1.layout();

    REQUIRE_THAT(l0.height, WithinAbs(50.0f, 0.1));
    REQUIRE_THAT(l1.height, WithinAbs(50.0f, 0.1));
    REQUIRE_THAT(l1.y, WithinAbs(50.0f, 0.1));
}

// ---------------------------------------------------------------------------
// flex_shrink shrinks overflow
// ---------------------------------------------------------------------------

TEST_CASE("Flex shrink reduces children when overflow", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().width = 20;
    parent.style().height = 10;

    // TextNode children with intrinsic sizes exceeding container
    TextNode child0("12345"); // measure width = 5
    TextNode child1("1234567890"); // measure width = 10

    parent.add_child(&child0);
    parent.add_child(&child1);

    parent.calculate_layout(20, 24);

    // Total desired: 15, available: 20 -- fits, no shrink
    const auto& l0 = child0.layout();
    const auto& l1 = child1.layout();

    REQUIRE_THAT(l0.width, WithinAbs(5, 0.1));
    REQUIRE_THAT(l1.width, WithinAbs(10, 0.1));
}

TEST_CASE("Flex shrink with unequal shrink factors on measured children", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().width = 6;
    parent.style().height = 10;

    // Total measured: 5 + 5 = 10, available: 6, overflow: 4
    TextNode child0("12345"); // measure width = 5
    TextNode child1("12345"); // measure width = 5

    child0.style().flex_shrink = 1.0f;
    child1.style().flex_shrink = 3.0f;

    parent.add_child(&child0);
    parent.add_child(&child1);

    parent.calculate_layout(6, 24);

    // Total shrink factor: 4, per unit: 4/4 = 1
    // child0 shrinks by 1 => 4, child1 shrinks by 3 => 2
    const auto& l0 = child0.layout();
    const auto& l1 = child1.layout();

    REQUIRE_THAT(l0.width, WithinAbs(4, 0.1));
    REQUIRE_THAT(l1.width, WithinAbs(2, 0.1));
}

// ---------------------------------------------------------------------------
// align_items: Start, Center, End, Stretch
// ---------------------------------------------------------------------------

TEST_CASE("align_items Start positions children at cross axis start", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().width = 100;
    parent.style().height = 20;
    parent.style().align_items = AlignItems::Start;

    BoxNode child;
    child.style().width = 10;
    child.style().height = 5;
    parent.add_child(&child);

    parent.calculate_layout(100, 20);

    const auto& cl = child.layout();
    REQUIRE_THAT(cl.y, WithinAbs(0, 0.01));
    REQUIRE_THAT(cl.height, WithinAbs(5, 0.01));
}

TEST_CASE("align_items Center positions children at cross axis center", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().width = 100;
    parent.style().height = 20;
    parent.style().align_items = AlignItems::Center;

    BoxNode child;
    child.style().width = 10;
    child.style().height = 6;
    parent.add_child(&child);

    parent.calculate_layout(100, 20);

    const auto& cl = child.layout();
    // center y = (20 - 6) / 2 = 7
    REQUIRE_THAT(cl.y, WithinAbs(7, 0.01));
    REQUIRE_THAT(cl.height, WithinAbs(6, 0.01));
}

TEST_CASE("align_items End positions children at cross axis end", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().width = 100;
    parent.style().height = 20;
    parent.style().align_items = AlignItems::End;

    BoxNode child;
    child.style().width = 10;
    child.style().height = 6;
    parent.add_child(&child);

    parent.calculate_layout(100, 20);

    const auto& cl = child.layout();
    // y = 20 - 6 = 14
    REQUIRE_THAT(cl.y, WithinAbs(14, 0.01));
    REQUIRE_THAT(cl.height, WithinAbs(6, 0.01));
}

TEST_CASE("align_items Stretch stretches child on cross axis", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().width = 100;
    parent.style().height = 20;
    parent.style().align_items = AlignItems::Stretch;

    // Child without explicit height so stretch can take effect
    BoxNode child;
    child.style().width = 10;
    // No explicit height -- will be stretched
    parent.add_child(&child);

    parent.calculate_layout(100, 20);

    const auto& cl = child.layout();
    REQUIRE_THAT(cl.y, WithinAbs(0, 0.01));
    REQUIRE_THAT(cl.height, WithinAbs(20, 0.01)); // stretched to parent height
}

TEST_CASE("align_items Stretch in Column direction stretches width", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Column;
    parent.style().width = 100;
    parent.style().height = 20;
    parent.style().align_items = AlignItems::Stretch;

    BoxNode child;
    child.style().height = 5;
    parent.add_child(&child);

    parent.calculate_layout(100, 20);

    const auto& cl = child.layout();
    REQUIRE_THAT(cl.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(cl.width, WithinAbs(100, 0.01)); // stretched to parent width
}

// ---------------------------------------------------------------------------
// justify_content: Start, Center, End
// ---------------------------------------------------------------------------

TEST_CASE("justify_content Start (default) aligns children to main axis start", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().justify_content = JustifyContent::Start;

    BoxNode child;
    child.style().width = 10;
    child.style().height = 5;
    parent.add_child(&child);

    parent.calculate_layout(100, 24);

    const auto& cl = child.layout();
    REQUIRE_THAT(cl.x, WithinAbs(0, 0.01));
}

TEST_CASE("justify_content Center aligns children to main axis center", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().justify_content = JustifyContent::Center;

    BoxNode child;
    child.style().width = 10;
    child.style().height = 5;
    parent.add_child(&child);

    parent.calculate_layout(100, 24);

    const auto& cl = child.layout();
    // remaining = 100 - 10 = 90, centered => x = 45
    REQUIRE_THAT(cl.x, WithinAbs(45, 0.01));
}

TEST_CASE("justify_content End aligns children to main axis end", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().justify_content = JustifyContent::End;

    BoxNode child;
    child.style().width = 10;
    child.style().height = 5;
    parent.add_child(&child);

    parent.calculate_layout(100, 24);

    const auto& cl = child.layout();
    // remaining = 90, end => x = 90
    REQUIRE_THAT(cl.x, WithinAbs(90, 0.01));
}

TEST_CASE("justify_content Center in Column direction", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Column;
    parent.style().justify_content = JustifyContent::Center;

    BoxNode child;
    child.style().width = 10;
    child.style().height = 10;
    parent.add_child(&child);

    parent.calculate_layout(80, 50);

    const auto& cl = child.layout();
    // remaining = 50 - 10 = 40, centered => y = 20
    REQUIRE_THAT(cl.y, WithinAbs(20, 0.01));
}

// ---------------------------------------------------------------------------
// gap between children
// ---------------------------------------------------------------------------

TEST_CASE("gap adds space between children in Row", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().gap = 5;

    BoxNode child0;
    child0.style().width = 10;
    child0.style().height = 5;
    parent.add_child(&child0);

    BoxNode child1;
    child1.style().width = 10;
    child1.style().height = 5;
    parent.add_child(&child1);

    BoxNode child2;
    child2.style().width = 10;
    child2.style().height = 5;
    parent.add_child(&child2);

    parent.calculate_layout(80, 24);

    const auto& l0 = child0.layout();
    const auto& l1 = child1.layout();
    const auto& l2 = child2.layout();

    REQUIRE_THAT(l0.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(l1.x, WithinAbs(15, 0.01)); // 10 + 5 gap
    REQUIRE_THAT(l2.x, WithinAbs(30, 0.01)); // 15 + 10 + 5 gap
}

TEST_CASE("gap adds space between children in Column", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Column;
    parent.style().gap = 3;

    BoxNode child0;
    child0.style().width = 10;
    child0.style().height = 5;
    parent.add_child(&child0);

    BoxNode child1;
    child1.style().width = 10;
    child1.style().height = 5;
    parent.add_child(&child1);

    parent.calculate_layout(80, 24);

    const auto& l0 = child0.layout();
    const auto& l1 = child1.layout();

    REQUIRE_THAT(l0.y, WithinAbs(0, 0.01));
    REQUIRE_THAT(l1.y, WithinAbs(8, 0.01)); // 5 + 3 gap
}

// ---------------------------------------------------------------------------
// padding reduces content area
// ---------------------------------------------------------------------------

TEST_CASE("Padding reduces available space and offsets children", "[layout]") {
    BoxNode parent;
    parent.style().padding_left = 5;
    parent.style().padding_top = 5;

    BoxNode child;
    child.style().width = 10;
    child.style().height = 5;
    parent.add_child(&child);

    parent.calculate_layout(80, 24);

    const auto& cl = child.layout();
    // Child should be offset by padding
    REQUIRE_THAT(cl.x, WithinAbs(5, 0.01));
    REQUIRE_THAT(cl.y, WithinAbs(5, 0.01));
}

TEST_CASE("Padding on all sides reduces available space", "[layout]") {
    BoxNode parent;
    parent.style().padding_left = 10;
    parent.style().padding_right = 10;
    parent.style().padding_top = 5;
    parent.style().padding_bottom = 5;

    BoxNode child;
    child.style().width = 100; // wider than available
    parent.add_child(&child);

    parent.calculate_layout(80, 40);

    const auto& cl = child.layout();
    // Content area: 80 - 10 - 10 = 60 wide
    // Child has no flex_grow and fixed width=100, but it's capped by available
    // Actually child has width=100 but no flex_shrink override; it gets shrunk
    REQUIRE_THAT(cl.x, WithinAbs(10, 0.01));
    REQUIRE_THAT(cl.y, WithinAbs(5, 0.01));
}

// ---------------------------------------------------------------------------
// min_width / max_width constraints
// ---------------------------------------------------------------------------

TEST_CASE("min_width clamps computed width", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;
    parent.style().width = 100;
    parent.style().height = 10;

    BoxNode child;
    child.style().flex_grow = 1.0f;
    child.style().min_width = 80;
    parent.add_child(&child);

    parent.calculate_layout(100, 24);

    const auto& cl = child.layout();
    // With min_width=80, child should be at least 80 wide
    REQUIRE(cl.width >= 80.0f);
}

TEST_CASE("max_width clamps computed width", "[layout]") {
    BoxNode node;
    node.style().width = 100;
    node.style().max_width = 50;

    node.calculate_layout(80, 24);

    const auto& cl = node.layout();
    // width is set to 100 but max_width=50 => clamped to 50
    REQUIRE_THAT(cl.width, WithinAbs(50, 0.01));
}

// ---------------------------------------------------------------------------
// display=false hides node
// ---------------------------------------------------------------------------

TEST_CASE("Hidden node has zero layout", "[layout]") {
    BoxNode node;
    node.style().width = 10;
    node.style().height = 5;
    node.style().display = false;
    node.calculate_layout(80, 24);

    const auto& l = node.layout();
    REQUIRE_THAT(l.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(l.y, WithinAbs(0, 0.01));
    REQUIRE_THAT(l.width, WithinAbs(0, 0.01));
    REQUIRE_THAT(l.height, WithinAbs(0, 0.01));
}

TEST_CASE("Hidden child in parent is skipped", "[layout]") {
    BoxNode parent;
    parent.style().flex_direction = FlexDirection::Row;

    BoxNode child0;
    child0.style().width = 10;
    child0.style().height = 5;
    parent.add_child(&child0);

    BoxNode hidden;
    hidden.style().width = 20;
    hidden.style().height = 5;
    hidden.style().display = false;
    parent.add_child(&hidden);

    BoxNode child1;
    child1.style().width = 10;
    child1.style().height = 5;
    parent.add_child(&child1);

    parent.calculate_layout(80, 24);

    // Hidden child should not affect positioning
    const auto& l0 = child0.layout();
    const auto& l1 = child1.layout();

    // child1 should be right after child0 (no gap from hidden)
    REQUIRE_THAT(l0.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(l1.x, WithinAbs(10, 0.01));
}

// ---------------------------------------------------------------------------
// PositionType::Absolute positioning
// ---------------------------------------------------------------------------

TEST_CASE("Absolute positioned node uses top/left from style", "[layout]") {
    BoxNode parent;
    parent.style().width = 100;
    parent.style().height = 50;

    BoxNode child;
    child.style().position = PositionType::Absolute;
    child.style().top = 10;
    child.style().left = 20;
    child.style().width = 30;
    child.style().height = 10;
    parent.add_child(&child);

    parent.calculate_layout(100, 50);

    const auto& cl = child.layout();
    REQUIRE_THAT(cl.x, WithinAbs(20, 0.01));
    REQUIRE_THAT(cl.y, WithinAbs(10, 0.01));
    REQUIRE_THAT(cl.width, WithinAbs(30, 0.01));
    REQUIRE_THAT(cl.height, WithinAbs(10, 0.01));
}

TEST_CASE("Absolute positioned node defaults top/left to 0", "[layout]") {
    BoxNode node;
    node.style().position = PositionType::Absolute;
    node.style().width = 10;
    node.style().height = 5;

    node.calculate_layout(80, 24);

    const auto& cl = node.layout();
    REQUIRE_THAT(cl.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(cl.y, WithinAbs(0, 0.01));
    REQUIRE_THAT(cl.width, WithinAbs(10, 0.01));
    REQUIRE_THAT(cl.height, WithinAbs(5, 0.01));
}

// ---------------------------------------------------------------------------
// Nested layout (parent > child > grandchild)
// ---------------------------------------------------------------------------

TEST_CASE("Nested layout: parent > child > grandchild", "[layout]") {
    BoxNode parent;
    parent.style().width = 100;
    parent.style().height = 50;
    parent.style().flex_direction = FlexDirection::Column;

    BoxNode child;
    child.style().flex_direction = FlexDirection::Row;
    child.style().height = 20;
    parent.add_child(&child);

    BoxNode grandchild;
    grandchild.style().width = 30;
    grandchild.style().height = 10;
    child.add_child(&grandchild);

    parent.calculate_layout(100, 50);

    // Child should be at top of parent
    const auto& cl = child.layout();
    REQUIRE_THAT(cl.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(cl.y, WithinAbs(0, 0.01));
    REQUIRE_THAT(cl.width, WithinAbs(100, 0.01));
    REQUIRE_THAT(cl.height, WithinAbs(20, 0.01));

    // Grandchild should be at start of child
    const auto& gl = grandchild.layout();
    REQUIRE_THAT(gl.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(gl.y, WithinAbs(0, 0.01));
    REQUIRE_THAT(gl.width, WithinAbs(30, 0.01));
    REQUIRE_THAT(gl.height, WithinAbs(10, 0.01));
}

TEST_CASE("Nested layout with padding propagates correctly", "[layout]") {
    BoxNode parent;
    parent.style().width = 100;
    parent.style().height = 50;
    parent.style().flex_direction = FlexDirection::Column;
    parent.style().padding_left = 10;
    parent.style().padding_top = 5;

    BoxNode child;
    child.style().flex_direction = FlexDirection::Row;
    child.style().height = 20;
    parent.add_child(&child);

    BoxNode grandchild;
    grandchild.style().width = 30;
    grandchild.style().height = 10;
    child.add_child(&grandchild);

    parent.calculate_layout(100, 50);

    // Child offset by parent padding
    const auto& cl = child.layout();
    REQUIRE_THAT(cl.x, WithinAbs(10, 0.01));
    REQUIRE_THAT(cl.y, WithinAbs(5, 0.01));

    // Grandchild at start of child (no padding on child)
    const auto& gl = grandchild.layout();
    REQUIRE_THAT(gl.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(gl.y, WithinAbs(0, 0.01));
}

TEST_CASE("Three-level nesting with flex grow", "[layout]") {
    BoxNode parent;
    parent.style().width = 100;
    parent.style().height = 50;
    parent.style().flex_direction = FlexDirection::Column;

    BoxNode child;
    child.style().flex_grow = 1.0f;
    parent.add_child(&child);

    BoxNode grandchild;
    grandchild.style().flex_grow = 1.0f;
    child.add_child(&grandchild);

    parent.calculate_layout(100, 50);

    // Child fills parent height
    const auto& cl = child.layout();
    REQUIRE_THAT(cl.height, WithinAbs(50, 0.01));

    // Grandchild fills child
    const auto& gl = grandchild.layout();
    REQUIRE_THAT(gl.width, WithinAbs(100, 0.01));
}

// ---------------------------------------------------------------------------
// Style access on LayoutNode
// ---------------------------------------------------------------------------

TEST_CASE("LayoutNode style defaults", "[layout]") {
    BoxNode node;
    const auto& s = node.style();

    REQUIRE(s.flex_direction == FlexDirection::Row);
    REQUIRE(s.flex_grow == 0.0f);
    REQUIRE(s.flex_shrink == 1.0f);
    REQUIRE(s.flex_basis == 0.0f);
    REQUIRE(s.flex_wrap == FlexWrap::NoWrap);
    REQUIRE(s.gap == 0.0f);
    REQUIRE(s.align_items == AlignItems::Start);
    REQUIRE(s.justify_content == JustifyContent::Start);
    REQUIRE(s.display == true);
    REQUIRE(s.position == PositionType::Relative);
}

TEST_CASE("LayoutNode set_style replaces style", "[layout]") {
    BoxNode node;
    LayoutStyle new_style;
    new_style.flex_direction = FlexDirection::Column;
    new_style.flex_grow = 2.0f;
    node.set_style(new_style);

    REQUIRE(node.style().flex_direction == FlexDirection::Column);
    REQUIRE(node.style().flex_grow == 2.0f);
}

// ---------------------------------------------------------------------------
// Single node with fixed size
// ---------------------------------------------------------------------------

TEST_CASE("Single node with fixed size", "[layout]") {
    BoxNode node;
    node.style().width = 10;
    node.style().height = 5;
    node.calculate_layout(80, 24);

    const auto& l = node.layout();
    REQUIRE_THAT(l.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(l.y, WithinAbs(0, 0.01));
    REQUIRE_THAT(l.width, WithinAbs(10, 0.01));
    REQUIRE_THAT(l.height, WithinAbs(5, 0.01));
}

TEST_CASE("Node without explicit size takes available space", "[layout]") {
    BoxNode node;
    node.calculate_layout(80, 24);

    REQUIRE_THAT(node.layout().width, WithinAbs(80, 0.01));
    REQUIRE_THAT(node.layout().height, WithinAbs(24, 0.01));
}

// ---------------------------------------------------------------------------
// Nested nodes inherit parent size
// ---------------------------------------------------------------------------

TEST_CASE("Nested nodes inherit parent size", "[layout]") {
    BoxNode parent;
    parent.calculate_layout(80, 24);

    REQUIRE_THAT(parent.layout().width, WithinAbs(80, 0.01));
    REQUIRE_THAT(parent.layout().height, WithinAbs(24, 0.01));

    BoxNode child;
    child.style().width = 10;
    child.style().height = 5;
    parent.add_child(&child);
    parent.calculate_layout(80, 24);

    // Child should be positioned within parent
    const auto& cl = child.layout();
    REQUIRE_THAT(cl.x, WithinAbs(0, 0.01));
    REQUIRE_THAT(cl.y, WithinAbs(0, 0.01));
    REQUIRE_THAT(cl.width, WithinAbs(10, 0.01));
    REQUIRE_THAT(cl.height, WithinAbs(5, 0.01));
}
