#ifndef BOOSTCHAIN_RESULT_HPP
#define BOOSTCHAIN_RESULT_HPP

#include <optional>
#include <string>
#include <functional>
#include <stdexcept>

namespace boostchain {

template<typename T>
class Result {
private:
    struct ok_tag {};
    struct error_tag {};

public:
    static Result ok(T value) {
        return Result(std::move(value), ok_tag{});
    }

    static Result error(std::string msg) {
        return Result(std::move(msg), error_tag{});
    }

    bool is_ok() const { return ok_value_.has_value(); }
    bool is_error() const { return !ok_value_.has_value(); }

    const T& unwrap() const {
        if (!ok_value_) {
            throw std::runtime_error("Attempted to unwrap error result: " + error_msg_);
        }
        return *ok_value_;
    }

    T& unwrap() {
        if (!ok_value_) {
            throw std::runtime_error("Attempted to unwrap error result: " + error_msg_);
        }
        return *ok_value_;
    }

    const std::string& error_msg() const {
        if (ok_value_) {
            throw std::runtime_error("Attempted to get error message from ok result");
        }
        return error_msg_;
    }

    template<typename F>
    auto map(F&& func) const -> Result<decltype(func(std::declval<const T&>()))> {
        using U = decltype(func(std::declval<const T&>()));
        if (ok_value_) {
            return Result<U>::ok(func(*ok_value_));
        } else {
            return Result<U>::error(error_msg_);
        }
    }

private:
    Result(T value, ok_tag) : ok_value_(std::move(value)), error_msg_("") {}
    Result(std::string msg, error_tag) : ok_value_(std::nullopt), error_msg_(std::move(msg)) {}

    std::optional<T> ok_value_;
    std::string error_msg_;
};

} // namespace boostchain

#endif // BOOSTCHAIN_RESULT_HPP
