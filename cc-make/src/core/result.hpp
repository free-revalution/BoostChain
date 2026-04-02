#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace ccmake {

/// A Result monad: represents either a success value (T) or an error (E).
/// Inspired by Rust's Result<T, E>.
template <typename T, typename E = std::string>
class Result {
public:
    /// Construct an Ok result holding a value.
    static Result ok(T value) {
        Result r;
        r.has_value_ = true;
        r.value_ = std::move(value);
        return r;
    }

    /// Construct an Err result holding an error.
    static Result err(E error) {
        Result r;
        r.has_value_ = false;
        r.error_ = std::move(error);
        return r;
    }

    /// Returns true if this is an Ok result.
    bool has_value() const noexcept { return has_value_; }

    /// Returns true if this is an Ok result.
    explicit operator bool() const noexcept { return has_value_; }

    /// Access the success value. Throws std::bad_optional_access if this is an Err.
    const T& value() const& {
        if (!has_value_) throw std::bad_optional_access();
        return *value_;
    }

    /// Access the success value (mutable). Throws std::bad_optional_access if this is an Err.
    T& value() & {
        if (!has_value_) throw std::bad_optional_access();
        return *value_;
    }

    /// Access the success value (rvalue). Throws std::bad_optional_access if this is an Err.
    T&& value() && {
        if (!has_value_) throw std::bad_optional_access();
        return std::move(*value_);
    }

    /// Access the error value. Throws std::bad_optional_access if this is an Ok.
    const E& error() const& {
        if (has_value_) throw std::bad_optional_access();
        return *error_;
    }

    /// Access the error value (mutable). Throws std::bad_optional_access if this is an Ok.
    E& error() & {
        if (has_value_) throw std::bad_optional_access();
        return *error_;
    }

    /// Returns the success value, or the provided default if this is an Err.
    T value_or(T default_val) const& {
        return has_value_ ? *value_ : std::move(default_val);
    }

    /// Returns the success value, or the provided default if this is an Err (rvalue).
    T value_or(T default_val) && {
        return has_value_ ? std::move(*value_) : std::move(default_val);
    }

    /// Transform the Ok value using f. Propagates Err unchanged.
    template <typename F>
    auto map(F&& f) const& -> Result<std::invoke_result_t<F, const T&>, E> {
        using U = std::invoke_result_t<F, const T&>;
        if (has_value_) return Result<U, E>::ok(f(*value_));
        return Result<U, E>::err(*error_);
    }

    /// Transform the Ok value using f (rvalue). Propagates Err unchanged.
    template <typename F>
    auto map(F&& f) && -> Result<std::invoke_result_t<F, T&&>, E> {
        using U = std::invoke_result_t<F, T&&>;
        if (has_value_) return Result<U, E>::ok(f(std::move(*value_)));
        return Result<U, E>::err(std::move(*error_));
    }

    /// Chain operations that may fail. If Ok, apply f which returns a new Result.
    /// If Err, propagate the error.
    template <typename F>
    auto and_then(F&& f) const& -> std::invoke_result_t<F, const T&> {
        using ResultType = std::invoke_result_t<F, const T&>;
        if (has_value_) return f(*value_);
        return ResultType::err(*error_);
    }

    /// Chain operations that may fail (rvalue).
    template <typename F>
    auto and_then(F&& f) && -> std::invoke_result_t<F, T&&> {
        using ResultType = std::invoke_result_t<F, T&&>;
        if (has_value_) return f(std::move(*value_));
        return ResultType::err(std::move(*error_));
    }

    /// Transform the Err value using f. Propagates Ok unchanged.
    template <typename F>
    auto map_error(F&& f) const& -> Result<T, std::invoke_result_t<F, const E&>> {
        using NewE = std::invoke_result_t<F, const E&>;
        if (has_value_) return Result<T, NewE>::ok(*value_);
        return Result<T, NewE>::err(f(*error_));
    }

    /// Transform the Err value using f (rvalue). Propagates Ok unchanged.
    template <typename F>
    auto map_error(F&& f) && -> Result<T, std::invoke_result_t<F, E&&>> {
        using NewE = std::invoke_result_t<F, E&&>;
        if (has_value_) return Result<T, NewE>::ok(std::move(*value_));
        return Result<T, NewE>::err(f(std::move(*error_)));
    }

private:
    Result() = default;

    bool has_value_ = true;
    std::optional<T> value_;
    std::optional<E> error_;
};

}  // namespace ccmake
