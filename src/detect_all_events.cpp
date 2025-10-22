#include "id_based_calculator.h"

using namespace Rcpp;
using namespace std;

// Enhanced Unified Events Calculator that combines 8 specific event detection functions
// with type and level categorization as requested by user
class EnhancedUnifiedEventsCalculator : public IdBasedCalculator {
private:
  // Structure to hold event data with type and level information
  struct UnifiedEventData {
    std::vector<std::string> ids;
    std::vector<std::string> types;      // "hypo" or "hyper"
    std::vector<std::string> levels;     // "lv1", "lv2", "extended", "lv1_excl"
    std::vector<int> total_events;
    std::vector<double> avg_episodes_per_day;
    std::vector<double> avg_episode_duration;

    void reserve(size_t capacity) {
      ids.reserve(capacity);
      types.reserve(capacity);
      levels.reserve(capacity);
      total_events.reserve(capacity);
      avg_episodes_per_day.reserve(capacity);
      avg_episode_duration.reserve(capacity);
    }

    void clear() {
      ids.clear();
      types.clear();
      levels.clear();
      total_events.clear();
      avg_episodes_per_day.clear();
      avg_episode_duration.clear();
    }

    void add_entry(const std::string& id, const std::string& type, const std::string& level,
                   int events, double per_day, double duration) {
      ids.push_back(id);
      types.push_back(type);
      levels.push_back(level);
      total_events.push_back(events);
      avg_episodes_per_day.push_back(per_day);
      avg_episode_duration.push_back(duration);
    }

    size_t size() const { return ids.size(); }
  };

  UnifiedEventData unified_data;

  // Helper structure to store per-ID statistics for each event type
  struct IDEventStatistics {
    std::vector<double> episode_durations;
    std::vector<double> episode_glucose_averages;
    std::vector<double> episode_times;
    std::vector<int> start_indices;
    std::vector<int> end_indices;
    double total_days = 0.0;

    void clear() {
      episode_durations.clear();
      episode_glucose_averages.clear();
      episode_times.clear();
      start_indices.clear();
      end_indices.clear();
      total_days = 0.0;
    }
  };

  // Statistics for each event type+level combination and ID
  std::map<std::string, std::map<std::string, IDEventStatistics>> all_statistics;

  // Helper function to calculate min_readings
  // Apply small tolerance (0.1 min) to handle timestamp jitter around 5-minute sampling
  inline int calculate_min_readings(double reading_minutes, double dur_length) const {
    const double tolerance_minutes = 0.1;
    const double effective_duration = std::max(0.0, dur_length - tolerance_minutes);
    // Retain original 3/4 heuristic and incorporate tolerance via effective_duration
    return static_cast<int>(std::ceil((effective_duration / reading_minutes) / 4 * 3));
  }

  // Calculate time spent below 54 mg/dL between indices (inclusive)
  inline double calculate_duration_below_54(const NumericVector& time_subset,
                                            const NumericVector& glucose_subset,
                                            int start_idx,
                                            int end_idx,
                                            double reading_minutes) const {
    if (start_idx < 0 || end_idx < start_idx) return 0.0;
    const int n_subset = time_subset.length();
    double duration_minutes = 0.0;
    const double threshold = 54.0;

    for (int i = start_idx; i <= end_idx; ++i) {
      if (NumericVector::is_na(glucose_subset[i])) continue;
      if (glucose_subset[i] < threshold) {
        double contrib = 0.0;
        if (i < end_idx && i + 1 < n_subset) {
          contrib = (time_subset[i + 1] - time_subset[i]) / 60.0;
        } else if (i + 1 < n_subset) {
          contrib = (time_subset[i + 1] - time_subset[i]) / 60.0;
        } else if (i > start_idx) {
          contrib = (time_subset[i] - time_subset[i - 1]) / 60.0;
        } else {
          contrib = reading_minutes;
        }
        duration_minutes += contrib;
      }
    }
    return duration_minutes;
  }

