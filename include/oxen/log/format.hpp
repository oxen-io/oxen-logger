#pragma once

/// This header defines a ""_format user-defined literal that works similarly to fmt::format, but
/// with a more clever syntax:
///
///     fmt::format("xyz {}", 42);
///
/// becomes:
///
///     "xyz {}"_format(42);
///
/// There is also a `_format_to` that allows in-place appending to an existing string (or
/// something string-like):
///
///     "xyz {}"_format_to(somestr, 42);
///
/// is a shortcut for:
///
///     fmt::format_to(std::back_inserter(somestr), "{}", 42);
///
/// which is equivalent to (but more efficient than):
///
///     somestr += "xyz {}"_format(42);
///
/// The functions live in the `oxen::log::literals` namespace; you should use them via:
///
///     #include <oxen/log/format.hpp>
///     // ...
///     using namespace oxen::log::literals;
///
/// to make them available (the header/namespace is not included by default from oxen-logging
/// headers).

#include <array>
#include <iterator>
#include <string_view>

#include <fmt/core.h>

namespace oxen::log {

namespace detail {

    template <size_t N>
    struct string_literal {
        std::array<char, N> str;

        consteval string_literal(const char (&s)[N]) { std::copy(s, s + N, str.begin()); }

        consteval std::string_view sv() const { return {str.data(), N - 1}; }
    };

    // Internal implementation of _format that holds the format as a compile-time string in the type
    // itself; when the (...) operator gets called we give that off to fmt::format (and so just like
    // using fmt::format directly, you get compiler errors if the arguments do not match).
    template <string_literal Format>
    struct fmt_wrapper {
        consteval fmt_wrapper() = default;

        /// Calling on this object forwards all the values to fmt::format, using the format string
        /// as provided during type definition (via the "..."_format user-defined function).
        template <typename... T>
        constexpr auto operator()(T&&... args) && {
            return fmt::format(Format.sv(), std::forward<T>(args)...);
        }
    };

    template <string_literal Format>
    struct fmt_append_wrapper : fmt_wrapper<Format> {
        consteval fmt_append_wrapper() = default;

        template <typename String, typename... T>
        constexpr auto operator()(String& s, T&&... args) && {
            return fmt::format_to(std::back_inserter(s), Format.sv(), std::forward<T>(args)...);
        }
    };

}  // namespace detail

inline namespace literals {

    template <detail::string_literal Format>
    inline consteval auto operator""_format() {
        return detail::fmt_wrapper<Format>{};
    }

    template <detail::string_literal Format>
    inline consteval auto operator""_format_to() {
        return detail::fmt_append_wrapper<Format>{};
    }

}  // namespace literals

}  // namespace oxen::log
