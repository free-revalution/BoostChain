#include <boostchain/memory.hpp>
#include <cassert>
#include <string>
#include <vector>
#include <chrono>

using namespace boostchain;

void test_memory_item_default() {
    MemoryItem item;
    assert(item.content.empty());
    assert(item.relevance == 1.0);
    assert(item.metadata.empty());
}

void test_memory_item_with_content() {
    MemoryItem item("hello world", 0.5);
    assert(item.content == "hello world");
    assert(item.relevance == 0.5);
}

void test_memory_item_with_metadata() {
    auto ts = std::chrono::system_clock::now();
    std::map<std::string, std::string> meta = {{"key1", "val1"}, {"key2", "val2"}};
    MemoryItem item("content", 1.0, ts, meta);
    assert(item.content == "content");
    assert(item.metadata.size() == 2);
    assert(item.metadata.at("key1") == "val1");
    assert(item.metadata.at("key2") == "val2");
}

void test_short_term_add_and_get() {
    ShortTermMemory mem(10);
    assert(mem.size() == 0);

    mem.add("first");
    assert(mem.size() == 1);

    mem.add("second", 0.8);
    assert(mem.size() == 2);

    auto all = mem.get_all();
    assert(all.size() == 2);
    assert(all[0].content == "first");
    assert(all[1].content == "second");
}

void test_short_term_sliding_window() {
    ShortTermMemory mem(3);
    mem.add("a");
    mem.add("b");
    mem.add("c");
    assert(mem.size() == 3);

    mem.add("d");
    // Oldest item "a" should be evicted
    assert(mem.size() == 3);

    auto all = mem.get_all();
    assert(all[0].content == "b");
    assert(all[1].content == "c");
    assert(all[2].content == "d");
}

void test_short_term_retrieve() {
    ShortTermMemory mem(10);
    mem.add("first");
    mem.add("second");
    mem.add("third");

    // Retrieve up to 2 items
    auto results = mem.retrieve("any", 2);
    assert(results.size() == 2);
    // Most recent first
    assert(results[0].content == "third");
    assert(results[1].content == "second");
}

void test_short_term_retrieve_more_than_available() {
    ShortTermMemory mem(10);
    mem.add("only");

    auto results = mem.retrieve("any", 100);
    assert(results.size() == 1);
    assert(results[0].content == "only");
}

void test_short_term_clear() {
    ShortTermMemory mem(10);
    mem.add("a");
    mem.add("b");
    assert(mem.size() == 2);

    mem.clear();
    assert(mem.size() == 0);
    assert(mem.get_all().empty());
}

void test_short_term_serialize_deserialize() {
    ShortTermMemory mem(5);
    mem.add("hello");
    mem.add("world", 0.7);

    // Add with metadata
    MemoryItem item("meta-test", 1.0);
    item.metadata["source"] = "test";
    mem.add(item);

    std::string serialized = mem.serialize();
    assert(!serialized.empty());
    assert(serialized.find("short_term") != std::string::npos);

    ShortTermMemory mem2(5);
    mem2.deserialize(serialized);

    assert(mem2.size() == 3);
    auto all = mem2.get_all();
    assert(all[0].content == "hello");
    assert(all[1].content == "world");
    assert(all[2].content == "meta-test");
    assert(all[2].metadata.at("source") == "test");
}

void test_short_term_deserialize_empty() {
    ShortTermMemory mem(10);
    mem.deserialize("");
    assert(mem.size() == 0);
}

void test_short_term_serialize_empty() {
    ShortTermMemory mem(10);
    std::string s = mem.serialize();
    assert(!s.empty());
    assert(s.find("short_term") != std::string::npos);
}

void test_long_term_basic() {
    LongTermMemory mem(100);
    assert(mem.size() == 0);

    mem.add("item1");
    mem.add("item2", 0.5);
    assert(mem.size() == 2);
}

void test_long_term_retrieve_no_embedding_fn() {
    LongTermMemory mem(100);
    mem.add("python programming");
    mem.add("cooking recipes");
    mem.add("python machine learning");

    // Without embedding function, retrieval uses substring matching
    auto results = mem.retrieve("python", 10);
    assert(results.size() == 3);
    // Items containing "python" should appear first
    assert(results[0].content.find("python") != std::string::npos);
}

void test_long_term_retrieve_with_embedding_fn() {
    // Simple embedding function: word count vector
    auto embed_fn = [](const std::string& text) -> std::vector<double> {
        // Count character frequencies for first 4 letters (a-d)
        std::vector<double> vec(4, 0.0);
        for (char c : text) {
            if (c >= 'a' && c <= 'd') vec[c - 'a'] += 1.0;
        }
        return vec;
    };

    LongTermMemory mem(100, embed_fn);
    mem.add("aabbb ccc ddd");
    mem.add("xxx yyy zzz");
    mem.add("aaa bbb ccc ddd"); // most similar to first

    auto results = mem.retrieve("aaa bbb", 2);
    assert(results.size() == 2);
    // "aabbb ccc ddd" and "aaa bbb ccc ddd" should be top results
    assert(results[0].content == "aaa bbb ccc ddd" ||
           results[0].content == "aabbb ccc ddd");
}

void test_long_term_clear() {
    LongTermMemory mem(100);
    mem.add("a");
    mem.add("b");
    assert(mem.size() == 2);

    mem.clear();
    assert(mem.size() == 0);
}

void test_long_term_serialize_deserialize() {
    LongTermMemory mem(100);
    mem.add("alpha");
    mem.add("beta", 0.3);

    std::string serialized = mem.serialize();
    assert(!serialized.empty());
    assert(serialized.find("long_term") != std::string::npos);

    LongTermMemory mem2(100);
    mem2.deserialize(serialized);

    assert(mem2.size() == 2);
    auto all = mem2.get_all();
    assert(all[0].content == "alpha");
    assert(all[1].content == "beta");
}

void test_long_term_deserialize_with_embedding_fn() {
    auto embed_fn = [](const std::string& text) -> std::vector<double> {
        return {static_cast<double>(text.size())};
    };

    LongTermMemory mem(100, embed_fn);
    mem.add("hello");

    std::string serialized = mem.serialize();
    LongTermMemory mem2(100, embed_fn);
    mem2.deserialize(serialized);

    assert(mem2.size() == 1);
    // Should be able to retrieve using embedding similarity
    auto results = mem2.retrieve("hi", 1);
    assert(results.size() == 1);
}

void test_polymorphism_through_interface() {
    std::unique_ptr<IMemory> mem = std::make_unique<ShortTermMemory>(10);
    mem->add("test");
    mem->add("test2", 0.5);

    assert(mem->size() == 2);
    auto all = mem->get_all();
    assert(all.size() == 2);

    mem->clear();
    assert(mem->size() == 0);

    auto serialized = mem->serialize();
    assert(!serialized.empty());
}

int main() {
    test_memory_item_default();
    test_memory_item_with_content();
    test_memory_item_with_metadata();
    test_short_term_add_and_get();
    test_short_term_sliding_window();
    test_short_term_retrieve();
    test_short_term_retrieve_more_than_available();
    test_short_term_clear();
    test_short_term_serialize_deserialize();
    test_short_term_deserialize_empty();
    test_short_term_serialize_empty();
    test_long_term_basic();
    test_long_term_retrieve_no_embedding_fn();
    test_long_term_retrieve_with_embedding_fn();
    test_long_term_clear();
    test_long_term_serialize_deserialize();
    test_long_term_deserialize_with_embedding_fn();
    test_polymorphism_through_interface();
    return 0;
}
