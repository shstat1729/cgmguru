// Portions of this file are derived from or structurally adapted from the
// GPL-2 licensed iglu R package. Original upstream copyright notices:
// Copyright (C) 2020 Texas A&M University.
// Copyright (C) 2023 The Regents of the University of Michigan.
// Modified for cgmguru in 2026.

#ifndef CGMGURU_EVENT_PREPROCESSING_H
#define CGMGURU_EVENT_PREPROCESSING_H

#include <Rcpp.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace cgmguru_events {

struct SegmentRange {
  int start;
  int end;
};

// One event interval as labelled by iglu::event_class().  start and end are
// zero-based, inclusive positions on one contiguous, non-missing segment.
struct EventRange {
  int start;
  int end;
};

struct PreparedIDData {
  Rcpp::NumericVector time;
  Rcpp::NumericVector glucose;
  Rcpp::IntegerVector segment;
  std::vector<SegmentRange> segments;
  double reading_minutes = 5.0;
};

struct InterpolatedDataStore {
  struct IDRun {
    std::string id;
    size_t length;
  };

  std::vector<IDRun> id_runs;
  std::vector<double> times;
  std::vector<double> glucose;
  std::vector<int> segments;
  std::vector<double> reading_minutes;
  bool metadata_available = true;

  void clear() {
    id_runs.clear();
    times.clear();
    glucose.clear();
    segments.clear();
    reading_minutes.clear();
    metadata_available = true;
  }

  void reserve_rows(size_t row_capacity, size_t run_capacity,
                    bool include_metadata = true) {
    id_runs.reserve(run_capacity);
    times.reserve(row_capacity);
    glucose.reserve(row_capacity);
    if (include_metadata) {
      segments.reserve(row_capacity);
      reading_minutes.reserve(row_capacity);
    }
  }

  static size_t grown_capacity(size_t current_capacity, size_t target_capacity) {
    if (current_capacity >= target_capacity) {
      return current_capacity;
    }
    size_t new_capacity = current_capacity == 0 ? target_capacity : current_capacity;
    while (new_capacity < target_capacity) {
      const size_t next_capacity = new_capacity + std::max<size_t>(new_capacity, 1024);
      if (next_capacity <= new_capacity) {
        return target_capacity;
      }
      new_capacity = next_capacity;
    }
    return new_capacity;
  }

  void reserve_for_append(size_t additional_rows, bool include_metadata) {
    const size_t target_rows = times.size() + additional_rows;
    const size_t target_runs = id_runs.size() + 1;

    if (id_runs.capacity() < target_runs) {
      id_runs.reserve(grown_capacity(id_runs.capacity(), target_runs));
    }
    if (times.capacity() < target_rows) {
      const size_t new_capacity = grown_capacity(times.capacity(), target_rows);
      times.reserve(new_capacity);
      glucose.reserve(new_capacity);
    }
    if (include_metadata && segments.capacity() < target_rows) {
      const size_t new_capacity = grown_capacity(segments.capacity(), target_rows);
      segments.reserve(new_capacity);
      reading_minutes.reserve(new_capacity);
    }
  }

  void append(const std::string& id, const PreparedIDData& prepared,
              bool include_metadata = true) {
    const int n = prepared.time.length();
    if (n <= 0) {
      return;
    }

    reserve_for_append(static_cast<size_t>(n), include_metadata);

    if (!id_runs.empty() && id_runs.back().id == id) {
      id_runs.back().length += static_cast<size_t>(n);
    } else {
      id_runs.push_back({id, static_cast<size_t>(n)});
    }

    if (!include_metadata) {
      metadata_available = false;
    }

    for (int i = 0; i < n; ++i) {
      times.push_back(prepared.time[i]);
      glucose.push_back(prepared.glucose[i]);
      if (include_metadata) {
        segments.push_back(prepared.segment[i]);
        reading_minutes.push_back(prepared.reading_minutes);
      }
    }
  }