  // HYPERGLYCEMIC EVENTS - Enhanced with improved logic from detect_hyperglycemic_events.cpp
  IntegerVector calculate_hyperglycemic_events(const NumericVector& time_subset,
                                             const NumericVector& glucose_subset,
                                             int min_readings,
                                             double dur_length = 120,
                                             double end_length = 15,
                                             double start_gl = 250,
                                             double end_gl = 180,
                                             double reading_minutes = 5.0) {
    const int n_subset = time_subset.length();
    IntegerVector events(n_subset, 0);

    if (n_subset == 0) return events;

    std::vector<bool> valid_glucose(n_subset);
    std::vector<double> glucose_values(n_subset);

    for (int i = 0; i < n_subset; ++i) {
      valid_glucose[i] = !NumericVector::is_na(glucose_subset[i]);
      glucose_values[i] = valid_glucose[i] ? glucose_subset[i] : 0.0;
    }

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
      
      // Full event mode: when start_gl = end_gl (e.g., >180 mg/dL with recovery at ≤180 mg/dL)
        
        // Check if this event should be merged with the previous event (no recovery between them)
        bool is_new_event = true;
        if (last_event_end_idx != -1) {
          // If the new core event starts after the previous event's recovery ended,
          // it should always be treated as a new event
          if (event_start_idx > last_event_end_idx) {
            is_new_event = true; // Always a new event if starting after previous recovery ended
          } else {
            // Core event overlaps with or starts during previous event's recovery period
            // Check if there was any recovery between the events
            bool recovery_found_between = false;
            
            for (int i = last_event_end_idx + 1; i < event_start_idx; ++i) {
              if (!valid_glucose[i]) continue;
              
              if (glucose_values[i] <= end_gl) {
                recovery_found_between = true;
                break;
              }
            }
            
            if (!recovery_found_between) {
              is_new_event = false; // Merge with previous event
            }
          }
        }
        
        // Only process if this is a new event
        if (is_new_event) {
          // Look for recovery after core definition and end event just before recovery
          double sustained_secs = 0.0;

          // Look for recovery starting from the end of core definition
          int recovery_scan_start = event_end_idx + 1;
          for (int i = recovery_scan_start; i < n_subset; ++i) {
            if (!valid_glucose[i]) continue;
            
            if (glucose_values[i] <= end_gl) {
              // Candidate recovery - check if sustained for end_length minutes

              sustained_secs = 0.0;
              int recovery_end_idx = -1;
              for (int k = i; k < n_subset - 1 && glucose_values[k] <= end_gl; k++) {
                sustained_secs += time_subset[k+1] - time_subset[k];
                // Determine when recovery duration first meets threshold
                double minutes_so_far = (sustained_secs / 60.0) - reading_minutes;
                if (minutes_so_far >= end_length) {
                  recovery_end_idx = k; // end of recovery period
                  break;
                }
              }
              
              // If recovery duration reached threshold, end event at recovery end
              if (recovery_end_idx != -1) {
                events[event_start_idx] = 2;
                events[recovery_end_idx] = -1; // End at end of recovery time

                last_event_end_idx = recovery_end_idx; // Update last event end
                break;
              }
            }
          }
          
          // Do not finalize event without confirmed sustained recovery
        }
      
    }

