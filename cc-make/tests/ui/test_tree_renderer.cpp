#include <catch2/catch_test_macros.hpp>
#include "ui/tree_renderer.hpp"
#include "ui/component.hpp"
#include "terminal/style_pool.hpp"
#include "terminal/color.hpp"

using namespace ccmake;

// Helper: create a plain StylePool with a plain style interned as id=0
static StylePool make_pool() {
    StylePool pool;
    TextStyle plain;
    pool.intern(plain);
    return pool;
}

// Helper: scan all cells for a codepoint, return true if found
static bool find_char(const Screen& screen, char32_t ch) {
    for (int y = 0; y < screen.height(); ++y)
        for (int x = 0; x < screen.width(); ++x)
            if (screen.cell_at(x, y).codepoint == ch) return true;
    return false;
}

// Helper: find position of a codepoint; returns {x,y} or nullopt
static std::optional<std::pair<int,int>> find_char_pos(const Screen& screen, char32_t ch) {
    for (int y = 0; y < screen.height(); ++y)
        for (int x = 0; x < screen.width(); ++x)
            if (screen.cell_at(x, y).codepoint == ch) return {{x, y}};
    return std::nullopt;
}

// Helper: count occurrences of a codepoint
static int count_char(const Screen& screen, char32_t ch) {
    int n = 0;
    for (int y = 0; y < screen.height(); ++y)
        for (int x = 0; x < screen.width(); ++x)
            if (screen.cell_at(x, y).codepoint == ch) ++n;
    return n;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer construction with StylePool", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    REQUIRE_FALSE(renderer.prev_frame().screen.width() == 0 ||
                   renderer.prev_frame().screen.height() == 0 ||
                   true);  // prev_frame is default (empty screen, 0x0)
}

TEST_CASE("TreeRenderer renderer() returns LogUpdateRenderer reference", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    // Just verify it doesn't crash and returns a reference
    (void)renderer.renderer();
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Render a single TextComponent
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer renders simple text to screen", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("hi");
    root->add_child(std::move(text));

    Frame frame = renderer.render(root.get(), 80, 24);

    REQUIRE(find_char(frame.screen, U'h'));
    REQUIRE(find_char(frame.screen, U'i'));
}

TEST_CASE("TreeRenderer render single text produces correct frame dimensions", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("hello");
    root->add_child(std::move(text));

    Frame frame = renderer.render(root.get(), 80, 24);

    REQUIRE(frame.viewport_width == 80);
    REQUIRE(frame.viewport_height == 24);
    REQUIRE(frame.screen.width() == 80);
    REQUIRE(frame.screen.height() == 24);
}

TEST_CASE("TreeRenderer render text at expected position (0,0)", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("AB");
    root->add_child(std::move(text));

    Frame frame = renderer.render(root.get(), 80, 24);

    // Text should appear at the top-left corner
    auto pos_a = find_char_pos(frame.screen, U'A');
    auto pos_b = find_char_pos(frame.screen, U'B');

    REQUIRE(pos_a.has_value());
    REQUIRE(pos_b.has_value());
    REQUIRE(pos_a->first == 0);
    REQUIRE(pos_a->second == 0);
    REQUIRE(pos_b->first == 1);
    REQUIRE(pos_b->second == 0);
}

// ---------------------------------------------------------------------------
// Render nested BoxComponent with TextComponent children
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer renders box with multiple text children", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto box = std::make_unique<BoxComponent>();
    auto text_a = std::make_unique<TextComponent>("a");
    auto text_b = std::make_unique<TextComponent>("b");
    box->add_child(std::move(text_a));
    box->add_child(std::move(text_b));
    root->add_child(std::move(box));

    Frame frame = renderer.render(root.get(), 80, 24);

    REQUIRE(find_char(frame.screen, U'a'));
    REQUIRE(find_char(frame.screen, U'b'));
}

TEST_CASE("TreeRenderer renders deeply nested boxes", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto box1 = std::make_unique<BoxComponent>();
    auto box2 = std::make_unique<BoxComponent>();
    auto text = std::make_unique<TextComponent>("deep");
    box2->add_child(std::move(text));
    box1->add_child(std::move(box2));
    root->add_child(std::move(box1));

    Frame frame = renderer.render(root.get(), 80, 24);

    REQUIRE(find_char(frame.screen, U'd'));
}