  Rcpp::DataFrame to_dataframe(const std::string& tzone,
                               bool include_metadata = true) const {
    const R_xlen_t n = static_cast<R_xlen_t>(times.size());
    Rcpp::CharacterVector id_vec(n);
    R_xlen_t out_pos = 0;
    for (const IDRun& run : id_runs) {
      Rcpp::String id_string(run.id);
      for (size_t i = 0; i < run.length; ++i) {
        id_vec[out_pos++] = id_string;
      }
    }

    Rcpp::NumericVector time_vec = Rcpp::wrap(times);
    time_vec.attr("class") = Rcpp::CharacterVector::create("POSIXct", "POSIXt");
    time_vec.attr("tzone") = tzone;

    if (!include_metadata) {
      Rcpp::DataFrame df = Rcpp::DataFrame::create(
        Rcpp::_["id"] = id_vec,
        Rcpp::_["time"] = time_vec,
        Rcpp::_["gl"] = Rcpp::wrap(glucose)
      );
      df.attr("class") = Rcpp::CharacterVector::create("tbl_df", "tbl", "data.frame");
      return df;
    }

    if (!metadata_available ||
        segments.size() != times.size() ||
        reading_minutes.size() != times.size()) {
      Rcpp::stop("interpolated metadata was not stored");
    }

    Rcpp::DataFrame df = Rcpp::DataFrame::create(
      Rcpp::_["id"] = id_vec,
      Rcpp::_["time"] = time_vec,
      Rcpp::_["gl"] = Rcpp::wrap(glucose),
      Rcpp::_["segment"] = Rcpp::wrap(segments),
      Rcpp::_["reading_minutes"] = Rcpp::wrap(reading_minutes)
    );
    df.attr("class") = Rcpp::CharacterVector::create("tbl_df", "tbl", "data.frame");
    return df;
  }
};

inline bool is_na(double value) {
  return Rcpp::NumericVector::is_na(value);
}

// Mirrors iglu::event_class() for the standard hypo- and hyperglycemia
// definitions.  In particular, an episode ends immediately before the first
// recovery run that lasts end_length.  A shorter recovery run remains part of
// the labelled episode, even when another qualifying excursion follows it.
inline std::vector<EventRange> classify_event_ranges(
    const Rcpp::NumericVector& glucose,
    bool is_hypo,
    double threshold,
    double duration_minutes,
    double end_minutes,
    double reading_minutes) {
  const int n = glucose.length();
  std::vector<EventRange> ranges;
  if (n == 0) {
    return ranges;
  }

  const int event_readings = std::max(
    1, static_cast<int>(std::ceil(duration_minutes / reading_minutes))
  );
  const int recovery_readings = std::max(
    1, static_cast<int>(std::ceil(end_minutes / reading_minutes))
  );

  std::vector<bool> level(n, false);
  for (int i = 0; i < n; ++i) {
    if (is_na(glucose[i])) {
      continue;
    }
    level[i] = is_hypo ? glucose[i] < threshold : glucose[i] > threshold;
  }

  std::vector<int> starts;
  std::vector<int> recovery_starts;
  int run_start = 0;
  while (run_start < n) {
    const bool run_level = level[run_start];
    int run_end = run_start;
    while (run_end + 1 < n && level[run_end + 1] == run_level) {
      ++run_end;
    }
    const int run_length = run_end - run_start + 1;
    if (run_level && run_length >= event_readings) {
      starts.push_back(run_start);
    }
    if (!run_level && run_length >= recovery_readings) {
      recovery_starts.push_back(run_start);
    }
    run_start = run_end + 1;
  }

  // iglu constructs the end candidates as c(0, recovery_starts, n + 1),
  // selects the first candidate after each start, then keeps the first start
  // for duplicate end candidates.  The zero-based equivalent is below.
  std::set<int> used_ends;
  for (int start : starts) {
    int end = n - 1;
    for (int recovery_start : recovery_starts) {
      if (recovery_start > start) {
        end = recovery_start - 1;
        break;
      }
    }
    if (used_ends.insert(end).second) {
      ranges.push_back({start, end});
    }
  }

  return ranges;
}

inline Rcpp::IntegerVector event_range_markers(
    int length,
    const std::vector<EventRange>& ranges) {
  Rcpp::IntegerVector markers(length, 0);
  for (const EventRange& range : ranges) {
    if (range.start < 0 || range.end < range.start || range.end >= length) {
      continue;
    }
    markers[range.start] = 2;
    if (range.end != range.start) {
      markers[range.end] = -1;
    } else if (range.end + 1 < length) {
      // A one-reading episode needs a separate closing marker.  Its first
      // recovery reading is outside the labelled event but lets marker-based
      // consumers preserve the one-reading event before scanning backward to
      // the last excursion value.
      markers[range.end + 1] = -1;
    }
  }
  return markers;
}

