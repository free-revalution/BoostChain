#include "ui/tree_renderer.hpp"
#include <algorithm>

namespace ccmake {

TreeRenderer::TreeRenderer(StylePool& style_pool)
    : style_pool_(style_pool),
      log_renderer_(style_pool) {}

// Recursively sync component styles to layout nodes before layout calculation
static void sync_styles(Component* node) {
    if (!node || !node->layout_node()) return;
    node->layout_node()->set_style(node->style());
    for (int i = 0; i < node->child_count(); ++i) {
        sync_styles(node->child_at(i));
    }
}

Frame TreeRenderer::render(Component* root, int width, int height) {
    // 1. Sync all component styles to layout nodes, then calculate layout
    if (root && root->layout_node()) {
        sync_styles(root);
        root->layout_node()->calculate_layout(
            static_cast<float>(width),
            static_cast<float>(height));
    }

    // 2. Create output buffer
    Output output(width, height, style_pool_);

    // 3. Render the component tree
    if (root) {
        render_node(root, output, 0, 0);
    }

    // 4. Build Frame from output screen
    Frame frame;
    frame.screen = std::move(output.screen());
    frame.viewport_width = width;
    frame.viewport_height = height;

    // 5. Save as previous frame and return
    prev_frame_ = frame;  // Copy (screen is valid at this point)
    return frame;
}

void TreeRenderer::render_node(Component* node, Output& output,
                                int x_offset, int y_offset) {
    if (!node) return;

    // Skip hidden components
    if (!node->style().display) return;

    // Get computed layout from the layout node
    if (!node->layout_node()) return;

    const auto& layout = node->layout_node()->layout();
    const auto& style = node->style();

    // Calculate absolute position (apply padding offset)
    int abs_x = x_offset + static_cast<int>(layout.x + style.padding_left);
    int abs_y = y_offset + static_cast<int>(layout.y + style.padding_top);

    // Apply background color fill if text_style has a bg_color
    if (node->text_style().bg_color) {
        uint32_t bg_style_id = style_pool_.intern(node->text_style());
        int box_w = static_cast<int>(layout.width);
        int box_h = static_cast<int>(layout.height);
        // Fill the box area with styled spaces
        for (int row = 0; row < box_h; ++row) {
            for (int col = 0; col < box_w; ++col) {
                int px = x_offset + static_cast<int>(layout.x) + col;
                int py = y_offset + static_cast<int>(layout.y) + row;
                output.write(px, py, " ", bg_style_id);
            }
        }
    }

    if (node->is_text()) {
        // Text component: render text content at absolute position
        uint32_t style_id = style_pool_.intern(node->text_style());
        output.write(abs_x, abs_y, node->text_content(), style_id);
    } else if (node->is_box() || node->is_root()) {
        // Box/Root component: recursively render children
        // Children positions are relative to this node's layout position (before padding)
        int child_x_offset = x_offset + static_cast<int>(layout.x);
        int child_y_offset = y_offset + static_cast<int>(layout.y);

        for (int i = 0; i < node->child_count(); ++i) {
            render_node(node->child_at(i), output, child_x_offset, child_y_offset);
        }
    }
}

RenderOutput TreeRenderer::diff(const Frame& prev, const Frame& next, bool full_clear) {
    return log_renderer_.render(prev, next, full_clear);
}

}  // namespace ccmake
