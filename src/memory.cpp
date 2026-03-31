#include <boostchain/memory.hpp>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace boostchain {

// ============================================================================
// ShortTermMemory
// ============================================================================

ShortTermMemory::ShortTermMemory(size_t max_items)
    : max_items_(max_items) {}

void ShortTermMemory::add(const MemoryItem& item) {
    items_.push_back(item);
    // Sliding window: remove oldest if over capacity
    while (items_.size() > max_items_) {
        items_.erase(items_.begin());
    }
}

void ShortTermMemory::add(const std::string& content, double relevance) {
    add(MemoryItem(content, relevance));
}

std::vector<MemoryItem> ShortTermMemory::retrieve(
    const std::string& /*query*/,
    size_t max_items) const {

    // Simple retrieval: return most recent items up to max_items
    // In a production system, this would use text similarity
    std::vector<MemoryItem> result;
    size_t start = (items_.size() > max_items) ? items_.size() - max_items : 0;
    for (size_t i = start; i < items_.size(); ++i) {
        result.push_back(items_[i]);
    }
    // Return in reverse chronological order (most recent first)
    std::reverse(result.begin(), result.end());
    return result;
}

std::vector<MemoryItem> ShortTermMemory::get_all() const {
    return items_;
}

size_t ShortTermMemory::size() const {
    return items_.size();
}

void ShortTermMemory::clear() {
    items_.clear();
}

std::string ShortTermMemory::serialize() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(9);
    oss << "short_term|" << max_items_ << "|" << items_.size() << "\n";
    for (const auto& item : items_) {
        auto ts = std::chrono::duration_cast<std::chrono::nanoseconds>(
            item.timestamp.time_since_epoch()).count();
        oss << ts << "|" << item.relevance << "|" << item.content.size()
            << "|" << item.content;
        // Serialize metadata
        oss << "|" << item.metadata.size();
        for (const auto& [k, v] : item.metadata) {
            oss << "|" << k.size() << "|" << k << "|" << v.size() << "|" << v;
        }
        oss << "\n";
    }
    return oss.str();
}

void ShortTermMemory::deserialize(const std::string& data) {
    items_.clear();
    if (data.empty()) return;

    std::istringstream iss(data);
    std::string header;
    std::getline(iss, header);
    // Parse header: "short_term|max_items|count"
    size_t pos1 = header.find('|');
    if (pos1 == std::string::npos) return;
    size_t pos2 = header.find('|', pos1 + 1);
    if (pos2 == std::string::npos) return;

    // max_items_ = std::stoul(header.substr(pos1 + 1, pos2 - pos1 - 1));
    // We keep max_items_ as configured; just read items

    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;

        MemoryItem item;
        size_t pos = 0;

        // Parse timestamp
        size_t sep = line.find('|', pos);
        long long ts_ns = std::stoll(line.substr(pos, sep - pos));
        item.timestamp = std::chrono::system_clock::time_point(
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                std::chrono::nanoseconds(ts_ns)));
        pos = sep + 1;

        // Parse relevance
        sep = line.find('|', pos);
        item.relevance = std::stod(line.substr(pos, sep - pos));
        pos = sep + 1;

        // Parse content
        sep = line.find('|', pos);
        size_t content_len = std::stoul(line.substr(pos, sep - pos));
        pos = sep + 1;
        item.content = line.substr(pos, content_len);
        pos += content_len;

        // Parse metadata count - skip '|' if we're right at it
        if (pos < line.size() && line[pos] == '|') pos++;
        if (pos >= line.size()) {
            items_.push_back(item);
            continue;
        }
        sep = line.find('|', pos);
        size_t meta_count = 0;
        if (sep != std::string::npos) {
            meta_count = std::stoul(line.substr(pos, sep - pos));
            pos = sep + 1;
        } else {
            meta_count = std::stoul(line.substr(pos));
        }

        for (size_t i = 0; i < meta_count; ++i) {
            // key size
            sep = line.find('|', pos);
            size_t klen = std::stoul(line.substr(pos, sep - pos));
            pos = sep + 1;
            // key
            std::string key = line.substr(pos, klen);
            pos += klen;
            // skip '|' separator before value size
            if (pos < line.size() && line[pos] == '|') pos++;
            // value size
            sep = line.find('|', pos);
            size_t vlen = 0;
            if (sep != std::string::npos) {
                vlen = std::stoul(line.substr(pos, sep - pos));
                pos = sep + 1;
            } else {
                vlen = std::stoul(line.substr(pos));
            }
            // value
            std::string val = line.substr(pos, vlen);
            pos += vlen;
            item.metadata[key] = val;
        }

        items_.push_back(item);
    }
}

// ============================================================================
// LongTermMemory
// ============================================================================

LongTermMemory::LongTermMemory(size_t max_items, EmbeddingFn embed_fn)
    : max_items_(max_items), embed_fn_(std::move(embed_fn)) {}

void LongTermMemory::add(const MemoryItem& item) {
    items_.push_back(item);

    // Compute embedding if we have an embedding function
    if (embed_fn_) {
        embeddings_.push_back(embed_fn_(item.content));
    } else {
        embeddings_.emplace_back(); // empty embedding
    }

    while (items_.size() > max_items_) {
        items_.erase(items_.begin());
        embeddings_.erase(embeddings_.begin());
    }
}

