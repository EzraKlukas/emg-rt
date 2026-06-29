#ifndef TIMER_H
#define TIMER_H

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <vector>

#ifdef EMG_RT_ENABLE_PROFILING
#define EMG_RT_PROFILE(section)                                                \
  emg_rt::prof::ScopedTimer timer_##__LINE__(section)
#else
#define EMG_RT_PROFILE(section)                                                \
  do {                                                                         \
  } while (0)
#endif

inline constexpr std::size_t histogram_bins = 100;
inline constexpr uint64_t ns_per_10us = 10000;

namespace emg_rt::prof {

enum class Section : std::uint8_t {
  cycle,
  ring_write,
  samp_from_ring,
  init_pulse_t,
  extend,
  demean,
  pulse_train,
  is_local_max,
  thresholding,
  decompose,
  count
};

// Map enum values to string literals
constexpr std::string_view section_to_string(Section s) {
  switch (s) {
  case Section::cycle:
    return "cycle";
  case Section::ring_write:
    return "ring_write";
  case Section::samp_from_ring:
    return "samp_from_ring";
  case Section::init_pulse_t:
    return "init_pulse_t";
  case Section::extend:
    return "extend";
  case Section::demean:
    return "demean";
  case Section::pulse_train:
    return "pulse_train (get_pulse_t)";
  case Section::is_local_max:
    return "is_local_max";
  case Section::thresholding:
    return "thresholding (get_discharge_t)";
  case Section::decompose:
    return "decompose";
  case Section::count:
    return "count";
  default:
    return "Unknown";
  }
}

struct Stats {
  uint64_t count = 0;
  uint64_t sum_ns = 0;
  uint64_t min_ns = std::numeric_limits<uint64_t>::max();
  uint64_t max_ns = 0;
  std::vector<uint64_t> histogram = std::vector<uint64_t>(histogram_bins);

  void add(uint64_t ns) noexcept {
    ++count;
    sum_ns += ns;
    min_ns = std::min(ns, min_ns);
    max_ns = std::max(ns, max_ns);
    ++histogram[std::size_t(std::min(
        histogram.size() - 1, std::size_t(std::llround(ns / ns_per_10us))))];
  }

  double mean_ns() const noexcept {
    return count == 0 ? 0.0 : static_cast<double>(sum_ns) / (double)count;
  }
};

inline std::array<Stats, static_cast<std::size_t>(Section::count)> stats{};

// Helper to generate a text-based sparkline from a histogram
static std::string make_sparkline(const std::vector<uint64_t> &histogram) {
  if (histogram.empty()) {
    return "";
  }

  // Find the highest bin to scale the graph properly
  uint64_t max_val = *std::max_element(histogram.begin(), histogram.end());
  if (max_val == 0) {
    return std::string(histogram.size(), '_');
  }

  // Unicode block elements from empty/low to full block
  const std::vector<std::string> blocks = {" ", " ", "▂", "▃", "▄",
                                           "▅", "▆", "▇", "█"};
  std::string sparkline;
  sparkline.reserve(histogram.size() * 4); // Reserve space for UTF-8 chars

  for (uint64_t val : histogram) {
    if (val == 0) {
      sparkline += "_"; // Clear visual marker for completely empty bins
    } else {
      // Scale value to index between 1 and 8
      std::size_t idx =
          1 + static_cast<std::size_t>(std::round((val * 7.0) / max_val));
      sparkline += blocks[idx];
    }
  }
  return sparkline;
}

inline std::string summarize_stats() {
  std::string formatted;
  for (std::size_t i = 0; i < stats.size(); ++i) {
    Section section = static_cast<Section>(i);
    const Stats &stat = stats[i];
    formatted += std::format("{:35} | ", section_to_string(section));
    formatted += std::format("count: {:8} |", stat.count);
    formatted += std::format("min_ns: {:8} |", stat.min_ns);
    formatted += std::format("max_ns: {:8} |", stat.max_ns);
    formatted += std::format("mean_ns: {:7.2f}\n", stat.mean_ns());
    formatted += std::format("dist: [{}]\n", make_sparkline(stat.histogram));
  }
  return formatted;
}

inline uint64_t now_ns() noexcept {
  using clock = std::chrono::steady_clock;
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             clock::now().time_since_epoch())
      .count();
}

struct ScopedTimer {
  Section section;
  uint64_t start_ns;

  explicit ScopedTimer(Section s) noexcept : section(s), start_ns(now_ns()) {}

  ~ScopedTimer() noexcept {
    const uint64_t end_ns = now_ns();
    stats[static_cast<std::size_t>(section)].add(end_ns - start_ns);
  }
};

} // namespace emg_rt::prof

#endif // TIMER_H
