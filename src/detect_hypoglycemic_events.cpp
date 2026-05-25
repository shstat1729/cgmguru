#include "id_based_calculator.h"
#include "event_preprocessing.h"

using namespace Rcpp;
using namespace std;

// Optimized HypoglycemicEvents calculator class
class OptimizedHypoglycemicEventsCalculator : public IdBasedCalculator {
private:
  // Pre-allocated event storage with better memory layout
  struct EventData {
    std::vector<std::string> ids;
    std::vector<double> start_times;
    std::vector<double> end_times;
    std::vector<double> start_glucose;
    std::vector<double> end_glucose;
    std::vector<int> start_indices;
    std::vector<int> end_indices;
    std::vector<double> duration_below_54_minutes;
    std::vector<std::string> timezones;

    void reserve(size_t capacity) {
      ids.reserve(capacity);
      start_times.reserve(capacity);
      end_times.reserve(capacity);
      start_glucose.reserve(capacity);
      end_glucose.reserve(capacity);
      start_indices.reserve(capacity);
      end_indices.reserve(capacity);
      duration_below_54_minutes.reserve(capacity);
      timezones.reserve(capacity);
    }

    void clear() {
      ids.clear();
      start_times.clear();
      end_times.clear();
      start_glucose.clear();
      end_glucose.clear();
      start_indices.clear();
      end_indices.clear();
      duration_below_54_minutes.clear();
      timezones.clear();
    }

    size_t size() const { return ids.size(); }
  };

  EventData total_event_data;
  cgmguru_events::InterpolatedDataStore interpolated_data;

  // Helper structure to store per-ID statistics
  struct IDStatistics {
    std::vector<double> episode_times; // for calculating episodes per day
    double total_days = 0.0;

    void clear() {
      episode_times.clear();
      total_days = 0.0;
    }
  };

  std::map<std::string, IDStatistics> id_statistics;

  std::string output_tzone = "UTC";

  // Helper function to calculate min_readings from reading_minutes and dur_length
  // Apply small tolerance (0.1 min) to handle timestamp jitter around 5-minute sampling
  inline int calculate_min_readings(double reading_minutes, double dur_length = 120) const {
    const double tolerance_minutes = 0.1;
    const double effective_duration = std::max(0.0, dur_length - tolerance_minutes);
    return static_cast<int>(std::ceil(effective_duration / reading_minutes));
  }

  inline bool duration_met(int reading_count, double duration_minutes,
                           double reading_minutes) const {
    const double tolerance_minutes = 0.1;
    return static_cast<double>(reading_count) * reading_minutes +
      tolerance_minutes >= duration_minutes;
  }

  // Helper function to calculate duration below 54 mg/dL and average glucose during whole episode
  double calculate_episode_metrics(const NumericVector& time_subset,
                                                     const NumericVector& glucose_subset,
                                                     int start_idx, int end_idx,
                                                     double threshold,
                                                     double reading_minutes) const {
    (void)time_subset;
    (void)threshold;
    double duration_below_54 = 0.0;
    
    for (int i = start_idx; i <= end_idx; ++i) {
      if (!NumericVector::is_na(glucose_subset[i])) {

        
        // Calculate time spent below 54 mg/dL during the entire episode
        if (glucose_subset[i] < 54.0) {
          duration_below_54 += reading_minutes;
        }
      }
    }

    return duration_below_54;
  }