    return events;
  }

  // WINDOW-BASED HYPERGLYCEMIC EVENTS - For extended level detection
  IntegerVector calculate_hyperglycemic_events_window(const NumericVector& time_subset,
                                                    const NumericVector& glucose_subset,
                                                    int min_readings,
                                                    double dur_length = 120,
                                                    double end_length = 15,
                                                    double start_gl = 250,
                                                    double end_gl = 180,
                                                    double reading_minutes = 5.0) {
    const int n_subset = time_subset.length();
    IntegerVector events(n_subset, 0);

    if (n_subset == 0) return events;

    std::vector<bool> valid_glucose(n_subset);
    std::vector<double> glucose_values(n_subset);

    for (int i = 0; i < n_subset; ++i) {
        valid_glucose[i] = !NumericVector::is_na(glucose_subset[i]);
        glucose_values[i] = valid_glucose[i] ? glucose_subset[i] : 0.0;
    }

    // Window parameters
    double window_duration = dur_length; // 120 minutes
    double required_duration = dur_length * 3.0 / 4.0; // 90 minutes (3/4 of 120)
    const double epsilon_minutes = 0.1; // tolerance for duration requirement

    // Phase 1: Find core definitions using sliding window approach
    struct CoreEvent {
        int start_idx;
        int end_idx;
    };
    std::vector<CoreEvent> core_events;

    // Slide window across time series
    for (int window_start = 0; window_start < n_subset; ++window_start) {
        if (!valid_glucose[window_start]) continue;
        
        // Find window end (120 minutes from window_start)
        int window_end = window_start;
        double window_time_start = time_subset[window_start];
        
        for (int j = window_start; j < n_subset; ++j) {
            if (valid_glucose[j] && (time_subset[j] - window_time_start) <= window_duration * 60.0) {
                window_end = j;
            } else {
                break;
            }
        }
        
        // Skip if window is too small
        if (window_end <= window_start) continue;
        
        // Check if glucose > start_gl for at least 90 minutes within this window
        double hyper_duration = 0.0;
        int valid_hyper_count = 0;
        int first_hyper_idx = -1;
        int last_hyper_idx = -1;
        
        for (int i = window_start; i <= window_end; ++i) {
            if (!valid_glucose[i]) continue;
            
            if (glucose_values[i] > start_gl) {
                if (first_hyper_idx == -1) {
                    first_hyper_idx = i;
                }
                last_hyper_idx = i;
                valid_hyper_count++;
                
                // Add duration contribution
                if (i < window_end) {
                    hyper_duration += (time_subset[i + 1] - time_subset[i]) / 60.0;
                } else {
                    // Last reading in window - add reading_minutes
                    hyper_duration += reading_minutes;
                }
            }
        }
        
        // Check if window meets criteria: > start_gl for at least 90 minutes
        if (hyper_duration + epsilon_minutes >= required_duration && valid_hyper_count >= min_readings) {
            
            // Check if this window overlaps significantly with existing events
            bool is_new_event = true;
            for (const auto& existing_event : core_events) {
                // If windows overlap by more than 50%, consider it the same event
                double overlap_start = std::max(time_subset[window_start], time_subset[existing_event.start_idx]);
                double overlap_end = std::min(time_subset[window_end], time_subset[existing_event.end_idx]);
                double overlap_duration = overlap_end - overlap_start;
                double window_duration_sec = (time_subset[window_end] - time_subset[window_start]);
                double existing_duration_sec = (time_subset[existing_event.end_idx] - time_subset[existing_event.start_idx]);
                
                if (overlap_duration > 0.5 * std::min(window_duration_sec, existing_duration_sec)) {
                    is_new_event = false;
                    break;
                }
            }
            
            if (is_new_event) {
                core_events.push_back({first_hyper_idx, last_hyper_idx});
            }
        }
    }

    // Phase 2: Process each core event and determine final event boundaries
    int last_event_end_idx = -1;
    
    for (const auto& core_event : core_events) {
        int event_start_idx = core_event.start_idx;
        int event_end_idx = core_event.end_idx;

        // Full event mode: when start_gl = end_gl (same logic as original)
        bool is_new_event = true;
        if (last_event_end_idx != -1) {
            // If the new core event starts after the previous event's recovery ended,
            // it should always be treated as a new event
            if (event_start_idx > last_event_end_idx) {
                is_new_event = true; // Always a new event if starting after previous recovery ended
            } else {
                // Core event overlaps with or starts during previous event's recovery period
                // Check if there was any recovery between the events
                bool recovery_found_between = false;
                
                for (int i = last_event_end_idx + 1; i < event_start_idx; ++i) {
                    if (!valid_glucose[i]) continue;
                    
                    if (glucose_values[i] <= end_gl) {
                        recovery_found_between = true;
                        break;
                    }
                }
                
                if (!recovery_found_between) {
                    is_new_event = false; // Merge with previous event
                }
            }
        }
        
        // Only process if this is a new event
        if (is_new_event) {
            // Look for recovery after core definition and end event just before recovery

            double sustained_secs = 0.0;

            
            // Look for recovery starting from the end of core definition
            int recovery_scan_start = event_end_idx + 1;
            for (int i = recovery_scan_start; i < n_subset; ++i) {
                if (!valid_glucose[i]) continue;
                
                if (glucose_values[i] <= end_gl) {
                    // Candidate recovery - check if sustained for end_length minutes

                    sustained_secs = 0.0;
                    int recovery_end_idx = -1;
                    for (int k = i; k < n_subset - 1 && glucose_values[k] <= end_gl; k++) {
                        sustained_secs += time_subset[k+1] - time_subset[k];

                        double minutes_so_far = (sustained_secs / 60.0) - reading_minutes;
                        if (minutes_so_far >= end_length) {
                            recovery_end_idx = k; // end of recovery period
                            break;
                        }
                    }
                    
                    // End event exactly at the end of recovery period
                    if (recovery_end_idx != -1) {
                        events[event_start_idx] = 2;
                        events[recovery_end_idx] = -1; // End at end of recovery time

                        last_event_end_idx = recovery_end_idx; // Update last event end
                        break;
                    }
                }
            }
            
            // Do not finalize event without confirmed sustained recovery
        }
    }

    return events;
  }

  // HYPOGLYCEMIC EVENTS - Updated to match detect_hypoglycemic_events.cpp exactly
  IntegerVector calculate_hypoglycemic_events(const NumericVector& time_subset,
                                            const NumericVector& glucose_subset,
                                            int min_readings,
                                            double dur_length = 120,
                                            double end_length = 15,
                                            double start_gl = 70,
                                            double reading_minutes = 5.0) {
    const int n_subset = time_subset.length();
    IntegerVector events(n_subset, 0);

    if (n_subset == 0) return events;

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
      // If there is a data gap > (end_length + small tolerance) minutes, do not finalize; just reset state
      double gap_threshold_secs = (end_length + epsilon_minutes) * 60.0;
      if (i > 0 && (time_subset[i] - time_subset[i - 1]) > gap_threshold_secs) {
        if (in_hypo_event && event_start != -1) {
          // Do not finalize the event without confirmed recovery; reset state
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
              for (int k = i; k < n_subset-1 && glucose_values[k] >= start_gl; k++) {
                if (valid_glucose[k] && glucose_values[k] < start_gl) {
                  recovery_broken = true;
                  break;
                }
                sustained_secs += time_subset[k+1] - time_subset[k];
                last_k = k;
              }
              
              // Add reading interval duration to account for the fact that each reading represents a time interval
              double total_recovery_minutes = (sustained_secs / 60.0) - reading_minutes;

              // Accept recovery if:
              // - sustained within window, or
              // - there is no reading within end_length window (large gap), hence treat as sustained by default
              bool no_reading_within_window = !(last_k + 1 < n_subset && (time_subset[last_k + 1] - recovery_start_time) <= recovery_needed_secs);
              if (!recovery_broken && (total_recovery_minutes >= end_length || no_reading_within_window)) {
                // End episode just before recovery starts (at last_hypo_idx)
                events[event_start] = 2;
                events[last_k] = -1; // End at last hypoglycemic reading


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

    return events;
  }





  // Process events and collect statistics with type and level
  void process_events_for_type_level(const std::string& current_id,
                                    const std::string& event_type,
                                    const std::string& event_level,
                                    const IntegerVector& events,
                                    const NumericVector& time_subset,
                                    const NumericVector& glucose_subset,
                                    double reading_minutes) {

    // Create composite key for event type + level
    std::string event_key = event_type + "_" + event_level;

    // Calculate total days for this ID and event type (only once)
    if (time_subset.length() > 0 && all_statistics[event_key][current_id].total_days == 0.0) {
      double total_time_seconds = time_subset[time_subset.length() - 1] - time_subset[0];
      all_statistics[event_key][current_id].total_days = total_time_seconds / (24.0 * 60.0 * 60.0);
    }

    // Process events and collect statistics
    int start_idx = -1;
    for (int i = 0; i < events.length(); ++i) {
      if (events[i] == 2) {
        start_idx = i;
      } else if (events[i] == -1 && start_idx != -1) {
        int end_idx_for_metrics = (i > start_idx) ? (i - 1) : i; // just before recovery start

        // For hypoglycemic episodes, compute duration spent below 54 mg/dL
        if (event_type == "hypo") {
          double dur_below_54 = calculate_duration_below_54(time_subset, glucose_subset,
                                                            start_idx, end_idx_for_metrics,
                                                            reading_minutes);
          all_statistics[event_key][current_id].episode_durations.push_back(dur_below_54);
        }

        all_statistics[event_key][current_id].episode_times.push_back(time_subset[start_idx]);
        all_statistics[event_key][current_id].start_indices.push_back(start_idx + 1); // Convert to 1-based R index
        all_statistics[event_key][current_id].end_indices.push_back(end_idx_for_metrics + 1); // Convert to 1-based R index

        start_idx = -1;
      }
    }
  }



  // Count events for a specific type
  int count_events(const IntegerVector& events) const {
    return std::count(events.begin(), events.end(), 2);
  }

  // Create final unified DataFrame with type and level columns as tibble
  DataFrame create_unified_events_total_df() const {
    if (unified_data.size() == 0) {
      DataFrame empty_df = DataFrame::create();
      empty_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
      return empty_df;
    }

    DataFrame df = DataFrame::create(
      _["id"] = wrap(unified_data.ids),
      _["type"] = wrap(unified_data.types),
      _["level"] = wrap(unified_data.levels),
      _["total_episodes"] = wrap(unified_data.total_events),
      _["avg_ep_per_day"] = wrap(unified_data.avg_episodes_per_day),
      _["avg_episode_duration_below_54"] = wrap(unified_data.avg_episode_duration)
    );

    // Set class attributes to make it a tibble
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    return df;
  }

public:
  EnhancedUnifiedEventsCalculator() {
    unified_data.reserve(800); // Reserve for 8 event types * ~100 IDs
  }

  // Enhanced main calculation method that implements all 8 requested event detection functions
  DataFrame calculate_all_events(const DataFrame& df, SEXP reading_minutes_sexp = R_NilValue) {
    // Clear previous results
    unified_data.clear();
    all_statistics.clear();

    // Extract columns from DataFrame
    int n = df.nrows();
    StringVector id = df["id"];
    NumericVector time = df["time"];
    NumericVector glucose = df["gl"];

    // Prepare per-id timezone mapping
    std::map<std::string, std::string> id_timezones;
    bool has_tz_column = df.containsElementNamed("tz");
    CharacterVector tz_column;
    if (has_tz_column) {
      tz_column = df["tz"];
    }
    // Fallback default timezone from time's tzone attribute or UTC
    std::string default_tz = "UTC";
    RObject tz_attr = time.attr("tzone");
    if (!tz_attr.isNULL()) {
      CharacterVector tz_attr_cv = as<CharacterVector>(tz_attr);
      if (tz_attr_cv.size() > 0 && !CharacterVector::is_na(tz_attr_cv[0])) {
        default_tz = as<std::string>(tz_attr_cv[0]);
      }
    }

    // Process reading_minutes argument (default to 5 minutes)
    std::map<std::string, int> id_min_readings_120;
    std::map<std::string, int> id_min_readings_15;

    double reading_minutes = 5.0;
    if (reading_minutes_sexp != R_NilValue) {
      if (TYPEOF(reading_minutes_sexp) == INTSXP) {
        IntegerVector reading_minutes_int = as<IntegerVector>(reading_minutes_sexp);
        reading_minutes = static_cast<double>(reading_minutes_int[0]);
      } else if (TYPEOF(reading_minutes_sexp) == REALSXP) {
        NumericVector reading_minutes_num = as<NumericVector>(reading_minutes_sexp);
        reading_minutes = reading_minutes_num[0];
      }
    }

    // Group by ID
    group_by_id(id, n);

    // Calculate min_readings for different durations
    for (auto const& id_pair : id_indices) {
      id_min_readings_120[id_pair.first] = calculate_min_readings(reading_minutes, 120);
      id_min_readings_15[id_pair.first] = calculate_min_readings(reading_minutes, 15);
      // Populate timezone per id
      if (has_tz_column) {
        const std::vector<int>& idxs = id_pair.second;
        // Use the first occurrence's tz for this id if available; fallback to default
        if (!idxs.empty() && idxs[0] >= 0 && idxs[0] < tz_column.size() && !CharacterVector::is_na(tz_column[idxs[0]])) {
          id_timezones[id_pair.first] = as<std::string>(tz_column[idxs[0]]);
        } else {
          id_timezones[id_pair.first] = default_tz;
        }
      } else {
        id_timezones[id_pair.first] = default_tz;
      }
    }

    // Process each ID separately for all 8 event types
    for (auto const& id_pair : id_indices) {
      std::string current_id = id_pair.first;
      const std::vector<int>& indices = id_pair.second;

      // Extract subset data for this ID
      NumericVector time_subset(indices.size());
      NumericVector glucose_subset(indices.size());
      extract_id_subset(current_id, indices, time, glucose, time_subset, glucose_subset);

      // Calculate all 8 event types as specified by user:

      // 1. detectHypoglycemicEvents(dataset,start_gl = 70,dur_length=15,end_length=15) # type : hypo, level = lv1
      IntegerVector hypo_lv1_events = calculate_hypoglycemic_events(
        time_subset, glucose_subset, id_min_readings_15[current_id], 15, 15, 70, reading_minutes);
      process_events_for_type_level(current_id, "hypo", "lv1", hypo_lv1_events, time_subset, glucose_subset, reading_minutes);

      // 2. detectHypoglycemicEvents(dataset,start_gl = 54,dur_length=15,end_length=15) # type : hypo, level = lv2
      IntegerVector hypo_lv2_events = calculate_hypoglycemic_events(
        time_subset, glucose_subset, id_min_readings_15[current_id], 15, 15, 54, reading_minutes);
      process_events_for_type_level(current_id, "hypo", "lv2", hypo_lv2_events, time_subset, glucose_subset, reading_minutes);

      // 3. detectHypoglycemicEvents(dataset) # type : hypo, level = extended (default: <70 mg/dL, 120 min)
      IntegerVector hypo_extended_events = calculate_hypoglycemic_events(
        time_subset, glucose_subset, id_min_readings_120[current_id], 120, 15, 70, reading_minutes);
      process_events_for_type_level(current_id, "hypo", "extended", hypo_extended_events, time_subset, glucose_subset, reading_minutes);

      // 4. detectLevel1HypoglycemicEvents(dataset) # type : hypo, level = lv1_excl (54-69 mg/dL)
      // Note: lv1_excl metrics will be calculated as average of lv1 and lv2 after processing all events

      // 5. detectHyperglycemicEvents(dataset, start_gl = 180, dur_length=15, end_length=15, end_gl=180)
      //    # type : hyper, level = lv1
      IntegerVector hyper_lv1_events = calculate_hyperglycemic_events(
        time_subset, glucose_subset, id_min_readings_15[current_id], 15, 15, 180, 180, reading_minutes);
      process_events_for_type_level(current_id, "hyper", "lv1", hyper_lv1_events, time_subset, glucose_subset, reading_minutes);

      // 6. detectHyperglycemicEvents(dataset, start_gl = 250, dur_length=15, end_length=15, end_gl=250)
      //    # type : hyper, level = lv2
      IntegerVector hyper_lv2_events = calculate_hyperglycemic_events(
        time_subset, glucose_subset, id_min_readings_15[current_id], 15, 15, 250, 250, reading_minutes);
      process_events_for_type_level(current_id, "hyper", "lv2", hyper_lv2_events, time_subset, glucose_subset, reading_minutes);

      // 7. detectHyperglycemicEvents(dataset) # type : hyper, level = extended
      //    # (default: >250 mg/dL, 120 min) - using window-based approach
      IntegerVector hyper_extended_events = calculate_hyperglycemic_events_window(
        time_subset, glucose_subset, id_min_readings_120[current_id], 120, 15, 250, 180, reading_minutes);
      process_events_for_type_level(current_id, "hyper", "extended",
                                   hyper_extended_events, time_subset, glucose_subset, reading_minutes);

      // 8. detectLevel1HyperglycemicEvents(dataset) # type : hyper, level = lv1_excl
      //    # (181-250 mg/dL)
      // Note: lv1_excl metrics will be calculated as average of lv1 and lv2 after processing all events
    }



    // Create unified results sorted by ID and event type+level combinations
    std::set<std::string> unique_ids;
    for (auto const& id_pair : id_indices) {
      unique_ids.insert(id_pair.first);
    }

    // Define all 8 event type+level combinations as specified by user
    std::vector<std::pair<std::string, std::string>> event_combinations = {
      {"hypo", "lv1"},       // detectHypoglycemicEvents(start_gl=70, dur_length=15)
      {"hypo", "lv2"},       // detectHypoglycemicEvents(start_gl=54, dur_length=15)
      {"hypo", "extended"},  // detectHypoglycemicEvents() default
      {"hypo", "lv1_excl"},  // detectLevel1HypoglycemicEvents()
      {"hyper", "lv1"},      // detectHyperglycemicEvents(start_gl=181, dur_length=15)
      {"hyper", "lv2"},      // detectHyperglycemicEvents(start_gl=251, dur_length=15)
      {"hyper", "extended"}, // detectHyperglycemicEvents() default
      {"hyper", "lv1_excl"}  // detectLevel1HyperglycemicEvents()
    };

    for (const std::string& id_str : unique_ids) {
      for (const auto& event_combo : event_combinations) {
        std::string event_type = event_combo.first;
        std::string event_level = event_combo.second;
        std::string event_key = event_type + "_" + event_level;

        int event_count = 0;
        double avg_duration = 0.0;
        double episodes_per_day = 0.0;

        if (event_level == "lv1_excl") {
          // Aggregate lv1_excl as lv1 minus lv2 for counts, durations, and ep/day
          std::string lv1_key = event_type + std::string("_") + std::string("lv1");
          std::string lv2_key = event_type + std::string("_") + std::string("lv2");

            const IDEventStatistics& lv1_stats = all_statistics[lv1_key][id_str];
            const IDEventStatistics& lv2_stats = all_statistics[lv2_key][id_str];

            int lv1_count = static_cast<int>(lv1_stats.start_indices.size());
            int lv2_count = static_cast<int>(lv2_stats.start_indices.size());
            int count_diff = lv1_count - lv2_count;
            if (count_diff < 0) count_diff = 0;


            double total_days = lv1_stats.total_days; // use lv1 reference
            if (total_days > 0.0) {
              episodes_per_day = static_cast<double>(count_diff) / total_days;
            }
            event_count = count_diff;



        } else {
          if (all_statistics[event_key].find(id_str) != all_statistics[event_key].end()) {
            const IDEventStatistics& stats = all_statistics[event_key][id_str];

            event_count = stats.start_indices.size();
            // For hypoglycemic events, compute average duration below 54 over episodes if available
            if (event_type == "hypo" && !stats.episode_durations.empty()) {
              double sum = 0.0;
              for (double d : stats.episode_durations) sum += d;
              avg_duration = sum / static_cast<double>(stats.episode_durations.size());
            }

            if (stats.total_days > 0) {
              episodes_per_day = static_cast<double>(event_count) / stats.total_days;
            }

          }
        }

        // Apply rounding with special handling for zero values
        double rounded_episodes_per_day = (episodes_per_day == 0.0) ? 0.0 : round(episodes_per_day * 100.0) / 100.0;
        double rounded_avg_duration = (avg_duration == 0.0) ? 0.0 : round(avg_duration * 100.0) / 100.0;

        unified_data.add_entry(id_str, event_type, event_level,
                               event_count,
                               rounded_episodes_per_day,
                               rounded_avg_duration);
      }
    }

    DataFrame result_df = create_unified_events_total_df();

    return result_df;
  }
};

// [[Rcpp::export]]
DataFrame detect_all_events(DataFrame df, SEXP reading_minutes = R_NilValue) {
  EnhancedUnifiedEventsCalculator calculator;
  return calculator.calculate_all_events(df, reading_minutes);
}

