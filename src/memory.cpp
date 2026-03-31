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
            << "|";
        // Write content, escaping pipe and backslash
        for (char c : item.content) {
            if (c == '\\') oss << "\\\\";
            else if (c == '\n') oss << "\\n";
            else if (c == '|') oss << "\\p";
            else oss << c;
        }
        // Serialize metadata
        oss << "|" << item.metadata.size();
        for (const auto& [k, v] : item.metadata) {
            oss << "|" << k.size() << "|";
            for (char c : k) {
                if (c == '\\') oss << "\\\\";
                else if (c == '\n') oss << "\\n";
                else if (c == '|') oss << "\\p";
                else oss << c;
            }
            oss << "|" << v.size() << "|";
            for (char c : v) {
                if (c == '\\') oss << "\\\\";
                else if (c == '\n') oss << "\\n";
                else if (c == '|') oss << "\\p";
                else oss << c;
            }
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
    // max_items_ stays as configured

    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;

        try {
            MemoryItem item;
            size_t pos = 0;

            // Parse timestamp
            size_t sep = line.find('|', pos);
            if (sep == std::string::npos) continue;
            long long ts_ns = std::stoll(line.substr(pos, sep - pos));
            item.timestamp = std::chrono::system_clock::time_point(
                std::chrono::duration_cast<std::chrono::system_clock::duration>(
                    std::chrono::nanoseconds(ts_ns)));
            pos = sep + 1;

            // Parse relevance
            sep = line.find('|', pos);
            if (sep == std::string::npos) continue;
            item.relevance = std::stod(line.substr(pos, sep - pos));
            pos = sep + 1;

            // Parse content length (before unescaping)
            sep = line.find('|', pos);
            if (sep == std::string::npos) continue;
            size_t content_len = std::stoul(line.substr(pos, sep - pos));
            pos = sep + 1;

            // Read exactly content_len raw bytes (may contain escaped sequences)
            std::string raw_content = line.substr(pos, content_len);
            pos += content_len;

            // Unescape content
            item.content.clear();
            for (size_t ci = 0; ci < raw_content.size(); ++ci) {
                if (raw_content[ci] == '\\' && ci + 1 < raw_content.size()) {
                    char next = raw_content[ci + 1];
                    if (next == 'n') { item.content += '\n'; ci++; }
                    else if (next == 'p') { item.content += '|'; ci++; }
                    else if (next == '\\') { item.content += '\\'; ci++; }
                    else { item.content += raw_content[ci]; }
                } else {
                    item.content += raw_content[ci];
                }
            }

            // Parse metadata
            if (pos >= line.size()) {
                items_.push_back(item);
                continue;
            }
            if (line[pos] == '|') pos++;
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
                if (sep == std::string::npos) break;
                size_t klen = std::stoul(line.substr(pos, sep - pos));
                pos = sep + 1;
                std::string raw_key = line.substr(pos, klen);
                pos += klen;

                // Unescape key
                std::string key;
                for (size_t ci = 0; ci < raw_key.size(); ++ci) {
                    if (raw_key[ci] == '\\' && ci + 1 < raw_key.size()) {
                        char next = raw_key[ci + 1];
                        if (next == 'n') { key += '\n'; ci++; }
                        else if (next == 'p') { key += '|'; ci++; }
                        else if (next == '\\') { key += '\\'; ci++; }
                        else { key += raw_key[ci]; }
                    } else { key += raw_key[ci]; }
                }

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
                std::string raw_val = line.substr(pos, vlen);
                pos += vlen;

                // Unescape value
                std::string val;
                for (size_t ci = 0; ci < raw_val.size(); ++ci) {
                    if (raw_val[ci] == '\\' && ci + 1 < raw_val.size()) {
                        char next = raw_val[ci + 1];
                        if (next == 'n') { val += '\n'; ci++; }
                        else if (next == 'p') { val += '|'; ci++; }
                        else if (next == '\\') { val += '\\'; ci++; }
                        else { val += raw_val[ci]; }
                    } else { val += raw_val[ci]; }
                }

                item.metadata[key] = val;
            }

            items_.push_back(item);
        } catch (...) {
            // Skip malformed items
        }
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
            << "|";
        for (char c : item.content) {
            if (c == '\\') oss << "\\\\";
            else if (c == '\n') oss << "\\n";
            else if (c == '|') oss << "\\p";
            else oss << c;
        }
        oss << "|" << item.metadata.size();
        for (const auto& [k, v] : item.metadata) {
            oss << "|" << k.size() << "|";
            for (char c : k) {
                if (c == '\\') oss << "\\\\";
                else if (c == '\n') oss << "\\n";
                else if (c == '|') oss << "\\p";
                else oss << c;
            }
            oss << "|" << v.size() << "|";
            for (char c : v) {
                if (c == '\\') oss << "\\\\";
                else if (c == '\n') oss << "\\n";
                else if (c == '|') oss << "\\p";
                else oss << c;
            }
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

    // Helper lambda to unescape serialized strings
    auto unescape = [](const std::string& raw) -> std::string {
        std::string result;
        for (size_t ci = 0; ci < raw.size(); ++ci) {
            if (raw[ci] == '\\' && ci + 1 < raw.size()) {
                char next = raw[ci + 1];
                if (next == 'n') { result += '\n'; ci++; }
                else if (next == 'p') { result += '|'; ci++; }
                else if (next == '\\') { result += '\\'; ci++; }
                else { result += raw[ci]; }
            } else { result += raw[ci]; }
        }
        return result;
    };

    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;

        try {
            MemoryItem item;
            size_t pos = 0;

            // Parse timestamp
            size_t sep = line.find('|', pos);
            if (sep == std::string::npos) continue;
            long long ts_ns = std::stoll(line.substr(pos, sep - pos));
            item.timestamp = std::chrono::system_clock::time_point(
                std::chrono::duration_cast<std::chrono::system_clock::duration>(
                    std::chrono::nanoseconds(ts_ns)));
            pos = sep + 1;

            // Parse relevance
            sep = line.find('|', pos);
            if (sep == std::string::npos) continue;
            item.relevance = std::stod(line.substr(pos, sep - pos));
            pos = sep + 1;

            // Parse content length
            sep = line.find('|', pos);
            if (sep == std::string::npos) continue;
            size_t content_len = std::stoul(line.substr(pos, sep - pos));
            pos = sep + 1;
            std::string raw_content = line.substr(pos, content_len);
            pos += content_len;
            item.content = unescape(raw_content);

            // Parse metadata
            if (pos >= line.size()) {
                items_.push_back(item);
                if (embed_fn_) {
                    embeddings_.push_back(embed_fn_(item.content));
                } else {
                    embeddings_.emplace_back();
                }
                continue;
            }
            if (line[pos] == '|') pos++;
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
                if (sep == std::string::npos) break;
                size_t klen = std::stoul(line.substr(pos, sep - pos));
                pos = sep + 1;
                std::string raw_key = line.substr(pos, klen);
                pos += klen;
                sep = line.find('|', pos);
                size_t vlen = 0;
                if (sep != std::string::npos) {
                    vlen = std::stoul(line.substr(pos, sep - pos));
                    pos = sep + 1;
                } else {
                    vlen = std::stoul(line.substr(pos));
                }
                std::string raw_val = line.substr(pos, vlen);
                pos += vlen;
                item.metadata[unescape(raw_key)] = unescape(raw_val);
            }

            items_.push_back(item);
            if (embed_fn_) {
                embeddings_.push_back(embed_fn_(item.content));
            } else {
                embeddings_.emplace_back();
            }
        } catch (...) {
            // Skip malformed items
        }
    }
}

} // namespace boostchain