  // Optimized hypoglycemic event detection for a single ID (stays within ID boundaries)
  List calculate_hypo_events_for_id(const NumericVector& time_subset,
                                   const NumericVector& glucose_subset,
                                   int min_readings,
                                   double dur_length = 120,
                                   double end_length = 15,
                                   double start_gl = 70,
                                   double reading_minutes = 5.0) {
    (void)min_readings;
    const int n_subset = time_subset.length();
    IntegerVector hypo_events_subset(n_subset, 0);
    
    // Store event metrics during detection
    std::vector<int> event_starts; // start indices
    std::vector<int> event_ends;   // end indices
    std::vector<double> event_durations_below_54;

    if (n_subset == 0) {
      return List::create(
        _["events"] = hypo_events_subset,
        _["event_starts"] = wrap(std::vector<int>()),
        _["event_ends"] = wrap(std::vector<int>()),
        _["total_episodes"] = wrap(std::vector<int>()),
        _["durations_below_54"] = wrap(std::vector<double>())
      );
    }

    // Pre-allocate vectors for better performance
    std::vector<bool> valid_glucose(n_subset);
    std::vector<double> glucose_values(n_subset);

    // Single pass to identify valid glucose readings and cache values
    for (int i = 0; i < n_subset; ++i) {
      valid_glucose[i] = !NumericVector::is_na(glucose_subset[i]);
      glucose_values[i] = valid_glucose[i] ? glucose_subset[i] : 0.0;
    }

    bool in_hypo_event = false;
    int event_start = -1;
    int hypo_count = 0; // retained but duration will be authoritative
    int last_hypo_idx = -1; // last index where glucose < start_gl

    for (int i = 0; i < n_subset; ++i) {
      if (!valid_glucose[i]) continue;

      if (!in_hypo_event) {
        // Looking for event start
        if (glucose_values[i] < start_gl) {
          hypo_count = 1;
          event_start = i;
          last_hypo_idx = i;
          in_hypo_event = true;
        }
      } else {
        // Currently in hypoglycemic event
        if (glucose_values[i] < start_gl) {
          hypo_count++;
          last_hypo_idx = i;
        } else { // glucose >= 70 (recovery candidate)
          // 1) Validate low-phase by whole-number readings on the interpolated grid.
          if (!duration_met(hypo_count, dur_length, reading_minutes)) {
            // Not enough consecutive low readings yet; CANCEL the event
            // because glucose exceeded threshold before meeting duration requirement
            in_hypo_event = false;
            event_start = -1;
            last_hypo_idx = -1;
            hypo_count = 0;
          } else {
            // 3) Require sustained recovery for end_length minutes using grid counts.
            int recovery_end_idx = -1;
            int recovery_count = 0;
            for (int k = i; k < n_subset; ++k) {
              if (!valid_glucose[k]) continue;
              if (glucose_values[k] < start_gl) {
                break;
              }
              ++recovery_count;
              if (duration_met(recovery_count, end_length, reading_minutes)) {
                recovery_end_idx = k; // end of recovery period
                break;
              }
            }

            if (recovery_end_idx != -1) {

              int reported_end_idx = (last_hypo_idx >= event_start) ? last_hypo_idx : event_start;
              hypo_events_subset[event_start] = 2;
              hypo_events_subset[recovery_end_idx] = -1; // Confirmation marker at end of recovery time
              
              // Calculate and store event metrics
              double duration_below_54 = calculate_episode_metrics(
                time_subset, glucose_subset, event_start, reported_end_idx,
                start_gl, reading_minutes);
              event_starts.push_back(event_start);
              event_ends.push_back(reported_end_idx);
              event_durations_below_54.push_back(duration_below_54);

              // Reset for next episode
              event_start = -1;
              hypo_count = 0;
              last_hypo_idx = -1;
              in_hypo_event = false;
            } else {
              // Recovery not yet sustained; remain in event
            }
          }
        }
      }
    }

    // iglu-compatible behavior: a qualifying event that reaches the segment end
    // is counted even without confirmed recovery.
    if (in_hypo_event && event_start != -1 &&
        duration_met(hypo_count, dur_length, reading_minutes)) {
      const int marker_end_idx = n_subset - 1;
      const int reported_end_idx =
        (last_hypo_idx >= event_start) ? last_hypo_idx : event_start;

      hypo_events_subset[event_start] = 2;
      if (marker_end_idx != event_start) {
        hypo_events_subset[marker_end_idx] = -1;
      }

      double duration_below_54 = calculate_episode_metrics(
        time_subset, glucose_subset, event_start, reported_end_idx,
        start_gl, reading_minutes);
      event_starts.push_back(event_start);
      event_ends.push_back(reported_end_idx);
      event_durations_below_54.push_back(duration_below_54);
    }


    return List::create(
      _["events"] = hypo_events_subset,
      _["event_starts"] = wrap(event_starts),
      _["event_ends"] = wrap(event_ends),
      _["total_episodes"] = wrap(std::vector<int>(1, event_starts.size())),
      _["durations_below_54"] = wrap(event_durations_below_54)
    );
  }

