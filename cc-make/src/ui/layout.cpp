#include "ui/layout.hpp"
#include <algorithm>
#include <numeric>
#include <cassert>

namespace ccmake {

// ---------------------------------------------------------------------------
// BoxNode -- simple container
// ---------------------------------------------------------------------------

int BoxNode::child_count() const {
    return static_cast<int>(children_.size());
}

LayoutNode* BoxNode::child_at(int index) {
    return children_.at(static_cast<size_t>(index));
}

void BoxNode::add_child(LayoutNode* child) {
    children_.push_back(child);
    child->set_parent(this);
}

void BoxNode::remove_child(LayoutNode* child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        children_.erase(it);
        child->set_parent(nullptr);
    }
}

// ---------------------------------------------------------------------------
// TextNode -- leaf with intrinsic size
// ---------------------------------------------------------------------------

std::pair<float, float> TextNode::measure(float /*width*/, float /*height*/) {
    float text_width = static_cast<float>(text_.size());
    return {text_width, 1.0f};
}

// ---------------------------------------------------------------------------
// LayoutNode -- compute_layout entry point
// ---------------------------------------------------------------------------

void LayoutNode::calculate_layout(float available_width, float available_height) {
    // If we are the root, use the full available space as our position
    compute_layout(available_width, available_height);
}

void LayoutNode::compute_layout(float available_width, float available_height) {
    const auto& s = style_;

    // 1. Hidden nodes
    if (!s.display) {
        layout_ = {0, 0, 0, 0};
        for (int i = 0; i < child_count(); ++i) {
            child_at(i)->compute_layout(0, 0);
        }
        return;
    }

    // 2. Absolute positioning
    if (s.position == PositionType::Absolute) {
        layout_absolute();
        return;
    }

    // 3. Determine own size
    float own_width = s.width.value_or(available_width);
    float own_height = s.height.value_or(available_height);

    // Apply min/max constraints
    if (s.min_width) own_width = std::max(own_width, *s.min_width);
    if (s.max_width) own_width = std::min(own_width, *s.max_width);
    if (s.min_height) own_height = std::max(own_height, *s.min_height);
    if (s.max_height) own_height = std::min(own_height, *s.max_height);

    // Don't exceed available space if no explicit size was set
    if (!s.width) own_width = std::min(own_width, available_width);
    if (!s.height) own_height = std::min(own_height, available_height);

    layout_.width = own_width;
    layout_.height = own_height;

    // 4. Layout children (flexbox)
    layout_flex(own_width, own_height);
}

void LayoutNode::layout_absolute() {
    const auto& s = style_;
    // Absolute nodes need parent context for reference; for now use style values
    layout_.x = s.left.value_or(0);
    layout_.y = s.top.value_or(0);
    layout_.width = s.width.value_or(0);
    layout_.height = s.height.value_or(0);

    if (s.right && s.width) {
        // right + width gives the distance from the right edge
        // We'd need parent width to compute, skip for simplicity
    }
    if (s.bottom && s.height) {
        // Similar, skip
    }
}

