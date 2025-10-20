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
    std::vector<std::string> timezones;

    void reserve(size_t capacity) {
      ids.reserve(capacity);
      start_times.reserve(capacity);
      end_times.reserve(capacity);
      start_glucose.reserve(capacity);
      end_glucose.reserve(capacity);
      start_indices.reserve(capacity);
      end_indices.reserve(capacity);
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

  // Timezone to use for outputs; defaults to UTC but will mirror input if present
  std::string output_tzone = "UTC";


  // Helper function to calculate min_readings from reading_minutes and dur_length
  // Apply small tolerance (0.1 min) to handle timestamp jitter around 5-minute sampling
  inline int calculate_min_readings(double reading_minutes, double dur_length = 120) const {
    const double tolerance_minutes = 0.1;
    const double effective_duration = std::max(0.0, dur_length - tolerance_minutes);
    // Retain original 3/4 heuristic and incorporate tolerance via effective_duration
    return static_cast<int>(std::ceil((effective_duration / reading_minutes) / 4 * 3));
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

    // Phase 1: Find core definitions - use original logic for Level 1 events
    // For Level 1 events (start_gl == end_gl), use the original continuous approach
    if (start_gl == end_gl) {
      // Use original continuous logic
      for (int i = 0; i < n_subset; ++i) {
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
            core_end = i;
            if (i > core_start) {  // Calculate duration only within this core event
              core_duration_minutes += (time_subset[i] - time_subset[i-1]) / 60.0;
            }
            // Count valid hyper reading
            core_valid_hyper_count += 1;
          } else {
            // Exited core (glucose <= start_gl) - check if we have enough duration
            // Add reading_minutes to account for the interval represented by the last valid reading
            double total_core_duration = core_duration_minutes + reading_minutes;
            if (total_core_duration + epsilon_minutes >= dur_length && core_valid_hyper_count >= min_readings) {
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
    } else {
      // Original logic for other event types
      for (int i = 0; i < n_subset; ++i) {
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
            core_end = i;
            if (i > core_start) {  // Calculate duration only within this core event
              core_duration_minutes += (time_subset[i] - time_subset[i-1]) / 60.0;
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
          bool event_finalized = false;
          
          for (int i = recovery_scan_start; i < n_subset && !event_finalized; ++i) {
            if (!valid_glucose[i]) continue;
            
            if (glucose_values[i] <= end_gl) {
              // Candidate recovery start - check if sustained for end_length minutes
              sustained_secs = 0.0;
              int recovery_end_idx = -1;
              bool recovery_sustained = false;
              
              for (int k = i; k < n_subset; k++) {
                if (!valid_glucose[k]) continue;
                
                // Check if glucose rises above end_gl (recovery broken)
                if (glucose_values[k] > end_gl) {
                  break; // Recovery broken, exit inner loop to continue searching
                }
                
                // Accumulate time while glucose remains <= end_gl
                if (k < n_subset - 1) {
                  sustained_secs += time_subset[k+1] - time_subset[k];
                } else {
                  // Last reading in the sequence - add reading_minutes
                  sustained_secs += reading_minutes * 60.0;
                }
                
                // Check if recovery duration meets threshold (with reading_minutes tolerance for clinical judgment)
                double minutes_so_far = sustained_secs / 60.0;
                double required_recovery_minutes = end_length + reading_minutes;
                if (minutes_so_far >= required_recovery_minutes) {
                  recovery_end_idx = k; // end of recovery period
                  recovery_sustained = true;
                  break;
                }
              }
              
              // Only finalize event if recovery was truly sustained for end_length
              if (recovery_sustained && recovery_end_idx != -1) {
                hyper_events_subset[event_start_idx] = 2;
                hyper_events_subset[recovery_end_idx] = -1; // End at end of recovery time
                last_event_end_idx = recovery_end_idx; // Update last event end
                event_finalized = true;
              }
              // If recovery wasn't sustained, continue scanning for next potential recovery
            }
          }
          
          // Do not finalize event without confirmed sustained recovery
        }
      }

    return hyper_events_subset;
  }

  // Window-based hyperglycemic event detection
  IntegerVector calculate_hyper_events_window_for_id(const NumericVector& time_subset,
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
                // Set last_hyper_idx to current position when still in core
                last_hyper_idx = i;
                valid_hyper_count++;
                
                // Add duration contribution
                if (i < window_end) {
                    hyper_duration += (time_subset[i] - time_subset[i-1]) / 60.0;
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
    // (Same logic as original function)
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
                bool event_finalized = false;
                
                for (int i = recovery_scan_start; i < n_subset && !event_finalized; ++i) {
                    if (!valid_glucose[i]) continue;
                    
                    if (glucose_values[i] <= end_gl) {
                        // Candidate recovery start - check if sustained for end_length minutes
                        sustained_secs = 0.0;
                        int recovery_end_idx = -1;
                        bool recovery_sustained = false;
                        
                        for (int k = i; k < n_subset; k++) {
                            if (!valid_glucose[k]) continue;
                            
                            // Check if glucose rises above end_gl (recovery broken)
                            if (glucose_values[k] > end_gl) {
                                break; // Recovery broken, exit inner loop to continue searching
                            }
                            
                            // Accumulate time while glucose remains <= end_gl
                            if (k < n_subset - 1) {
                                sustained_secs += time_subset[k+1] - time_subset[k];
                            } else {
                                // Last reading in the sequence - add reading_minutes
                                sustained_secs += reading_minutes * 60.0;
                            }

                            // Check if recovery duration meets threshold (with reading_minutes tolerance for clinical judgment)
                            double minutes_so_far = sustained_secs / 60.0;
                            double required_recovery_minutes = end_length + reading_minutes;
                            if (minutes_so_far >= required_recovery_minutes) {
                                recovery_end_idx = k; // end of recovery period
                                recovery_sustained = true;
                                break;
                            }
                        }
                        
                        // Only finalize event if recovery was truly sustained for end_length
                        if (recovery_sustained && recovery_end_idx != -1) {
                            hyper_events_subset[event_start_idx] = 2;
                            hyper_events_subset[recovery_end_idx] = -1; // End at end of recovery time
                            last_event_end_idx = recovery_end_idx; // Update last event end
                            event_finalized = true;
                        }
                        // If recovery wasn't sustained, continue scanning for next potential recovery
                    }
                }
                
                // Do not finalize event without confirmed sustained recovery
            }
        
        
    }

    return hyper_events_subset;
  }


  // Enhanced episode processing that also stores data for total DataFrame
  void process_events_with_total_optimized(const std::string& current_id,
                                         const IntegerVector& hyper_events_subset,
                                         const NumericVector& time_subset,
                                         const NumericVector& glucose_subset,
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

          // Store in total_event_data
          total_event_data.ids.push_back(current_id);
          total_event_data.start_times.push_back(time_subset[start_idx]);
          total_event_data.start_glucose.push_back(glucose_subset[start_idx]);
          total_event_data.end_times.push_back(time_subset[end_idx_for_metrics]);
          total_event_data.end_glucose.push_back(glucose_subset[end_idx_for_metrics]);
          total_event_data.start_indices.push_back(indices[start_idx] + 1); // Convert to 1-based R index
          total_event_data.end_indices.push_back(indices[end_idx_for_metrics] + 1); // Convert to 1-based R index

          total_event_data.timezones.push_back(output_tzone);

          // Store statistics for this ID - use updated event duration
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
    std::string tz0 = total_event_data.timezones.empty() ? output_tzone : total_event_data.timezones[0];
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
      _["end_indices"] = wrap(total_event_data.end_indices)
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

    // --- Step 1: Extract columns from DataFrame ---
    int n = df.nrows();
    StringVector id = df["id"];
    NumericVector time = df["time"];
    NumericVector glucose = df["gl"];

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
      
      // Calculate hyperglycemic events using appropriate method
      IntegerVector hyper_events_subset;
      if (start_gl == end_gl) {
        hyper_events_subset = calculate_hyper_events_for_id(
          time_subset, glucose_subset, id_min_readings[current_id], dur_length, end_length, start_gl, end_gl, id_reading_minutes);
      } else {
        hyper_events_subset = calculate_hyper_events_window_for_id(
          time_subset, glucose_subset, id_min_readings[current_id], dur_length, end_length, start_gl, end_gl, id_reading_minutes);
      }

      // Store result
      id_hyper_results[current_id] = hyper_events_subset;

      // Process events for this ID (both standard and total)
      process_events_with_total_optimized(current_id, hyper_events_subset, time_subset, glucose_subset, id_reading_minutes);
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
