#include "event_preprocessing.h"

#include <Rcpp.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <string>
#include <vector>

using namespace Rcpp;

namespace {

struct MageRow {
  double start;
  double end;
  double mage;
  std::string plus_or_minus;
  int first_excursion;
};

struct SegmentBounds {
  int start;
  int end;
};

std::string timezone_from_time_or_arg(const NumericVector& time,
                                      const std::string& tz_arg) {
  if (!tz_arg.empty()) {
    return tz_arg;
  }

  std::string tzone = "UTC";
  RObject tz_attr = time.attr("tzone");
  if (!tz_attr.isNULL()) {
    CharacterVector tz_values = as<CharacterVector>(tz_attr);
    if (tz_values.size() > 0 && !CharacterVector::is_na(tz_values[0])) {
      tzone = as<std::string>(tz_values[0]);
    }
  }

  if (tzone.empty()) {
    tzone = "UTC";
  }
  return tzone;
}

void set_posixct_attributes(NumericVector& x, const std::string& tzone) {
  x.attr("class") = CharacterVector::create("POSIXct", "POSIXt");
  x.attr("tzone") = tzone.empty() ? "UTC" : tzone;
}

void set_tibble_class(DataFrame& x) {
  x.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
}

bool is_missing(double x) {
  return NumericVector::is_na(x) || std::isnan(x);
}

double sample_sd(const std::vector<double>& values) {
  double sum = 0.0;
  int n = 0;
  for (double value : values) {
    if (is_missing(value)) continue;
    sum += value;
    ++n;
  }
  if (n < 2) {
    return NA_REAL;
  }

  const double mean = sum / static_cast<double>(n);
  double ss = 0.0;
  for (double value : values) {
    if (is_missing(value)) continue;
    const double centered = value - mean;
    ss += centered * centered;
  }

  return std::sqrt(ss / static_cast<double>(n - 1));
}

double mean_or_nan(const std::vector<double>& values) {
  double sum = 0.0;
  int n = 0;
  for (double value : values) {
    if (is_missing(value)) continue;
    sum += value;
    ++n;
  }
  if (n == 0) {
    return R_NaN;
  }
  return sum / static_cast<double>(n);
}

double mean_or_na(const std::vector<double>& values) {
  double sum = 0.0;
  int n = 0;
  for (double value : values) {
    if (is_missing(value)) continue;
    sum += value;
    ++n;
  }
  if (n == 0) {
    return NA_REAL;
  }
  return sum / static_cast<double>(n);
}

std::map<std::string, std::vector<int>> valid_id_indices(const DataFrame& df) {
  const int n = df.nrows();
  StringVector id = df["id"];
  NumericVector time = df["time"];
  NumericVector glucose = df["gl"];

  std::map<std::string, std::vector<int>> id_indices;
  for (int i = 0; i < n; ++i) {
    if (StringVector::is_na(id[i]) ||
        cgmguru_events::is_na(time[i]) ||
        cgmguru_events::is_na(glucose[i])) {
      continue;
    }
    id_indices[as<std::string>(id[i])].push_back(i);
  }

  for (auto& id_pair : id_indices) {
    std::vector<int>& indices = id_pair.second;
    std::sort(indices.begin(), indices.end(), [&](int lhs, int rhs) {
      return time[lhs] < time[rhs];
    });
  }

  return id_indices;
}

double calculate_conga_for_id(const NumericVector& time,
                              const NumericVector& glucose,
                              const std::vector<int>& indices,
                              int hours,
                              double inter_gap,
                              const std::string& tzone,
                              int full_length) {
  if (indices.size() < 2) {
    return NA_REAL;
  }

  double reading_minutes =
    cgmguru_events::reading_minutes_for_id(R_NilValue, time, indices, full_length);
  if (reading_minutes > inter_gap + 1e-7) {
    stop("identified measurement frequency is above inter_gap");
  }
  reading_minutes = cgmguru_events::iglu_day_grid_reading_minutes(reading_minutes);

  cgmguru_events::PreparedIDData prepared =
    cgmguru_events::prepare_id_data(time, glucose, indices, reading_minutes,
                                    inter_gap, tzone, true, false);
  const int lag = static_cast<int>(
    std::round(60.0 / prepared.reading_minutes)
  ) * hours;

  if (lag <= 0 || prepared.glucose.length() <= lag) {
    return NA_REAL;
  }

  std::vector<double> differences;
  differences.reserve(prepared.glucose.length() - lag);
  for (int i = lag; i < prepared.glucose.length(); ++i) {
    const double current = prepared.glucose[i];
    const double previous = prepared.glucose[i - lag];
    if (is_missing(current) || is_missing(previous)) {
      continue;
    }
    differences.push_back(current - previous);
  }

  return sample_sd(differences);
}

std::vector<double> rolling_mean_right(const std::vector<double>& glucose,
                                       int width) {
  const int n = static_cast<int>(glucose.size());
  std::vector<double> out(n, NA_REAL);
  if (width <= 0 || n == 0 || width > n) {
    return out;
  }

  for (int i = width - 1; i < n; ++i) {
    double sum = 0.0;
    int count = 0;
    for (int j = i - width + 1; j <= i; ++j) {
      if (is_missing(glucose[j])) continue;
      sum += glucose[j];
      ++count;
    }
    out[i] = count == 0 ? NA_REAL : sum / static_cast<double>(count);
  }

  const double first_complete = out[width - 1];
  for (int i = 0; i < width && i < n; ++i) {
    out[i] = first_complete;
  }

  return out;
}

MageRow na_mage_row(const std::vector<double>& time) {
  const double start = time.empty() ? NA_REAL : time.front();
  const double end = time.empty() ? NA_REAL : time.back();
  return {start, end, NA_REAL, "", NA_LOGICAL};
}

std::pair<double, int> interval_extreme(const std::vector<double>& glucose,
                                        int start,
                                        int end,
                                        bool find_minimum) {
  start = std::max(start, 0);
  end = std::min(end, static_cast<int>(glucose.size()) - 1);
  if (start > end) {
    return {NA_REAL, start};
  }

  double best = find_minimum
    ? std::numeric_limits<double>::infinity()
    : -std::numeric_limits<double>::infinity();
  int best_index = -1;

  for (int i = start; i <= end; ++i) {
    const double value = glucose[i];
    if (is_missing(value)) continue;
    if (best_index < 0 ||
        (find_minimum ? value < best : value > best)) {
      best = value;
      best_index = i;
    }
  }

  if (best_index < 0) {
    return {NA_REAL, start};
  }
  return {best, best_index};
}

double extrema_difference(const std::vector<double>& minmax,
                          int row,
                          int col) {
  if (row < 0 || col < 0 ||
      row >= static_cast<int>(minmax.size()) ||
      col >= static_cast<int>(minmax.size()) ||
      is_missing(minmax[row]) || is_missing(minmax[col])) {
    return NA_REAL;
  }
  return minmax[col] - minmax[row];
}

std::vector<MageRow> mage_atomic(const std::vector<double>& time,
                                 const std::vector<double>& glucose,
                                 int short_ma,
                                 int long_ma) {
  const int n = static_cast<int>(glucose.size());
  if (n == 0) {
    return std::vector<MageRow>{na_mage_row(time)};
  }

  bool any_glucose = false;
  std::vector<double> valid_glucose;
  valid_glucose.reserve(n);
  for (double value : glucose) {
    if (!is_missing(value)) {
      any_glucose = true;
      valid_glucose.push_back(value);
    }
  }

  if (!any_glucose || n < 7 || n < long_ma) {
    return std::vector<MageRow>{na_mage_row(time)};
  }

  const double standardD = sample_sd(valid_glucose);
  if (is_missing(standardD) || standardD < 1.0) {
    return std::vector<MageRow>{na_mage_row(time)};
  }

  const std::vector<double> short_mean = rolling_mean_right(glucose, short_ma);
  const std::vector<double> long_mean = rolling_mean_right(glucose, long_ma);

  std::vector<double> delta(n, NA_REAL);
  for (int i = 0; i < n; ++i) {
    if (!is_missing(short_mean[i]) && !is_missing(long_mean[i])) {
      delta[i] = short_mean[i] - long_mean[i];
    }
  }

  const int rel_min = 0;
  const int rel_max = 1;
  std::vector<int> cross_pos;
  std::vector<int> cross_type;
  cross_pos.reserve(n + 2);
  cross_type.reserve(n + 2);

  cross_pos.push_back(0);
  cross_type.push_back(!is_missing(delta[0]) && delta[0] > 0.0 ? rel_max : rel_min);

  for (int i = 1; i < n; ++i) {
    if (is_missing(glucose[i]) || is_missing(glucose[i - 1]) ||
        is_missing(delta[i]) || is_missing(delta[i - 1])) {
      continue;
    }

    const double adjacent_product = delta[i] * delta[i - 1];
    if (adjacent_product < 0.0) {
      cross_pos.push_back(i);
      cross_type.push_back(delta[i] < delta[i - 1] ? rel_min : rel_max);
    } else {
      const double previous_cross_delta = delta[cross_pos.back()];
      if (!is_missing(previous_cross_delta) &&
          delta[i] * previous_cross_delta < 0.0) {
        cross_pos.push_back(i);
        cross_type.push_back(delta[i] < previous_cross_delta ? rel_min : rel_max);
      }
    }
  }

  cross_pos.push_back(n - 1);
  cross_type.push_back(!is_missing(delta[n - 1]) && delta[n - 1] > 0.0 ? rel_max : rel_min);

  const int num_extrema = static_cast<int>(cross_pos.size()) - 1;
  if (num_extrema <= 0) {
    return std::vector<MageRow>{na_mage_row(time)};
  }

  std::vector<double> minmax(num_extrema, NA_REAL);
  std::vector<int> extrema_indexes(num_extrema, 0);

  for (int i = 0; i < num_extrema; ++i) {
    const int s1 = i == 0 ? cross_pos[i] : extrema_indexes[i - 1];
    const int s2 = cross_pos[i + 1];
    const bool find_minimum = cross_type[i] == rel_min;
    const std::pair<double, int> extreme =
      interval_extreme(glucose, s1, s2, find_minimum);
    minmax[i] = extreme.first;
    extrema_indexes[i] = extreme.second;
  }

  std::vector<double> mage_plus_heights;
  std::vector<std::pair<int, int>> mage_plus_pairs;
  int j = 0;
  int prev_j = 0;
  while (j < num_extrema) {
    double max_v = -std::numeric_limits<double>::infinity();
    int max_i = prev_j;
    bool found = false;
    for (int r = prev_j; r <= j; ++r) {
      const double value = extrema_difference(minmax, r, j);
      if (is_missing(value)) continue;
      if (!found || value > max_v) {
        max_v = value;
        max_i = r;
        found = true;
      }
    }

    if (found && max_v >= standardD) {
      for (int k = j; k < num_extrema; ++k) {
        if (!is_missing(minmax[k]) && !is_missing(minmax[j]) &&
            minmax[k] > minmax[j]) {
          j = k;
        }
        const double forward_diff = extrema_difference(minmax, j, k);
        if ((!is_missing(forward_diff) && forward_diff < -standardD) ||
            k == num_extrema - 1) {
          const double height = minmax[j] - minmax[max_i];
          if (!is_missing(height)) {
            mage_plus_heights.push_back(height);
            mage_plus_pairs.push_back(std::make_pair(max_i, j));
          }
          prev_j = k;
          j = k;
          break;
        }
      }
    } else {
      ++j;
    }
  }

  std::vector<double> mage_minus_heights;
  std::vector<std::pair<int, int>> mage_minus_pairs;
  j = 0;
  prev_j = 0;
  while (j < num_extrema) {
    double min_v = std::numeric_limits<double>::infinity();
    int min_i = prev_j;
    bool found = false;
    for (int r = prev_j; r <= j; ++r) {
      const double value = extrema_difference(minmax, r, j);
      if (is_missing(value)) continue;
      if (!found || value < min_v) {
        min_v = value;
        min_i = r;
        found = true;
      }
    }

    if (found && min_v <= -standardD) {
      for (int k = j; k < num_extrema; ++k) {
        if (!is_missing(minmax[k]) && !is_missing(minmax[j]) &&
            minmax[k] < minmax[j]) {
          j = k;
        }
        const double forward_diff = extrema_difference(minmax, j, k);
        if ((!is_missing(forward_diff) && forward_diff > standardD) ||
            k == num_extrema - 1) {
          const double height = minmax[j] - minmax[min_i];
          if (!is_missing(height)) {
            mage_minus_heights.push_back(height);
            mage_minus_pairs.push_back(std::make_pair(min_i, j));
          }
          prev_j = j;
          j = k;
          break;
        }
      }
    } else {
      ++j;
    }
  }

  if (mage_minus_heights.empty() && mage_plus_heights.empty()) {
    return std::vector<MageRow>{na_mage_row(time)};
  }

  const bool is_plus_first =
    !mage_plus_heights.empty() &&
    (mage_minus_heights.empty() ||
     mage_plus_pairs.front().second <= mage_minus_pairs.front().first);

  const double mage_plus = mage_plus_heights.empty()
    ? NA_REAL
    : mean_or_na(mage_plus_heights);
  const double mage_minus = mage_minus_heights.empty()
    ? NA_REAL
    : std::fabs(mean_or_na(mage_minus_heights));

  return std::vector<MageRow>{
    {time.front(), time.back(), mage_plus, "PLUS", is_plus_first ? TRUE : FALSE},
    {time.front(), time.back(), mage_minus, "MINUS", is_plus_first ? FALSE : TRUE}
  };
}

std::vector<SegmentBounds> mage_segments(const std::vector<double>& glucose,
                                         double interval_minutes,
                                         double max_gap) {
  const int n = static_cast<int>(glucose.size());
  std::vector<SegmentBounds> qualifying_gaps;
  int i = 0;
  while (i < n) {
    const bool is_gap = is_missing(glucose[i]);
    int j = i + 1;
    while (j < n && is_missing(glucose[j]) == is_gap) {
      ++j;
    }
    const int run_length = j - i;
    if (is_gap && static_cast<double>(run_length) * interval_minutes > max_gap) {
      qualifying_gaps.push_back({i, j - 1});
    }
    i = j;
  }

  if (qualifying_gaps.empty()) {
    return std::vector<SegmentBounds>{{0, n - 1}};
  }

  std::vector<SegmentBounds> segments;
  for (size_t gap_index = 0; gap_index < qualifying_gaps.size(); ++gap_index) {
    const SegmentBounds current_gap = qualifying_gaps[gap_index];
    if (gap_index == 0 && current_gap.start > 0) {
      segments.push_back({0, current_gap.start - 1});
    }

    segments.push_back(current_gap);

    if (current_gap.end < n - 1) {
      const int data_start = current_gap.end + 1;
      const int data_end = gap_index == qualifying_gaps.size() - 1
        ? n - 1
        : qualifying_gaps[gap_index + 1].start - 1;
      if (data_start <= data_end) {
        segments.push_back({data_start, data_end});
      }
    }
  }

  return segments;
}

std::vector<MageRow> calculate_mage_ma_for_id(const NumericVector& time,
                                              const NumericVector& glucose,
                                              const std::vector<int>& indices,
                                              int short_ma,
                                              int long_ma,
                                              double inter_gap,
                                              double max_gap,
                                              const std::string& tzone) {
  cgmguru_events::PreparedIDData prepared =
    cgmguru_events::prepare_id_data(time, glucose, indices, 5.0,
                                    inter_gap, tzone, true, false);

  const int n = prepared.glucose.length();
  int first_valid = -1;
  int last_valid = -1;
  for (int i = 0; i < n; ++i) {
    if (!is_missing(prepared.glucose[i])) {
      if (first_valid < 0) first_valid = i;
      last_valid = i;
    }
  }

  if (first_valid < 0 || last_valid < first_valid) {
    return std::vector<MageRow>{na_mage_row(std::vector<double>())};
  }

  std::vector<double> trimmed_time;
  std::vector<double> trimmed_glucose;
  trimmed_time.reserve(last_valid - first_valid + 1);
  trimmed_glucose.reserve(last_valid - first_valid + 1);
  for (int i = first_valid; i <= last_valid; ++i) {
    trimmed_time.push_back(prepared.time[i]);
    trimmed_glucose.push_back(prepared.glucose[i]);
  }

  if (static_cast<int>(trimmed_glucose.size()) < 7) {
    return std::vector<MageRow>{na_mage_row(trimmed_time)};
  }

  if (short_ma >= long_ma) {
    warning("The short moving average window size should be smaller than the long moving average window size for correct MAGE calculation. Swapping automatically.");
    std::swap(short_ma, long_ma);
  }

  if (static_cast<int>(trimmed_glucose.size()) < long_ma) {
    return std::vector<MageRow>{na_mage_row(trimmed_time)};
  }

  const std::vector<SegmentBounds> segments =
    mage_segments(trimmed_glucose, 5.0, max_gap);

  std::vector<MageRow> rows;
  for (const SegmentBounds& segment : segments) {
    std::vector<double> segment_time;
    std::vector<double> segment_glucose;
    segment_time.reserve(segment.end - segment.start + 1);
    segment_glucose.reserve(segment.end - segment.start + 1);
    for (int i = segment.start; i <= segment.end; ++i) {
      segment_time.push_back(trimmed_time[i]);
      segment_glucose.push_back(trimmed_glucose[i]);
    }

    std::vector<MageRow> segment_rows =
      mage_atomic(segment_time, segment_glucose, short_ma, long_ma);
    rows.insert(rows.end(), segment_rows.begin(), segment_rows.end());
  }

  return rows;
}

DataFrame mage_rows_to_dataframe(const std::vector<MageRow>& rows,
                                 const std::string& tzone) {
  const int n = static_cast<int>(rows.size());
  NumericVector start(n);
  NumericVector end(n);
  NumericVector mage(n);
  CharacterVector plus_or_minus(n);
  LogicalVector first_excursion(n);

  for (int i = 0; i < n; ++i) {
    start[i] = rows[i].start;
    end[i] = rows[i].end;
    mage[i] = rows[i].mage;
    if (rows[i].plus_or_minus.empty()) {
      plus_or_minus[i] = NA_STRING;
    } else {
      plus_or_minus[i] = rows[i].plus_or_minus;
    }
    first_excursion[i] = rows[i].first_excursion;
  }

  set_posixct_attributes(start, tzone);
  set_posixct_attributes(end, tzone);

  DataFrame out = DataFrame::create(
    _["start"] = start,
    _["end"] = end,
    _["mage"] = mage,
    _["plus_or_minus"] = plus_or_minus,
    _["first_excursion"] = first_excursion
  );
  set_tibble_class(out);
  return out;
}

double summarize_mage_rows(const std::vector<MageRow>& rows,
                           const std::string& direction) {
  std::vector<MageRow> selected;

  if (direction == "plus" || direction == "minus") {
    const std::string target = direction == "plus" ? "PLUS" : "MINUS";
    for (const MageRow& row : rows) {
      if (row.plus_or_minus == target) {
        selected.push_back(row);
      }
    }
  } else if (direction == "avg") {
    for (const MageRow& row : rows) {
      if (!is_missing(row.mage)) {
        selected.push_back(row);
      }
    }
  } else if (direction == "service") {
    for (const MageRow& row : rows) {
      if (row.first_excursion == TRUE) {
        selected.push_back(row);
      }
    }
  } else if (direction == "max") {
    std::map<std::string, std::vector<MageRow>> by_segment;
    for (const MageRow& row : rows) {
      const std::string key =
        std::to_string(row.start) + "\r" + std::to_string(row.end);
      by_segment[key].push_back(row);
    }

    for (const auto& segment_pair : by_segment) {
      const std::vector<MageRow>& segment_rows = segment_pair.second;
      bool any_missing = false;
      double segment_max = -std::numeric_limits<double>::infinity();
      bool has_value = false;
      for (const MageRow& row : segment_rows) {
        if (is_missing(row.mage)) {
          any_missing = true;
          break;
        }
        if (!has_value || row.mage > segment_max) {
          segment_max = row.mage;
          has_value = true;
        }
      }
      if (any_missing || !has_value) {
        continue;
      }
      for (const MageRow& row : segment_rows) {
        if (row.mage == segment_max) {
          selected.push_back(row);
        }
      }
    }
  }

  if (selected.empty()) {
    return NA_REAL;
  }

  bool any_missing_mage = false;
  double total_duration = 0.0;
  for (const MageRow& row : selected) {
    if (is_missing(row.mage)) {
      any_missing_mage = true;
    }
    const double duration = row.end - row.start;
    if (!is_missing(duration) && duration > 0.0) {
      total_duration += duration;
    }
  }

  if (any_missing_mage || total_duration <= 0.0) {
    return NA_REAL;
  }

  double weighted_sum = 0.0;
  for (const MageRow& row : selected) {
    const double duration = row.end - row.start;
    if (is_missing(duration) || duration <= 0.0) {
      continue;
    }
    weighted_sum += row.mage * (duration / total_duration);
  }

  return weighted_sum;
}

double calculate_mage_naive_for_id(const NumericVector& glucose,
                                   const std::vector<int>& indices,
                                   double sd_multiplier) {
  std::vector<double> values;
  values.reserve(indices.size());
  for (int idx : indices) {
    if (!is_missing(glucose[idx])) {
      values.push_back(glucose[idx]);
    }
  }

  if (values.empty()) {
    return R_NaN;
  }

  const double mean_glucose = std::accumulate(values.begin(), values.end(), 0.0) /
    static_cast<double>(values.size());
  const double sd_glucose = sample_sd(values);
  if (is_missing(sd_glucose)) {
    return R_NaN;
  }

  const double threshold = sd_multiplier * sd_glucose;
  std::vector<double> countable;
  countable.reserve(values.size());
  for (double value : values) {
    const double abs_diff = std::fabs(value - mean_glucose);
    if (abs_diff > threshold) {
      countable.push_back(abs_diff);
    }
  }

  return mean_or_nan(countable);
}

} // namespace

