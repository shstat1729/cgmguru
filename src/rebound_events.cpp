#include "event_preprocessing.h"
#include "id_based_calculator.h"
#include "rebound_events_core.h"

#include <Rcpp.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>

using namespace Rcpp;

namespace {

std::string timezone_from_time(const NumericVector& time) {
  std::string tzone = "UTC";
  RObject tz_attr = time.attr("tzone");

  if (!tz_attr.isNULL()) {
    CharacterVector tz_values = as<CharacterVector>(tz_attr);
    if (tz_values.size() > 0 && !CharacterVector::is_na(tz_values[0])) {
      tzone = as<std::string>(tz_values[0]);
    }
  }

  return tzone.empty() ? "UTC" : tzone;
}

NumericVector posixct_vector(const std::vector<double>& values,
                             const std::string& tzone) {
  NumericVector out = wrap(values);
  out.attr("class") = CharacterVector::create("POSIXct", "POSIXt");
  out.attr("tzone") = tzone;
  return out;
}

double round_to_two_decimals(double value) {
  if (NumericVector::is_na(value) || !std::isfinite(value)) return value;
  return std::round(value * 100.0) / 100.0;
}

cgmguru_events::PreparedIDData prepare_preprocessed_id_data(
    const NumericVector& time,
    const NumericVector& glucose,
    const std::vector<int>& indices,
    double reading_minutes) {
  cgmguru_events::PreparedIDData prepared;
  prepared.reading_minutes = reading_minutes;

  std::vector<double> prepared_time;
  std::vector<double> prepared_glucose;
  prepared_time.reserve(indices.size());
  prepared_glucose.reserve(indices.size());

  for (int idx : indices) {
    const double current_time = time[idx];
    const double current_glucose = glucose[idx];
    if (cgmguru_events::is_na(current_time) ||
        cgmguru_events::is_na(current_glucose)) {
      continue;
    }

    if (!prepared_time.empty() &&
        std::fabs(current_time - prepared_time.back()) < 1e-7) {
      prepared_glucose.back() = current_glucose;
    } else {
      prepared_time.push_back(current_time);
      prepared_glucose.push_back(current_glucose);
    }
  }

  const int n = static_cast<int>(prepared_time.size());
  prepared.time = wrap(prepared_time);
  prepared.glucose = wrap(prepared_glucose);
  prepared.segment = IntegerVector(n, 0);

  if (n == 0) {
    return prepared;
  }

  const double dt_seconds = reading_minutes * 60.0;
  const double time_epsilon = std::max(1e-7, dt_seconds * 1e-6);
  int segment_id = 1;
  int segment_start = 0;
  prepared.segment[0] = segment_id;

  for (int i = 1; i < n; ++i) {
    const double diff_seconds = prepared.time[i] - prepared.time[i - 1];
    if (diff_seconds > dt_seconds + time_epsilon) {
      prepared.segments.push_back({segment_start, i - 1});
      ++segment_id;
      segment_start = i;
    }
    prepared.segment[i] = segment_id;
  }
  prepared.segments.push_back({segment_start, n - 1});

  return prepared;
}

class ReboundEventsCalculator : public IdBasedCalculator {
private:
  struct DetailedData {
    std::vector<std::string> ids;
    std::vector<std::string> types;
    std::vector<double> start_times;
    std::vector<double> start_glucose;
    std::vector<double> end_times;
    std::vector<double> end_glucose;
    std::vector<int> start_indices;
    std::vector<int> end_indices;
    std::vector<double> initial_start_times;
    std::vector<double> initial_start_glucose;
    std::vector<double> initial_end_times;
    std::vector<double> initial_end_glucose;
    std::vector<int> initial_start_indices;
    std::vector<int> initial_end_indices;
    std::vector<double> rebound_times;
    std::vector<double> rebound_glucose;
    std::vector<int> rebound_indices;
    std::vector<double> minutes_to_rebound;

    void clear() {
      ids.clear();
      types.clear();
      start_times.clear();
      start_glucose.clear();
      end_times.clear();
      end_glucose.clear();
      start_indices.clear();
      end_indices.clear();
      initial_start_times.clear();
      initial_start_glucose.clear();
      initial_end_times.clear();
      initial_end_glucose.clear();
      initial_start_indices.clear();
      initial_end_indices.clear();
      rebound_times.clear();
      rebound_glucose.clear();
      rebound_indices.clear();
      minutes_to_rebound.clear();
    }

    void reserve(size_t capacity) {
      ids.reserve(capacity);
      types.reserve(capacity);
      start_times.reserve(capacity);
      start_glucose.reserve(capacity);
      end_times.reserve(capacity);
      end_glucose.reserve(capacity);
      start_indices.reserve(capacity);
      end_indices.reserve(capacity);
      initial_start_times.reserve(capacity);
      initial_start_glucose.reserve(capacity);
      initial_end_times.reserve(capacity);
      initial_end_glucose.reserve(capacity);
      initial_start_indices.reserve(capacity);
      initial_end_indices.reserve(capacity);
      rebound_times.reserve(capacity);
      rebound_glucose.reserve(capacity);
      rebound_indices.reserve(capacity);
      minutes_to_rebound.reserve(capacity);
    }

