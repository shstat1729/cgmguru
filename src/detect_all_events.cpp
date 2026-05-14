#include "id_based_calculator.h"
#include "event_preprocessing.h"
#include <set>

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

  struct CGMSummaryMetrics {
    double TIR = NA_REAL;
    double TITR = NA_REAL;
    double TBR70 = NA_REAL;
    double TBR54 = NA_REAL;
    double TAR180 = NA_REAL;
    double TAR250 = NA_REAL;
    double CV = NA_REAL;
    double SD = NA_REAL;
    double mean_glucose = NA_REAL;
    double GMI = NA_REAL;
    double uGMI = NA_REAL;
    double GRI = NA_REAL;
    double sensor_wear = NA_REAL;
  };

  struct EventSummaryValues {
    int event_count = 0;
  };

  std::map<std::string, CGMSummaryMetrics> cgm_summary_by_id;
  std::map<std::string, std::map<std::string, EventSummaryValues>> event_summary_by_id;

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
    return static_cast<int>(std::ceil(effective_duration / reading_minutes));
  }

  inline bool duration_met(int reading_count, double duration_minutes,
                           double reading_minutes) const {
    const double tolerance_minutes = 0.1;
    return static_cast<double>(reading_count) * reading_minutes +
      tolerance_minutes >= duration_minutes;
  }

  CGMSummaryMetrics calculate_cgm_summary_metrics(
      const NumericVector& glucose_subset) const {
    CGMSummaryMetrics metrics;

    int valid_count = 0;
    int tir_count = 0;
    int titr_count = 0;
    int tbr70_count = 0;
    int tbr54_count = 0;
    int tar180_count = 0;
    int tar250_count = 0;
    int low_count = 0;
    int high_count = 0;
    double sum_glucose = 0.0;
    double sumsq_glucose = 0.0;

    for (int i = 0; i < glucose_subset.length(); ++i) {
      if (NumericVector::is_na(glucose_subset[i])) continue;

      const double gl = glucose_subset[i];
      ++valid_count;
      sum_glucose += gl;
      sumsq_glucose += gl * gl;

      if (gl >= 70.0 && gl <= 180.0) ++tir_count;
      if (gl >= 70.0 && gl <= 140.0) ++titr_count;
      if (gl < 70.0) ++tbr70_count;
      if (gl < 54.0) ++tbr54_count;
      if (gl > 180.0) ++tar180_count;
      if (gl > 250.0) ++tar250_count;
      if (gl >= 54.0 && gl < 70.0) ++low_count;
      if (gl > 180.0 && gl <= 250.0) ++high_count;
    }

    if (valid_count == 0) return metrics;

    const double valid_denominator = static_cast<double>(valid_count);
    metrics.TIR = 100.0 * static_cast<double>(tir_count) / valid_denominator;
    metrics.TITR = 100.0 * static_cast<double>(titr_count) / valid_denominator;
    metrics.TBR70 = 100.0 * static_cast<double>(tbr70_count) / valid_denominator;
    metrics.TBR54 = 100.0 * static_cast<double>(tbr54_count) / valid_denominator;
    metrics.TAR180 = 100.0 * static_cast<double>(tar180_count) / valid_denominator;
    metrics.TAR250 = 100.0 * static_cast<double>(tar250_count) / valid_denominator;
    const double vlow_percent =
      100.0 * static_cast<double>(tbr54_count) / valid_denominator;
    const double low_percent =
      100.0 * static_cast<double>(low_count) / valid_denominator;
    const double vhigh_percent =
      100.0 * static_cast<double>(tar250_count) / valid_denominator;
    const double high_percent =
      100.0 * static_cast<double>(high_count) / valid_denominator;
    metrics.GRI = 3.0 * vlow_percent + 2.4 * low_percent +
      1.6 * vhigh_percent + 0.8 * high_percent;
    metrics.mean_glucose = sum_glucose / valid_denominator;
    metrics.GMI = 3.31 + 0.02392 * metrics.mean_glucose;
    if (metrics.mean_glucose != 0.0) {
      metrics.uGMI = 1.0 / (15.36 / metrics.mean_glucose + 0.0425);
    }

    if (valid_count > 1) {
      double variance =
        (sumsq_glucose - (sum_glucose * sum_glucose / valid_denominator)) /
        static_cast<double>(valid_count - 1);
      if (variance < 0.0 && variance > -1e-9) variance = 0.0;
      if (variance >= 0.0) {
        metrics.SD = std::sqrt(variance);
        if (metrics.mean_glucose != 0.0) {
          metrics.CV = metrics.SD / metrics.mean_glucose;
        }
      }
    }

    return metrics;
  }

  double calculate_sensor_wear_percent(const NumericVector& time,
                                       const NumericVector& glucose,
                                       const std::vector<int>& indices,
                                       double reading_minutes) const {
    std::vector<double> valid_times;
    valid_times.reserve(indices.size());

    for (int idx : indices) {
      if (NumericVector::is_na(time[idx]) || NumericVector::is_na(glucose[idx])) {
        continue;
      }

      const double current_time = time[idx];
      if (!valid_times.empty() &&
          std::fabs(current_time - valid_times.back()) < 1e-7) {
        valid_times.back() = current_time;
      } else {
        valid_times.push_back(current_time);
      }
    }

    if (valid_times.empty() || reading_minutes <= 0.0) return NA_REAL;
    if (valid_times.size() == 1) return 100.0;

    const double dt_seconds = reading_minutes * 60.0;
    const double total_minutes =
      std::round((valid_times.back() - valid_times.front()) / 60.0);
    const double theoretical_gl_values =
      std::round(total_minutes / reading_minutes) + 1.0;

    if (theoretical_gl_values <= 0.0) return NA_REAL;

    double gap_minutes = 0.0;
    int gap_count = 0;
    for (size_t i = 1; i < valid_times.size(); ++i) {
      const double diff_minutes = (valid_times[i] - valid_times[i - 1]) / 60.0;
      if (std::round(diff_minutes) > reading_minutes) {
        gap_minutes += diff_minutes;
        ++gap_count;
      }
    }

    const double missing_gl_values =
      std::round((gap_minutes - static_cast<double>(gap_count) *
        (dt_seconds / 60.0)) / reading_minutes);
    return 100.0 * (theoretical_gl_values - missing_gl_values) /
      theoretical_gl_values;
  }

  // Calculate time spent below 54 mg/dL between indices (inclusive)
  inline double calculate_duration_below_54(const NumericVector& time_subset,
                                            const NumericVector& glucose_subset,
                                            int start_idx,
                                            int end_idx,
                                            double reading_minutes) const {
    (void)time_subset;
    if (start_idx < 0 || end_idx < start_idx) return 0.0;
    double duration_minutes = 0.0;
    const double threshold = 54.0;

    for (int i = start_idx; i <= end_idx; ++i) {
      if (NumericVector::is_na(glucose_subset[i])) continue;
      if (glucose_subset[i] < threshold) {
        duration_minutes += reading_minutes;
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
    (void)min_readings;
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
          // Look for recovery starting from the end of core definition
          int recovery_scan_start = event_end_idx + 1;
          for (int i = recovery_scan_start; i < n_subset; ++i) {
            if (!valid_glucose[i]) continue;
            
            if (glucose_values[i] <= end_gl) {
              // Candidate recovery - check whole recovery reading count.
              int recovery_end_idx = -1;
              int recovery_count = 0;
              for (int k = i; k < n_subset; ++k) {
                if (!valid_glucose[k]) continue;
                if (glucose_values[k] > end_gl) {
                  break;
                }
                ++recovery_count;
                if (duration_met(recovery_count, end_length, reading_minutes)) {
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
          if (events[event_start_idx] != 2) {
            events[event_start_idx] = 2;
            if (n_subset - 1 != event_start_idx) {
              events[n_subset - 1] = -1;
            }
            last_event_end_idx = n_subset - 1;
          }
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
    (void)min_readings;
    const int n_subset = time_subset.length();
    IntegerVector events(n_subset, 0);

    if (n_subset == 0) return events;

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
            // Look for recovery starting from the end of core definition
            int recovery_scan_start = event_end_idx + 1;
            for (int i = recovery_scan_start; i < n_subset; ++i) {
                if (!valid_glucose[i]) continue;
                
                if (glucose_values[i] <= end_gl) {
                    // Candidate recovery - check whole recovery reading count.
                    int recovery_end_idx = -1;
                    int recovery_count = 0;
                    for (int k = i; k < n_subset; ++k) {
                        if (!valid_glucose[k]) continue;
                        if (glucose_values[k] > end_gl) {
                            break;
                        }
                        ++recovery_count;
                        if (duration_met(recovery_count, end_length, reading_minutes)) {
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
            if (events[event_start_idx] != 2) {
                events[event_start_idx] = 2;
                if (n_subset - 1 != event_start_idx) {
                    events[n_subset - 1] = -1;
                }
                last_event_end_idx = n_subset - 1;
            }
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
    (void)min_readings;
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

    for (int i = 0; i < n_subset; ++i) {
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
        } else { // glucose >= 70 (recovery candidate)
          // 1) Validate low-phase by whole-number readings on the interpolated grid.
          if (!duration_met(hypo_count, dur_length, reading_minutes)) {
            // Not enough consecutive low readings yet; CANCEL the event
            // because glucose exceeded threshold before meeting duration requirement
            in_hypo_event = false;
            event_start = -1;
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
                recovery_end_idx = k;
                break;
              }
            }

            if (recovery_end_idx != -1) {
              events[event_start] = 2;
              events[recovery_end_idx] = -1; // Confirmation marker at end of recovery time


              // Reset for next episode
              event_start = -1;
              hypo_count = 0;
              in_hypo_event = false;
            } else {
              // Recovery not yet sustained; remain in event
            }
          }
        }
      }
    }

    if (in_hypo_event && event_start != -1 &&
        duration_met(hypo_count, dur_length, reading_minutes)) {
      events[event_start] = 2;
      if (n_subset - 1 != event_start) {
        events[n_subset - 1] = -1;
      }
    }

    return events;
  }

  IntegerVector calculate_segmented_hypoglycemic_events(
      const cgmguru_events::PreparedIDData& prepared,
      int min_readings,
      double dur_length,
      double end_length,
      double start_gl,
      double reading_minutes) {
    IntegerVector events(prepared.time.length(), 0);

    for (const auto& segment : prepared.segments) {
      NumericVector segment_time =
        cgmguru_events::slice_numeric(prepared.time, segment.start, segment.end);
      NumericVector segment_glucose =
        cgmguru_events::slice_numeric(prepared.glucose, segment.start, segment.end);

      IntegerVector segment_events = calculate_hypoglycemic_events(
        segment_time, segment_glucose, min_readings, dur_length, end_length,
        start_gl, reading_minutes);

      for (int i = 0; i < segment_events.length(); ++i) {
        events[segment.start + i] = segment_events[i];
      }
    }

    return events;
  }

  IntegerVector calculate_segmented_hyperglycemic_events(
      const cgmguru_events::PreparedIDData& prepared,
      int min_readings,
      double dur_length,
      double end_length,
      double start_gl,
      double end_gl,
      double reading_minutes,
      bool window_based) {
    IntegerVector events(prepared.time.length(), 0);

    for (const auto& segment : prepared.segments) {
      NumericVector segment_time =
        cgmguru_events::slice_numeric(prepared.time, segment.start, segment.end);
      NumericVector segment_glucose =
        cgmguru_events::slice_numeric(prepared.glucose, segment.start, segment.end);

      IntegerVector segment_events = window_based
        ? calculate_hyperglycemic_events_window(segment_time, segment_glucose,
            min_readings, dur_length, end_length, start_gl, end_gl, reading_minutes)
        : calculate_hyperglycemic_events(segment_time, segment_glucose,
            min_readings, dur_length, end_length, start_gl, end_gl, reading_minutes);

      for (int i = 0; i < segment_events.length(); ++i) {
        events[segment.start + i] = segment_events[i];
      }
    }

    return events;
  }





  // Process events and collect statistics with type and level
  void process_events_for_type_level(const std::string& current_id,
                                    const std::string& event_type,
                                    const std::string& event_level,
                                    const IntegerVector& events,
                                    const NumericVector& time_subset,
                                    const NumericVector& glucose_subset,
                                    double reporting_threshold,
                                    double reading_minutes) {

    // Create composite key for event type + level
    std::string event_key = event_type + "_" + event_level;

    // Calculate total days for this ID and event type (only once)
    if (time_subset.length() > 0 && all_statistics[event_key][current_id].total_days == 0.0) {
      all_statistics[event_key][current_id].total_days =
        cgmguru_events::recording_days(glucose_subset, reading_minutes);
    }

    // Process events and collect statistics
    int start_idx = -1;
    for (int i = 0; i < events.length(); ++i) {
      if (events[i] == 2) {
        start_idx = i;
      } else if (events[i] == -1 && start_idx != -1) {
        int end_idx_for_metrics = start_idx;
        for (int r = i; r >= start_idx; --r) {
          if (NumericVector::is_na(glucose_subset[r])) continue;
          bool is_event_range = (event_type == "hypo")
            ? glucose_subset[r] < reporting_threshold
            : glucose_subset[r] > reporting_threshold;
          if (is_event_range) {
            end_idx_for_metrics = r;
            break;
          }
        }

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

    if (start_idx != -1) {
      int end_idx_for_metrics = start_idx;
      for (int r = events.length() - 1; r >= start_idx; --r) {
        if (NumericVector::is_na(glucose_subset[r])) continue;
        bool is_event_range = (event_type == "hypo")
          ? glucose_subset[r] < reporting_threshold
          : glucose_subset[r] > reporting_threshold;
        if (is_event_range) {
          end_idx_for_metrics = r;
          break;
        }
      }

      if (event_type == "hypo") {
        double dur_below_54 = calculate_duration_below_54(time_subset, glucose_subset,
                                                          start_idx, end_idx_for_metrics,
                                                          reading_minutes);
        all_statistics[event_key][current_id].episode_durations.push_back(dur_below_54);
      }

      all_statistics[event_key][current_id].episode_times.push_back(time_subset[start_idx]);
      all_statistics[event_key][current_id].start_indices.push_back(start_idx + 1);
      all_statistics[event_key][current_id].end_indices.push_back(end_idx_for_metrics + 1);
    }
  }



  // Count events for a specific type
  int count_events(const IntegerVector& events) const {
    return std::count(events.begin(), events.end(), 2);
  }

  int count_lv1_exclusive_events(const IDEventStatistics& lv1_stats,
                                 const IDEventStatistics& lv2_stats) const {
    int exclusive_count = 0;

    for (size_t i = 0; i < lv1_stats.start_indices.size(); ++i) {
      const int lv1_start = lv1_stats.start_indices[i];
      const int lv1_end = lv1_stats.end_indices[i];
      bool overlaps_lv2 = false;

      for (size_t j = 0; j < lv2_stats.start_indices.size(); ++j) {
        const int lv2_start = lv2_stats.start_indices[j];
        const int lv2_end = lv2_stats.end_indices[j];
        if (lv2_start <= lv1_end && lv2_end >= lv1_start) {
          overlaps_lv2 = true;
          break;
        }
      }

      if (!overlaps_lv2) {
        ++exclusive_count;
      }
    }

    return exclusive_count;
  }

  // Create final unified DataFrame with type and level columns as tibble
  DataFrame create_unified_events_total_df() const {
    if (unified_data.size() == 0) {
      DataFrame empty_df = DataFrame::create(
        _["id"] = CharacterVector(),
        _["type"] = CharacterVector(),
        _["level"] = CharacterVector(),
        _["event_count"] = IntegerVector(),
        _["avg_ep_per_day"] = NumericVector(),
        _["avg_episode_duration_below_54"] = NumericVector()
      );
      empty_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
      return empty_df;
    }

    DataFrame df = DataFrame::create(
      _["id"] = wrap(unified_data.ids),
      _["type"] = wrap(unified_data.types),
      _["level"] = wrap(unified_data.levels),
      _["event_count"] = wrap(unified_data.total_events),
      _["avg_ep_per_day"] = wrap(unified_data.avg_episodes_per_day),
      _["avg_episode_duration_below_54"] = wrap(unified_data.avg_episode_duration)
    );

    // Set class attributes to make it a tibble
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    return df;
  }

  DataFrame create_cgm_summary_metrics_df(
      const std::set<std::string>& unique_ids,
      const std::vector<std::pair<std::string, std::string>>& event_combinations) const {
    std::vector<std::string> ids;
    std::vector<double> tir_values;
    std::vector<double> titr_values;
    std::vector<double> tbr70_values;
    std::vector<double> tbr54_values;
    std::vector<double> tar180_values;
    std::vector<double> tar250_values;
    std::vector<double> cv_values;
    std::vector<double> sd_values;
    std::vector<double> mean_glucose_values;
    std::vector<double> gmi_values;
    std::vector<double> ugmi_values;
    std::vector<double> gri_values;
    std::vector<double> sensor_wear_values;

    ids.reserve(unique_ids.size());
    tir_values.reserve(unique_ids.size());
    titr_values.reserve(unique_ids.size());
    tbr70_values.reserve(unique_ids.size());
    tbr54_values.reserve(unique_ids.size());
    tar180_values.reserve(unique_ids.size());
    tar250_values.reserve(unique_ids.size());
    cv_values.reserve(unique_ids.size());
    sd_values.reserve(unique_ids.size());
    mean_glucose_values.reserve(unique_ids.size());
    gmi_values.reserve(unique_ids.size());
    ugmi_values.reserve(unique_ids.size());
    gri_values.reserve(unique_ids.size());
    sensor_wear_values.reserve(unique_ids.size());

    for (const std::string& id_str : unique_ids) {
      ids.push_back(id_str);
      auto metric_it = cgm_summary_by_id.find(id_str);
      const CGMSummaryMetrics metrics =
        (metric_it == cgm_summary_by_id.end()) ? CGMSummaryMetrics() : metric_it->second;

      tir_values.push_back(metrics.TIR);
      titr_values.push_back(metrics.TITR);
      tbr70_values.push_back(metrics.TBR70);
      tbr54_values.push_back(metrics.TBR54);
      tar180_values.push_back(metrics.TAR180);
      tar250_values.push_back(metrics.TAR250);
      cv_values.push_back(metrics.CV);
      sd_values.push_back(metrics.SD);
      mean_glucose_values.push_back(metrics.mean_glucose);
      gmi_values.push_back(metrics.GMI);
      ugmi_values.push_back(metrics.uGMI);
      gri_values.push_back(metrics.GRI);
      sensor_wear_values.push_back(metrics.sensor_wear);
    }

    List columns;
    std::vector<std::string> column_names;
    auto add_column = [&](const std::string& name, const RObject& values) {
      columns.push_back(values);
      column_names.push_back(name);
    };

    add_column("id", wrap(ids));
    add_column("TIR", wrap(tir_values));
    add_column("TITR", wrap(titr_values));
    add_column("TBR70", wrap(tbr70_values));
    add_column("TBR54", wrap(tbr54_values));
    add_column("TAR180", wrap(tar180_values));
    add_column("TAR250", wrap(tar250_values));
    add_column("CV", wrap(cv_values));
    add_column("SD", wrap(sd_values));
    add_column("mean_glucose", wrap(mean_glucose_values));
    add_column("GMI", wrap(gmi_values));
    add_column("uGMI", wrap(ugmi_values));
    add_column("GRI", wrap(gri_values));
    add_column("sensor_wear", wrap(sensor_wear_values));

    for (const auto& event_combo : event_combinations) {
      const std::string prefix = event_combo.first + "_" + event_combo.second;
      std::vector<int> event_counts;

      event_counts.reserve(unique_ids.size());

      for (const std::string& id_str : unique_ids) {
        EventSummaryValues values;
        auto id_event_it = event_summary_by_id.find(id_str);
        if (id_event_it != event_summary_by_id.end()) {
          auto event_it = id_event_it->second.find(prefix);
          if (event_it != id_event_it->second.end()) {
            values = event_it->second;
          }
        }

        event_counts.push_back(values.event_count);
      }

      add_column(prefix + "_event_count", wrap(event_counts));
    }

    columns.attr("names") = wrap(column_names);
    columns.attr("row.names") =
      IntegerVector::create(NA_INTEGER, -static_cast<int>(ids.size()));

    DataFrame df(columns);
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    return df;
  }

public:
  EnhancedUnifiedEventsCalculator() {
    unified_data.reserve(800); // Reserve for 8 event types * ~100 IDs
  }

  // Enhanced main calculation method that implements all 8 requested event detection functions
  RObject calculate_all_events(const DataFrame& df,
                               SEXP reading_minutes_sexp = R_NilValue,
                               bool sort_time = false,
                               double inter_gap = 45,
                               bool return_interpolated = false) {
    (void)return_interpolated;

    // Clear previous results
    unified_data.clear();
    all_statistics.clear();
    cgm_summary_by_id.clear();
    event_summary_by_id.clear();

    // Extract columns from DataFrame
    int n = df.nrows();
    StringVector id = df["id"];
    NumericVector time = df["time"];
    NumericVector glucose = df["gl"];

    // Fallback default timezone from time's tzone attribute or UTC
    std::string default_tz = "UTC";
    RObject tz_attr = time.attr("tzone");
    if (!tz_attr.isNULL()) {
      CharacterVector tz_attr_cv = as<CharacterVector>(tz_attr);
      if (tz_attr_cv.size() > 0 && !CharacterVector::is_na(tz_attr_cv[0])) {
        default_tz = as<std::string>(tz_attr_cv[0]);
      }
    }

    // Group by ID, then optionally sort only the per-id index vectors.
    group_by_id(id, n);
    cgmguru_events::sort_or_validate_id_indices(id_indices, time, sort_time);

    // Process each ID separately for all 8 event types
    for (auto const& id_pair : id_indices) {
      std::string current_id = id_pair.first;
      const std::vector<int>& indices = id_pair.second;

      double reading_minutes =
        cgmguru_events::reading_minutes_for_id(reading_minutes_sexp, time, indices, n);
      const double sensor_wear_reading_minutes = reading_minutes;
      reading_minutes =
        cgmguru_events::iglu_day_grid_reading_minutes(reading_minutes);
      int min_readings_120 = calculate_min_readings(reading_minutes, 120);
      int min_readings_15 = calculate_min_readings(reading_minutes, 15);

      cgmguru_events::PreparedIDData prepared =
        cgmguru_events::prepare_id_data(time, glucose, indices, reading_minutes,
                                        inter_gap, default_tz, true, true);
      CGMSummaryMetrics cgm_summary =
        calculate_cgm_summary_metrics(prepared.glucose);
      cgm_summary.sensor_wear =
        calculate_sensor_wear_percent(time, glucose, indices,
                                      sensor_wear_reading_minutes);
      cgm_summary_by_id[current_id] = cgm_summary;

      // Calculate all 8 event types as specified by user:

      // 1. detectHypoglycemicEvents(dataset,start_gl = 70,dur_length=15,end_length=15) # type : hypo, level = lv1
      IntegerVector hypo_lv1_events = calculate_segmented_hypoglycemic_events(
        prepared, min_readings_15, 15, 15, 70, reading_minutes);
      process_events_for_type_level(current_id, "hypo", "lv1", hypo_lv1_events,
                                    prepared.time, prepared.glucose, 70, reading_minutes);

      // 2. detectHypoglycemicEvents(dataset,start_gl = 54,dur_length=15,end_length=15) # type : hypo, level = lv2
      IntegerVector hypo_lv2_events = calculate_segmented_hypoglycemic_events(
        prepared, min_readings_15, 15, 15, 54, reading_minutes);
      process_events_for_type_level(current_id, "hypo", "lv2", hypo_lv2_events,
                                    prepared.time, prepared.glucose, 54, reading_minutes);

      // 3. detectHypoglycemicEvents(dataset) # type : hypo, level = extended (default: <70 mg/dL, 120 min)
      IntegerVector hypo_extended_events = calculate_segmented_hypoglycemic_events(
        prepared, min_readings_120, 120, 15, 70, reading_minutes);
      process_events_for_type_level(current_id, "hypo", "extended", hypo_extended_events,
                                    prepared.time, prepared.glucose, 70, reading_minutes);

      // 4. detectLevel1HypoglycemicEvents(dataset) # type : hypo, level = lv1_excl (54-69 mg/dL)
      // Note: lv1_excl metrics will be calculated as average of lv1 and lv2 after processing all events

      // 5. detectHyperglycemicEvents(dataset, start_gl = 180, dur_length=15, end_length=15, end_gl=180)
      //    # type : hyper, level = lv1
      IntegerVector hyper_lv1_events = calculate_segmented_hyperglycemic_events(
        prepared, min_readings_15, 15, 15, 180, 180, reading_minutes, false);
      process_events_for_type_level(current_id, "hyper", "lv1", hyper_lv1_events,
                                    prepared.time, prepared.glucose, 180, reading_minutes);

      // 6. detectHyperglycemicEvents(dataset, start_gl = 250, dur_length=15, end_length=15, end_gl=250)
      //    # type : hyper, level = lv2
      IntegerVector hyper_lv2_events = calculate_segmented_hyperglycemic_events(
        prepared, min_readings_15, 15, 15, 250, 250, reading_minutes, false);
      process_events_for_type_level(current_id, "hyper", "lv2", hyper_lv2_events,
                                    prepared.time, prepared.glucose, 250, reading_minutes);

      // 7. detectHyperglycemicEvents(dataset) # type : hyper, level = extended
      //    # (default: >250 mg/dL, 120 min) - using window-based approach
      IntegerVector hyper_extended_events = calculate_segmented_hyperglycemic_events(
        prepared, min_readings_120, 120, 15, 250, 180, reading_minutes, true);
      process_events_for_type_level(current_id, "hyper", "extended",
                                   hyper_extended_events, prepared.time,
                                   prepared.glucose, 180, reading_minutes);

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
          // Match iglu semantics: a Level 1 episode is exclusive only when no
          // Level 2 episode overlaps that Level 1 episode interval.
          std::string lv1_key = event_type + std::string("_") + std::string("lv1");
          std::string lv2_key = event_type + std::string("_") + std::string("lv2");

            const IDEventStatistics& lv1_stats = all_statistics[lv1_key][id_str];
            const IDEventStatistics& lv2_stats = all_statistics[lv2_key][id_str];

            double total_days = lv1_stats.total_days; // use lv1 reference
            event_count = count_lv1_exclusive_events(lv1_stats, lv2_stats);
            if (total_days > 0.0) {
              episodes_per_day = static_cast<double>(event_count) / total_days;
            }



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
        EventSummaryValues event_summary;
        event_summary.event_count = event_count;
        event_summary_by_id[id_str][event_key] = event_summary;
      }
    }

    DataFrame events_long_df = create_unified_events_total_df();
    DataFrame summary_df =
      create_cgm_summary_metrics_df(unique_ids, event_combinations);

    return List::create(
      _["events_long_df"] = events_long_df,
      _["summary_df"] = summary_df
    );
  }
};

// [[Rcpp::export]]
RObject detect_all_events(DataFrame df,
                          SEXP reading_minutes = R_NilValue,
                          bool sort_time = false,
                          double inter_gap = 45,
                          bool return_interpolated = false) {
  EnhancedUnifiedEventsCalculator calculator;
  return calculator.calculate_all_events(df, reading_minutes, sort_time,
                                         inter_gap, return_interpolated);
}