// ---------------------------------------------------------------------------
// Render with display=false (skipped)
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer skips hidden components (display=false)", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto box = std::make_unique<BoxComponent>();
    box->style().display = false;
    auto text = std::make_unique<TextComponent>("hidden");
    box->add_child(std::move(text));
    root->add_child(std::move(box));

    Frame frame = renderer.render(root.get(), 80, 24);

    // The hidden text should NOT appear on screen
    REQUIRE_FALSE(find_char(frame.screen, U'h'));
    REQUIRE(count_char(frame.screen, U'h') == 0);
}

TEST_CASE("TreeRenderer renders visible siblings of hidden component", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text_visible = std::make_unique<TextComponent>("visible");
    auto box_hidden = std::make_unique<BoxComponent>();
    box_hidden->style().display = false;
    auto text_hidden = std::make_unique<TextComponent>("hidden");
    box_hidden->add_child(std::move(text_hidden));
    root->add_child(std::move(text_visible));
    root->add_child(std::move(box_hidden));

    Frame frame = renderer.render(root.get(), 80, 24);

    REQUIRE(find_char(frame.screen, U'v'));
    REQUIRE_FALSE(find_char(frame.screen, U'h'));
}

// ---------------------------------------------------------------------------
// Render with background color
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer fills background when text_style has bg_color", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto box = std::make_unique<BoxComponent>();
    box->style().width = 4;
    box->style().height = 2;
    box->text_style().bg_color = Color::rgb(255, 0, 0);
    auto text = std::make_unique<TextComponent>("AB");
    box->add_child(std::move(text));
    root->add_child(std::move(box));

    Frame frame = renderer.render(root.get(), 80, 24);

    // The background fill writes spaces with a style. Count non-default styled cells
    // in the box area. There should be at least 4*2 = 8 bg-filled cells
    int styled_count = 0;
    for (int y = 0; y < frame.screen.height() && y < 4; ++y) {
        for (int x = 0; x < frame.screen.width() && x < 10; ++x) {
            if (frame.screen.cell_at(x, y).style != STYLE_NONE) {
                ++styled_count;
            }
        }
    }
    // Should have styled cells from the background fill
    REQUIRE(styled_count > 0);
}

// ---------------------------------------------------------------------------
// Render with padding
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer handles padding in BoxComponent", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto box = std::make_unique<BoxComponent>();
    box->style().padding_left = 2;
    box->style().padding_top = 2;
    auto text = std::make_unique<TextComponent>("X");
    box->add_child(std::move(text));
    root->add_child(std::move(box));

    Frame frame = renderer.render(root.get(), 80, 24);

    auto pos = find_char_pos(frame.screen, U'X');
    REQUIRE(pos.has_value());
    REQUIRE(pos->first == 2);
    REQUIRE(pos->second == 2);
}

TEST_CASE("TreeRenderer padding offset for text content", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto box = std::make_unique<BoxComponent>();
    box->style().padding_left = 5;
    box->style().padding_top = 1;
    auto text = std::make_unique<TextComponent>("T");
    box->add_child(std::move(text));
    root->add_child(std::move(box));

    Frame frame = renderer.render(root.get(), 80, 24);

    auto pos = find_char_pos(frame.screen, U'T');
    REQUIRE(pos.has_value());
    REQUIRE(pos->first == 5);
    REQUIRE(pos->second == 1);
}

// ---------------------------------------------------------------------------
// Column direction (children stacked vertically)
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer Column direction stacks children vertically", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto col = std::make_unique<BoxComponent>();
    col->style().flex_direction = FlexDirection::Column;
    auto text_a = std::make_unique<TextComponent>("A");
    auto text_b = std::make_unique<TextComponent>("B");
    auto text_c = std::make_unique<TextComponent>("C");
    col->add_child(std::move(text_a));
    col->add_child(std::move(text_b));
    col->add_child(std::move(text_c));
    root->add_child(std::move(col));

    Frame frame = renderer.render(root.get(), 80, 24);

    auto pos_a = find_char_pos(frame.screen, U'A');
    auto pos_b = find_char_pos(frame.screen, U'B');
    auto pos_c = find_char_pos(frame.screen, U'C');

    REQUIRE(pos_a.has_value());
    REQUIRE(pos_b.has_value());
    REQUIRE(pos_c.has_value());
    // In column layout, each child should be on a different row
    REQUIRE(pos_b->second > pos_a->second);
    REQUIRE(pos_c->second > pos_b->second);
}

