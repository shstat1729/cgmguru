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
    std::vector<double> avg_episode_duration;
    std::vector<double> avg_episodes_per_day;
    std::vector<double> avg_glucose_in_episodes;

    void reserve(size_t capacity) {
      ids.reserve(capacity);
      types.reserve(capacity);
      levels.reserve(capacity);
      total_events.reserve(capacity);
      avg_episode_duration.reserve(capacity);
      avg_episodes_per_day.reserve(capacity);
      avg_glucose_in_episodes.reserve(capacity);
    }

    void clear() {
      ids.clear();
      types.clear();
      levels.clear();
      total_events.clear();
      avg_episode_duration.clear();
      avg_episodes_per_day.clear();
      avg_glucose_in_episodes.clear();
    }

    void add_entry(const std::string& id, const std::string& type, const std::string& level,
                   int events, double duration, double per_day, double glucose) {
      ids.push_back(id);
      types.push_back(type);
      levels.push_back(level);
      total_events.push_back(events);
      avg_episode_duration.push_back(duration);
      avg_episodes_per_day.push_back(per_day);
      avg_glucose_in_episodes.push_back(glucose);
    }

    size_t size() const { return ids.size(); }
  };

  UnifiedEventData unified_data;

  // Helper structure to store per-ID statistics for each event type
  struct IDEventStatistics {
    std::vector<double> episode_durations;
    std::vector<double> episode_glucose_averages;
    std::vector<double> episode_times;
    double total_days = 0.0;

    void clear() {
      episode_durations.clear();
      episode_glucose_averages.clear();
      episode_times.clear();
      total_days = 0.0;
    }
  };

  // Statistics for each event type+level combination and ID
  std::map<std::string, std::map<std::string, IDEventStatistics>> all_statistics;

  // Helper function to calculate min_readings
  inline int calculate_min_readings(double reading_minutes, double dur_length) const {
    return static_cast<int>(std::ceil((dur_length / reading_minutes) / 4 * 3));
  }

  // Calculate episode metrics (duration and average glucose)
  std::pair<double, double> calculate_episode_metrics(const NumericVector& time_subset,
                                                     const NumericVector& glucose_subset,
                                                     int start_idx, int end_idx) const {
    double glucose_sum = 0.0;
    int glucose_count = 0;

    for (int i = start_idx; i <= end_idx; ++i) {
      if (!NumericVector::is_na(glucose_subset[i])) {
        glucose_sum += glucose_subset[i];
        glucose_count++;
      }
    }

    double duration_minutes = (time_subset[end_idx] - time_subset[start_idx]) / 60.0;
    double average_glucose = (glucose_count > 0) ? glucose_sum / glucose_count : 0.0;

    return std::make_pair(duration_minutes, average_glucose);
  }

  // HYPERGLYCEMIC EVENTS (>250 mg/dL)
  IntegerVector calculate_hyperglycemic_events(const NumericVector& time_subset,
                                             const NumericVector& glucose_subset,
                                             int min_readings,
                                             double dur_length = 120,
                                             double end_length = 15,
                                             double start_gl = 250,
                                             double end_gl = 180) {
    const int n_subset = time_subset.length();
    IntegerVector events(n_subset, 0);

    if (n_subset == 0) return events;

    bool in_event = false;
    int start_idx = -1;
    int j = 0;

    std::vector<bool> valid_glucose(n_subset);
    std::vector<double> glucose_values(n_subset);

    for (int i = 0; i < n_subset; ++i) {
      valid_glucose[i] = !NumericVector::is_na(glucose_subset[i]);
      glucose_values[i] = valid_glucose[i] ? glucose_subset[i] : 0.0;
    }

    while (j < n_subset) {
      // Check for data gap > 3 hours
      if (j < n_subset - 1 && (time_subset[j+1] - time_subset[j]) > 3 * 60 * 60) {
        if (in_event && start_idx != -1) {
          events[j] = -1;
          in_event = false;
          start_idx = -1;
        }
        j++;
        continue;
      }

      // Handle end of data
      if (j == n_subset - 1) {
        if (in_event && start_idx != -1) {
          events[j] = -1;
        }
        break;
      }

      if (!in_event) {
        if (valid_glucose[j] && glucose_values[j] > start_gl) {
          if (validate_hyperglycemic_event(time_subset, valid_glucose, glucose_values,
                                         j, n_subset, min_readings, dur_length, start_gl)) {
            in_event = true;
            start_idx = j;
            events[start_idx] = 2;
          }
        }
      } else {
        if (valid_glucose[j] && glucose_values[j] <= end_gl) {
          if (validate_event_end(time_subset, valid_glucose, glucose_values,
                               j, n_subset, end_length, end_gl, false)) {
            events[j] = -1;
            in_event = false;
            start_idx = -1;
          }
        }
      }
      j++;
    }

    return events;
  }

  // HYPOGLYCEMIC EVENTS (<70 mg/dL)
  IntegerVector calculate_hypoglycemic_events(const NumericVector& time_subset,
                                            const NumericVector& glucose_subset,
                                            int min_readings,
                                            double dur_length = 120,
                                            double end_length = 15,
                                            double start_gl = 70) {
    const int n_subset = time_subset.length();
    IntegerVector events(n_subset, 0);

    if (n_subset == 0) return events;

    bool in_event = false;
    int start_idx = -1;
    int j = 0;

    std::vector<bool> valid_glucose(n_subset);
    std::vector<double> glucose_values(n_subset);

    for (int i = 0; i < n_subset; ++i) {
      valid_glucose[i] = !NumericVector::is_na(glucose_subset[i]);
      glucose_values[i] = valid_glucose[i] ? glucose_subset[i] : 0.0;
    }

    while (j < n_subset) {
      // Check for data gap > 3 hours
      if (j < n_subset - 1 && (time_subset[j+1] - time_subset[j]) > 3 * 60 * 60) {
        if (in_event && start_idx != -1) {
          events[j] = -1;
          in_event = false;
          start_idx = -1;
        }
        j++;
        continue;
      }

      // Handle end of data
      if (j == n_subset - 1) {
        if (in_event && start_idx != -1) {
          events[j] = -1;
        }
        break;
      }

      if (!in_event) {
        if (valid_glucose[j] && glucose_values[j] < start_gl) {
          if (validate_hypoglycemic_event(time_subset, valid_glucose, glucose_values,
                                        j, n_subset, min_readings, dur_length)) {
            in_event = true;
            start_idx = j;
            events[start_idx] = 2;
          }
        }
      } else {
        // Always end at 70 mg/dL for hypoglycemic events
        if (valid_glucose[j] && glucose_values[j] >= 70) {
          if (validate_event_end(time_subset, valid_glucose, glucose_values,
                               j, n_subset, end_length, 70, true)) {
            events[j] = -1;
            in_event = false;
            start_idx = -1;
          }
        }
      }
      j++;
    }

    return events;
  }

  // LEVEL 1 HYPERGLYCEMIC EVENTS (181-250 mg/dL)
  IntegerVector calculate_level1_hyperglycemic_events(const NumericVector& time_subset,
                                                    const NumericVector& glucose_subset,
                                                    int min_readings,
                                                    double dur_length = 15,
                                                    double end_length = 15,
                                                    double start_gl_min = 181,
                                                    double start_gl_max = 250,
                                                    double end_gl = 180) {
    const int n_subset = time_subset.length();
    IntegerVector events(n_subset, 0);

    if (n_subset == 0) return events;

    bool in_event = false;
    int start_idx = -1;
    int j = 0;

    std::vector<bool> valid_glucose(n_subset);
    std::vector<double> glucose_values(n_subset);

    for (int i = 0; i < n_subset; ++i) {
      valid_glucose[i] = !NumericVector::is_na(glucose_subset[i]);
      glucose_values[i] = valid_glucose[i] ? glucose_subset[i] : 0.0;
    }

    while (j < n_subset) {
      // Check for data gap > 3 hours
      if (j < n_subset - 1 && (time_subset[j+1] - time_subset[j]) > 3 * 60 * 60) {
        if (in_event && start_idx != -1) {
          events[j] = -1;
          in_event = false;
          start_idx = -1;
        }
        j++;
        continue;
      }

      // Handle end of data
      if (j == n_subset - 1) {
        if (in_event && start_idx != -1) {
          events[j] = -1;
        }
        break;
      }

      if (!in_event) {
        if (valid_glucose[j] && glucose_values[j] >= start_gl_min && glucose_values[j] <= start_gl_max) {
          if (validate_level1_hyperglycemic_event(time_subset, valid_glucose, glucose_values,
                                                j, n_subset, min_readings, dur_length,
                                                start_gl_min, start_gl_max)) {
            in_event = true;
            start_idx = j;
            events[start_idx] = 2;
          }
        }
      } else {
        if (valid_glucose[j] && glucose_values[j] <= end_gl) {
          if (validate_event_end(time_subset, valid_glucose, glucose_values,
                               j, n_subset, end_length, end_gl, false)) {
            events[j] = -1;
            in_event = false;
            start_idx = -1;
          }
        }
      }
      j++;
    }

    return events;
  }

  // LEVEL 1 HYPOGLYCEMIC EVENTS (54-69 mg/dL)
  IntegerVector calculate_level1_hypoglycemic_events(const NumericVector& time_subset,
                                                   const NumericVector& glucose_subset,
                                                   int min_readings,
                                                   double dur_length = 15,
                                                   double end_length = 15,
                                                   double start_gl_min = 54,
                                                   double start_gl_max = 69,
                                                   double end_gl = 70) {
    const int n_subset = time_subset.length();
    IntegerVector events(n_subset, 0);

    if (n_subset == 0) return events;

    bool in_event = false;
    int start_idx = -1;
    int j = 0;

    std::vector<bool> valid_glucose(n_subset);
    std::vector<double> glucose_values(n_subset);

    for (int i = 0; i < n_subset; ++i) {
      valid_glucose[i] = !NumericVector::is_na(glucose_subset[i]);
      glucose_values[i] = valid_glucose[i] ? glucose_subset[i] : 0.0;
    }

    while (j < n_subset) {
      // Check for data gap > 3 hours
      if (j < n_subset - 1 && (time_subset[j+1] - time_subset[j]) > 3 * 60 * 60) {
        if (in_event && start_idx != -1) {
          events[j] = -1;
          in_event = false;
          start_idx = -1;
        }
        j++;
        continue;
      }

      // Handle end of data
      if (j == n_subset - 1) {
        if (in_event && start_idx != -1) {
          events[j] = -1;
        }
        break;
      }

      if (!in_event) {
        if (valid_glucose[j] && glucose_values[j] >= start_gl_min && glucose_values[j] <= start_gl_max) {
          if (validate_level1_hypoglycemic_event(time_subset, valid_glucose, glucose_values,
                                               j, n_subset, min_readings, dur_length,
                                               start_gl_min, start_gl_max)) {
            in_event = true;
            start_idx = j;
            events[start_idx] = 2;
          }
        }
      } else {
        if (valid_glucose[j] && glucose_values[j] >= end_gl) {
          if (validate_event_end(time_subset, valid_glucose, glucose_values,
                               j, n_subset, end_length, end_gl, true)) {
            events[j] = -1;
            in_event = false;
            start_idx = -1;
          }
        }
      }
      j++;
    }

    return events;
  }

  // Validation functions
  bool validate_hyperglycemic_event(const NumericVector& time_subset,
                                  const std::vector<bool>& valid_glucose,
                                  const std::vector<double>& glucose_values,
                                  int start_idx, int n_subset,
                                  int min_readings, double dur_length,
                                  double start_gl) const {
    const double event_start_time = time_subset[start_idx];

    int valid_readings_count = 1;
    int k = start_idx + 1;
    int last_valid_hyper_idx = start_idx; // Track last valid hyperglycemic reading

    // Process ALL readings, not just within target_end_time
    int recovery_idx = -1; // Track recovery point (first reading <= start_gl)
    while (k < n_subset) {
      if (valid_glucose[k]) {
        valid_readings_count++;
        if (glucose_values[k] <= start_gl) {
          recovery_idx = k; // Mark recovery point
          break; // Exit when glucose recovers
        }
        // Update last valid hyperglycemic reading index
        last_valid_hyper_idx = k;
      }
      k++;
    }

    // Use the recovery point to calculate total episode duration (start to recovery)
    bool reached_duration = false;
    if (recovery_idx != -1) {
      // Episode duration is from start to recovery point
      reached_duration = (time_subset[recovery_idx] - event_start_time) >= dur_length * 60;
    } else if (last_valid_hyper_idx > start_idx) {
      // If no recovery point found, use last hyperglycemic reading (fallback)
      reached_duration = (time_subset[last_valid_hyper_idx] - event_start_time) >= dur_length * 60;
    }

    return reached_duration && (valid_readings_count >= min_readings);
  }

  bool validate_hypoglycemic_event(const NumericVector& time_subset,
                                 const std::vector<bool>& valid_glucose,
                                 const std::vector<double>& glucose_values,
                                 int start_idx, int n_subset,
                                 int min_readings, double dur_length) const {
    const double event_start_time = time_subset[start_idx];

    int valid_readings_count = 1;
    int k = start_idx + 1;
    int last_valid_hypo_idx = start_idx; // Track last valid hypoglycemic reading

    // Process ALL readings, not just within target_end_time
    int recovery_idx = -1; // Track recovery point (first reading >= 70)
    while (k < n_subset) {
      if (valid_glucose[k]) {
        valid_readings_count++;
        if (glucose_values[k] >= 70.0) {
          recovery_idx = k; // Mark recovery point
          break; // Exit when glucose recovers
        }
        // Update last valid hypoglycemic reading index
        last_valid_hypo_idx = k;
      }
      k++;
    }

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

  bool validate_level1_hyperglycemic_event(const NumericVector& time_subset,
                                         const std::vector<bool>& valid_glucose,
                                         const std::vector<double>& glucose_values,
                                         int start_idx, int n_subset,
                                         int min_readings, double dur_length,
                                         double start_gl_min, double start_gl_max) const {
    

    int valid_readings_count = 1;
    double accumulated_time = 0.0;
    int k = start_idx + 1;

    // Process ALL readings, not just within target_end_time
    while (k < n_subset) {
      if (valid_glucose[k]) {
        valid_readings_count++;

        // Exit if glucose goes outside the level 1 range (recovery)
        if (glucose_values[k] < start_gl_min || glucose_values[k] > start_gl_max) {
          break;
        }

        if (glucose_values[k] >= start_gl_min && glucose_values[k] <= start_gl_max) {
          if (k > start_idx) {
            accumulated_time += (time_subset[k] - time_subset[k-1]) / 60.0;
          }
        }
      }
      k++;
    }

    bool reached_duration = accumulated_time >= dur_length;
    return reached_duration && (valid_readings_count >= min_readings);
  }

  bool validate_level1_hypoglycemic_event(const NumericVector& time_subset,
                                        const std::vector<bool>& valid_glucose,
                                        const std::vector<double>& glucose_values,
                                        int start_idx, int n_subset,
                                        int min_readings, double dur_length,
                                        double start_gl_min, double start_gl_max) const {
    

    int valid_readings_count = 1;
    double accumulated_time = 0.0;
    int k = start_idx + 1;

    // Process ALL readings, not just within target_end_time
    while (k < n_subset) {
      if (valid_glucose[k]) {
        valid_readings_count++;

        // Exit if glucose goes outside the level 1 range (recovery)
        if (glucose_values[k] < start_gl_min || glucose_values[k] > start_gl_max) {
          break;
        }

        if (glucose_values[k] >= start_gl_min && glucose_values[k] <= start_gl_max) {
          if (k > start_idx) {
            accumulated_time += (time_subset[k] - time_subset[k-1]) / 60.0;
          }
        }
      }
      k++;
    }

    bool reached_duration = accumulated_time >= dur_length;
    return reached_duration && (valid_readings_count >= min_readings);
  }

  bool validate_event_end(const NumericVector& time_subset,
                        const std::vector<bool>& valid_glucose,
                        const std::vector<double>& glucose_values,
                        int recovery_start, int n_subset,
                        double end_length, double end_gl, bool is_hypo) const {
    const double recovery_start_time = time_subset[recovery_start];
    const double target_end_time = recovery_start_time + end_length * 60;

    int k = recovery_start + 1;

    while (k < n_subset && time_subset[k] <= target_end_time) {
      if (valid_glucose[k]) {
        if (is_hypo && glucose_values[k] < end_gl) {
          return false;
        } else if (!is_hypo && glucose_values[k] > end_gl) {
          return false;
        }
      }
      k++;
    }

    bool reached_duration = (k > recovery_start + 1) ?
      (time_subset[k - 1] - recovery_start_time) >= end_length * 60 : false;

    return reached_duration;
  }

  // Process events and collect statistics with type and level
  void process_events_for_type_level(const std::string& current_id,
                                    const std::string& event_type,
                                    const std::string& event_level,
                                    const IntegerVector& events,
                                    const NumericVector& time_subset,
                                    const NumericVector& glucose_subset) {

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
        auto metrics = calculate_episode_metrics(time_subset, glucose_subset, start_idx, i);
        double duration_mins = metrics.first;
        double avg_glucose = metrics.second;

        all_statistics[event_key][current_id].episode_durations.push_back(duration_mins);
        all_statistics[event_key][current_id].episode_glucose_averages.push_back(avg_glucose);
        all_statistics[event_key][current_id].episode_times.push_back(time_subset[start_idx]);

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
      _["avg_ep_per_day"] = wrap(unified_data.avg_episodes_per_day),
      _["avg_ep_duration"] = wrap(unified_data.avg_episode_duration),
      _["avg_ep_gl"] = wrap(unified_data.avg_glucose_in_episodes),
      _["total_episodes"] = wrap(unified_data.total_events)
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
        time_subset, glucose_subset, id_min_readings_15[current_id], 15, 15, 70);
      process_events_for_type_level(current_id, "hypo", "lv1", hypo_lv1_events, time_subset, glucose_subset);

      // 2. detectHypoglycemicEvents(dataset,start_gl = 54,dur_length=15,end_length=15) # type : hypo, level = lv2
      IntegerVector hypo_lv2_events = calculate_hypoglycemic_events(
        time_subset, glucose_subset, id_min_readings_15[current_id], 15, 15, 54);
      process_events_for_type_level(current_id, "hypo", "lv2", hypo_lv2_events, time_subset, glucose_subset);

      // 3. detectHypoglycemicEvents(dataset) # type : hypo, level = extended (default: <70 mg/dL, 120 min)
      IntegerVector hypo_extended_events = calculate_hypoglycemic_events(
        time_subset, glucose_subset, id_min_readings_120[current_id], 120, 15, 70);
      process_events_for_type_level(current_id, "hypo", "extended", hypo_extended_events, time_subset, glucose_subset);

      // 4. detectLevel1HypoglycemicEvents(dataset) # type : hypo, level = lv1_excl (54-69 mg/dL)
      IntegerVector hypo_lv1_excl_events = calculate_level1_hypoglycemic_events(
        time_subset, glucose_subset, id_min_readings_15[current_id], 15, 15, 54, 69, 70);
      process_events_for_type_level(current_id, "hypo", "lv1_excl", hypo_lv1_excl_events, time_subset, glucose_subset);

      // 5. detectHyperglycemicEvents(dataset, start_gl = 181, dur_length=15, end_length=15, end_gl=180)
      //    # type : hyper, level = lv1
      IntegerVector hyper_lv1_events = calculate_hyperglycemic_events(
        time_subset, glucose_subset, id_min_readings_15[current_id], 15, 15, 181, 180);
      process_events_for_type_level(current_id, "hyper", "lv1", hyper_lv1_events, time_subset, glucose_subset);

      // 6. detectHyperglycemicEvents(dataset, start_gl = 251, dur_length=15, end_length=15, end_gl=250)
      //    # type : hyper, level = lv2
      IntegerVector hyper_lv2_events = calculate_hyperglycemic_events(
        time_subset, glucose_subset, id_min_readings_15[current_id], 15, 15, 251, 250);
      process_events_for_type_level(current_id, "hyper", "lv2", hyper_lv2_events, time_subset, glucose_subset);

      // 7. detectHyperglycemicEvents(dataset) # type : hyper, level = extended
      //    # (default: >250 mg/dL, 120 min)
      IntegerVector hyper_extended_events = calculate_hyperglycemic_events(
        time_subset, glucose_subset, id_min_readings_120[current_id], 120, 15, 250, 180);
      process_events_for_type_level(current_id, "hyper", "extended",
                                   hyper_extended_events, time_subset, glucose_subset);

      // 8. detectLevel1HyperglycemicEvents(dataset) # type : hyper, level = lv1_excl
      //    # (181-250 mg/dL)
      IntegerVector hyper_lv1_excl_events = calculate_level1_hyperglycemic_events(
        time_subset, glucose_subset, id_min_readings_15[current_id], 15, 15, 181, 250, 180);
      process_events_for_type_level(current_id, "hyper", "lv1_excl",
                                   hyper_lv1_excl_events, time_subset, glucose_subset);
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
        double avg_glucose = 0.0;

        if (all_statistics[event_key].find(id_str) != all_statistics[event_key].end()) {
          const IDEventStatistics& stats = all_statistics[event_key][id_str];

          event_count = stats.episode_durations.size();

          if (!stats.episode_durations.empty()) {
            double sum_duration = 0.0;
            for (double d : stats.episode_durations) {
              sum_duration += d;
            }
            avg_duration = sum_duration / stats.episode_durations.size();
          }

          if (stats.total_days > 0) {
            episodes_per_day = static_cast<double>(event_count) / stats.total_days;
          }

          if (!stats.episode_glucose_averages.empty()) {
            double sum_glucose = 0.0;
            for (double g : stats.episode_glucose_averages) {
              sum_glucose += g;
            }
            avg_glucose = sum_glucose / stats.episode_glucose_averages.size();
          }
        }

        // Apply rounding with special handling for zero values
        double rounded_episodes_per_day = (episodes_per_day == 0.0) ? 0.0 : round(episodes_per_day * 100.0) / 100.0;
        double rounded_avg_duration = (avg_duration == 0.0) ? 0.0 : round(avg_duration * 10.0) / 10.0;
        double rounded_avg_glucose = (avg_glucose == 0.0) ? 0.0 : round(avg_glucose * 10.0) / 10.0;

        unified_data.add_entry(id_str, event_type, event_level, event_count,
                              rounded_avg_duration, rounded_episodes_per_day, rounded_avg_glucose);
      }
    }

    return create_unified_events_total_df();
  }
};

// [[Rcpp::export]]
DataFrame detect_all_events(DataFrame df, SEXP reading_minutes = R_NilValue) {
  EnhancedUnifiedEventsCalculator calculator;
  return calculator.calculate_all_events(df, reading_minutes);
}
