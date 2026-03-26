#include <boostchain/result.hpp>
#include <cassert>
#include <string>

void test_result_ok() {
    boostchain::Result<int> r = boostchain::Result<int>::ok(42);
    assert(r.is_ok() == true);
    assert(r.is_error() == false);
    assert(r.unwrap() == 42);
}

void test_result_error() {
    auto r = boostchain::Result<int>::error("Something failed");
    assert(r.is_ok() == false);
    assert(r.is_error() == true);
    assert(r.error_msg() == "Something failed");
}

void test_result_map() {
    auto r = boostchain::Result<int>::ok(5);
    auto mapped = r.map([](int x) { return x * 2; });
    assert(mapped.is_ok());
    assert(mapped.unwrap() == 10);
}

void test_result_map_error_propagates() {
    auto r = boostchain::Result<int>::error("bad");
    auto mapped = r.map([](int x) { return x * 2; });
    assert(mapped.is_error());
    assert(mapped.error_msg() == "bad");
}

void test_result_string() {
    auto r = boostchain::Result<std::string>::ok(std::string("hello"));
    assert(r.is_ok());
    assert(r.unwrap() == "hello");
}

int main() {
    test_result_ok();
    test_result_error();
    test_result_map();
    test_result_map_error_propagates();
    test_result_string();
    return 0;
}
