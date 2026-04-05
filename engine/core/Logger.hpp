#pragma once

#include <fmt/core.h>
#include <fmt/color.h>
#include <string_view>

namespace hiromi::log {

template<typename... Args>
void info(fmt::format_string<Args...> fmt_str, Args&&... args) {
    fmt::print("[INFO]  {}\n", fmt::format(fmt_str, std::forward<Args>(args)...));
}

template<typename... Args>
void warn(fmt::format_string<Args...> fmt_str, Args&&... args) {
    fmt::print(fg(fmt::color::yellow), "[WARN]  {}\n",
               fmt::format(fmt_str, std::forward<Args>(args)...));
}

template<typename... Args>
void error(fmt::format_string<Args...> fmt_str, Args&&... args) {
    fmt::print(stderr, fg(fmt::color::red), "[ERROR] {}\n",
               fmt::format(fmt_str, std::forward<Args>(args)...));
}

template<typename... Args>
void debug([[maybe_unused]] fmt::format_string<Args...> fmt_str,
           [[maybe_unused]] Args&&... args) {
#ifndef NDEBUG
    fmt::print("[DEBUG] {}\n", fmt::format(fmt_str, std::forward<Args>(args)...));
#endif
}

} // namespace hiromi::log