  bool overlaps_any_event(int start_idx, int end_idx,
                          const std::vector<int>& other_starts,
                          const std::vector<int>& other_ends) const {
    for (size_t i = 0; i < other_starts.size(); ++i) {
      if (other_starts[i] <= end_idx && other_ends[i] >= start_idx) {
        return true;
      }
    }
    return false;
  }
  

  // Enhanced episode processing that also stores data for total DataFrame
  void process_events_with_total_optimized(const std::string& current_id,
                                         const IntegerVector& hypo_events_subset,
                                         const NumericVector& time_subset,
                                         const NumericVector& glucose_subset,
                                         const std::vector<int>& event_starts,
                                         const std::vector<int>& event_ends,
                                         const std::vector<double>& durations_below_54,
                                         double start_gl,
                                         double reading_minutes,
                                         int interpolated_row_offset) {
    (void)hypo_events_subset;
    (void)start_gl;

    // Calculate total days for this ID
    id_statistics[current_id].total_days =
      cgmguru_events::recording_days(glucose_subset, reading_minutes);

    // Pre-allocate for expected events in this ID
    size_t estimated_events = event_starts.size();
    if (total_event_data.size() + estimated_events > total_event_data.ids.capacity()) {
      // Grow capacity efficiently
      size_t new_capacity = std::max(total_event_data.ids.capacity() * 2,
                                   total_event_data.size() + estimated_events);
      total_event_data.reserve(new_capacity);
    }

    // Process events using pre-calculated bounds and metrics
    for (size_t event_idx = 0; event_idx < event_starts.size(); ++event_idx) {
      int start_idx = event_starts[event_idx];
      int end_idx = event_ends[event_idx];
      double duration_below_54 = durations_below_54[event_idx];
      
      // Add with bounds checking against the interpolated grid
      if (start_idx >= 0 && start_idx < time_subset.length() &&
          end_idx >= 0 && end_idx < time_subset.length()) {
        
        // Store in total_event_data
        total_event_data.ids.push_back(current_id);
        total_event_data.start_times.push_back(time_subset[start_idx]);
        total_event_data.start_glucose.push_back(glucose_subset[start_idx]);
        total_event_data.end_times.push_back(time_subset[end_idx]);
        total_event_data.end_glucose.push_back(glucose_subset[end_idx]);
        total_event_data.start_indices.push_back(interpolated_row_offset + start_idx + 1);
        total_event_data.end_indices.push_back(interpolated_row_offset + end_idx + 1);
        total_event_data.duration_below_54_minutes.push_back(duration_below_54);
        total_event_data.timezones.push_back(output_tzone);

        id_statistics[current_id].episode_times.push_back(time_subset[start_idx]);
      }
    }
  }

  // Optimized DataFrame creation
  DataFrame create_hypo_events_total_df() const {
    if (total_event_data.size() == 0) {
      DataFrame empty_df = DataFrame::create();
      empty_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
      return empty_df;
    }

    // Create POSIXct time vectors efficiently
    NumericVector start_time_vec = wrap(total_event_data.start_times);
    start_time_vec.attr("class") = CharacterVector::create("POSIXct", "POSIXt");
    // Use a single tzone attribute only if uniform across events

    std::string tz0 = output_tzone;

    start_time_vec.attr("tzone") = tz0;

    NumericVector end_time_vec = wrap(total_event_data.end_times);
    end_time_vec.attr("class") = CharacterVector::create("POSIXct", "POSIXt");
    end_time_vec.attr("tzone") = tz0;


    DataFrame df = DataFrame::create(
      _["id"] = wrap(total_event_data.ids),
      _["start_time"] = start_time_vec,
      _["start_glucose"] = wrap(total_event_data.start_glucose),
      _["end_time"] = end_time_vec,
      _["end_glucose"] = wrap(total_event_data.end_glucose),
      _["start_index"] = wrap(total_event_data.start_indices),
      _["end_index"] = wrap(total_event_data.end_indices),
      _["duration_below_54_minutes"] = wrap(total_event_data.duration_below_54_minutes)
    );

    // Set class attributes to make it a tibble
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    return df;
  }

