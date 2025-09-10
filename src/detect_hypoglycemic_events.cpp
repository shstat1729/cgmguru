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
  // Apply small tolerance (0.1 min) to handle timestamp jitter around 5-minute sampling
  inline int calculate_min_readings(double reading_minutes, double dur_length = 120) const {
    const double tolerance_minutes = 0.1;
    const double effective_duration = std::max(0.0, dur_length - tolerance_minutes);
    // Retain original 3/4 heuristic and incorporate tolerance via effective_duration
    return static_cast<int>(std::ceil((effective_duration / reading_minutes) / 4 * 3));
  }

  // Helper function to calculate duration below 54 mg/dL and average glucose during whole episode
  std::pair<double, double> calculate_episode_metrics(const NumericVector& time_subset,
                                                     const NumericVector& glucose_subset,
                                                     int start_idx, int end_idx,
                                                     double threshold) const {
    double duration_below_54 = 0.0;
    double glucose_sum = 0.0;
    int glucose_count = 0;
    
    // Calculate average glucose for the ENTIRE episode (core definition + recovery time)
    for (int i = start_idx; i <= end_idx; ++i) {
      if (!NumericVector::is_na(glucose_subset[i])) {
        glucose_sum += glucose_subset[i];
        glucose_count++;
        
        // Calculate time spent below 54 mg/dL during the entire episode
        if (glucose_subset[i] < 54.0) {
          double time_duration = 0.0;
          if (i < end_idx) {
            // Use interval to next point
            time_duration = (time_subset[i + 1] - time_subset[i]) / 60.0; // Convert to minutes
          } else {
            // For the last point, use interval to next point if available, otherwise use previous interval
            if (i + 1 < time_subset.length()) {
              time_duration = (time_subset[i + 1] - time_subset[i]) / 60.0;
            } else if (i > start_idx) {
              time_duration = (time_subset[i] - time_subset[i - 1]) / 60.0;
            }
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
                                           double start_gl = 70,
                                           double reading_minutes = 5.0) {
    const int n_subset = time_subset.length();
    IntegerVector hypo_events_subset(n_subset, 0);

    if (n_subset == 0) return hypo_events_subset;

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
    const double epsilon_minutes = 0.1; // tolerance for 15-min requirement

    for (int i = 0; i < n_subset; ++i) {
      // If there is a data gap > (end_length + small tolerance) minutes, end any ongoing event
      double gap_threshold_secs = (end_length + epsilon_minutes) * 60.0;
      if (i > 0 && (time_subset[i] - time_subset[i - 1]) > gap_threshold_secs) {
        if (in_hypo_event && event_start != -1) {
          // Check if the event meets the core definition (duration requirement)
          double event_duration_minutes = 0.0;
          if (last_hypo_idx >= event_start) {
            event_duration_minutes = (time_subset[last_hypo_idx] - time_subset[event_start]) / 60.0;
            // Add reading interval duration to account for the fact that each reading represents a time interval
            event_duration_minutes += reading_minutes;
          }
          
          // If event meets core definition (duration and min_readings), end it at the last hypoglycemic reading
          if ((event_duration_minutes + epsilon_minutes >= dur_length) && (hypo_count >= min_readings)) {
            hypo_events_subset[event_start] = 2;
            if (last_hypo_idx >= 0) {
              hypo_events_subset[last_hypo_idx] = -1; // End at last hypoglycemic reading
            } else {
              int end_marker_idx = i - 1;
              if (end_marker_idx >= 0) {
                hypo_events_subset[end_marker_idx] = -1; // Fallback to previous reading
              }
            }
          }
          // If event doesn't meet core definition, don't mark it as valid
          
          in_hypo_event = false;
          event_start = -1;
          last_hypo_idx = -1;
        }
        continue;
      }

      if (!valid_glucose[i]) continue;

      if (!in_hypo_event) {
        // Looking for event start
        if (glucose_values[i] < start_gl) {
          hypo_count = 1;
          event_start = i;
          in_hypo_event = true;
        }
      } else {
        // Currently in hypoglycemic event
        if (glucose_values[i] < start_gl) {
          hypo_count++;
          last_hypo_idx = i;
        } else { // glucose >= 70 (recovery candidate)
          // 1) Validate low-phase by whole-number readings (use min_readings)
          if (hypo_count < min_readings) {
            // Not enough consecutive low readings yet; keep waiting within the event
          } else {
            // 2) Check if the consecutive duration meets the dur_length requirement
            double consecutive_duration_minutes = 0.0;
            if (last_hypo_idx >= event_start) {
              consecutive_duration_minutes = (time_subset[last_hypo_idx] - time_subset[event_start]) / 60.0;
              // Add reading interval duration to account for the fact that each reading represents a time interval
              consecutive_duration_minutes += reading_minutes;
            }
            
            // Only proceed to check for recovery if consecutive duration meets requirement
            if (consecutive_duration_minutes + epsilon_minutes >= dur_length) {
              // 3) Require sustained recovery for end_length minutes with tolerance
              double recovery_needed_secs = end_length * 60.0;
              double recovery_start_time = time_subset[i];
              int k = i;
              int last_recovery_idx = i;
              bool recovery_broken = false;
              while (k + 1 < n_subset && (time_subset[k + 1] - recovery_start_time) <= recovery_needed_secs) {
                if (valid_glucose[k + 1] && glucose_values[k + 1] < start_gl) {
                  recovery_broken = true;
                  break;
                }
                last_recovery_idx = k + 1;
                k++;
              }
              double sustained_secs = time_subset[last_recovery_idx] - recovery_start_time;
              // Add reading interval duration to account for the fact that each reading represents a time interval
              double total_recovery_minutes = (sustained_secs / 60.0) + reading_minutes;

              // Accept recovery if:
              // - sustained within window, or
              // - there is no reading within end_length window (large gap), hence treat as sustained by default
              bool no_reading_within_window = !(k + 1 < n_subset && (time_subset[k + 1] - recovery_start_time) <= recovery_needed_secs);
              if (!recovery_broken && ((total_recovery_minutes + epsilon_minutes) >= end_length || no_reading_within_window)) {
                // End episode just before recovery starts (at last_hypo_idx)
                hypo_events_subset[event_start] = 2;
                if (last_hypo_idx >= 0) {
                  hypo_events_subset[last_hypo_idx] = -1; // End at last hypoglycemic reading
                } else {
                  hypo_events_subset[i-1] = -1; // Fallback to recovery start if no last_hypo_idx
                }

                // Reset for next episode
                event_start = -1;
                hypo_count = 0;
                last_hypo_idx = -1;
                in_hypo_event = false;
              } else {
                // Recovery not yet sustained; remain in event
              }
            } else {
              // Consecutive duration not yet met; remain in event
            }
          }
        }
      }
    }

    // Handle case where data ends while still in an event (no recovery due to missing data)
    if (in_hypo_event && event_start != -1) {
      // Check if the event meets the core definition (duration and min_readings requirements)
      double event_duration_minutes = 0.0;
      if (last_hypo_idx >= event_start) {
        event_duration_minutes = (time_subset[last_hypo_idx] - time_subset[event_start]) / 60.0;
        // Add reading interval duration to account for the fact that each reading represents a time interval
        event_duration_minutes += reading_minutes;
      }
      
      // If event meets core definition, end it at the last hypoglycemic reading
      if ((event_duration_minutes + epsilon_minutes) >= dur_length && (hypo_count >= min_readings)) {
        hypo_events_subset[event_start] = 2;
        if (last_hypo_idx >= 0) {
          hypo_events_subset[last_hypo_idx] = -1; // End at last hypoglycemic reading
        } else {
          // Fallback: end at the last available reading
          int last_idx = n_subset - 1;
          if (last_idx >= 0) {
            hypo_events_subset[last_idx] = -1;
          }
        }
      }
    }

    return hypo_events_subset;
  }



  // Enhanced episode processing that also stores data for total DataFrame
  void process_events_with_total_optimized(const std::string& current_id,
                                         const IntegerVector& hypo_events_subset,
                                         const NumericVector& time_subset,
                                         const NumericVector& glucose_subset,
                                         double start_gl,
                                         double reading_minutes) {
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
        if (start_idx >= 0 && start_idx < static_cast<int>(indices.size()) && 
            i >= 0 && i < static_cast<int>(indices.size())) {
          // Event now ends just before recovery (at the -1 marker), so use i directly
          int end_idx_for_metrics = i;

          // Calculate episode metrics up to just before recovery start
          auto metrics = calculate_episode_metrics(time_subset, glucose_subset, start_idx, end_idx_for_metrics, start_gl);
          double duration_below_54 = metrics.first;
          double avg_glucose = metrics.second;
          
          // Calculate event duration up to just before recovery start
          double event_duration_mins = 0.0;
          if (end_idx_for_metrics > start_idx) {
            event_duration_mins = (time_subset[end_idx_for_metrics] - time_subset[start_idx]) / 60.0;
            // Add reading interval duration to be consistent with detection logic
            event_duration_mins += reading_minutes;
          }

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
          total_event_data.duration_below_54_minutes.push_back(duration_below_54);
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

      // Get the reading_minutes value for this specific ID
      double id_reading_minutes = 5.0; // default value
      if (TYPEOF(reading_minutes_sexp) == INTSXP) {
        IntegerVector reading_minutes_int = as<IntegerVector>(reading_minutes_sexp);
        if (reading_minutes_int.length() == 1) {
          id_reading_minutes = static_cast<double>(reading_minutes_int[0]);
        } else {
          id_reading_minutes = static_cast<double>(reading_minutes_int[indices[0]]);
        }
      } else if (TYPEOF(reading_minutes_sexp) == REALSXP) {
        NumericVector reading_minutes_num = as<NumericVector>(reading_minutes_sexp);
        if (reading_minutes_num.length() == 1) {
          id_reading_minutes = reading_minutes_num[0];
        } else {
          id_reading_minutes = reading_minutes_num[indices[0]];
        }
      }

      // Calculate hypoglycemic events for this ID only, passing start_gl
      IntegerVector hypo_events_subset = calculate_hypo_events_for_id(time_subset, glucose_subset,
                                                               min_readings, dur_length, end_length, start_gl, id_reading_minutes);

      // Store result
      id_hypo_results[current_id] = hypo_events_subset;

      // Process events for this ID (both standard and total)
      process_events_with_total_optimized(current_id, hypo_events_subset, time_subset, glucose_subset, start_gl, id_reading_minutes);
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