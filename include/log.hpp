#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <sstream>
#include <string>

// Check if std::format is available
#if __has_include(<format>) && __cpp_lib_format >= 202110L
#include <format>
#define HAS_STD_FORMAT 1
#else
#define HAS_STD_FORMAT 0
#endif

namespace Log {

    const std::string_view WHITE = "\033[1;37m";
    const std::string_view YELLOW = "\033[1;33m";
    const std::string_view RED = "\033[1;31m";
    const std::string_view GRAY = "\033[1;90m";
    const std::string_view RESET = "\033[0m";

#if HAS_STD_FORMAT
    template<typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) {
        std::cerr << WHITE << std::format(fmt, std::forward<Args>(args)...) << RESET << '\n';
    }

    template<typename... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args) {
        std::cerr << YELLOW << std::format(fmt, std::forward<Args>(args)...) << RESET << '\n';
    }

    template<typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) {
        std::cerr << RED << std::format(fmt, std::forward<Args>(args)...) << RESET << '\n';
    }

    template<typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args) {
        std::cerr << GRAY << std::format(fmt, std::forward<Args>(args)...) << RESET << '\n';
    }
#else
    // Simple replacement function for basic {} placeholders
    template<typename T>
    std::string simple_format_impl(std::string str, T&& value) {
        size_t pos = str.find("{}");
        if (pos != std::string::npos) {
            std::ostringstream oss;
            oss << std::forward<T>(value);
            str.replace(pos, 2, oss.str());
        }
        return str;
    }

    template<typename T, typename... Args>
    std::string simple_format_impl(std::string str, T&& value, Args&&... args) {
        size_t pos = str.find("{}");
        if (pos != std::string::npos) {
            std::ostringstream oss;
            oss << std::forward<T>(value);
            str.replace(pos, 2, oss.str());
        }
        return simple_format_impl(str, std::forward<Args>(args)...);
    }

    // Fallback implementations for older compilers
    template<typename... Args>
    void info(const std::string& fmt, Args&&... args) {
        if constexpr (sizeof...(Args) > 0) {
            std::cerr << WHITE << simple_format_impl(fmt, std::forward<Args>(args)...) << RESET << '\n';
        } else {
            std::cerr << WHITE << fmt << RESET << '\n';
        }
    }

    template<typename... Args>
    void warn(const std::string& fmt, Args&&... args) {
        if constexpr (sizeof...(Args) > 0) {
            std::cerr << YELLOW << simple_format_impl(fmt, std::forward<Args>(args)...) << RESET << '\n';
        } else {
            std::cerr << YELLOW << fmt << RESET << '\n';
        }
    }

    template<typename... Args>
    void error(const std::string& fmt, Args&&... args) {
        if constexpr (sizeof...(Args) > 0) {
            std::cerr << RED << simple_format_impl(fmt, std::forward<Args>(args)...) << RESET << '\n';
        } else {
            std::cerr << RED << fmt << RESET << '\n';
        }
    }

    template<typename... Args>
    void debug(const std::string& fmt, Args&&... args) {
        if constexpr (sizeof...(Args) > 0) {
            std::cerr << GRAY << simple_format_impl(fmt, std::forward<Args>(args)...) << RESET << '\n';
        } else {
            std::cerr << GRAY << fmt << RESET << '\n';
        }
    }
#endif

}

#endif // LOG_HPP