  // Enhanced events_total DataFrame creation with additional statistics
  DataFrame create_events_total_df(const std::vector<std::string>& all_ids) const {
    // Use map for consistency with base class
    std::map<std::string, int> id_event_counts;

    // Initialize all IDs with 0 events
    for (const std::string& id_str : all_ids) {
      id_event_counts[id_str] = 0;
    }

    // Count confirmed events collected from the interpolated grid
    for (const std::string& event_id : total_event_data.ids) {
      id_event_counts[event_id]++;
    }

    // Create vectors for DataFrame
    std::vector<std::string> unique_ids;
    std::vector<int> event_counts;
    std::vector<double> avg_episodes_per_day;

    unique_ids.reserve(id_event_counts.size());
    event_counts.reserve(id_event_counts.size());
    avg_episodes_per_day.reserve(id_event_counts.size());

    for (const auto& pair : id_event_counts) {
      std::string id_str = pair.first;
      int count = pair.second;

      unique_ids.push_back(id_str);
      event_counts.push_back(count);

      // Calculate averages if statistics exist for this ID
      if (id_statistics.find(id_str) != id_statistics.end()) {
        const IDStatistics& stats = id_statistics.at(id_str);
        // Average episodes per day
        double episodes_per_day = 0.0;
        if (stats.total_days > 0) {
          episodes_per_day = static_cast<double>(count) / stats.total_days;
        }

        // Apply rounding with special handling for zero values
        double rounded_episodes_per_day = (episodes_per_day == 0.0) ? 0.0 : round(episodes_per_day * 100.0) / 100.0;
        avg_episodes_per_day.push_back(rounded_episodes_per_day);

      } else {
        // No statistics available for this ID
        avg_episodes_per_day.push_back(0.0);
      }
    }

    DataFrame df = DataFrame::create(
      _["id"] = wrap(unique_ids),
      _["total_episodes"] = wrap(event_counts),
      _["avg_ep_per_day"] = wrap(avg_episodes_per_day)
    );

    // Set class attributes to make it a tibble
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    return df;
  }

public:
  OptimizedHypoglycemicEventsCalculator() {
    // Reserve initial capacity for event data
    total_event_data.reserve(100);
  }

