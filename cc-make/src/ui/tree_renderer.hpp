#pragma once
#include "ui/component.hpp"
#include "ui/output.hpp"
#include "terminal/renderer.hpp"
#include <memory>

namespace ccmake {

// Renders a component tree to a Screen buffer, then diffs against previous frame
class TreeRenderer {
public:
    explicit TreeRenderer(StylePool& style_pool);

    // Render a component tree to a Frame
    // Calls calculate_layout on all nodes, then renders to screen
    Frame render(Component* root, int width, int height);

    // Diff and produce ANSI output
    RenderOutput diff(const Frame& prev, const Frame& next, bool full_clear = false);

    // Get the log-update renderer
    LogUpdateRenderer& renderer() { return log_renderer_; }

    // Get previous frame
    const Frame& prev_frame() const { return prev_frame_; }

private:
    StylePool& style_pool_;
    LogUpdateRenderer log_renderer_;
    Frame prev_frame_;

    void render_node(Component* node, Output& output, int x_offset, int y_offset);
};

}  // namespace ccmake
