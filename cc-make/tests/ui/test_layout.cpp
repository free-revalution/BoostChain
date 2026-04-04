#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "ui/layout.hpp"

using namespace ccmake;
using Catch::Matchers::WithinAbs;

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
    REQUIRE_THAT(l1.x, WithinAbs(10, 0.01));
}

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
    REQUIRE_THAT(l1.y, WithinAbs(5, 0.01));
}

TEST_CASE("Flex grow distributes space", "[layout]") {
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
}

TEST_CASE("Padding reduces available space", "[layout]") {
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

TEST_CASE("Justify content center", "[layout]") {
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
