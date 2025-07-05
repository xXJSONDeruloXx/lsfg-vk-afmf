#ifndef LOG_HPP
#define LOG_HPP

#include <format>
#include <iostream>

namespace Log {

    const std::string_view WHITE = "\033[1;37m";
    const std::string_view YELLOW = "\033[1;33m";
    const std::string_view RED = "\033[1;31m";
    const std::string_view GRAY = "\033[1;90m";
    const std::string_view RESET = "\033[0m";

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

}

#endif // LOG_HPP
