#include "id_based_calculator.h"

using namespace Rcpp;
using namespace std;

// Optimized Level1HyperglycemicEvents calculator class
// Detects events in the range: 181 <= glucose <= 250 mg/dL
class OptimizedLevel1HyperglycemicEventsCalculator : public IdBasedCalculator {
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
  inline int calculate_min_readings(double reading_minutes, double dur_length = 15) const {
    return static_cast<int>(std::ceil((dur_length / reading_minutes) / 4 * 3));
  }

  // Helper function to calculate episode duration (full event including recovery) and average glucose during whole episode
  std::pair<double, double> calculate_episode_metrics(const NumericVector& time_subset,
                                                     const NumericVector& glucose_subset,
                                                     int start_idx, int end_idx,
                                                     double min_threshold, double max_threshold) const {
    double glucose_sum = 0.0;
    int glucose_count = 0;
    
    // Calculate average glucose for the ENTIRE episode (core definition + recovery time)
    for (int i = start_idx; i <= end_idx; ++i) {
      if (!NumericVector::is_na(glucose_subset[i])) {
        glucose_sum += glucose_subset[i];
        glucose_count++;
      }
    }
    
    // Calculate full event duration (start to end including recovery time)
    double full_event_duration_minutes = 0.0;
    if (end_idx > start_idx) {
      full_event_duration_minutes = (time_subset[end_idx] - time_subset[start_idx]) / 60.0;
    }
    
    double average_glucose = (glucose_count > 0) ? glucose_sum / glucose_count : 0.0;

    return std::make_pair(full_event_duration_minutes, average_glucose);
  }


  // Optimized Level 1 hyperglycemic event detection for a single ID (stays within ID boundaries)
  // Level 1: 181 <= glucose <= 250 mg/dL
  IntegerVector calculate_level1_hyper_events_for_id(const NumericVector& time_subset,
                                                   const NumericVector& glucose_subset,
                                                   int min_readings,
                                                   double dur_length = 15,
                                                   double end_length = 15,
                                                   double start_gl_min = 180,
                                                   double start_gl_max = 250,
                                                   double end_gl = 180) {
    const int n_subset = time_subset.length();
    IntegerVector level1_hyper_events_subset(n_subset, 0);

    if (n_subset == 0) return level1_hyper_events_subset;

    bool level1_hyper_event = false;
    int start_idx = -1;
    int j = 0;
    int last_in_range_idx = -1; // last idx with gl within [start_gl_min, start_gl_max]
    const double epsilon_minutes = 0.1; // tolerance for duration

    // Pre-allocate vectors for better performance
    std::vector<bool> valid_glucose(n_subset);
    std::vector<double> glucose_values(n_subset);

    // Single pass to identify valid glucose readings and cache values
    for (int i = 0; i < n_subset; ++i) {
      valid_glucose[i] = !NumericVector::is_na(glucose_subset[i]);
      glucose_values[i] = valid_glucose[i] ? glucose_subset[i] : 0.0;
    }

    while (j < n_subset) {
      // Check for data gap > (end_length + tolerance) minutes - end any ongoing event
      double gap_threshold_secs = (end_length + epsilon_minutes) * 60.0;
      if (j < n_subset - 1 && (time_subset[j+1] - time_subset[j]) > gap_threshold_secs) {
        if (level1_hyper_event && start_idx != -1) {
          level1_hyper_events_subset[j] = -1;
          level1_hyper_event = false;
          start_idx = -1;
        }
        j++;
        continue;
      }

      // Handle end of data - close any ongoing event
      if (j == n_subset - 1) {
        if (level1_hyper_event && start_idx != -1) {
          level1_hyper_events_subset[j] = -1;
        }
        break;
      }

      if (!level1_hyper_event) {
        // Look for potential Level 1 hyperglycemic event start: 181 <= glucose <= 250
        if (valid_glucose[j] && glucose_values[j] > start_gl_min && glucose_values[j] <= start_gl_max) {
          level1_hyper_event = true;
          start_idx = j;
          last_in_range_idx = j;
          level1_hyper_events_subset[start_idx] = 2; // Event start marker
        }
      } else {
        // Currently in Level 1 hyperglycemic event
        if (valid_glucose[j] && glucose_values[j] > start_gl_min && glucose_values[j] <= start_gl_max) {
          last_in_range_idx = j;
        } else if (valid_glucose[j] && glucose_values[j] <= end_gl) {
          // Candidate recovery: validate in-range duration and sustained recovery
          double duration_minutes = 0.0;
          if (last_in_range_idx >= start_idx) {
            duration_minutes = (time_subset[last_in_range_idx] - time_subset[start_idx]) / 60.0;
          }
          if (duration_minutes + epsilon_minutes >= dur_length) {
            // Require sustained recovery <= end_gl for end_length minutes
            double recovery_needed_secs = end_length * 60.0;
            double recovery_start_time = time_subset[j];
            int k2 = j;
            int last_recovery_idx = j;
            bool recovery_broken = false;
            while (k2 + 1 < n_subset && (time_subset[k2 + 1] - recovery_start_time) <= recovery_needed_secs) {
              if (valid_glucose[k2 + 1] && glucose_values[k2 + 1] > end_gl) {
                recovery_broken = true;
                break;
              }
              last_recovery_idx = k2 + 1;
              k2++;
            }
            double sustained_secs = time_subset[last_recovery_idx] - recovery_start_time;
            if (!recovery_broken && (sustained_secs / 60.0 + epsilon_minutes) >= end_length) {
              // Mark end at the start of recovery (j)
              level1_hyper_events_subset[j] = -1;
              level1_hyper_event = false;
              start_idx = -1;
              last_in_range_idx = -1;
            }
          }
          // else: not enough in-range duration yet; remain in event
        } else {
          // In-between zone (> end_gl and < start_gl_min): neither extending nor recovered
        }
      }

      j++;
    }

    return level1_hyper_events_subset;
  }

