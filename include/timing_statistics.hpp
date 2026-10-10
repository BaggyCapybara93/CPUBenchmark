#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <vector>

struct timing_statistics {
    double mean = 0.0;
    double min = 0.0;
    double max = 0.0;
    double median = 0.0;
    double std_dev = 0.0;
    double p95 = 0.0;
    double p99 = 0.0;
    std::size_t count = 0;
};

inline timing_statistics calculate_timing_statistics(std::vector<double> samples) {
    timing_statistics stats;
    if (samples.empty()) {
        return stats;
    }

    std::sort(samples.begin(), samples.end());
    stats.count = samples.size();
    stats.min = samples.front();
    stats.max = samples.back();
    stats.mean = std::accumulate(samples.begin(), samples.end(), 0.0) /
                 static_cast<double>(samples.size());

    double variance = 0.0;
    for (double sample : samples) {
        variance += (sample - stats.mean) * (sample - stats.mean);
    }
    stats.std_dev = std::sqrt(variance / static_cast<double>(samples.size()));

    const auto percentile = [&samples](double fraction) {
        const double position = fraction * static_cast<double>(samples.size() - 1);
        const auto lower = static_cast<std::size_t>(std::floor(position));
        const auto upper = static_cast<std::size_t>(std::ceil(position));
        const double weight = position - static_cast<double>(lower);
        return samples[lower] + weight * (samples[upper] - samples[lower]);
    };
    stats.median = percentile(0.5);
    stats.p95 = percentile(0.95);
    stats.p99 = percentile(0.99);
    return stats;
}
