#include <catch2/catch_test_macros.hpp>
#include "ui/output.hpp"
#include "ui/component.hpp"
#include "ui/tree_renderer.hpp"
#include "terminal/style_pool.hpp"

using namespace ccmake;

// Helper: create a plain StylePool with a plain style interned as id=0
static StylePool make_pool() {
    StylePool pool;
    TextStyle plain;
    pool.intern(plain);
    return pool;
}

TEST_CASE("Output write text places characters on screen", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    output.write(0, 0, "hello", 0);

    REQUIRE(output.screen().cell_at(0, 0).codepoint == U'h');
    REQUIRE(output.screen().cell_at(1, 0).codepoint == U'e');
    REQUIRE(output.screen().cell_at(2, 0).codepoint == U'l');
    REQUIRE(output.screen().cell_at(3, 0).codepoint == U'l');
    REQUIRE(output.screen().cell_at(4, 0).codepoint == U'o');
}

TEST_CASE("Output clear region resets cells to defaults", "[output]") {
    StylePool pool = make_pool();
    Output output(10, 5, pool);

    // Write some text
    output.write(0, 0, "hello", 0);
    REQUIRE(output.screen().cell_at(2, 0).codepoint == U'l');

    // Clear the region containing the text
    output.clear(0, 0, 5, 1);

    REQUIRE(output.screen().cell_at(0, 0).codepoint == U' ');
    REQUIRE(output.screen().cell_at(1, 0).codepoint == U' ');
    REQUIRE(output.screen().cell_at(2, 0).codepoint == U' ');
    REQUIRE(output.screen().cell_at(3, 0).codepoint == U' ');
    REQUIRE(output.screen().cell_at(4, 0).codepoint == U' ');
}

TEST_CASE("TreeRenderer renders simple text to screen", "[output]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("hi");
    root->add_child(std::move(text));

    Frame frame = renderer.render(root.get(), 80, 24);

    // The text "hi" should appear somewhere on the screen
    bool found_h = false, found_i = false;
    for (int y = 0; y < frame.screen.height(); ++y) {
        for (int x = 0; x < frame.screen.width(); ++x) {
            if (frame.screen.cell_at(x, y).codepoint == U'h') found_h = true;
            if (frame.screen.cell_at(x, y).codepoint == U'i') found_i = true;
        }
    }
    REQUIRE(found_h);
    REQUIRE(found_i);
}

TEST_CASE("TreeRenderer renders box with multiple text children", "[output]") {
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

    // Both 'a' and 'b' should appear on screen
    bool found_a = false, found_b = false;
    for (int y = 0; y < frame.screen.height(); ++y) {
        for (int x = 0; x < frame.screen.width(); ++x) {
            if (frame.screen.cell_at(x, y).codepoint == U'a') found_a = true;
            if (frame.screen.cell_at(x, y).codepoint == U'b') found_b = true;
        }
    }
    REQUIRE(found_a);
    REQUIRE(found_b);
}

TEST_CASE("TreeRenderer applies text styles", "[output]") {
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
                // Should have a non-zero style id (bold style interned)
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

TEST_CASE("TreeRenderer handles padding in BoxComponent", "[output]") {
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

    // Find the 'X' character and verify it's at position (2, 2)
    bool found = false;
    for (int y = 0; y < frame.screen.height(); ++y) {
        for (int x = 0; x < frame.screen.width(); ++x) {
            if (frame.screen.cell_at(x, y).codepoint == U'X') {
                REQUIRE(x == 2);
                REQUIRE(y == 2);
                found = true;
                break;
            }
        }
        if (found) break;
    }
    REQUIRE(found);
}

TEST_CASE("Diff produces non-empty ANSI output when frames differ", "[output]") {
    StylePool pool = make_pool();
    TreeRenderer renderer(pool);

    auto root = std::make_unique<RootComponent>();
    auto text = std::make_unique<TextComponent>("A");
    root->add_child(std::move(text));

    // Render frame A
    Frame frame_a = renderer.render(root.get(), 80, 24);

    // Modify the tree
    root->child_at(0)->set_text("B");

    // Render frame B
    Frame frame_b = renderer.render(root.get(), 80, 24);

    // Diff should produce non-empty output
    auto result = renderer.diff(frame_a, frame_b);
    REQUIRE_FALSE(result.ansi.empty());
}