inline int last_excursion_index(const Rcpp::NumericVector& glucose,
                                     const EventRange& range,
                                     bool is_hypo,
                                     double threshold) {
  for (int i = range.end; i >= range.start; --i) {
    if (is_na(glucose[i])) {
      continue;
    }
    const bool in_excursion = is_hypo ? glucose[i] < threshold : glucose[i] > threshold;
    if (in_excursion) {
      return i;
    }
  }
  return range.start;
}

inline void sort_or_validate_id_indices(std::map<std::string, std::vector<int>>& id_indices,
                                        const Rcpp::NumericVector& time,
                                        bool sort_time) {
  for (auto& id_pair : id_indices) {
    std::vector<int>& indices = id_pair.second;
    if (sort_time) {
      std::sort(indices.begin(), indices.end(), [&](int lhs, int rhs) {
        return time[lhs] < time[rhs];
      });
    } else {
      for (size_t i = 1; i < indices.size(); ++i) {
        const double prev_time = time[indices[i - 1]];
        const double current_time = time[indices[i]];
        if (!is_na(prev_time) && !is_na(current_time) && current_time < prev_time) {
          Rcpp::stop("time must be nondecreasing within each id when sort_time = FALSE");
        }
      }
    }
  }
}

inline double median_positive_diff_minutes(const Rcpp::NumericVector& time,
                                           const std::vector<int>& indices) {
  std::vector<double> diffs;
  diffs.reserve(indices.size() > 0 ? indices.size() - 1 : 0);

  for (size_t i = 1; i < indices.size(); ++i) {
    const double prev_time = time[indices[i - 1]];
    const double current_time = time[indices[i]];
    if (is_na(prev_time) || is_na(current_time)) continue;

    const double diff_minutes = (current_time - prev_time) / 60.0;
    if (diff_minutes > 0.0) {
      diffs.push_back(diff_minutes);
    }
  }

  if (diffs.empty()) {
    Rcpp::stop("reading_minutes could not be inferred: each id needs at least two distinct time points or an explicit reading_minutes value");
  }

  std::sort(diffs.begin(), diffs.end());
  const size_t n = diffs.size();
  const double median = (n % 2 == 1)
    ? diffs[n / 2]
    : (diffs[n / 2 - 1] + diffs[n / 2]) / 2.0;

  const double rounded = std::round(median);
  return rounded > 0.0 ? rounded : median;
}

inline double reading_minutes_for_id(SEXP reading_minutes_sexp,
                                     const Rcpp::NumericVector& time,
                                     const std::vector<int>& indices,
                                     int full_length) {
  if (reading_minutes_sexp == R_NilValue) {
    return median_positive_diff_minutes(time, indices);
  }

  if (TYPEOF(reading_minutes_sexp) == INTSXP) {
    Rcpp::IntegerVector reading_minutes_int = Rcpp::as<Rcpp::IntegerVector>(reading_minutes_sexp);
    if (reading_minutes_int.length() == 1) {
      return static_cast<double>(reading_minutes_int[0]);
    }
    if (reading_minutes_int.length() != full_length) {
      Rcpp::stop("reading_minutes vector length must match data length");
    }
    return static_cast<double>(reading_minutes_int[indices[0]]);
  }

  if (TYPEOF(reading_minutes_sexp) == REALSXP) {
    Rcpp::NumericVector reading_minutes_num = Rcpp::as<Rcpp::NumericVector>(reading_minutes_sexp);
    if (reading_minutes_num.length() == 1) {
      return reading_minutes_num[0];
    }
    if (reading_minutes_num.length() != full_length) {
      Rcpp::stop("reading_minutes vector length must match data length");
    }
    return reading_minutes_num[indices[0]];
  }

  Rcpp::stop("reading_minutes must be numeric or integer");
}

inline double recording_days(const Rcpp::NumericVector& glucose, double reading_minutes) {
  int valid_count = 0;
  for (int i = 0; i < glucose.length(); ++i) {
    if (!is_na(glucose[i])) ++valid_count;
  }
  return static_cast<double>(valid_count) * reading_minutes / (24.0 * 60.0);
}

