#include "id_based_calculator.h"

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
    std::vector<double> duration_minutes;
    std::vector<double> duration_below_54_minutes;
    std::vector<double> average_glucose;

    void reserve(size_t capacity) {
      ids.reserve(capacity);
      start_times.reserve(capacity);
      end_times.reserve(capacity);
      start_glucose.reserve(capacity);
      end_glucose.reserve(capacity);
      start_indices.reserve(capacity);
      end_indices.reserve(capacity);
      duration_minutes.reserve(capacity);
      duration_below_54_minutes.reserve(capacity);
      average_glucose.reserve(capacity);
    }

    void clear() {
      ids.clear();
      start_times.clear();
      end_times.clear();
      start_glucose.clear();
      end_glucose.clear();
      start_indices.clear();
      end_indices.clear();
      duration_minutes.clear();
      duration_below_54_minutes.clear();
      average_glucose.clear();
    }

    size_t size() const { return ids.size(); }
  };

  EventData total_event_data;

  // Helper structure to store per-ID statistics
  struct IDStatistics {
    std::vector<double> episode_durations;
    std::vector<double> episode_glucose_averages;
    std::vector<double> episode_times; // for calculating episodes per day
    double total_days = 0.0;

    void clear() {
      episode_durations.clear();
      episode_glucose_averages.clear();
      episode_times.clear();
      total_days = 0.0;
    }
  };

  std::map<std::string, IDStatistics> id_statistics;

  // Helper function to calculate min_readings from reading_minutes and dur_length
  inline int calculate_min_readings(double reading_minutes, double dur_length = 120) const {
    return static_cast<int>(std::ceil((dur_length / reading_minutes) / 4 * 3));
  }

  // Helper function to calculate duration below 54 mg/dL and average glucose during episode
  std::pair<double, double> calculate_episode_metrics(const NumericVector& time_subset,
                                                     const NumericVector& glucose_subset,
                                                     int start_idx, int end_idx) const {
    double duration_below_54 = 0.0;
    double glucose_sum = 0.0;
    int glucose_count = 0;

    for (int i = start_idx; i <= end_idx; ++i) {
      if (!NumericVector::is_na(glucose_subset[i])) {
        glucose_sum += glucose_subset[i];
        glucose_count++;

        // Calculate time spent below 54 mg/dL
        if (glucose_subset[i] < 54.0) {
          double time_duration = 0.0;
          if (i < end_idx) {
            time_duration = (time_subset[i + 1] - time_subset[i]) / 60.0; // Convert to minutes
          } else if (i > start_idx) {
            // For the last point, use the same interval as the previous point
            time_duration = (time_subset[i] - time_subset[i - 1]) / 60.0;
          }
          duration_below_54 += time_duration;
        }
      }
    }

    double average_glucose = (glucose_count > 0) ? glucose_sum / glucose_count : 0.0;
    return std::make_pair(duration_below_54, average_glucose);
  }

  // Optimized hypoglycemic event detection for a single ID (stays within ID boundaries)
  IntegerVector calculate_hypo_events_for_id(const NumericVector& time_subset,
                                           const NumericVector& glucose_subset,
                                           int min_readings,
                                           double dur_length = 120,
                                           double end_length = 15,
                                           double start_gl = 70) {
    const int n_subset = time_subset.length();
    IntegerVector hypo_events_subset(n_subset, 0);

    if (n_subset == 0) return hypo_events_subset;

    bool hypo_event = false;
    int start_idx = -1;
    int j = 0;

    // Pre-allocate vectors for better performance
    std::vector<bool> valid_glucose(n_subset);
    std::vector<double> glucose_values(n_subset);

    // Single pass to identify valid glucose readings and cache values
    for (int i = 0; i < n_subset; ++i) {
      valid_glucose[i] = !NumericVector::is_na(glucose_subset[i]);
      glucose_values[i] = valid_glucose[i] ? glucose_subset[i] : 0.0;
    }

    while (j < n_subset) {
      // Check for data gap > 3 hours - end any ongoing event
      if (j < n_subset - 1 && (time_subset[j+1] - time_subset[j]) > 3 * 60 * 60) {
        if (hypo_event && start_idx != -1) {
          hypo_events_subset[j] = -1;
          hypo_event = false;
          start_idx = -1;
        }
        j++;
        continue;
      }

      // Handle end of data - close any ongoing event
      if (j == n_subset - 1) {
        if (hypo_event && start_idx != -1) {
          hypo_events_subset[j] = -1;
        }
        break;
      }

      if (!hypo_event) {
        // Look for potential hypoglycemic event start using configurable start_gl
        if (valid_glucose[j] && glucose_values[j] < start_gl) {
          // Optimized event validation using pre-computed data
          if (validate_hypoglycemic_event_optimized(time_subset, valid_glucose, glucose_values,
                                                  j, n_subset, min_readings, dur_length, start_gl)) {
            hypo_event = true;
            start_idx = j;
            hypo_events_subset[start_idx] = 2; // Event start marker
          }
        }
      } else {
        // Currently in hypoglycemic event, look for end condition
        // Always end at 70 mg/dL regardless of start threshold (clinical requirement)
        if (valid_glucose[j] && glucose_values[j] >= 70) {
          // Optimized end validation
          if (validate_event_end_optimized(time_subset, valid_glucose, glucose_values,
                                         j, n_subset, end_length)) {
            hypo_events_subset[j] = -1; // Event end marker
            hypo_event = false;
            start_idx = -1;
          }
        }
      }

      j++;
    }

    return hypo_events_subset;
  }

  // Optimized event validation (stays within the same ID's data)
  bool validate_hypoglycemic_event_optimized(const NumericVector& time_subset,
                                           const std::vector<bool>& valid_glucose,
                                           const std::vector<double>& glucose_values,
                                           int start_idx, int n_subset,
                                           int min_readings,
                                           double dur_length,
                                           double start_gl = 70) const {

    const double event_start_time = time_subset[start_idx];

    int valid_readings_count = 1; // Count the starting reading
    int k = start_idx + 1;
    int last_valid_hypo_idx = start_idx; // Track last valid hypoglycemic reading

    // Look ahead within this ID's data only - process ALL readings, not just within target_end_time
    int recovery_idx = -1; // Track recovery point (first reading >= 70)
    while (k < n_subset) {
      if (valid_glucose[k]) {
        valid_readings_count++;

        // Check for recovery point
        if (glucose_values[k] >= 70.0) {
          recovery_idx = k; // Mark recovery point
          break; // Exit loop when glucose recovers
        }

        // Update last valid hypoglycemic reading index
        last_valid_hypo_idx = k;
      }
      k++;
    }

    // Check if we have sufficient duration and readings
    // Use the recovery point to calculate total episode duration (start to recovery)
    bool reached_duration = false;
    if (recovery_idx != -1) {
      // Episode duration is from start to recovery point
      reached_duration = (time_subset[recovery_idx] - event_start_time) >= dur_length * 60;
    } else if (last_valid_hypo_idx > start_idx) {
      // If no recovery point found, use last hypoglycemic reading (fallback)
      reached_duration = (time_subset[last_valid_hypo_idx] - event_start_time) >= dur_length * 60;
    }

    return reached_duration && (valid_readings_count >= min_readings);
  }

  // Optimized event end validation (stays within the same ID's data)
  bool validate_event_end_optimized(const NumericVector& time_subset,
                                   const std::vector<bool>& valid_glucose,
                                   const std::vector<double>& glucose_values,
                                   int recovery_start, int n_subset,
                                   double end_length) const {

    const double recovery_start_time = time_subset[recovery_start];
    const double target_end_time = recovery_start_time + end_length * 60;

    int k = recovery_start + 1;

    // Look ahead within this ID's data only
    while (k < n_subset && time_subset[k] <= target_end_time) {
      // Always check against 70 mg/dL for recovery (clinical requirement)
      if (valid_glucose[k] && glucose_values[k] < 70.0) {
        return false; // Glucose dropped back below 70
      }
      k++;
    }

    // Check if we maintained recovery for sufficient time
    bool reached_duration = (k > recovery_start + 1) ?
      (time_subset[k - 1] - recovery_start_time) >= end_length * 60 : false;

    return reached_duration;
  }

  // Enhanced episode processing that also stores data for total DataFrame
  void process_events_with_total_optimized(const std::string& current_id,
                                         const IntegerVector& hypo_events_subset,
                                         const NumericVector& time_subset,
                                         const NumericVector& glucose_subset) {
    // First do the standard episode processing
    process_episodes(current_id, hypo_events_subset, time_subset, glucose_subset);

    // Calculate total days for this ID
    if (time_subset.length() > 0) {
      double total_time_seconds = time_subset[time_subset.length() - 1] - time_subset[0];
      id_statistics[current_id].total_days = total_time_seconds / (24.0 * 60.0 * 60.0);
    }

    // Then collect data for total DataFrame efficiently
    const std::vector<int>& indices = id_indices[current_id];
    int start_idx = -1;

    // Pre-allocate for expected events in this ID
    size_t estimated_events = std::count(hypo_events_subset.begin(), hypo_events_subset.end(), 2);
    if (total_event_data.size() + estimated_events > total_event_data.ids.capacity()) {
      // Grow capacity efficiently
      size_t new_capacity = std::max(total_event_data.ids.capacity() * 2,
                                   total_event_data.size() + estimated_events);
      total_event_data.reserve(new_capacity);
    }

    for (int i = 0; i < hypo_events_subset.length(); ++i) {
      if (hypo_events_subset[i] == 2) {
        // Event start
        start_idx = i;
      } else if (hypo_events_subset[i] == -1 && start_idx != -1) {
        // Event end - add with bounds checking
        if (start_idx < static_cast<int>(indices.size()) && i < static_cast<int>(indices.size())) {
          // Calculate duration in minutes
          double duration_mins = (time_subset[i] - time_subset[start_idx]) / 60.0;

          // Calculate episode metrics (duration below 54 mg/dL and average glucose)
          auto metrics = calculate_episode_metrics(time_subset, glucose_subset, start_idx, i);
          double duration_below_54 = metrics.first;
          double avg_glucose = metrics.second;

          // Store in total_event_data
          total_event_data.ids.push_back(current_id);
          total_event_data.start_times.push_back(time_subset[start_idx]);
          total_event_data.start_glucose.push_back(glucose_subset[start_idx]);
          total_event_data.end_times.push_back(time_subset[i]);
          total_event_data.end_glucose.push_back(glucose_subset[i]);
          total_event_data.start_indices.push_back(indices[start_idx]);
          total_event_data.end_indices.push_back(indices[i]);
          total_event_data.duration_minutes.push_back(duration_mins);
          total_event_data.duration_below_54_minutes.push_back(duration_below_54);
          total_event_data.average_glucose.push_back(avg_glucose);

          // Store statistics for this ID
          id_statistics[current_id].episode_durations.push_back(duration_mins);
          id_statistics[current_id].episode_glucose_averages.push_back(avg_glucose);
          id_statistics[current_id].episode_times.push_back(time_subset[start_idx]);
        }
        start_idx = -1;
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
    start_time_vec.attr("class") = CharacterVector::create("POSIXct");
    start_time_vec.attr("tzone") = "UTC";

    NumericVector end_time_vec = wrap(total_event_data.end_times);
    end_time_vec.attr("class") = CharacterVector::create("POSIXct");
    end_time_vec.attr("tzone") = "UTC";

    DataFrame df = DataFrame::create(
      _["id"] = wrap(total_event_data.ids),
      _["start_time"] = start_time_vec,
      _["start_glucose"] = wrap(total_event_data.start_glucose),
      _["end_time"] = end_time_vec,
      _["end_glucose"] = wrap(total_event_data.end_glucose),
      _["start_indices"] = wrap(total_event_data.start_indices),
      _["end_indices"] = wrap(total_event_data.end_indices),
      _["duration_minutes"] = wrap(total_event_data.duration_minutes),
      _["duration_below_54_minutes"] = wrap(total_event_data.duration_below_54_minutes),
      _["average_glucose"] = wrap(total_event_data.average_glucose)
    );

    // Set class attributes to make it a tibble
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    return df;
  }

  // Enhanced events_total DataFrame creation with additional statistics
  DataFrame create_events_total_df(const StringVector& all_ids, const IntegerVector& hypo_events_final) const {
    // Use map for consistency with base class
    std::map<std::string, int> id_event_counts;

    // Initialize all IDs with 0 events
    for (int i = 0; i < all_ids.length(); ++i) {
      std::string id_str = as<std::string>(all_ids[i]);
      id_event_counts[id_str] = 0;
    }

    // Count events (start markers = 2) for each ID
    for (int i = 0; i < hypo_events_final.length(); ++i) {
      if (hypo_events_final[i] == 2) {  // Event start marker
        std::string id_str = as<std::string>(all_ids[i]);
        id_event_counts[id_str]++;
      }
    }

    // Create vectors for DataFrame
    std::vector<std::string> unique_ids;
    std::vector<int> event_counts;
    std::vector<double> avg_episode_duration;
    std::vector<double> avg_episodes_per_day;
    std::vector<double> avg_glucose_in_episodes;

    unique_ids.reserve(id_event_counts.size());
    event_counts.reserve(id_event_counts.size());
    avg_episode_duration.reserve(id_event_counts.size());
    avg_episodes_per_day.reserve(id_event_counts.size());
    avg_glucose_in_episodes.reserve(id_event_counts.size());

    for (const auto& pair : id_event_counts) {
      std::string id_str = pair.first;
      int count = pair.second;

      unique_ids.push_back(id_str);
      event_counts.push_back(count);

      // Calculate averages if statistics exist for this ID
      if (id_statistics.find(id_str) != id_statistics.end()) {
        const IDStatistics& stats = id_statistics.at(id_str);

        // Average episode duration
        double avg_duration = 0.0;
        if (!stats.episode_durations.empty()) {
          double sum_duration = 0.0;
          for (double d : stats.episode_durations) {
            sum_duration += d;
          }
          avg_duration = sum_duration / stats.episode_durations.size();
        }

        // Apply rounding with special handling for zero values
        double rounded_avg_duration = (avg_duration == 0.0) ? 0.0 : round(avg_duration * 10.0) / 10.0;
        avg_episode_duration.push_back(rounded_avg_duration);

        // Average episodes per day
        double episodes_per_day = 0.0;
        if (stats.total_days > 0) {
          episodes_per_day = static_cast<double>(count) / stats.total_days;
        }

        // Apply rounding with special handling for zero values
        double rounded_episodes_per_day = (episodes_per_day == 0.0) ? 0.0 : round(episodes_per_day * 100.0) / 100.0;
        avg_episodes_per_day.push_back(rounded_episodes_per_day);

        // Average glucose in episodes
        double avg_glucose = 0.0;
        if (!stats.episode_glucose_averages.empty()) {
          double sum_glucose = 0.0;
          for (double g : stats.episode_glucose_averages) {
            sum_glucose += g;
          }
          avg_glucose = sum_glucose / stats.episode_glucose_averages.size();
        }

        // Apply rounding with special handling for zero values
        double rounded_avg_glucose = (avg_glucose == 0.0) ? 0.0 : round(avg_glucose * 10.0) / 10.0;
        avg_glucose_in_episodes.push_back(rounded_avg_glucose);
      } else {
        // No statistics available for this ID
        avg_episode_duration.push_back(0.0);
        avg_episodes_per_day.push_back(0.0);
        avg_glucose_in_episodes.push_back(0.0);
      }
    }

    DataFrame df = DataFrame::create(
      _["id"] = wrap(unique_ids),
      _["total_events"] = wrap(event_counts),
      _["avg_ep_per_day"] = wrap(avg_episodes_per_day),
      _["avg_ep_duration"] = wrap(avg_episode_duration),
      _["avg_ep_gl"] = wrap(avg_glucose_in_episodes)
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
                                double start_gl = 70) {
    // Clear previous results
    total_event_data.clear();
    id_statistics.clear();

    // --- Step 1: Extract columns from DataFrame ---
    int n = df.nrows();
    StringVector id = df["id"];
    NumericVector time = df["time"];
    NumericVector glucose = df["gl"];

    // --- Step 2: Process reading_minutes argument efficiently ---
    std::map<std::string, int> id_min_readings;

    // Process reading_minutes argument
    if (TYPEOF(reading_minutes_sexp) == INTSXP) {
      IntegerVector reading_minutes_int = as<IntegerVector>(reading_minutes_sexp);
      if (reading_minutes_int.length() == 1) {
        int min_readings = calculate_min_readings(static_cast<double>(reading_minutes_int[0]), dur_length);
        group_by_id(id, n);
        for (auto const& id_pair : id_indices) {
          id_min_readings[id_pair.first] = min_readings;
        }
      } else {
        if (reading_minutes_int.length() != n) {
          stop("reading_minutes vector length must match data length");
        }
        group_by_id(id, n);
        for (auto const& id_pair : id_indices) {
          std::string current_id = id_pair.first;
          const std::vector<int>& indices = id_pair.second;
          double id_reading_minutes = static_cast<double>(reading_minutes_int[indices[0]]);
          id_min_readings[current_id] = calculate_min_readings(id_reading_minutes, dur_length);
        }
      }
    } else if (TYPEOF(reading_minutes_sexp) == REALSXP) {
      NumericVector reading_minutes_num = as<NumericVector>(reading_minutes_sexp);
      if (reading_minutes_num.length() == 1) {
        int min_readings = calculate_min_readings(reading_minutes_num[0], dur_length);
        group_by_id(id, n);
        for (auto const& id_pair : id_indices) {
          id_min_readings[id_pair.first] = min_readings;
        }
      } else {
        if (reading_minutes_num.length() != n) {
          stop("reading_minutes vector length must match data length");
        }
        group_by_id(id, n);
        for (auto const& id_pair : id_indices) {
          std::string current_id = id_pair.first;
          const std::vector<int>& indices = id_pair.second;
          double id_reading_minutes = reading_minutes_num[indices[0]];
          id_min_readings[current_id] = calculate_min_readings(id_reading_minutes, dur_length);
        }
      }
    } else {
      stop("reading_minutes must be numeric or integer");
    }

    // --- Step 3: Calculate for each ID separately (CRITICAL for correctness) ---
    std::map<std::string, IntegerVector> id_hypo_results;

    // Calculate hypoglycemic events for each ID separately to ensure proper boundaries
    for (auto const& id_pair : id_indices) {
      std::string current_id = id_pair.first;
      const std::vector<int>& indices = id_pair.second;

      // Extract subset data for this ID (minimize copying)
      NumericVector time_subset(indices.size());
      NumericVector glucose_subset(indices.size());
      extract_id_subset(current_id, indices, time, glucose, time_subset, glucose_subset);

      // Get min_readings for this ID
      int min_readings = id_min_readings[current_id];

      // Calculate hypoglycemic events for this ID only, passing start_gl
      IntegerVector hypo_events_subset = calculate_hypo_events_for_id(time_subset, glucose_subset,
                                                               min_readings, dur_length, end_length, start_gl);

      // Store result
      id_hypo_results[current_id] = hypo_events_subset;

      // Process events for this ID (both standard and total)
      process_events_with_total_optimized(current_id, hypo_events_subset, time_subset, glucose_subset);
    }

    // --- Step 4: Merge results back to original order ---
    IntegerVector hypo_events_final = merge_results(id_hypo_results, n);

    // --- Step 5: Create output structures ---
    DataFrame hypo_events_total_df = create_hypo_events_total_df();
    DataFrame events_total_df = create_events_total_df(id, hypo_events_final);

    // --- Step 6: Return final results ---
    return List::create(
      _["events_total"] = events_total_df,
      _["events_detailed"] = hypo_events_total_df
    );
  }
};

// [[Rcpp::export]]
List detect_hypoglycemic_events(DataFrame new_df,
                             SEXP reading_minutes = R_NilValue,
                             double dur_length = 120,
                             double end_length = 15,
                             double start_gl = 70) {
  OptimizedHypoglycemicEventsCalculator calculator;

  if (reading_minutes == R_NilValue) {
    return calculator.calculate_with_parameters(new_df, wrap(5), dur_length, end_length, start_gl);
  } else {
    return calculator.calculate_with_parameters(new_df, reading_minutes, dur_length, end_length, start_gl);
  }
}