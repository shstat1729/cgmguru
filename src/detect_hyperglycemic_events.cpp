#include "id_based_calculator.h"

using namespace Rcpp;
using namespace std;

// Optimized HyperglycemicEvents calculator class
class OptimizedHyperglycemicEventsCalculator : public IdBasedCalculator {
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
    std::vector<std::string> timezones;

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
      duration_minutes.clear();
      average_glucose.clear();
      timezones.clear();
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

  // Timezone to use for outputs; defaults to UTC but will mirror input if present
  std::string output_tzone = "UTC";
  std::map<std::string, std::string> id_tzone;

  // Helper function to calculate min_readings from reading_minutes and dur_length
  // Apply small tolerance (0.1 min) to handle timestamp jitter around 5-minute sampling
  inline int calculate_min_readings(double reading_minutes, double dur_length = 120) const {
    const double tolerance_minutes = 0.1;
    const double effective_duration = std::max(0.0, dur_length - tolerance_minutes);
    // Retain original 3/4 heuristic and incorporate tolerance via effective_duration
    return static_cast<int>(std::ceil((effective_duration / reading_minutes) / 4 * 3));
  }

  // Helper function to calculate episode duration (full event including recovery) and average glucose during whole episode
  std::pair<double, double> calculate_episode_metrics(const NumericVector& time_subset,
                                                     const NumericVector& glucose_subset,
                                                     int start_idx, int end_idx,
                                                     double reading_minutes) const {
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
      // Add reading interval duration to be consistent with detection logic
      full_event_duration_minutes += reading_minutes;
    }
    
    double average_glucose = (glucose_count > 0) ? glucose_sum / glucose_count : 0.0;

