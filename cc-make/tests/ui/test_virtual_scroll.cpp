#include <catch2/catch_test_macros.hpp>
#include "ui/virtual_scroll.hpp"

using namespace ccmake;

TEST_CASE("ScrollViewport clamp") {
    ScrollViewport vp;
    vp.total_lines = 100;
    vp.visible_lines = 24;
    vp.scroll_offset = 200;
    vp.clamp();
    REQUIRE(vp.scroll_offset == 76);
}

TEST_CASE("ScrollViewport clamp at zero") {
    ScrollViewport vp;
    vp.total_lines = 10;
    vp.visible_lines = 24;
    vp.scroll_offset = -5;
    vp.clamp();
    REQUIRE(vp.scroll_offset == 0);
}

TEST_CASE("ScrollViewport at_bottom") {
    ScrollViewport vp;
    vp.total_lines = 50;
    vp.visible_lines = 24;
    vp.scroll_offset = 0;
    REQUIRE(vp.at_bottom());

    vp.scroll_offset = 10;
    REQUIRE_FALSE(vp.at_bottom());
}

TEST_CASE("ScrollViewport visible_range") {
    ScrollViewport vp;
    vp.total_lines = 50;
    vp.visible_lines = 24;
    vp.scroll_offset = 0;
    auto [top, bottom] = vp.visible_range();
    REQUIRE(top == 26);
    REQUIRE(bottom == 49);
}

TEST_CASE("ScrollViewport is_visible") {
    ScrollViewport vp;
    vp.total_lines = 50;
    vp.visible_lines = 24;
    vp.scroll_offset = 0;
    REQUIRE(vp.is_visible(49));
    REQUIRE_FALSE(vp.is_visible(0));
}

TEST_CASE("VirtualScrollManager page_up/page_down") {
    VirtualScrollManager mgr;
    mgr.set_content_height(100);
    mgr.set_viewport_height(24);
    mgr.scroll_to_bottom();
    REQUIRE(mgr.viewport().scroll_offset == 0);

    mgr.page_up();
    REQUIRE(mgr.viewport().scroll_offset > 0);

    mgr.page_down();
    REQUIRE(mgr.viewport().scroll_offset < mgr.viewport().total_lines - 24);
}

TEST_CASE("VirtualScrollManager scroll_to_top/bottom") {
    VirtualScrollManager mgr;
    mgr.set_content_height(100);
    mgr.set_viewport_height(24);

    mgr.scroll_to_top();
    REQUIRE_FALSE(mgr.viewport().at_bottom());

    mgr.scroll_to_bottom();
    REQUIRE(mgr.viewport().at_bottom());
}

TEST_CASE("VirtualScrollManager lines_above/below") {
    VirtualScrollManager mgr;
    mgr.set_content_height(100);
    mgr.set_viewport_height(24);
    mgr.scroll_to_bottom();
    REQUIRE(mgr.viewport().lines_above() == 0);
    REQUIRE(mgr.viewport().lines_below() == 76);

    mgr.page_up();
    REQUIRE(mgr.viewport().lines_above() > 0);
    REQUIRE(mgr.viewport().lines_below() < 76);
}