  // Main optimized calculation method that properly handles ID boundaries
  List calculate_with_parameters(const DataFrame& df,
                                SEXP reading_minutes_sexp,
                                double dur_length = 120,
                                double end_length = 15,
                                double start_gl = 70,
                                bool sort_time = false,
                                double inter_gap = 45,
                                bool return_interpolated = true,
                                bool lv1_excl = false) {
    // Clear previous results
    total_event_data.clear();
    id_statistics.clear();
    interpolated_data.clear();

    // --- Step 1: Extract columns from DataFrame ---
    int n = df.nrows();
    StringVector id = df["id"];
    NumericVector time = df["time"];
    NumericVector glucose = df["gl"];

    // Mirror input time column's timezone if available; otherwise keep default UTC
    {
      RObject time_col = df["time"];
      if (time_col.hasAttribute("tzone")) {
        CharacterVector tzAttr = time_col.attr("tzone");
        if (tzAttr.size() > 0 && tzAttr[0] != NA_STRING) {
          output_tzone = as<std::string>(tzAttr[0]);
        }
      }
    }

    // --- Step 2: Group, optionally sort, and validate time order ---
    group_by_id(id, n);
    cgmguru_events::sort_or_validate_id_indices(id_indices, time, sort_time);

    std::vector<std::string> unique_ids;
    unique_ids.reserve(id_indices.size());

    // Calculate hypoglycemic events for each ID separately to ensure proper boundaries
    for (auto const& id_pair : id_indices) {
      std::string current_id = id_pair.first;
      const std::vector<int>& indices = id_pair.second;
      unique_ids.push_back(current_id);

      double id_reading_minutes =
        cgmguru_events::reading_minutes_for_id(reading_minutes_sexp, time, indices, n);
      id_reading_minutes =
        cgmguru_events::iglu_day_grid_reading_minutes(id_reading_minutes);
      const int min_readings = calculate_min_readings(id_reading_minutes, dur_length);

      cgmguru_events::PreparedIDData prepared =
        cgmguru_events::prepare_id_data(time, glucose, indices, id_reading_minutes,
                                        inter_gap, output_tzone, true, true);
      const int interpolated_row_offset = static_cast<int>(interpolated_data.times.size());
      interpolated_data.append(current_id, prepared);

      IntegerVector hypo_events_subset(prepared.time.length(), 0);
      std::vector<int> event_starts;
      std::vector<int> event_ends;
      std::vector<double> durations_below_54;

      for (const auto& segment : prepared.segments) {
        NumericVector segment_time =
          cgmguru_events::slice_numeric(prepared.time, segment.start, segment.end);
        NumericVector segment_glucose =
          cgmguru_events::slice_numeric(prepared.glucose, segment.start, segment.end);

        List hypo_result = calculate_hypo_events_for_id(
          segment_time, segment_glucose, min_readings, dur_length, end_length,
          start_gl, id_reading_minutes);

        IntegerVector segment_events = as<IntegerVector>(hypo_result["events"]);
        std::vector<int> segment_starts = as<std::vector<int>>(hypo_result["event_starts"]);
        std::vector<int> segment_ends = as<std::vector<int>>(hypo_result["event_ends"]);
        std::vector<double> segment_durations =
          as<std::vector<double>>(hypo_result["durations_below_54"]);

        if (lv1_excl) {
          List lv2_result = calculate_hypo_events_for_id(
            segment_time, segment_glucose, min_readings, dur_length, end_length,
            54.0, id_reading_minutes);
          std::vector<int> lv2_starts =
            as<std::vector<int>>(lv2_result["event_starts"]);
          std::vector<int> lv2_ends =
            as<std::vector<int>>(lv2_result["event_ends"]);

          IntegerVector filtered_events(segment_events.length(), 0);
          std::vector<int> filtered_starts;
          std::vector<int> filtered_ends;
          std::vector<double> filtered_durations;

          for (size_t i = 0; i < segment_starts.size(); ++i) {
            if (!overlaps_any_event(segment_starts[i], segment_ends[i],
                                    lv2_starts, lv2_ends)) {
              filtered_starts.push_back(segment_starts[i]);
              filtered_ends.push_back(segment_ends[i]);
              filtered_durations.push_back(segment_durations[i]);
              filtered_events[segment_starts[i]] = 2;
              if (segment_ends[i] != segment_starts[i]) {
                filtered_events[segment_ends[i]] = -1;
              }
            }
          }

          segment_events = filtered_events;
          segment_starts = filtered_starts;
          segment_ends = filtered_ends;
          segment_durations = filtered_durations;
        }

        for (int i = 0; i < segment_events.length(); ++i) {
          hypo_events_subset[segment.start + i] = segment_events[i];
        }
        for (size_t i = 0; i < segment_starts.size(); ++i) {
          event_starts.push_back(segment.start + segment_starts[i]);
          event_ends.push_back(segment.start + segment_ends[i]);
          durations_below_54.push_back(segment_durations[i]);
        }
      }

      // Process events for this ID (both standard and total)
      process_events_with_total_optimized(current_id, hypo_events_subset,
                                          prepared.time, prepared.glucose,
                                          event_starts, event_ends,
                                          durations_below_54, start_gl,
                                          id_reading_minutes,
                                          interpolated_row_offset);
    }

    // --- Step 3: Create output structures ---
    DataFrame hypo_events_total_df = create_hypo_events_total_df();
    DataFrame events_total_df = create_events_total_df(unique_ids);

    List result = List::create(
      _["events_total"] = events_total_df,
      _["events_detailed"] = hypo_events_total_df
    );

    if (return_interpolated) {
      result["interpolated_data"] = interpolated_data.to_dataframe(output_tzone, false);
    }

    return result;
  }
};

// [[Rcpp::export]]
List detect_hypoglycemic_events(DataFrame df,
                             SEXP reading_minutes = R_NilValue,
                             double dur_length = 120,
                             double end_length = 15,
                             double start_gl = 70,
                             bool sort_time = false,
                             double inter_gap = 45,
                             bool return_interpolated = true,
                             bool lv1_excl = false) {
  OptimizedHypoglycemicEventsCalculator calculator;
  return calculator.calculate_with_parameters(df, reading_minutes, dur_length,
                                              end_length, start_gl, sort_time,
                                              inter_gap, return_interpolated,
                                              lv1_excl);
}
