#pragma once

#include <array>
#include <memory>
#include <spdlog/spdlog.h>
#include "type.hpp"
#include "level.hpp"

#if __has_include(<version>)
#include <version>
#endif

#ifdef __cpp_lib_source_location

#include <source_location>
namespace oxen::log {
using source_location = std::source_location;
}

#elif __has_include(<experimental/source_location>)

#include <experimental/source_location>
#ifdef __cpp_lib_experimental_source_location
namespace oxen::log {
using source_location = std::experimental::source_location;
}
#else
#error "Unable to find a working <source_location> or <experimental/source_location>"
#endif

#else
#error "This compiler and/or stdlib does not support <source_location>"
#endif

namespace oxen::log {
using logger_ptr = std::shared_ptr<spdlog::logger>;
}

namespace oxen::log::detail {

/** internal */

spdlog::sink_ptr make_sink(Type type, const std::string& file);

bool is_ansicolor_sink(const spdlog::sink_ptr& sink);

inline auto spdlog_sloc(const source_location& loc) {
    std::string_view filename{loc.file_name()};
    if (auto pos = filename.rfind('/'); pos != std::string_view::npos)
        filename.remove_prefix(pos + 1);
    return spdlog::source_loc{filename.data(), static_cast<int>(loc.line()), loc.function_name()};
}

inline void make_lc(std::string& s) {
    for (char& c : s)
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
}

}  // namespace oxen::log::detail