void LayoutNode::layout_flex(float container_width, float container_height) {
    const auto& s = style_;
    const bool is_row = (s.flex_direction == FlexDirection::Row);
    const float gap = s.gap;

    // Content area after padding
    float content_x = s.padding_left;
    float content_y = s.padding_top;
    float content_width = container_width - s.padding_left - s.padding_right;
    float content_height = container_height - s.padding_top - s.padding_bottom;

    if (content_width < 0) content_width = 0;
    if (content_height < 0) content_height = 0;

    int n = child_count();
    if (n == 0) return;

    // ---- Measure children (resolve their initial sizes) ----
    struct ChildInfo {
        LayoutNode* node;
        float main_size;   // width if Row, height if Column
        float cross_size;  // height if Row, width if Column
        float flex_grow;
        float flex_shrink;
        bool has_fixed_main;  // true if the child has an explicit size on the main axis
    };

    std::vector<ChildInfo> infos;
    infos.reserve(static_cast<size_t>(n));

    float total_main = 0;
    float total_grow = 0;
    float total_shrink = 0;
    int num_gaps = (n > 1 && gap > 0) ? (n - 1) : 0;
    float total_gap = num_gaps * gap;

    for (int i = 0; i < n; ++i) {
        LayoutNode* child = child_at(i);
        const auto& cs = child->style();

        // Skip hidden children
        if (!cs.display) {
            child->compute_layout(0, 0);
            continue;
        }

        float child_main = 0;
        float child_cross = 0;
        bool has_fixed = false;

        if (is_row) {
            if (cs.width) {
                child_main = *cs.width;
                has_fixed = true;
            } else {
                // Use measure or flex_basis
                auto [mw, mh] = child->measure(content_width - total_main - total_gap, content_height);
                child_main = (cs.flex_basis > 0) ? cs.flex_basis : mw;
            }
            if (cs.height) {
                child_cross = *cs.height;
            } else {
                auto [mw, mh] = child->measure(child_main, content_height);
                child_cross = mh;
            }
        } else {
            // Column
            if (cs.height) {
                child_main = *cs.height;
                has_fixed = true;
            } else {
                auto [mw, mh] = child->measure(content_width, content_height - total_main - total_gap);
                child_main = (cs.flex_basis > 0) ? cs.flex_basis : mh;
            }
            if (cs.width) {
                child_cross = *cs.width;
            } else {
                auto [mw, mh] = child->measure(content_width, child_main);
                child_cross = mw;
            }
        }

        total_main += child_main;
        total_grow += cs.flex_grow;
        total_shrink += cs.flex_shrink;

        infos.push_back({child, child_main, child_cross, cs.flex_grow, cs.flex_shrink, has_fixed});
    }

    total_main += total_gap;

    // ---- Distribute free space (grow) or collect overflow (shrink) ----
    float available_main = is_row ? content_width : content_height;
    float remaining = available_main - total_main;

    if (remaining > 0 && total_grow > 0) {
        // Distribute extra space to flex_grow children
        float per_unit = remaining / total_grow;
        for (auto& info : infos) {
            if (info.flex_grow > 0) {
                float extra = per_unit * info.flex_grow;
                info.main_size += extra;
            }
        }
    } else if (remaining < 0 && total_shrink > 0) {
        // Shrink flex_shrink children
        float per_unit = -remaining / total_shrink;
        for (auto& info : infos) {
            if (info.flex_shrink > 0) {
                float shrink = per_unit * info.flex_shrink;
                info.main_size = std::max(0.0f, info.main_size - shrink);
            }
        }
    }

    // ---- Position children along main axis (justify_content) ----
    float main_pos = is_row ? content_x : content_y;

    if (remaining > 0) {
        switch (s.justify_content) {
            case JustifyContent::Start:
                main_pos = is_row ? content_x : content_y;
                break;
            case JustifyContent::Center:
                main_pos = (is_row ? content_x : content_y) + remaining / 2.0f;
                break;
            case JustifyContent::End:
                main_pos = (is_row ? content_x : content_y) + remaining;
                break;
            case JustifyContent::SpaceBetween:
                // First child at start, last at end, equal space between
                // We distribute extra space between children
                main_pos = is_row ? content_x : content_y;
                if (infos.size() > 1) {
                    // Adjust each child's position later
                }
                break;
            case JustifyContent::SpaceAround:
                main_pos = (is_row ? content_x : content_y) + remaining / (2.0f * static_cast<float>(infos.size()));
                break;
        }
    }

    float space_between = 0;
    if (remaining > 0 && infos.size() > 1) {
        switch (s.justify_content) {
            case JustifyContent::SpaceBetween:
                space_between = remaining / static_cast<float>(infos.size() - 1);
                break;
            case JustifyContent::SpaceAround:
                space_between = remaining / static_cast<float>(infos.size());
                break;
            default:
                break;
        }
    }

    float cross_available = is_row ? content_height : content_width;

    // ---- Position and size each child ----
    for (size_t i = 0; i < infos.size(); ++i) {
        auto& info = infos[i];

        // Position along main axis
        float child_main_pos = main_pos;
        if (is_row) {
            info.node->layout_.x = child_main_pos;
            info.node->layout_.width = info.main_size;
        } else {
            info.node->layout_.y = child_main_pos;
            info.node->layout_.height = info.main_size;
        }

        // Align on cross axis
        AlignItems align = info.node->style().align_self.value_or(s.align_items);

        if (is_row) {
            // Cross axis = vertical (y)
            info.node->layout_.height = info.cross_size;

            switch (align) {
                case AlignItems::Start:
                    info.node->layout_.y = content_y;
                    break;
                case AlignItems::Center:
                    info.node->layout_.y = content_y + (content_height - info.cross_size) / 2.0f;
                    break;
                case AlignItems::End:
                    info.node->layout_.y = content_y + content_height - info.cross_size;
                    break;
                case AlignItems::Stretch:
                    info.node->layout_.y = content_y;
                    info.node->layout_.height = content_height;
                    break;
            }
        } else {
            // Cross axis = horizontal (x)
            info.node->layout_.width = info.cross_size;

            switch (align) {
                case AlignItems::Start:
                    info.node->layout_.x = content_x;
                    break;
                case AlignItems::Center:
                    info.node->layout_.x = content_x + (content_width - info.cross_size) / 2.0f;
                    break;
                case AlignItems::End:
                    info.node->layout_.x = content_x + content_width - info.cross_size;
                    break;
                case AlignItems::Stretch:
                    info.node->layout_.x = content_x;
                    info.node->layout_.width = content_width;
                    break;
            }
        }

        // Recursively layout the child's own children
        float child_avail_w = is_row ? info.main_size : cross_available;
        float child_avail_h = is_row ? cross_available : info.main_size;

        // If the child was stretched, use the stretched size
        if (is_row && align == AlignItems::Stretch) {
            child_avail_h = content_height;
        }
        if (!is_row && align == AlignItems::Stretch) {
            child_avail_w = content_width;
        }

        info.node->compute_layout(child_avail_w, child_avail_h);

        // Advance main position for next child
        main_pos += info.main_size + gap + space_between;
    }
}

}  // namespace ccmake
