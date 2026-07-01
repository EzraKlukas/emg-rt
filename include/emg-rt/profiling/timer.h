/*
 * Compile-time scoped profiling utilities.
 *
 * Use `EMG_RT_PROFILE(section)` at the beginning of a scope to measure how long
 * that scope takes to execute. When profiling is enabled, the macro constructs
 * a `ScopedTimer`. The timer records its start time on construction and adds
 * the elapsed time to global profiling statistics when the scope exits.
 *
 * Example:
 *
 *   {
 *     EMG_RT_PROFILE(emg_rt::prof::Section::pulse_train);
 *     get_pulse_train(...);
 *   }
 *
 * Profiling is controlled by the `EMG_RT_ENABLE_PROFILING` compile definition:
 *
 *   - enabled:  `EMG_RT_PROFILE(...)` creates a real scoped timer
 *   - disabled: `EMG_RT_PROFILE(...)` compiles to a no-op
 *
 * This lets profiling markers remain in the source code without adding timing
 * overhead to normal builds.
 *
 * The collected statistics include:
 *
 *   - call count
 *   - minimum elapsed time
 *   - maximum elapsed time
 *   - mean elapsed time
 *   - histogram counts
 *
 * Print the accumulated results with:
 *
 *   std::cout << emg_rt::prof::summarize_stats();
 *
 * Note: a timer only records its result when its scope exits. If you profile
 * the whole body of `main`, print the summary after the timer's scope has
 * ended.
 */

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

#define EMG_RT_CONCAT_IMPL(x, y) x##y
#define EMG_RT_CONCAT(x, y) EMG_RT_CONCAT_IMPL(x, y)

#ifdef EMG_RT_ENABLE_PROFILING
#define EMG_RT_PROFILE(section)                                                \
  emg_rt::prof::ScopedTimer EMG_RT_CONCAT(_emg_rt_prof_timer_,                 \
                                          __LINE__)(section)
#else
#define EMG_RT_PROFILE(section)                                                \
  do {                                                                         \
  } while (0)
#endif

#define NS_PER_BIN_DEFAULT 4000
#define HISTOGRAM_BINS_DEFAULT 25

namespace emg_rt::prof {

enum class Section : std::uint8_t {
  cycle,
  ring_write,
  samp_from_ring,
  init_pulse_train,
  extend,
  demean,
  pulse_train,
  is_local_max,
  thresholding,
  decompose,
  count
};

constexpr std::string_view section_to_string(Section s) {
  switch (s) {
  case Section::cycle:
    return "cycle";
  case Section::ring_write:
    return "ring_write";
  case Section::samp_from_ring:
    return "samp_from_ring";
  case Section::init_pulse_train:
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

// histogram_bins: the number of time buckets to profile a scope's runtime into
inline std::size_t histogram_bins = HISTOGRAM_BINS_DEFAULT;
inline uint64_t ns_per_bin = NS_PER_BIN_DEFAULT;

struct Stats {
  uint64_t count = 0;
  uint64_t sum_ns = 0;
  uint64_t min_ns = std::numeric_limits<uint64_t>::max();
  uint64_t max_ns = 0;
  std::vector<uint64_t> histogram = std::vector<uint64_t>(histogram_bins);

  // Logs ns (representing time elapsed in a ScopeTimer's life) to Stats.
  void add(uint64_t ns) noexcept {
    ++count;
    sum_ns += ns;
    min_ns = std::min(ns, min_ns);
    max_ns = std::max(ns, max_ns);
    ++histogram[std::size_t(std::min(
        histogram.size() - 1, std::size_t(std::llround(ns / ns_per_bin))))];
  }

  double mean_ns() const noexcept {
    return count == 0 ? 0.0 : static_cast<double>(sum_ns) / (double)count;
  }
};

// This is the persistent global object storing persistent profiling data.
inline std::array<Stats, static_cast<std::size_t>(Section::count)> stats{};

inline std::string histogram_to_string(const std::vector<uint64_t> &histogram) {
  std::string out = "[";

  for (std::size_t i = 0; i < histogram.size(); ++i) {
    if (i != 0) {
      out += ", ";
    }

    out += std::format("{}", histogram[i]);
  }

  out += "]";
  return out;
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
    formatted += std::format("histogram, ({} ns / bin): {}\n", ns_per_bin,
                             histogram_to_string(stat.histogram));
  }
  return formatted;
}

inline uint64_t now_ns() noexcept {
  using clock = std::chrono::steady_clock;
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             clock::now().time_since_epoch())
      .count();
}

// This is the object which survives only the scope in which it's initialized,
// but who writes its profiling data contribution to the stats global object
// upon going out of scope.
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