    return std::make_pair(full_event_duration_minutes, average_glucose);
  }

  // Two-phase hyperglycemic event detection
  IntegerVector calculate_hyper_events_for_id(const NumericVector& time_subset,
                                            const NumericVector& glucose_subset,
                                            int min_readings,
                                            double dur_length = 120,
                                            double end_length = 15,
                                            double start_gl = 250,
                                            double end_gl = 180,
                                            double reading_minutes = 5.0) {
    const int n_subset = time_subset.length();
    IntegerVector hyper_events_subset(n_subset, 0);

    if (n_subset == 0) return hyper_events_subset;

    std::vector<bool> valid_glucose(n_subset);
    std::vector<double> glucose_values(n_subset);

    for (int i = 0; i < n_subset; ++i) {
      valid_glucose[i] = !NumericVector::is_na(glucose_subset[i]);
      glucose_values[i] = valid_glucose[i] ? glucose_subset[i] : 0.0;
    }

    // Determine if we should use core-only mode
    // Core-only mode: when start_gl != end_gl (e.g., >250 mg/dL for 120 min)
    // Full event mode: when start_gl = end_gl (e.g., >180 mg/dL with recovery at ≤180 mg/dL)
    bool core_only = (std::abs(start_gl - end_gl) >= 1e-9);

    // Phase 1: Find core definitions (start and end points within core)
    struct CoreEvent {
      int start_idx;
      int end_idx;
    };
    std::vector<CoreEvent> core_events;

    bool in_core = false;
    int core_start = -1;
    int core_end = -1;
    double core_duration_minutes = 0.0;
    int core_valid_hyper_count = 0;
    const double epsilon_minutes = 0.1; // tolerance for duration requirement

    // Phase 1: Find core definitions (continuous glucose > start_gl for ≥ dur_length)
    for (int i = 0; i < n_subset - 1; ++i) {
      if (!valid_glucose[i]) continue;

      if (!in_core) {
        // Looking for core start
        if (glucose_values[i] > start_gl) {
          core_start = i;
          core_end = i;
          core_duration_minutes = 0.0;
          core_valid_hyper_count = 1; // current reading is valid and > start_gl
          in_core = true;
        }
      } else {
        // Currently in core
        if (glucose_values[i] > start_gl) {
          // Still in core - extend duration
          core_end = i-1;
          if (i > 0) {
            core_duration_minutes += (time_subset[i+1] - time_subset[i]) / 60.0;
          }
          // Count valid hyper reading
          core_valid_hyper_count += 1;
        } else {
          // Exited core - check if we have enough duration
          if (core_duration_minutes + reading_minutes + epsilon_minutes >= dur_length && core_valid_hyper_count >= min_readings) {
            // Valid core event found - store it
            core_events.push_back({core_start, core_end});
          }
          // Reset for next potential core
          in_core = false;
          core_start = -1;
          core_end = -1;
          core_duration_minutes = 0.0;
          core_valid_hyper_count = 0;
        }
      }
    }

    // Handle case where core continues until end of data
    if (in_core && core_start != -1) {
      // Add reading interval duration to account for the fact that each reading represents a time interval
      double total_core_duration = core_duration_minutes + reading_minutes;
      if (total_core_duration + epsilon_minutes >= dur_length && core_valid_hyper_count >= min_readings) {
        core_events.push_back({core_start, core_end});
      }
    }

    // Phase 2: Process each core event and determine final event boundaries
    int last_event_end_idx = -1; // Track the end of the last processed event for recovery check
    
    for (const auto& core_event : core_events) {
      int event_start_idx = core_event.start_idx;
      int event_end_idx = core_event.end_idx;
      
      if (core_only) {
        // Core-only mode: when start_gl != end_gl (e.g., >250 mg/dL for 120 min)
        // Do not count as new event if event did not meet recovery time definition (end_gl for end_duration)
        bool is_new_event = true;
                // Check if this event meets recovery criteria from the previous event
        
          int scan_start = (last_event_end_idx == -1) ? 0 : (last_event_end_idx + 1);
          bool recovery_found = false;
          double sustained_secs = 0.0;
          double total_recovery_minutes = 0.0;
          for (int i = scan_start; i < event_start_idx; ++i) {
            if (!valid_glucose[i]) continue;
            
            if (glucose_values[i] <= end_gl) {
              // Candidate recovery - check if sustained for end_length minutes
              // recovery candidate start at time_subset[i]
              bool recovery_broken = false;
              sustained_secs = 0.0;
              for (int k = i; k < n_subset - 1 && glucose_values[k] <= end_gl; k++) {
                sustained_secs += time_subset[k+1] - time_subset[k];
              }
  
              
              
              // Add reading interval duration to account for the fact that each reading represents a time interval
              total_recovery_minutes = (sustained_secs / 60.0) - reading_minutes;
              if (total_recovery_minutes >= end_length) {
                // Valid recovery found - mark event end just before recovery starts
                recovery_found = true;
                break;
              }
            }
          }
          
          // If no recovery found, this is not a new event (part of previous event)
          if (!recovery_found) {
            is_new_event = false;
          }
        
        
        // Only mark as new event if recovery criteria is met or this is the first event
        if (is_new_event) {
          hyper_events_subset[event_start_idx] = 2;
          hyper_events_subset[event_end_idx] = -1;
          last_event_end_idx = event_end_idx;
          
        }
      } else {
        // Full event mode: when start_gl = end_gl (e.g., >180 mg/dL with recovery at ≤180 mg/dL)
        
        // Check if this event should be merged with the previous event (no recovery between them)
        bool is_new_event = true;
        if (last_event_end_idx != -1) {
          // Check for recovery between last event end and current event start
          int scan_start = last_event_end_idx + 1;
          bool recovery_found_between = false;
          double sustained_secs_between = 0.0;
          double total_recovery_minutes_between = 0.0;
          
          for (int i = scan_start; i < event_start_idx; ++i) {
            if (!valid_glucose[i]) continue;
            
            if (glucose_values[i] <= end_gl) {
              sustained_secs_between = 0.0;
              for (int k = i; k < n_subset - 1 && glucose_values[k] <= end_gl; k++) {
                sustained_secs_between += time_subset[k+1] - time_subset[k];
              }
              
              total_recovery_minutes_between = (sustained_secs_between / 60.0) - reading_minutes;
              if (total_recovery_minutes_between >= end_length) {
                recovery_found_between = true;
                break;
              }
            }
          }
          
          if (!recovery_found_between) {
            is_new_event = false; // Merge with previous event
          }
        }
        
        // Only process if this is a new event
        if (is_new_event) {
          // Look for recovery after core definition and end event just before recovery
          bool recovery_found = false;
          double sustained_secs = 0.0;
          double total_recovery_minutes = 0.0;
          double missing_gap_secs = 30 * 60.0;
          int last_k = event_end_idx;
          // Look for recovery starting from the end of core definition
          int recovery_scan_start = event_end_idx + 1;
          for (int i = recovery_scan_start; i < n_subset; ++i) {
            if (!valid_glucose[i]) continue;
            
            if (glucose_values[i] <= end_gl) {
              // Candidate recovery - check if sustained for end_length minutes
              bool recovery_broken = false;
              sustained_secs = 0.0;
              for (int k = i; k < n_subset - 1 && glucose_values[k] <= end_gl; k++) {
                sustained_secs += time_subset[k+1] - time_subset[k];
                last_k = k;
              }
              
              // Add reading interval duration to account for the fact that each reading represents a time interval
              total_recovery_minutes = (sustained_secs / 60.0) - reading_minutes;
              if (total_recovery_minutes >= end_length) {
                // Valid recovery found - mark event end just before recovery starts
                hyper_events_subset[event_start_idx] = 2;
                hyper_events_subset[i-1] = -1; // End just before recovery starts
                recovery_found = true;
                last_event_end_idx = i-1; // Update last event end
                break;
              }
            }
          }
          
          // If no recovery found (due to missing data), end event at the end of core definition
          // This handles case 2: missing data but core definition is met
          
          bool missing_gap_window = (last_k + 1 < n_subset && (time_subset[last_k + 1] - time_subset[last_k] ) > missing_gap_secs);
          if (!recovery_found && missing_gap_window) {
            hyper_events_subset[event_start_idx] = 2;
            hyper_events_subset[event_end_idx] = -1;
            last_event_end_idx = event_end_idx; // Update last event end
          }
        }
      }
    }

    return hyper_events_subset;
  }

  // Enhanced episode processing that also stores data for total DataFrame
  void process_events_with_total_optimized(const std::string& current_id,
                                         const IntegerVector& hyper_events_subset,
                                         const NumericVector& time_subset,
                                         const NumericVector& glucose_subset,
                                         double start_gl,
                                         double reading_minutes) {
    // First do the standard episode processing
    process_episodes(current_id, hyper_events_subset, time_subset, glucose_subset);

    // Calculate total days for this ID
    if (time_subset.length() > 0) {
      double total_time_seconds = time_subset[time_subset.length() - 1] - time_subset[0];
      id_statistics[current_id].total_days = total_time_seconds / (24.0 * 60.0 * 60.0);
    }

    // Then collect data for total DataFrame efficiently
    const std::vector<int>& indices = id_indices[current_id];
    int start_idx = -1;

    // Pre-allocate for expected events in this ID
    size_t estimated_events = std::count(hyper_events_subset.begin(), hyper_events_subset.end(), 2);
    if (total_event_data.size() + estimated_events > total_event_data.ids.capacity()) {
      // Grow capacity efficiently
      size_t new_capacity = std::max(total_event_data.ids.capacity() * 2,
                                   total_event_data.size() + estimated_events);
      total_event_data.reserve(new_capacity);
    }

    for (int i = 0; i < hyper_events_subset.length(); ++i) {
      if (hyper_events_subset[i] == 2) {
        // Event start
        start_idx = i;
      } else if (hyper_events_subset[i] == -1 && start_idx != -1) {
        // Event end - add with bounds checking
        if (start_idx < static_cast<int>(indices.size()) && i < static_cast<int>(indices.size())) {
          // Use the index just before recovery starts for metrics and outputs
          int end_idx_for_metrics = i;

          // Calculate episode metrics up to just before recovery start
          auto metrics = calculate_episode_metrics(time_subset, glucose_subset, start_idx, end_idx_for_metrics, reading_minutes);
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
          
          total_event_data.end_indices.push_back(indices[end_idx_for_metrics] + 1); // Convert to 1-based R index
          total_event_data.duration_minutes.push_back(event_duration_mins);
          total_event_data.average_glucose.push_back(avg_glucose);
          // Record timezone per event using id-specific timezone if available
          auto tzIt = id_tzone.find(current_id);
          total_event_data.timezones.push_back(tzIt != id_tzone.end() ? tzIt->second : output_tzone);

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
  DataFrame create_hyper_events_total_df() const {
    if (total_event_data.size() == 0) {
      DataFrame empty_df = DataFrame::create();
      empty_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
      return empty_df;
    }

    // Create POSIXct time vectors efficiently
    NumericVector start_time_vec = wrap(total_event_data.start_times);
    start_time_vec.attr("class") = CharacterVector::create("POSIXct", "POSIXt");
    // Only set tzone attribute if all events share the same timezone
    bool uniform_tz = true;
    std::string tz0 = total_event_data.timezones.empty() ? output_tzone : total_event_data.timezones[0];
    for (size_t i = 1; i < total_event_data.timezones.size(); ++i) {
      if (total_event_data.timezones[i] != tz0) { uniform_tz = false; break; }
    }
    if (uniform_tz) {
      start_time_vec.attr("tzone") = tz0;
    }
    NumericVector end_time_vec = wrap(total_event_data.end_times);
    end_time_vec.attr("class") = CharacterVector::create("POSIXct", "POSIXt");
    if (uniform_tz) {
      end_time_vec.attr("tzone") = tz0;
    }

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
  DataFrame create_events_total_df(const StringVector& all_ids, const IntegerVector& hyper_events_final) const {
    // Use map for consistency with base class
    std::map<std::string, int> id_event_counts;

    // Initialize all IDs with 0 events
    for (int i = 0; i < all_ids.length(); ++i) {
      std::string id_str = as<std::string>(all_ids[i]);
      id_event_counts[id_str] = 0;
    }

    // Count events (start markers = 2) for each ID
    for (int i = 0; i < hyper_events_final.length(); ++i) {
      if (hyper_events_final[i] == 2) {  // Event start marker
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
  OptimizedHyperglycemicEventsCalculator() {
    // Reserve initial capacity for event data
    total_event_data.reserve(100);
  }

  // Main optimized calculation method that properly handles ID boundaries
  List calculate_with_parameters(const DataFrame& df,
                                SEXP reading_minutes_sexp,
                                double dur_length = 120,
                                double end_length = 15,
                                double start_gl = 250,
                                double end_gl = 180) {
    // Clear previous results
    total_event_data.clear();
    id_statistics.clear();
    id_tzone.clear();

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



    // --- Step 2: Process reading_minutes to derive per-ID min_readings ---
    std::map<std::string, int> id_min_readings;
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

    // --- Optional: Per-row timezone column ---
    bool has_tzone_col = df.containsElementNamed("tzone");
    CharacterVector tz_col;
    if (has_tzone_col) {
      tz_col = as<CharacterVector>(df["tzone"]);
    }

    // --- Step 3: Calculate for each ID separately (CRITICAL for correctness) ---
    std::map<std::string, IntegerVector> id_hyper_results;

    // Calculate hyperglycemic events for each ID separately to ensure proper boundaries
    for (auto const& id_pair : id_indices) {
      std::string current_id = id_pair.first;
      const std::vector<int>& indices = id_pair.second;

      // Assign timezone for this ID
      // If a per-row tzone column exists, use the first non-missing tz within this ID
      std::string tz_for_id = output_tzone;
      if (has_tzone_col) {
        for (size_t j = 0; j < indices.size(); ++j) {
          SEXP val = tz_col[indices[j]];
          if (val != NA_STRING) {
            std::string cand = as<std::string>(val);
            if (!cand.empty()) { tz_for_id = cand; break; }
          }
        }
      }
      id_tzone[current_id] = tz_for_id;

      // Extract subset data for this ID (minimize copying)
      NumericVector time_subset(indices.size());
      NumericVector glucose_subset(indices.size());
      extract_id_subset(current_id, indices, time, glucose, time_subset, glucose_subset);

      // Calculate hyperglycemic events for this ID only, passing start_gl and end_gl and min_readings
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
      
      IntegerVector hyper_events_subset = calculate_hyper_events_for_id(
        time_subset, glucose_subset, id_min_readings[current_id], dur_length, end_length, start_gl, end_gl, id_reading_minutes);

      // Store result
      id_hyper_results[current_id] = hyper_events_subset;

      // Process events for this ID (both standard and total)
      process_events_with_total_optimized(current_id, hyper_events_subset, time_subset, glucose_subset, start_gl, id_reading_minutes);
    }

    // --- Step 4: Merge results back to original order ---
    IntegerVector hyper_events_final = merge_results(id_hyper_results, n);

    // --- Step 5: Create output structures ---
    DataFrame hyper_events_total_df = create_hyper_events_total_df();
    DataFrame events_total_df = create_events_total_df(id, hyper_events_final);

    // --- Step 6: Return final results ---
    return List::create(
      _["events_total"] = events_total_df,
      _["events_detailed"] = hyper_events_total_df
    );
  }
};

// [[Rcpp::export]]
List detect_hyperglycemic_events(DataFrame new_df,
                              SEXP reading_minutes = R_NilValue,
                              double dur_length = 120,
                              double end_length = 15,
                              double start_gl = 250,
                              double end_gl = 180) {
  OptimizedHyperglycemicEventsCalculator calculator;

  if (reading_minutes == R_NilValue) {
    return calculator.calculate_with_parameters(new_df, wrap(5), dur_length, end_length, start_gl, end_gl);
  } else {
    return calculator.calculate_with_parameters(new_df, reading_minutes, dur_length, end_length, start_gl, end_gl);
  }
}
