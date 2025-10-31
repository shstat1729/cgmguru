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
    // Retain original 3/4 heuristic and incorporate tolerance via effective_duration
    return static_cast<int>(std::ceil((effective_duration / reading_minutes) / 4 * 3));
  }

  // Helper function to calculate duration below 54 mg/dL and average glucose during whole episode
  double calculate_episode_metrics(const NumericVector& time_subset,
                                                     const NumericVector& glucose_subset,
                                                     int start_idx, int end_idx,
                                                     double threshold) const {
    double duration_below_54 = 0.0;
    
    for (int i = start_idx; i <= end_idx; ++i) {
      if (!NumericVector::is_na(glucose_subset[i])) {

        
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
    const double epsilon_minutes = 0.1; // tolerance for 15-min requirement

    for (int i = 0; i < n_subset; ++i) {
      // If there is a data gap > (end_length + small tolerance) minutes, we cannot confirm recovery
      double gap_threshold_secs = (end_length + epsilon_minutes) * 60.0;
      if (i > 0 && (time_subset[i] - time_subset[i - 1]) > gap_threshold_secs) {
        if (in_hypo_event && event_start != -1) {
          // Do not finalize the event without confirmed recovery; reset state to avoid leaking into next event
          in_hypo_event = false;
          event_start = -1;
          last_hypo_idx = -1;
          hypo_count = 0;
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
            // Not enough consecutive low readings yet; CANCEL the event
            // because glucose exceeded threshold before meeting duration requirement
            in_hypo_event = false;
            event_start = -1;
            last_hypo_idx = -1;
            hypo_count = 0;
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
              double sustained_secs = 0.0;
              bool recovery_broken = false;
              int last_k = i;
              int recovery_end_idx = -1;
              for (int k = i; k < n_subset-1 && glucose_values[k] >= start_gl; k++) {
                if (valid_glucose[k] && glucose_values[k] < start_gl) {
                  recovery_broken = true;
                  break;
                }
                sustained_secs += time_subset[k+1] - time_subset[k];
                last_k = k;
                // Determine when recovery duration first meets threshold
                double minutes_so_far = (sustained_secs / 60.0) - reading_minutes;
                if (minutes_so_far >= end_length) {
                  recovery_end_idx = k; // end of recovery period
                  break;
                }
              }
              
              // Add reading interval duration to account for the fact that each reading represents a time interval
              // double total_recovery_minutes = (sustained_secs / 60.0) - reading_minutes;

              // Accept recovery if sustained >= threshold or if no reading occurs within the window
              bool no_reading_within_window = !(last_k + 1 < n_subset && (time_subset[last_k + 1] - recovery_start_time) <= recovery_needed_secs);
              if (!recovery_broken && (recovery_end_idx != -1 || no_reading_within_window)) {

                int end_idx_for_event = (recovery_end_idx != -1) ? recovery_end_idx : last_k;
                hypo_events_subset[event_start] = 2;
                hypo_events_subset[end_idx_for_event] = -1; // End at end of recovery time
                
                // Calculate and store event metrics
                double duration_below_54 = calculate_episode_metrics(time_subset, glucose_subset, event_start, end_idx_for_event, start_gl);
                event_starts.push_back(event_start);
                event_ends.push_back(end_idx_for_event);
                event_durations_below_54.push_back(duration_below_54);

                // Reset for next episode
                event_start = -1;
                hypo_count = 0;
                last_hypo_idx = -1;
                in_hypo_event = false;
              } else {
                // Recovery not yet sustained; remain in event
              }
            } else {
              // Consecutive duration not yet met; CANCEL the event
              // because glucose exceeded threshold before meeting duration requirement
              in_hypo_event = false;
              event_start = -1;
              last_hypo_idx = -1;
              hypo_count = 0;
            }
          }
        }
      }
    }

    // Do not finalize an event at end-of-data without confirmed recovery
    (void)in_hypo_event; // state intentionally not used to finalize without recovery


    return List::create(
      _["events"] = hypo_events_subset,
      _["event_starts"] = wrap(event_starts),
      _["event_ends"] = wrap(event_ends),
      _["total_episodes"] = wrap(std::vector<int>(1, event_starts.size())),
      _["durations_below_54"] = wrap(event_durations_below_54)
    );
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
                                         double reading_minutes) {
    // First do the standard episode processing
    process_episodes(current_id, hypo_events_subset, time_subset, glucose_subset);

    // Calculate total days for this ID
    if (time_subset.length() > 0) {
      double total_time_seconds = time_subset[time_subset.length() - 1] - time_subset[0];
      id_statistics[current_id].total_days = total_time_seconds / (24.0 * 60.0 * 60.0);
    }

    // Then collect data for total DataFrame efficiently using pre-calculated event metrics
    const std::vector<int>& indices = id_indices[current_id];

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
      
      // Add with bounds checking
      if (start_idx >= 0 && start_idx < static_cast<int>(indices.size()) && 
          end_idx >= 0 && end_idx < static_cast<int>(indices.size())) {
        
        // Store in total_event_data
        total_event_data.ids.push_back(current_id);
        total_event_data.start_times.push_back(time_subset[start_idx]);
        total_event_data.start_glucose.push_back(glucose_subset[start_idx]);
        total_event_data.end_times.push_back(time_subset[end_idx]);
        total_event_data.end_glucose.push_back(glucose_subset[end_idx]);
        total_event_data.start_indices.push_back(indices[start_idx] + 1); // Convert to 1-based R index
        total_event_data.end_indices.push_back(indices[end_idx] + 1); // Convert to 1-based R index
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
      _["start_indices"] = wrap(total_event_data.start_indices),
      _["end_indices"] = wrap(total_event_data.end_indices),
      _["duration_below_54_minutes"] = wrap(total_event_data.duration_below_54_minutes)
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
      _["total_events"] = wrap(event_counts),
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
                                double start_gl = 70) {
    // Clear previous results
    total_event_data.clear();
    id_statistics.clear();

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
    // Optional: Per-row timezone column to support per-ID timezones
    bool has_tzone_col = df.containsElementNamed("tzone");
    CharacterVector tz_col;
    if (has_tzone_col) {
      tz_col = as<CharacterVector>(df["tzone"]);
    }

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
      List hypo_result = calculate_hypo_events_for_id(time_subset, glucose_subset,
                                                     min_readings, dur_length, end_length, start_gl, id_reading_minutes);
      
      IntegerVector hypo_events_subset = as<IntegerVector>(hypo_result["events"]);
      std::vector<int> event_starts = as<std::vector<int>>(hypo_result["event_starts"]);
      std::vector<int> event_ends = as<std::vector<int>>(hypo_result["event_ends"]);
      std::vector<double> durations_below_54 = as<std::vector<double>>(hypo_result["durations_below_54"]);

      // Store result
      id_hypo_results[current_id] = hypo_events_subset;

      // Process events for this ID (both standard and total)
      process_events_with_total_optimized(current_id, hypo_events_subset, time_subset, glucose_subset, event_starts, event_ends, durations_below_54, start_gl, id_reading_minutes);
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
List detect_hypoglycemic_events(DataFrame df,
                             SEXP reading_minutes = R_NilValue,
                             double dur_length = 120,
                             double end_length = 15,
                             double start_gl = 70) {
  OptimizedHypoglycemicEventsCalculator calculator;

  if (reading_minutes == R_NilValue) {
    return calculator.calculate_with_parameters(df, wrap(5), dur_length, end_length, start_gl);
  } else {
    return calculator.calculate_with_parameters(df, reading_minutes, dur_length, end_length, start_gl);
  }
}