// [[Rcpp::export]]
DataFrame conga_rcpp_cpp(DataFrame df,
                         int n = 24,
                         std::string tz = "",
                         double inter_gap = 45) {
  if (!df.containsElementNamed("id") ||
      !df.containsElementNamed("time") ||
      !df.containsElementNamed("gl")) {
    stop("conga_rcpp requires columns 'id', 'time', and 'gl'");
  }

  NumericVector time = df["time"];
  NumericVector glucose = df["gl"];
  const std::string tzone = timezone_from_time_or_arg(time, tz);
  std::map<std::string, std::vector<int>> id_indices = valid_id_indices(df);

  CharacterVector out_id(id_indices.size());
  NumericVector out_conga(id_indices.size());

  int out_pos = 0;
  for (const auto& id_pair : id_indices) {
    out_id[out_pos] = id_pair.first;
    out_conga[out_pos] = calculate_conga_for_id(
      time, glucose, id_pair.second, n, inter_gap, tzone, df.nrows()
    );
    ++out_pos;
  }

  DataFrame out = DataFrame::create(
    _["id"] = out_id,
    _["CONGA"] = out_conga
  );
  set_tibble_class(out);
  return out;
}

// [[Rcpp::export]]
DataFrame mage_rcpp_cpp(DataFrame df,
                        std::string version = "ma",
                        double sd_multiplier = 1,
                        int short_ma = 5,
                        int long_ma = 32,
                        std::string return_type = "num",
                        std::string direction = "avg",
                        std::string tz = "",
                        double inter_gap = 45,
                        double max_gap = 180) {
  if (!df.containsElementNamed("id") ||
      !df.containsElementNamed("time") ||
      !df.containsElementNamed("gl")) {
    stop("mage_rcpp requires columns 'id', 'time', and 'gl'");
  }

  NumericVector time = df["time"];
  NumericVector glucose = df["gl"];
  const std::string tzone = timezone_from_time_or_arg(time, tz);
  std::map<std::string, std::vector<int>> id_indices = valid_id_indices(df);

  CharacterVector out_id(id_indices.size());

  if (version == "naive") {
    NumericVector out_mage(id_indices.size());
    int out_pos = 0;
    for (const auto& id_pair : id_indices) {
      out_id[out_pos] = id_pair.first;
      out_mage[out_pos] =
        calculate_mage_naive_for_id(glucose, id_pair.second, sd_multiplier);
      ++out_pos;
    }

    DataFrame out = DataFrame::create(
      _["id"] = out_id,
      _["MAGE"] = out_mage
    );
    set_tibble_class(out);
    return out;
  }

  if (version != "ma") {
    stop("version must be 'ma' or 'naive'");
  }

  if (return_type == "df") {
    List out_mage(id_indices.size());
    int out_pos = 0;
    for (const auto& id_pair : id_indices) {
      out_id[out_pos] = id_pair.first;
      std::vector<MageRow> rows = calculate_mage_ma_for_id(
        time, glucose, id_pair.second, short_ma, long_ma,
        inter_gap, max_gap, tzone
      );
      out_mage[out_pos] = mage_rows_to_dataframe(rows, tzone);
      ++out_pos;
    }
    out_mage.attr("class") = "AsIs";

    DataFrame out = DataFrame::create(
      _["id"] = out_id,
      _["MAGE"] = out_mage
    );
    set_tibble_class(out);
    return out;
  }

  if (return_type != "num") {
    stop("return_type must be 'num' or 'df'");
  }

  NumericVector out_mage(id_indices.size());
  int out_pos = 0;
  for (const auto& id_pair : id_indices) {
    out_id[out_pos] = id_pair.first;
    std::vector<MageRow> rows = calculate_mage_ma_for_id(
      time, glucose, id_pair.second, short_ma, long_ma,
      inter_gap, max_gap, tzone
    );
    out_mage[out_pos] = summarize_mage_rows(rows, direction);
    ++out_pos;
  }

  DataFrame out = DataFrame::create(
    _["id"] = out_id,
    _["MAGE"] = out_mage
  );
  set_tibble_class(out);
  return out;
}