  // Optimized Level 1 hyperglycemic event validation (stays within the same ID's data)
  // Only count time when glucose is between 181 <= gl <= 250
  bool validate_level1_hyperglycemic_event_optimized(const NumericVector& time_subset,
                                                    const std::vector<bool>& valid_glucose,
                                                    const std::vector<double>& glucose_values,
                                                    int start_idx, int n_subset,
                                                    int min_readings,
                                                    double dur_length,
                                                    double start_gl_min = 180,
                                                    double start_gl_max = 250) const {

    

    int valid_readings_count = 1; // Count the starting reading
    double accumulated_time = 0.0; // Only count time when glucose is in range
    int k = start_idx + 1;

    // Process ALL readings within this ID's data, not just within target_end_time
    while (k < n_subset) {
      if (valid_glucose[k]) {
        valid_readings_count++;

        // Exit if glucose goes outside the level 1 range (recovery)
        if (glucose_values[k] < start_gl_min || glucose_values[k] > start_gl_max) {
          break;
        }

        // Only count time when glucose is between 181 <= gl <= 250
        if (glucose_values[k] > start_gl_min && glucose_values[k] <= start_gl_max) {
          if (k > start_idx) {
            accumulated_time += (time_subset[k] - time_subset[k-1]) / 60.0; // Convert to minutes
          }
        }
      }
      k++;
    }

    // Check if we have sufficient accumulated time in range and readings
    bool reached_duration = accumulated_time >= dur_length;

    return reached_duration && (valid_readings_count >= min_readings);
  }