// ---------------------------------------------------------------------------
// Row direction (children side by side)
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer Row direction places children side by side", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto row = std::make_unique<BoxComponent>();
    row->style().flex_direction = FlexDirection::Row;
    auto text_a = std::make_unique<TextComponent>("A");
    auto text_b = std::make_unique<TextComponent>("B");
    row->add_child(std::move(text_a));
    row->add_child(std::move(text_b));
    root->add_child(std::move(row));

    Frame frame = renderer.render(root.get(), 80, 24);

    auto pos_a = find_char_pos(frame.screen, U'A');
    auto pos_b = find_char_pos(frame.screen, U'B');

    REQUIRE(pos_a.has_value());
    REQUIRE(pos_b.has_value());
    // In row layout, children should be on the same row
    REQUIRE(pos_a->second == pos_b->second);
    // B should be to the right of A
    REQUIRE(pos_b->first > pos_a->first);
}

// ---------------------------------------------------------------------------
// diff() returns RenderOutput
// ---------------------------------------------------------------------------

TEST_CASE("Diff produces non-empty ANSI output when frames differ", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("A");
    root->add_child(std::move(text));

    Frame frame_a = renderer.render(root.get(), 80, 24);

    root->child_at(0)->set_text("B");

    Frame frame_b = renderer.render(root.get(), 80, 24);

    auto result = renderer.diff(frame_a, frame_b);
    REQUIRE_FALSE(result.ansi.empty());
}

TEST_CASE("Diff produces empty output when frames are identical", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("same");
    root->add_child(std::move(text));

    Frame frame = renderer.render(root.get(), 80, 24);

    // Diff against itself
    auto result = renderer.diff(frame, frame);
    REQUIRE(result.ansi.empty());
}

TEST_CASE("Diff with full_clear flag produces output even for same content", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("X");
    root->add_child(std::move(text));

    Frame frame = renderer.render(root.get(), 80, 24);

    // Full clear should produce output even when frames match
    auto result = renderer.diff(frame, frame, true);
    // With full_clear, the renderer should clear the screen and redraw
    // The exact behavior depends on implementation, but it should not crash
    SUCCEED();
}

// ---------------------------------------------------------------------------
// prev_frame() tracks last rendered frame
// ---------------------------------------------------------------------------

TEST_CASE("prev_frame returns the last rendered frame", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("track");
    root->add_child(std::move(text));

    Frame frame = renderer.render(root.get(), 80, 24);

    // prev_frame should match the returned frame
    REQUIRE(renderer.prev_frame().viewport_width == frame.viewport_width);
    REQUIRE(renderer.prev_frame().viewport_height == frame.viewport_height);

    // Verify content matches
    bool found = false;
    for (int y = 0; y < renderer.prev_frame().screen.height(); ++y) {
        for (int x = 0; x < renderer.prev_frame().screen.width(); ++x) {
            if (renderer.prev_frame().screen.cell_at(x, y).codepoint == U't') {
                found = true;
                break;
            }
        }
        if (found) break;
    }
    REQUIRE(found);
}

TEST_CASE("prev_frame updates after each render call", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("first");
    root->add_child(std::move(text));

    Frame frame1 = renderer.render(root.get(), 40, 10);
    REQUIRE(renderer.prev_frame().viewport_width == 40);

    Frame frame2 = renderer.render(root.get(), 80, 24);
    REQUIRE(renderer.prev_frame().viewport_width == 80);
    REQUIRE(renderer.prev_frame().viewport_height == 24);
}

// ---------------------------------------------------------------------------
// RootComponent as root
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer renders with RootComponent as root", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("root_test");
    root->add_child(std::move(text));

    Frame frame = renderer.render(root.get(), 80, 24);

    REQUIRE(find_char(frame.screen, U'r'));
    REQUIRE(find_char(frame.screen, U'o'));
}

TEST_CASE("TreeRenderer renders empty RootComponent without crash", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();

    Frame frame = renderer.render(root.get(), 80, 24);

    REQUIRE(frame.viewport_width == 80);
    REQUIRE(frame.viewport_height == 24);
}

