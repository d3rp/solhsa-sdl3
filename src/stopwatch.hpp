#pragma once

#include <iostream>
#include <locale>
#include <iomanip>
#include <sstream>

namespace stopwatch
{
// Function to format an integer with space as thousands separator
std::string format_with_space(long long number)
{
    // Custom numpunct facet to use space as thousands separator
    struct SpaceSep : std::numpunct<char>
    {
        char do_thousands_sep() const override { return ' '; }

        std::string do_grouping() const override { return "\3"; }
    };

    std::stringstream ss;
    ss.imbue(std::locale(std::locale::classic(), new SpaceSep));
    ss << number;
    return ss.str();
}

const auto metric_time_unit = [](const auto value) {
    return value < 1'000'000L ? "ms" : value < 1'000'000'000L ? "µs" : "ns";
};
// wrap a function into a timed context
template <typename Unit>
auto time_in_ = [](std::string label, auto func, auto&... args)
{
    using Clock                 = std::chrono::high_resolution_clock;
    const auto then             = Clock::now();
    auto result                 = func(args...);
    const auto elapsed          = Clock::now() - then;
    const auto elapsed_in_units = std::chrono::duration_cast<Unit>(elapsed);

    std::cout << std::setw(12) << label << ": " << std::setw(8)
              << format_with_space((elapsed_in_units).count()) << " "
              << metric_time_unit(typename Unit::period().den) << "\n";

    return std::move(result);
};
auto time_in_s    = time_in_<std::chrono::seconds>;
auto time_in_ms   = time_in_<std::chrono::milliseconds>;
auto time_in_mcrs = time_in_<std::chrono::microseconds>;
auto time_in_ns   = time_in_<std::chrono::nanoseconds>;

// Wall clock benchmark
struct Measurement
{
    using Clock = std::chrono::high_resolution_clock;

    Measurement(Clock::duration& _sum, int& _n)
        : sum(_sum)
    {
        _n++;
        then = Clock::now();
    }

    ~Measurement() { sum += Clock::now() - then; }

    Clock::time_point then;
    Clock::duration& sum;
};

struct Aggregate
{
    explicit Aggregate(std::string _label)
        : label(std::move(_label))
    {
    }

    ~Aggregate() {
        volatile static auto _ = print_header();
        print_stats();
    }

    int print_header() const
    {
        // clang-format off
        std::cout
            << std::setw(15) << "label"
            << std::setw(3) << " | " << std::setw(8) << "calls"
            << std::setw(3) << " | " << std::setw(17) << "total time"
            << std::setw(3) << " | " << std::setw(15) << "avg"
            << "\n";
        // clang-format on
        return 0;
    }

    void print_stats() const
    {
        auto ticks_total = sum.count();
        auto time_unit   = metric_time_unit(Measurement::Clock::period::den);

        std::stringstream total_time;
        total_time << format_with_space(sum.count()) << " " << time_unit;

        std::stringstream avg_in_units;
        avg_in_units << format_with_space(float(sum.count()) / float(measurements)) << " " << time_unit;

        // clang-format off

        std::cout
            << std::setw(15) << label
            << std::setw(3) << " | " << std::setw(8) << std::to_string(measurements)
            << std::setw(3) << " | " << std::setw(17) << total_time.str()
            << std::setw(3) << " | "     << std::setw(15) << avg_in_units.str()
            << "\n";
        // clang-format on
    }

    Measurement::Clock::duration sum { 0 };
    std::string label;
    int measurements = 0;
};
} // namespace stopwatch
#ifndef STOPWATCH
#define STOPWATCH(label)                                                  \
        static stopwatch::Aggregate stopwatch_aggregate(label);               \
        stopwatch::Measurement stopwatch_measurement(stopwatch_aggregate.sum, \
                                                     stopwatch_aggregate.measurements);
#endif