inline double iglu_day_grid_reading_minutes(double reading_minutes) {
  double adjusted = reading_minutes;
  const double intervals_per_day = 1440.0 / adjusted;

  if (std::fabs(intervals_per_day - std::round(intervals_per_day)) > 1e-7) {
    if (adjusted > 20.0) {
      adjusted = 20.0;
    } else {
      const double remainder = std::fmod(adjusted, 5.0);
      if (remainder > 2.0) {
        adjusted = adjusted + 5.0 - remainder;
      } else {
        adjusted = adjusted - remainder;
      }
      if (adjusted <= 0.0) adjusted = reading_minutes;
    }
  }

  return adjusted;
}

inline Rcpp::NumericVector slice_numeric(const Rcpp::NumericVector& x, int start, int end) {
  const int n = end - start + 1;
  Rcpp::NumericVector out(n);
  for (int i = 0; i < n; ++i) {
    out[i] = x[start + i];
  }
  return out;
}

inline void compact_non_missing_rows(PreparedIDData& prepared) {
  int non_missing_count = 0;
  for (int i = 0; i < prepared.glucose.length(); ++i) {
    if (!is_na(prepared.glucose[i])) {
      ++non_missing_count;
    }
  }

  Rcpp::NumericVector compact_time(non_missing_count);
  Rcpp::NumericVector compact_glucose(non_missing_count);
  Rcpp::IntegerVector compact_segment(non_missing_count);

  int out_pos = 0;
  for (int i = 0; i < prepared.glucose.length(); ++i) {
    if (is_na(prepared.glucose[i])) {
      continue;
    }
    compact_time[out_pos] = prepared.time[i];
    compact_glucose[out_pos] = prepared.glucose[i];
    compact_segment[out_pos] = prepared.segment[i];
    ++out_pos;
  }

  prepared.time = compact_time;
  prepared.glucose = compact_glucose;
  prepared.segment = compact_segment;
  prepared.segments.clear();

  bool in_segment = false;
  int segment_start = -1;
  int current_segment_id = 0;

  for (int i = 0; i < prepared.segment.length(); ++i) {
    const int segment_id = prepared.segment[i];
    if (segment_id <= 0) {
      if (in_segment) {
        prepared.segments.push_back({segment_start, i - 1});
        in_segment = false;
        segment_start = -1;
        current_segment_id = 0;
      }
      continue;
    }

    if (!in_segment || segment_id != current_segment_id) {
      if (in_segment) {
        prepared.segments.push_back({segment_start, i - 1});
      }
      in_segment = true;
      segment_start = i;
      current_segment_id = segment_id;
    }
  }

  if (in_segment) {
    prepared.segments.push_back(
      {segment_start, static_cast<int>(prepared.segment.length()) - 1}
    );
  }
}

inline double local_midnight(double time_value, const std::string& tzone) {
  Rcpp::Environment base = Rcpp::Environment::base_env();
  Rcpp::Function as_posixlt = base["as.POSIXlt"];
  Rcpp::Function as_posixct = base["as.POSIXct"];

  const std::string tz = tzone.empty() ? "UTC" : tzone;
  Rcpp::NumericVector time_vec = Rcpp::NumericVector::create(time_value);
  time_vec.attr("class") = Rcpp::CharacterVector::create("POSIXct", "POSIXt");
  time_vec.attr("tzone") = tz;

  Rcpp::List lt = as_posixlt(time_vec, Rcpp::_["tz"] = tz);
  lt["sec"] = Rcpp::NumericVector::create(0.0);
  lt["min"] = Rcpp::IntegerVector::create(0);
  lt["hour"] = Rcpp::IntegerVector::create(0);

  Rcpp::NumericVector out = as_posixct(lt, Rcpp::_["tz"] = tz);
  return out[0];
}

