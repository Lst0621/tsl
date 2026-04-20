#pragma once

#include <cstdio>
#include <string_view>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#else
#include <chrono>
#endif

class ScopedTimer {
   public:
    explicit ScopedTimer(std::string_view label) : label_(label) {
#ifdef __EMSCRIPTEN__
        start_ms_ = emscripten_get_now();
#else
        start_tp_ = std::chrono::steady_clock::now();
#endif
    }

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

    ~ScopedTimer() {
        const double elapsed_ms = elapsed_ms_now();
        std::fprintf(stderr, "[timer] %.*s: %.3f ms\n",
                     static_cast<int>(label_.size()), label_.data(),
                     elapsed_ms);
    }

   private:
    std::string_view label_;

#ifdef __EMSCRIPTEN__
    double start_ms_ = 0.0;
#else
    std::chrono::steady_clock::time_point start_tp_{};
#endif

    double elapsed_ms_now() const {
#ifdef __EMSCRIPTEN__
        const double now = emscripten_get_now();
        return now - start_ms_;
#else
        const auto now = std::chrono::steady_clock::now();
        const auto dt = now - start_tp_;
        return std::chrono::duration<double, std::milli>(dt).count();
#endif
    }
};

