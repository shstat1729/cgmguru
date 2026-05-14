#include "id_based_calculator.h"
#include "event_preprocessing.h"

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

  // Timezone to use for outputs; defaults to UTC but will mirror input if present
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



  // Two-phase hyperglycemic event detection
  List calculate_hyper_events_for_id(const NumericVector& time_subset,
                                     const NumericVector& glucose_subset,
                                     int min_readings,
                                     double dur_length = 120,
                                     double end_length = 15,
                                     double start_gl = 250,
                                     double end_gl = 180,
                                     double reading_minutes = 5.0) {
    (void)min_readings;
    const int n_subset = time_subset.length();
    IntegerVector hyper_events_subset(n_subset, 0);
    std::vector<int> event_starts;
    std::vector<int> reported_ends;

    if (n_subset == 0) {
      return List::create(
        _["events"] = hyper_events_subset,
        _["event_starts"] = wrap(event_starts),
        _["reported_ends"] = wrap(reported_ends)
      );
    }

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
    int core_valid_hyper_count = 0;

    // Phase 1: Find continuous core definitions using whole grid-point counts.
    for (int i = 0; i < n_subset; ++i) {
      if (!valid_glucose[i]) continue;

      if (!in_core) {
        if (glucose_values[i] > start_gl) {
          core_start = i;
          core_end = i;
          core_valid_hyper_count = 1;
          in_core = true;
        }
      } else if (glucose_values[i] > start_gl) {
        core_end = i;
        ++core_valid_hyper_count;
      } else {
        if (duration_met(core_valid_hyper_count, dur_length, reading_minutes)) {
          core_events.push_back({core_start, core_end});
        }
        in_core = false;
        core_start = -1;
        core_end = -1;
        core_valid_hyper_count = 0;
      }
    }

    // Handle case where core continues until end of data
    if (in_core && core_start != -1) {
      if (duration_met(core_valid_hyper_count, dur_length, reading_minutes)) {
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

          // Look for recovery starting from the end of core definition
          int recovery_scan_start = event_end_idx + 1;
          bool event_finalized = false;
          
          for (int i = recovery_scan_start; i < n_subset && !event_finalized; ++i) {
            if (!valid_glucose[i]) continue;
            
            if (glucose_values[i] <= end_gl) {
              // Candidate recovery start - check whole recovery reading count.
              int recovery_end_idx = -1;
              int recovery_count = 0;
              
              for (int k = i; k < n_subset; k++) {
                if (!valid_glucose[k]) continue;
                
                // Check if glucose rises above end_gl (recovery broken)
                if (glucose_values[k] > end_gl) {
                  break; // Recovery broken, exit inner loop to continue searching
                }

                ++recovery_count;
                if (duration_met(recovery_count, end_length, reading_minutes)) {
                  recovery_end_idx = k; // end of recovery period
                  break;
                }
              }
              
              // Only finalize event if recovery was truly sustained for end_length
              if (recovery_end_idx != -1) {
                int reported_end_idx = event_end_idx;
                for (int r = i - 1; r >= event_start_idx; --r) {
                  if (valid_glucose[r]) {
                    reported_end_idx = r;
                    break;
                  }
                }
                hyper_events_subset[event_start_idx] = 2;
                hyper_events_subset[recovery_end_idx] = -1; // Confirmation marker at end of recovery time
                event_starts.push_back(event_start_idx);
                reported_ends.push_back(reported_end_idx);
                last_event_end_idx = recovery_end_idx; // Update last event end
                event_finalized = true;
              }
              // If recovery wasn't sustained, continue scanning for next potential recovery
            }
          }
          if (!event_finalized) {
            hyper_events_subset[event_start_idx] = 2;
            if (n_subset - 1 != event_start_idx) {
              hyper_events_subset[n_subset - 1] = -1;
            }
            event_starts.push_back(event_start_idx);
            reported_ends.push_back(event_end_idx);
            last_event_end_idx = n_subset - 1;
          }
        }
      }

    return List::create(
      _["events"] = hyper_events_subset,
      _["event_starts"] = wrap(event_starts),
      _["reported_ends"] = wrap(reported_ends)
    );
  }

  // Window-based hyperglycemic event detection
  List calculate_hyper_events_window_for_id(const NumericVector& time_subset,
                                           const NumericVector& glucose_subset,
                                           int min_readings,
                                           double dur_length = 120,
                                           double end_length = 15,
                                           double start_gl = 250,
                                           double end_gl = 180,
                                           double reading_minutes = 5.0) {
    (void)min_readings;
    const int n_subset = time_subset.length();
    IntegerVector hyper_events_subset(n_subset, 0);
    std::vector<int> event_starts;
    std::vector<int> reported_ends;

    if (n_subset == 0) {
      return List::create(
        _["events"] = hyper_events_subset,
        _["event_starts"] = wrap(event_starts),
        _["reported_ends"] = wrap(reported_ends)
      );
    }

    std::vector<bool> valid_glucose(n_subset);
    std::vector<double> glucose_values(n_subset);

    for (int i = 0; i < n_subset; ++i) {
        valid_glucose[i] = !NumericVector::is_na(glucose_subset[i]);
        glucose_values[i] = valid_glucose[i] ? glucose_subset[i] : 0.0;
    }

    // Default extended hyperglycemia is 90 minutes within a 120-minute window.
    const double window_duration = dur_length;
    const double required_duration = dur_length * 3.0 / 4.0;
    const double tolerance_minutes = 0.1;

    // Phase 1: Find core definitions using sliding window approach
    struct CoreEvent {
        int start_idx;
        int end_idx;
    };
    std::vector<CoreEvent> core_events;

    // Slide window across time series
    for (int window_start = 0; window_start < n_subset; ++window_start) {
        if (!valid_glucose[window_start]) continue;
        
        // Find window end using whole grid-point counts.
        int window_end = window_start;
        
        for (int j = window_start; j < n_subset; ++j) {
            const int window_count = j - window_start + 1;
            if (valid_glucose[j] &&
                static_cast<double>(window_count) * reading_minutes <=
                  window_duration + tolerance_minutes) {
                window_end = j;
            } else {
                break;
            }
        }
        
        // Skip if window is too small
        if (window_end <= window_start) continue;
        
        // Check if glucose > start_gl for at least 90 minutes within this window
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
            }
        }
        
        // Check if window meets criteria: > start_gl for at least 90 minutes
        if (duration_met(valid_hyper_count, required_duration, reading_minutes)) {

            
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

                // Look for recovery starting from the end of core definition
                int recovery_scan_start = event_end_idx + 1;
                bool event_finalized = false;
                
                for (int i = recovery_scan_start; i < n_subset && !event_finalized; ++i) {
                    if (!valid_glucose[i]) continue;
                    
                    if (glucose_values[i] <= end_gl) {
                        // Candidate recovery start - check whole recovery reading count.
                        int recovery_end_idx = -1;
                        int recovery_count = 0;
                        
                        for (int k = i; k < n_subset; k++) {
                            if (!valid_glucose[k]) continue;
                            
                            // Check if glucose rises above end_gl (recovery broken)
                            if (glucose_values[k] > end_gl) {
                                break; // Recovery broken, exit inner loop to continue searching
                            }

                            ++recovery_count;
                            if (duration_met(recovery_count, end_length, reading_minutes)) {
                                recovery_end_idx = k; // end of recovery period
                                break;
                            }
                        }
                        
                        // Only finalize event if recovery was truly sustained for end_length
                        if (recovery_end_idx != -1) {
                            int reported_end_idx = event_end_idx;
                            for (int r = i - 1; r >= event_start_idx; --r) {
                                if (valid_glucose[r]) {
                                    reported_end_idx = r;
                                    break;
                                }
                            }
                            hyper_events_subset[event_start_idx] = 2;
                            hyper_events_subset[recovery_end_idx] = -1; // Confirmation marker at end of recovery time
                            event_starts.push_back(event_start_idx);
                            reported_ends.push_back(reported_end_idx);
                            last_event_end_idx = recovery_end_idx; // Update last event end
                            event_finalized = true;
                        }
                        // If recovery wasn't sustained, continue scanning for next potential recovery
                    }
                }
                if (!event_finalized) {
                    hyper_events_subset[event_start_idx] = 2;
                    if (n_subset - 1 != event_start_idx) {
                        hyper_events_subset[n_subset - 1] = -1;
                    }
                    event_starts.push_back(event_start_idx);
                    reported_ends.push_back(event_end_idx);
                    last_event_end_idx = n_subset - 1;
                }
            }
        
        
    }

    return List::create(
      _["events"] = hyper_events_subset,
      _["event_starts"] = wrap(event_starts),
      _["reported_ends"] = wrap(reported_ends)
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
                                         const IntegerVector& hyper_events_subset,
                                         const NumericVector& time_subset,
                                         const NumericVector& glucose_subset,
                                         const std::vector<int>& event_starts,
                                         const std::vector<int>& reported_ends,
                                         double reading_minutes,
                                         int interpolated_row_offset) {
    (void)hyper_events_subset;

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

    for (size_t event_idx = 0; event_idx < event_starts.size(); ++event_idx) {
      int start_idx = event_starts[event_idx];
      int end_idx_for_metrics = reported_ends[event_idx];

      if (start_idx >= 0 && start_idx < time_subset.length() &&
          end_idx_for_metrics >= start_idx && end_idx_for_metrics < time_subset.length()) {
        // Store in total_event_data
        total_event_data.ids.push_back(current_id);
        total_event_data.start_times.push_back(time_subset[start_idx]);
        total_event_data.start_glucose.push_back(glucose_subset[start_idx]);
        total_event_data.end_times.push_back(time_subset[end_idx_for_metrics]);
        total_event_data.end_glucose.push_back(glucose_subset[end_idx_for_metrics]);
        total_event_data.start_indices.push_back(interpolated_row_offset + start_idx + 1);
        total_event_data.end_indices.push_back(interpolated_row_offset + end_idx_for_metrics + 1);

        total_event_data.timezones.push_back(output_tzone);

        id_statistics[current_id].episode_times.push_back(time_subset[start_idx]);
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
      _["start_index"] = wrap(total_event_data.start_indices),
      _["end_index"] = wrap(total_event_data.end_indices)
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
                                double end_gl = 180,
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

    // Calculate hyperglycemic events for each ID separately to ensure proper boundaries
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

      IntegerVector hyper_events_subset(prepared.time.length(), 0);
      std::vector<int> event_starts;
      std::vector<int> reported_ends;

      for (const auto& segment : prepared.segments) {
        NumericVector segment_time =
          cgmguru_events::slice_numeric(prepared.time, segment.start, segment.end);
        NumericVector segment_glucose =
          cgmguru_events::slice_numeric(prepared.glucose, segment.start, segment.end);

        List hyper_result;
        if (start_gl == end_gl) {
          hyper_result = calculate_hyper_events_for_id(
            segment_time, segment_glucose, min_readings, dur_length, end_length,
            start_gl, end_gl, id_reading_minutes);
        } else {
          hyper_result = calculate_hyper_events_window_for_id(
            segment_time, segment_glucose, min_readings, dur_length, end_length,
            start_gl, end_gl, id_reading_minutes);
        }

        IntegerVector segment_events = as<IntegerVector>(hyper_result["events"]);
        std::vector<int> segment_starts = as<std::vector<int>>(hyper_result["event_starts"]);
        std::vector<int> segment_reported_ends =
          as<std::vector<int>>(hyper_result["reported_ends"]);

        if (lv1_excl) {
          List lv2_result = calculate_hyper_events_for_id(
            segment_time, segment_glucose, min_readings, dur_length, end_length,
            250.0, 250.0, id_reading_minutes);
          std::vector<int> lv2_starts =
            as<std::vector<int>>(lv2_result["event_starts"]);
          std::vector<int> lv2_reported_ends =
            as<std::vector<int>>(lv2_result["reported_ends"]);

          IntegerVector filtered_events(segment_events.length(), 0);
          std::vector<int> filtered_starts;
          std::vector<int> filtered_reported_ends;

          for (size_t i = 0; i < segment_starts.size(); ++i) {
            if (!overlaps_any_event(segment_starts[i], segment_reported_ends[i],
                                    lv2_starts, lv2_reported_ends)) {
              filtered_starts.push_back(segment_starts[i]);
              filtered_reported_ends.push_back(segment_reported_ends[i]);
              filtered_events[segment_starts[i]] = 2;
              if (segment_reported_ends[i] != segment_starts[i]) {
                filtered_events[segment_reported_ends[i]] = -1;
              }
            }
          }

          segment_events = filtered_events;
          segment_starts = filtered_starts;
          segment_reported_ends = filtered_reported_ends;
        }

        for (int i = 0; i < segment_events.length(); ++i) {
          hyper_events_subset[segment.start + i] = segment_events[i];
        }
        for (size_t i = 0; i < segment_starts.size(); ++i) {
          event_starts.push_back(segment.start + segment_starts[i]);
          reported_ends.push_back(segment.start + segment_reported_ends[i]);
        }
      }

      // Process events for this ID (both standard and total)
      process_events_with_total_optimized(current_id, hyper_events_subset,
                                          prepared.time, prepared.glucose,
                                          event_starts, reported_ends,
                                          id_reading_minutes,
                                          interpolated_row_offset);
    }

    // --- Step 3: Create output structures ---
    DataFrame hyper_events_total_df = create_hyper_events_total_df();
    DataFrame events_total_df = create_events_total_df(unique_ids);

    List result = List::create(
      _["events_total"] = events_total_df,
      _["events_detailed"] = hyper_events_total_df
    );

    if (return_interpolated) {
      result["interpolated_data"] = interpolated_data.to_dataframe(output_tzone, false);
    }

    return result;
  }
};

// [[Rcpp::export]]
List detect_hyperglycemic_events(DataFrame df,
                              SEXP reading_minutes = R_NilValue,
                              double dur_length = 120,
                              double end_length = 15,
                              double start_gl = 250,
                              double end_gl = 180,
                              bool sort_time = false,
                              double inter_gap = 45,
                              bool return_interpolated = true,
                              bool lv1_excl = false) {
  OptimizedHyperglycemicEventsCalculator calculator;
  return calculator.calculate_with_parameters(df, reading_minutes, dur_length,
                                              end_length, start_gl, end_gl,
                                              sort_time, inter_gap,
                                              return_interpolated, lv1_excl);
}