inline PreparedIDData prepare_id_data(const Rcpp::NumericVector& time,
                                      const Rcpp::NumericVector& glucose,
                                      const std::vector<int>& indices,
                                      double reading_minutes,
                                      double inter_gap,
                                      const std::string& tzone,
                                      bool align_to_iglu_day_grid = false,
                                      bool drop_missing_rows = false) {
  PreparedIDData prepared;
  if (align_to_iglu_day_grid) {
    reading_minutes = iglu_day_grid_reading_minutes(reading_minutes);
  }
  prepared.reading_minutes = reading_minutes;

  if (indices.empty()) {
    prepared.time = Rcpp::NumericVector(0);
    prepared.glucose = Rcpp::NumericVector(0);
    prepared.segment = Rcpp::IntegerVector(0);
    return prepared;
  }

  std::vector<double> source_time;
  std::vector<double> source_glucose;
  source_time.reserve(indices.size());
  source_glucose.reserve(indices.size());

  for (int idx : indices) {
    const double t = time[idx];
    const double g = glucose[idx];
    if (is_na(t) || is_na(g)) continue;

    if (!source_time.empty() && std::fabs(t - source_time.back()) < 1e-7) {
      source_glucose.back() = g;
    } else {
      source_time.push_back(t);
      source_glucose.push_back(g);
    }
  }

  if (source_time.empty()) {
    prepared.time = Rcpp::NumericVector(0);
    prepared.glucose = Rcpp::NumericVector(0);
    prepared.segment = Rcpp::IntegerVector(0);
    return prepared;
  }

  const double dt_seconds = reading_minutes * 60.0;
  if (dt_seconds <= 0.0) {
    Rcpp::stop("reading_minutes must be positive");
  }

  const double first_time = source_time.front();
  const double last_time = source_time.back();
  const double time_epsilon = std::max(1e-7, dt_seconds * 1e-6);
  double grid_start = first_time;
  int grid_length = static_cast<int>(
    std::floor(((last_time - grid_start) + time_epsilon) / dt_seconds)
  ) + 1;

  if (align_to_iglu_day_grid) {
    const double first_midnight = local_midnight(first_time, tzone);
    grid_start = first_midnight + dt_seconds;
    const int ndays = static_cast<int>(
      std::ceil(((last_time - first_time) / (24.0 * 60.0 * 60.0)) + 1.0)
    );
    const int intervals_per_day = static_cast<int>(
      std::round((24.0 * 60.0) / reading_minutes)
    );
    grid_length = std::max(1, ndays * intervals_per_day);
  }
  if (grid_length < 1) grid_length = 1;

  prepared.time = Rcpp::NumericVector(grid_length);
  prepared.glucose = Rcpp::NumericVector(grid_length, NA_REAL);
  prepared.segment = Rcpp::IntegerVector(grid_length, 0);

  size_t source_pos = 0;
  for (int i = 0; i < grid_length; ++i) {
    const double target_time = grid_start + static_cast<double>(i) * dt_seconds;
    prepared.time[i] = target_time;

    while (source_pos + 1 < source_time.size() &&
           source_time[source_pos + 1] < target_time - time_epsilon) {
      ++source_pos;
    }

    if (std::fabs(target_time - source_time[source_pos]) <= time_epsilon) {
      prepared.glucose[i] = source_glucose[source_pos];
      continue;
    }

    if (source_pos + 1 < source_time.size()) {
      const double t0 = source_time[source_pos];
      const double t1 = source_time[source_pos + 1];

      if (std::fabs(target_time - t1) <= time_epsilon) {
        prepared.glucose[i] = source_glucose[source_pos + 1];
        continue;
      }

      if (target_time > t0 && target_time < t1) {
        const double gap_minutes = (t1 - t0) / 60.0;
        if (gap_minutes <= inter_gap + 1e-7) {
          const double fraction = (target_time - t0) / (t1 - t0);
          prepared.glucose[i] = source_glucose[source_pos] +
            fraction * (source_glucose[source_pos + 1] - source_glucose[source_pos]);
        }
      }
    }
  }

  int next_segment_id = 0;
  bool in_segment = false;
  int segment_start = -1;

  for (int i = 0; i < grid_length; ++i) {
    if (!is_na(prepared.glucose[i])) {
      if (!in_segment) {
        in_segment = true;
        segment_start = i;
        ++next_segment_id;
      }
      prepared.segment[i] = next_segment_id;
    } else if (in_segment) {
      prepared.segments.push_back({segment_start, i - 1});
      in_segment = false;
      segment_start = -1;
    }
  }

  if (in_segment) {
    prepared.segments.push_back({segment_start, grid_length - 1});
  }

  if (drop_missing_rows) {
    compact_non_missing_rows(prepared);
  }

  return prepared;
}

} // namespace cgmguru_events

#endif // CGMGURU_EVENT_PREPROCESSING_H
