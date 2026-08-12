#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

struct LatencyStats {
    double mean_ns;
    uint64_t p50_ns;
    uint64_t p90_ns;
    uint64_t p95_ns;
    uint64_t p99_ns;
    uint64_t max_ns;
};

inline LatencyStats compute_stats(std::vector<uint64_t>& samples) {
    if (samples.empty())
        return {};

    std::sort(samples.begin(), samples.end());

    double sum = 0;
    for (uint64_t v : samples) {
        sum += static_cast<double>(v);
    }

    auto get_pct = [&](double pct) -> uint64_t {
        size_t idx = static_cast<size_t>(std::ceil((pct / 100.0) * samples.size())) - 1;
        return samples[std::min(idx, samples.size() - 1)];
    };

    return {sum / static_cast<double>(samples.size()),
            get_pct(50.0),
            get_pct(90.0),
            get_pct(95.0),
            get_pct(99.0),
            samples.back()};
}

inline void print_stats(std::string_view label, const LatencyStats& stats) {
    std::cout << label << " Latency Profile:\n"
              << "  Mean : " << std::fixed << std::setprecision(1) << stats.mean_ns << " ns\n"
              << "  p50  : " << stats.p50_ns << " ns\n"
              << "  p90  : " << stats.p90_ns << " ns\n"
              << "  p95  : " << stats.p95_ns << " ns\n"
              << "  p99  : " << stats.p99_ns << " ns\n"
              << "  Max  : " << stats.max_ns << " ns\n";
}