void LongTermMemory::add(const std::string& content, double relevance) {
    add(MemoryItem(content, relevance));
}

double LongTermMemory::cosine_similarity(const std::vector<double>& a,
                                          const std::vector<double>& b) const {
    if (a.empty() || b.empty() || a.size() != b.size()) return 0.0;

    double dot = 0.0, norm_a = 0.0, norm_b = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }

    norm_a = std::sqrt(norm_a);
    norm_b = std::sqrt(norm_b);

    if (norm_a < 1e-10 || norm_b < 1e-10) return 0.0;
    return dot / (norm_a * norm_b);
}

std::vector<MemoryItem> LongTermMemory::retrieve(
    const std::string& query,
    size_t max_items) const {

    if (items_.empty()) return {};

    // If no embedding function, fall back to simple substring matching + relevance
    if (!embed_fn_) {
        // Score by relevance and substring match
        std::vector<std::pair<double, size_t>> scored;
        for (size_t i = 0; i < items_.size(); ++i) {
            double score = items_[i].relevance;
            // Simple substring bonus
            if (items_[i].content.find(query) != std::string::npos) {
                score += 1.0;
            }
            scored.emplace_back(score, i);
        }

        // Sort by score descending
        std::sort(scored.begin(), scored.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        std::vector<MemoryItem> result;
        size_t count = std::min(max_items, scored.size());
        for (size_t i = 0; i < count; ++i) {
            result.push_back(items_[scored[i].second]);
        }
        return result;
    }

    // With embedding function: compute query embedding and find similar items
    auto query_embedding = embed_fn_(query);

    std::vector<std::pair<double, size_t>> scored;
    for (size_t i = 0; i < items_.size(); ++i) {
        double sim = cosine_similarity(query_embedding, embeddings_[i]);
        scored.emplace_back(sim, i);
    }

    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<MemoryItem> result;
    size_t count = std::min(max_items, scored.size());
    for (size_t i = 0; i < count; ++i) {
        result.push_back(items_[scored[i].second]);
    }
    return result;
}

std::vector<MemoryItem> LongTermMemory::get_all() const {
    return items_;
}

size_t LongTermMemory::size() const {
    return items_.size();
}

void LongTermMemory::clear() {
    items_.clear();
    embeddings_.clear();
}

std::string LongTermMemory::serialize() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(9);
    oss << "long_term|" << max_items_ << "|" << items_.size() << "\n";
    for (const auto& item : items_) {
        auto ts = std::chrono::duration_cast<std::chrono::nanoseconds>(
            item.timestamp.time_since_epoch()).count();
        oss << ts << "|" << item.relevance << "|" << item.content.size()
            << "|" << item.content;
        oss << "|" << item.metadata.size();
        for (const auto& [k, v] : item.metadata) {
            oss << "|" << k.size() << "|" << k << "|" << v.size() << "|" << v;
        }
        oss << "\n";
    }
    return oss.str();
}

void LongTermMemory::deserialize(const std::string& data) {
    items_.clear();
    embeddings_.clear();
    if (data.empty()) return;

    std::istringstream iss(data);
    std::string header;
    std::getline(iss, header);
    if (header.empty()) return;

    // Parse header: "long_term|max_items|count"
    // We keep max_items_ as configured

    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;

        MemoryItem item;
        size_t pos = 0;

        // Parse timestamp
        size_t sep = line.find('|', pos);
        long long ts_ns = std::stoll(line.substr(pos, sep - pos));
        item.timestamp = std::chrono::system_clock::time_point(
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                std::chrono::nanoseconds(ts_ns)));
        pos = sep + 1;

        // Parse relevance
        sep = line.find('|', pos);
        item.relevance = std::stod(line.substr(pos, sep - pos));
        pos = sep + 1;

        // Parse content
        sep = line.find('|', pos);
        size_t content_len = std::stoul(line.substr(pos, sep - pos));
        pos = sep + 1;
        item.content = line.substr(pos, content_len);
        pos += content_len;

        // Parse metadata count - skip '|' if we're right at it
        if (pos < line.size() && line[pos] == '|') pos++;
        if (pos >= line.size()) {
            items_.push_back(item);
            continue;
        }
        sep = line.find('|', pos);
        size_t meta_count = 0;
        if (sep != std::string::npos) {
            meta_count = std::stoul(line.substr(pos, sep - pos));
            pos = sep + 1;
        } else {
            meta_count = std::stoul(line.substr(pos));
        }

        for (size_t i = 0; i < meta_count; ++i) {
            sep = line.find('|', pos);
            size_t klen = std::stoul(line.substr(pos, sep - pos));
            pos = sep + 1;
            std::string key = line.substr(pos, klen);
            pos += klen;
            sep = line.find('|', pos);
            size_t vlen = std::stoul(line.substr(pos, sep - pos));
            pos = sep + 1;
            std::string val = line.substr(pos, vlen);
            pos += vlen;
            item.metadata[key] = val;
        }

        items_.push_back(item);
        // Re-embed on deserialize if function available
        if (embed_fn_) {
            embeddings_.push_back(embed_fn_(item.content));
        } else {
            embeddings_.emplace_back();
        }
    }
}

} // namespace boostchain