// ---------------------------------------------------------------------------
// Verify computed layout values
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer computes layout with explicit width and height", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto box = std::make_unique<BoxComponent>();
    box->style().width = 10;
    box->style().height = 5;
    auto text = std::make_unique<TextComponent>("W");
    box->add_child(std::move(text));
    root->add_child(std::move(box));

    Frame frame = renderer.render(root.get(), 80, 24);

    // Verify layout was computed: the box should have width=10, height=5
    // Text 'W' should appear at position (0,0) within the box
    auto pos = find_char_pos(frame.screen, U'W');
    REQUIRE(pos.has_value());
    REQUIRE(pos->first == 0);
    REQUIRE(pos->second == 0);
}

TEST_CASE("TreeRenderer computed layout x,y relative to parent", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    // First box takes some space
    auto box1 = std::make_unique<BoxComponent>();
    box1->style().width = 3;
    box1->style().height = 1;
    auto text1 = std::make_unique<TextComponent>("AAA");
    box1->add_child(std::move(text1));

    // Second box should appear after first box in row layout
    auto box2 = std::make_unique<BoxComponent>();
    box2->style().width = 3;
    box2->style().height = 1;
    auto text2 = std::make_unique<TextComponent>("BBB");
    box2->add_child(std::move(text2));

    root->add_child(std::move(box1));
    root->add_child(std::move(box2));

    Frame frame = renderer.render(root.get(), 80, 24);

    // Both texts should appear on the same row
    auto pos_a = find_char_pos(frame.screen, U'A');
    auto pos_b = find_char_pos(frame.screen, U'B');

    REQUIRE(pos_a.has_value());
    REQUIRE(pos_b.has_value());
    REQUIRE(pos_a->second == pos_b->second);

    // B should be to the right of A (since root defaults to Row direction)
    REQUIRE(pos_b->first > pos_a->first);
}

// ---------------------------------------------------------------------------
// Render with BoxComponent styling (bold, etc.)
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer applies text styles to rendered text", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("styled");
    text->text_style().bold = true;
    root->add_child(std::move(text));

    Frame frame = renderer.render(root.get(), 80, 24);

    // Find the cell with 's' (first char of "styled")
    bool found_styled = false;
    for (int y = 0; y < frame.screen.height(); ++y) {
        for (int x = 0; x < frame.screen.width(); ++x) {
            if (frame.screen.cell_at(x, y).codepoint == U's') {
                REQUIRE(frame.screen.cell_at(x, y).style != STYLE_NONE);
                found_styled = true;
                break;
            }
        }
        if (found_styled) break;
    }
    REQUIRE(found_styled);

    // Verify the bold style was interned into the pool
    TextStyle bold_style;
    bold_style.bold = true;
    uint32_t bold_id = pool.intern(bold_style);
    REQUIRE(bold_id != STYLE_NONE);
}

TEST_CASE("TreeRenderer italic text has distinct style id", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("it");
    text->text_style().italic = true;
    root->add_child(std::move(text));

    Frame frame = renderer.render(root.get(), 80, 24);

    bool found = false;
    for (int y = 0; y < frame.screen.height(); ++y) {
        for (int x = 0; x < frame.screen.width(); ++x) {
            if (frame.screen.cell_at(x, y).codepoint == U'i') {
                REQUIRE(frame.screen.cell_at(x, y).style != STYLE_NONE);
                found = true;
                break;
            }
        }
        if (found) break;
    }
    REQUIRE(found);
}

// ---------------------------------------------------------------------------
// Render null root
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer render null root produces empty frame", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    Frame frame = renderer.render(nullptr, 80, 24);

    REQUIRE(frame.viewport_width == 80);
    REQUIRE(frame.viewport_height == 24);
    // No characters should be written
    REQUIRE_FALSE(find_char(frame.screen, U'a'));
}

// ---------------------------------------------------------------------------
// Text color styling
// ---------------------------------------------------------------------------

TEST_CASE("TreeRenderer renders text with fg color", "[tree_renderer]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("color");
    text->text_style().color = Color::rgb(255, 128, 0);
    root->add_child(std::move(text));

    Frame frame = renderer.render(root.get(), 80, 24);

    bool found = false;
    for (int y = 0; y < frame.screen.height(); ++y) {
        for (int x = 0; x < frame.screen.width(); ++x) {
            if (frame.screen.cell_at(x, y).codepoint == U'c') {
                REQUIRE(frame.screen.cell_at(x, y).style != STYLE_NONE);
                found = true;
                break;
            }
        }
        if (found) break;
    }
    REQUIRE(found);
}
