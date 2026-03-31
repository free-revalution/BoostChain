#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <map>
#include <functional>
#include <sstream>

namespace boostchain {

// A single item stored in memory
struct MemoryItem {
    std::string content;
    double relevance = 1.0;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> metadata;

    MemoryItem()
        : timestamp(std::chrono::system_clock::now()) {}

    MemoryItem(const std::string& c, double rel = 1.0)
        : content(c), relevance(rel), timestamp(std::chrono::system_clock::now()) {}

    MemoryItem(const std::string& c, double rel,
               std::chrono::system_clock::time_point ts,
               std::map<std::string, std::string> meta)
        : content(c), relevance(rel), timestamp(ts), metadata(std::move(meta)) {}
};

// Abstract memory interface
class IMemory {
public:
    virtual ~IMemory() = default;

    // Add an item to memory
    virtual void add(const MemoryItem& item) = 0;

    // Add an item with just content and relevance
    virtual void add(const std::string& content, double relevance = 1.0) = 0;

    // Retrieve items, optionally filtered by a relevance callback
    virtual std::vector<MemoryItem> retrieve(
        const std::string& query,
        size_t max_items = 10) const = 0;

    // Get all items in memory
    virtual std::vector<MemoryItem> get_all() const = 0;

    // Get number of items
    virtual size_t size() const = 0;

    // Clear all items
    virtual void clear() = 0;

    // Serialize memory to string
    virtual std::string serialize() const = 0;

    // Deserialize memory from string (replaces current contents)
    virtual void deserialize(const std::string& data) = 0;
};

// Short-term memory: in-RAM storage with sliding window
class ShortTermMemory : public IMemory {
public:
    explicit ShortTermMemory(size_t max_items = 100);

    void add(const MemoryItem& item) override;
    void add(const std::string& content, double relevance = 1.0) override;

    std::vector<MemoryItem> retrieve(
        const std::string& query,
        size_t max_items = 10) const override;

    std::vector<MemoryItem> get_all() const override;
    size_t size() const override;
    void clear() override;

    std::string serialize() const override;
    void deserialize(const std::string& data) override;

private:
    size_t max_items_;
    std::vector<MemoryItem> items_;
};

// Long-term memory: vector retrieval with embedding-based similarity
// Uses a simple cosine-similarity approach with embedding vectors.
class LongTermMemory : public IMemory {
public:
    using EmbeddingFn = std::function<std::vector<double>(const std::string&)>;

    explicit LongTermMemory(size_t max_items = 10000,
                            EmbeddingFn embed_fn = nullptr);

    void add(const MemoryItem& item) override;
    void add(const std::string& content, double relevance = 1.0) override;

    std::vector<MemoryItem> retrieve(
        const std::string& query,
        size_t max_items = 10) const override;

    std::vector<MemoryItem> get_all() const override;
    size_t size() const override;
    void clear() override;

    std::string serialize() const override;
    void deserialize(const std::string& data) override;

private:
    double cosine_similarity(const std::vector<double>& a,
                             const std::vector<double>& b) const;

    size_t max_items_;
    EmbeddingFn embed_fn_;
    std::vector<MemoryItem> items_;
    std::vector<std::vector<double>> embeddings_;
};

} // namespace boostchain