  // Optimized event end validation (stays within the same ID's data)
  bool validate_event_end_optimized(const NumericVector& time_subset,
                                   const std::vector<bool>& valid_glucose,
                                   const std::vector<double>& glucose_values,
                                   int recovery_start, int n_subset,
                                   double end_length,
                                   double end_gl = 180) const {

    const double recovery_start_time = time_subset[recovery_start];
    const double target_end_time = recovery_start_time + end_length * 60;

    int k = recovery_start + 1;

    // Look ahead within this ID's data only
    while (k < n_subset && time_subset[k] <= target_end_time) {
      if (valid_glucose[k] && glucose_values[k] > end_gl) {
        return false; // Glucose went back above end_gl
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
                                         const IntegerVector& level1_hyper_events_subset,
                                         const NumericVector& time_subset,
                                         const NumericVector& glucose_subset,
                                         double start_gl_min, double start_gl_max) {
    // First do the standard episode processing
    process_episodes(current_id, level1_hyper_events_subset, time_subset, glucose_subset);

    // Calculate total days for this ID
    if (time_subset.length() > 0) {
      double total_time_seconds = time_subset[time_subset.length() - 1] - time_subset[0];
      id_statistics[current_id].total_days = total_time_seconds / (24.0 * 60.0 * 60.0);
    }

    // Then collect data for total DataFrame efficiently
    const std::vector<int>& indices = id_indices[current_id];
    int start_idx = -1;

    // Pre-allocate for expected events in this ID
    size_t estimated_events = std::count(level1_hyper_events_subset.begin(), level1_hyper_events_subset.end(), 2);
    if (total_event_data.size() + estimated_events > total_event_data.ids.capacity()) {
      // Grow capacity efficiently
      size_t new_capacity = std::max(total_event_data.ids.capacity() * 2,
                                   total_event_data.size() + estimated_events);
      total_event_data.reserve(new_capacity);
    }

    for (int i = 0; i < level1_hyper_events_subset.length(); ++i) {
      if (level1_hyper_events_subset[i] == 2) {
        // Event start
        start_idx = i;
      } else if (level1_hyper_events_subset[i] == -1 && start_idx != -1) {
        // Event end - add with bounds checking
        if (start_idx < static_cast<int>(indices.size()) && i < static_cast<int>(indices.size())) {
          // Use the index just before recovery starts for metrics and outputs
          int end_idx_for_metrics = (i > start_idx) ? (i - 1) : i;

          // Calculate episode metrics up to just before recovery start
          auto metrics = calculate_episode_metrics(time_subset, glucose_subset, start_idx, end_idx_for_metrics, start_gl_min, start_gl_max);
          double event_duration_mins = metrics.first;
          double avg_glucose = metrics.second;

          // Store in total_event_data
          total_event_data.ids.push_back(current_id);
          total_event_data.start_times.push_back(time_subset[start_idx]);
          total_event_data.start_glucose.push_back(glucose_subset[start_idx]);
          total_event_data.end_times.push_back(time_subset[end_idx_for_metrics]);
          total_event_data.end_glucose.push_back(glucose_subset[end_idx_for_metrics]);
          total_event_data.start_indices.push_back(indices[start_idx] + 1); // Convert to 1-based R index
          // Store end_indices as the point just before recovery starts (if available)
          int end_index_for_indices = end_idx_for_metrics;
          total_event_data.end_indices.push_back(indices[end_index_for_indices] + 1); // Convert to 1-based R index
          total_event_data.duration_minutes.push_back(event_duration_mins);
          total_event_data.average_glucose.push_back(avg_glucose);

          // Store statistics for this ID - use updated event duration
          id_statistics[current_id].episode_durations.push_back(event_duration_mins);
          id_statistics[current_id].episode_glucose_averages.push_back(avg_glucose);
          id_statistics[current_id].episode_times.push_back(time_subset[start_idx]);
        }
        start_idx = -1;
      }
    }
  }

  // Optimized DataFrame creation
  DataFrame create_level1_hyper_events_total_df() const {
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
      _["average_glucose"] = wrap(total_event_data.average_glucose)
    );

    // Set class attributes to make it a tibble
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    return df;
  }

  // Enhanced events_total DataFrame creation with additional statistics
  DataFrame create_events_total_df(const StringVector& all_ids, const IntegerVector& Elevel1hyperE_final) const {
    // Use map for consistency with base class
    std::map<std::string, int> id_event_counts;

    // Initialize all IDs with 0 events
    for (int i = 0; i < all_ids.length(); ++i) {
      std::string id_str = as<std::string>(all_ids[i]);
      id_event_counts[id_str] = 0;
    }

    // Count events (start markers = 2) for each ID
    for (int i = 0; i < Elevel1hyperE_final.length(); ++i) {
      if (Elevel1hyperE_final[i] == 2) {  // Event start marker
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
          // Calculate average duration in minutes (total duration / number of episodes)
          double total_duration_minutes = 0.0;
          for (double duration_mins : stats.episode_durations) {
            total_duration_minutes += duration_mins;
          }
          avg_duration = (count > 0) ? total_duration_minutes / count : 0.0;
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
  OptimizedLevel1HyperglycemicEventsCalculator() {
    // Reserve initial capacity for event data
    total_event_data.reserve(100);
  }

  // Main optimized calculation method that properly handles ID boundaries
  List calculate_with_parameters(const DataFrame& df,
                                SEXP reading_minutes_sexp,
                                double dur_length = 15,
                                double end_length = 15,
                                double start_gl_min = 180,
                                double start_gl_max = 250,
                                double end_gl = 180) {
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
    std::map<std::string, IntegerVector> id_level1_hyper_results;

    // Calculate Level 1 hyperglycemic events for each ID separately to ensure proper boundaries
    for (auto const& id_pair : id_indices) {
      std::string current_id = id_pair.first;
      const std::vector<int>& indices = id_pair.second;

      // Extract subset data for this ID (minimize copying)
      NumericVector time_subset(indices.size());
      NumericVector glucose_subset(indices.size());
      extract_id_subset(current_id, indices, time, glucose, time_subset, glucose_subset);

      // Get min_readings for this ID
      int min_readings = id_min_readings[current_id];

      // Calculate Level 1 hyperglycemic events for this ID only
      IntegerVector level1_hyper_events_subset = calculate_level1_hyper_events_for_id(time_subset, glucose_subset,
                                                                             min_readings, dur_length, end_length,
                                                                             start_gl_min, start_gl_max, end_gl);

      // Store result
      id_level1_hyper_results[current_id] = level1_hyper_events_subset;

      // Process events for this ID (both standard and total)
      process_events_with_total_optimized(current_id, level1_hyper_events_subset, time_subset, glucose_subset, start_gl_min, start_gl_max);
    }

    // --- Step 4: Merge results back to original order ---
    IntegerVector level1_hyper_events_final = merge_results(id_level1_hyper_results, n);

    // --- Step 5: Create output structures ---
    DataFrame level1_hyper_events_total_df = create_level1_hyper_events_total_df();
    DataFrame events_total_df = create_events_total_df(id, level1_hyper_events_final);

    // --- Step 6: Return final results ---
    return List::create(
      _["events_total"] = events_total_df,
      _["events_detailed"] = level1_hyper_events_total_df
    );
  }
};

// [[Rcpp::export]]
List detect_excl_level1_hyperglycemic_events(DataFrame new_df,
                                    SEXP reading_minutes = R_NilValue,
                                    double dur_length = 15,
                                    double end_length = 15,
                                    double start_gl_min = 180,
                                    double start_gl_max = 250,
                                    double end_gl = 180) {
  OptimizedLevel1HyperglycemicEventsCalculator calculator;

  if (reading_minutes == R_NilValue) {
    return calculator.calculate_with_parameters(new_df, wrap(5), dur_length, end_length,
                                               start_gl_min, start_gl_max, end_gl);
  } else {
    return calculator.calculate_with_parameters(new_df, reading_minutes, dur_length, end_length,
                                               start_gl_min, start_gl_max, end_gl);
  }
}