    size_t size() const {
      return ids.size();
    }
  };

  DetailedData detailed_data;
  cgmguru_events::InterpolatedDataStore interpolated_data;
  std::map<std::string, double> total_days_by_id;
  std::map<std::string, std::map<std::string, int>> counts_by_id_type;
  std::string output_tzone = "UTC";

  void append_rebound_event(const std::string& current_id,
                            const cgmguru_rebound::ReboundEvent& event,
                            const cgmguru_events::PreparedIDData& prepared,
                            int row_offset) {
    const int bridge_start_idx = event.bridge_start_idx;
    const int rebound_idx = event.rebound_idx;
    const int initial_start_idx = event.initial_start_idx;
    const int initial_end_idx = event.initial_end_idx;

    if (bridge_start_idx < 0 || bridge_start_idx >= prepared.time.length() ||
        rebound_idx < 0 || rebound_idx >= prepared.time.length() ||
        initial_start_idx < 0 || initial_start_idx >= prepared.time.length() ||
        initial_end_idx < 0 || initial_end_idx >= prepared.time.length()) {
      return;
    }

    detailed_data.ids.push_back(current_id);
    detailed_data.types.push_back(event.type);
    detailed_data.start_times.push_back(prepared.time[bridge_start_idx]);
    detailed_data.start_glucose.push_back(prepared.glucose[bridge_start_idx]);
    detailed_data.end_times.push_back(prepared.time[rebound_idx]);
    detailed_data.end_glucose.push_back(prepared.glucose[rebound_idx]);
    detailed_data.start_indices.push_back(row_offset + bridge_start_idx + 1);
    detailed_data.end_indices.push_back(row_offset + rebound_idx + 1);
    detailed_data.initial_start_times.push_back(prepared.time[initial_start_idx]);
    detailed_data.initial_start_glucose.push_back(prepared.glucose[initial_start_idx]);
    detailed_data.initial_end_times.push_back(prepared.time[initial_end_idx]);
    detailed_data.initial_end_glucose.push_back(prepared.glucose[initial_end_idx]);
    detailed_data.initial_start_indices.push_back(row_offset + initial_start_idx + 1);
    detailed_data.initial_end_indices.push_back(row_offset + initial_end_idx + 1);
    detailed_data.rebound_times.push_back(prepared.time[rebound_idx]);
    detailed_data.rebound_glucose.push_back(prepared.glucose[rebound_idx]);
    detailed_data.rebound_indices.push_back(row_offset + rebound_idx + 1);
    detailed_data.minutes_to_rebound.push_back(
      round_to_two_decimals(event.minutes_to_rebound));

    ++counts_by_id_type[current_id][event.type];
  }

  DataFrame create_events_detailed_df() const {
    NumericVector start_time_vec = posixct_vector(detailed_data.start_times, output_tzone);
    NumericVector end_time_vec = posixct_vector(detailed_data.end_times, output_tzone);
    NumericVector initial_start_time_vec =
      posixct_vector(detailed_data.initial_start_times, output_tzone);
    NumericVector initial_end_time_vec =
      posixct_vector(detailed_data.initial_end_times, output_tzone);
    NumericVector rebound_time_vec =
      posixct_vector(detailed_data.rebound_times, output_tzone);

    DataFrame df = DataFrame::create(
      _["id"] = wrap(detailed_data.ids),
      _["type"] = wrap(detailed_data.types),
      _["start_time"] = start_time_vec,
      _["start_glucose"] = wrap(detailed_data.start_glucose),
      _["end_time"] = end_time_vec,
      _["end_glucose"] = wrap(detailed_data.end_glucose),
      _["start_index"] = wrap(detailed_data.start_indices),
      _["end_index"] = wrap(detailed_data.end_indices),
      _["initial_start_time"] = initial_start_time_vec,
      _["initial_start_glucose"] = wrap(detailed_data.initial_start_glucose),
      _["initial_end_time"] = initial_end_time_vec,
      _["initial_end_glucose"] = wrap(detailed_data.initial_end_glucose),
      _["initial_start_index"] = wrap(detailed_data.initial_start_indices),
      _["initial_end_index"] = wrap(detailed_data.initial_end_indices),
      _["rebound_time"] = rebound_time_vec,
      _["rebound_glucose"] = wrap(detailed_data.rebound_glucose),
      _["rebound_index"] = wrap(detailed_data.rebound_indices),
      _["minutes_to_rebound"] = wrap(detailed_data.minutes_to_rebound)
    );
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    return df;
  }

  DataFrame create_events_total_df(const std::vector<std::string>& all_ids,
                                   const std::vector<std::string>& requested_types) const {
    std::vector<std::string> ids;
    std::vector<std::string> types;
    std::vector<int> total_episodes;
    std::vector<double> avg_ep_per_day;

    ids.reserve(all_ids.size() * requested_types.size());
    types.reserve(all_ids.size() * requested_types.size());
    total_episodes.reserve(all_ids.size() * requested_types.size());
    avg_ep_per_day.reserve(all_ids.size() * requested_types.size());

    for (const std::string& id : all_ids) {
      const double total_days =
        total_days_by_id.count(id) == 0 ? 0.0 : total_days_by_id.at(id);

      for (const std::string& type : requested_types) {
        int count = 0;
        auto id_it = counts_by_id_type.find(id);
        if (id_it != counts_by_id_type.end()) {
          auto type_it = id_it->second.find(type);
          if (type_it != id_it->second.end()) {
            count = type_it->second;
          }
        }

        ids.push_back(id);
        types.push_back(type);
        total_episodes.push_back(count);
        const double rate = total_days > 0.0
          ? static_cast<double>(count) / total_days
          : 0.0;
        avg_ep_per_day.push_back(round_to_two_decimals(rate));
      }
    }

    DataFrame df = DataFrame::create(
      _["id"] = wrap(ids),
      _["type"] = wrap(types),
      _["total_episodes"] = wrap(total_episodes),
      _["avg_ep_per_day"] = wrap(avg_ep_per_day)
    );
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    return df;
  }

public:
  ReboundEventsCalculator() {
    detailed_data.reserve(100);
  }

  List calculate(const DataFrame& df,
                 const std::string& type,
                 const std::string& data_source,
                 SEXP reading_minutes_sexp,
                 bool sort_time,
                 double inter_gap,
                 double rebound_minutes,
                 bool return_interpolated) {
    detailed_data.clear();
    interpolated_data.clear();
    total_days_by_id.clear();
    counts_by_id_type.clear();

    const bool include_hypo = type == "all" || type == "hypo";
    const bool include_hyper = type == "all" || type == "hyper";
    std::vector<std::string> requested_types;
    if (include_hypo) requested_types.push_back("hypo");
    if (include_hyper) requested_types.push_back("hyper");

    const int n = df.nrows();
    StringVector id = df["id"];
    NumericVector time = df["time"];
    NumericVector glucose = df["gl"];
    output_tzone = timezone_from_time(time);

    group_by_id(id, n);
    cgmguru_events::sort_or_validate_id_indices(id_indices, time, sort_time);

    std::vector<std::string> all_ids;
    all_ids.reserve(id_indices.size());
    int row_offset = 0;
    if (return_interpolated) {
      interpolated_data.reserve_rows(static_cast<size_t>(n), id_indices.size(), false);
    }

    for (const auto& id_pair : id_indices) {
      const std::string current_id = id_pair.first;
      const std::vector<int>& indices = id_pair.second;
      all_ids.push_back(current_id);

      double id_reading_minutes =
        cgmguru_events::reading_minutes_for_id(reading_minutes_sexp, time, indices, n);

      cgmguru_events::PreparedIDData prepared;
      if (data_source == "raw") {
        id_reading_minutes =
          cgmguru_events::iglu_day_grid_reading_minutes(id_reading_minutes);
        prepared = cgmguru_events::prepare_id_data(
          time, glucose, indices, id_reading_minutes, inter_gap, output_tzone,
          true, true);
      } else if (data_source == "preprocessed") {
        prepared = prepare_preprocessed_id_data(
          time, glucose, indices, id_reading_minutes);
      } else {
        stop("data_source must be 'raw' or 'preprocessed'");
      }

      const int current_row_offset = row_offset;
      row_offset += prepared.time.length();

      if (return_interpolated) {
        interpolated_data.append(current_id, prepared, false);
      }

      total_days_by_id[current_id] =
        cgmguru_events::recording_days(prepared.glucose, id_reading_minutes);

      std::vector<cgmguru_rebound::ReboundEvent> rebound_events =
        cgmguru_rebound::detect_rebound_events(
          prepared, id_reading_minutes, rebound_minutes,
          include_hypo, include_hyper);

      for (const cgmguru_rebound::ReboundEvent& event : rebound_events) {
        append_rebound_event(current_id, event, prepared, current_row_offset);
      }
    }

    List result = List::create(
      _["events_total"] = create_events_total_df(all_ids, requested_types),
      _["events_detailed"] = create_events_detailed_df()
    );

    if (return_interpolated) {
      result["interpolated_data"] =
        interpolated_data.to_dataframe(output_tzone, false);
    }

    return result;
  }
};

} // namespace

// [[Rcpp::export]]
List rebound_events_cpp(DataFrame df,
                        std::string type = "all",
                        std::string data_source = "raw",
                        SEXP reading_minutes = R_NilValue,
                        bool sort_time = false,
                        double inter_gap = 45,
                        double rebound_minutes = 120,
                        bool return_interpolated = true) {
  ReboundEventsCalculator calculator;
  return calculator.calculate(df, type, data_source, reading_minutes, sort_time,
                              inter_gap, rebound_minutes, return_interpolated);
}
