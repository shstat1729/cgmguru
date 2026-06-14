#ifndef CGMGURU_REBOUND_EVENTS_CORE_H
#define CGMGURU_REBOUND_EVENTS_CORE_H

#include "event_preprocessing.h"

#include <Rcpp.h>
#include <string>
#include <vector>

namespace cgmguru_rebound {

struct LevelOneEvent {
  int start_idx;
  int end_idx;
};

struct ReboundEvent {
  std::string type;
  int bridge_start_idx;
  int rebound_idx;
  int initial_start_idx;
  int initial_end_idx;
  double minutes_to_rebound;
};

inline bool duration_met(int reading_count, double duration_minutes,
                         double reading_minutes) {
  const double tolerance_minutes = 0.1;
  return static_cast<double>(reading_count) * reading_minutes +
    tolerance_minutes >= duration_minutes;
}

inline bool is_valid_glucose(const Rcpp::NumericVector& glucose, int idx) {
  return !Rcpp::NumericVector::is_na(glucose[idx]);
}

inline std::vector<LevelOneEvent> detect_hypo_level1_events(
    const Rcpp::NumericVector& glucose,
    const cgmguru_events::SegmentRange& segment,
    double reading_minutes) {
  std::vector<LevelOneEvent> events;

  bool in_event = false;
  int event_start = -1;
  int hypo_count = 0;
  int last_hypo_idx = -1;

  for (int i = segment.start; i <= segment.end; ++i) {
    if (!is_valid_glucose(glucose, i)) continue;

    if (!in_event) {
      if (glucose[i] < 70.0) {
        in_event = true;
        event_start = i;
        last_hypo_idx = i;
        hypo_count = 1;
      }
      continue;
    }

    if (glucose[i] < 70.0) {
      ++hypo_count;
      last_hypo_idx = i;
      continue;
    }

    if (!duration_met(hypo_count, 15.0, reading_minutes)) {
      in_event = false;
      event_start = -1;
      last_hypo_idx = -1;
      hypo_count = 0;
      continue;
    }

    int recovery_count = 0;
    int recovery_end_idx = -1;
    for (int k = i; k <= segment.end; ++k) {
      if (!is_valid_glucose(glucose, k)) continue;
      if (glucose[k] < 70.0) break;

      ++recovery_count;
      if (duration_met(recovery_count, 15.0, reading_minutes)) {
        recovery_end_idx = k;
        break;
      }
    }

    if (recovery_end_idx != -1) {
      const int reported_end_idx =
        (last_hypo_idx >= event_start) ? last_hypo_idx : event_start;
      events.push_back({event_start, reported_end_idx});

      in_event = false;
      event_start = -1;
      last_hypo_idx = -1;
      hypo_count = 0;
    }
  }

  if (in_event && event_start != -1 &&
      duration_met(hypo_count, 15.0, reading_minutes)) {
    const int reported_end_idx =
      (last_hypo_idx >= event_start) ? last_hypo_idx : event_start;
    events.push_back({event_start, reported_end_idx});
  }

  return events;
}

inline std::vector<LevelOneEvent> detect_hyper_level1_events(
    const Rcpp::NumericVector& glucose,
    const cgmguru_events::SegmentRange& segment,
    double reading_minutes) {
  struct CoreEvent {
    int start_idx;
    int end_idx;
  };

  std::vector<CoreEvent> core_events;
  std::vector<LevelOneEvent> events;

  bool in_core = false;
  int core_start = -1;
  int core_end = -1;
  int hyper_count = 0;

  for (int i = segment.start; i <= segment.end; ++i) {
    if (!is_valid_glucose(glucose, i)) continue;

    if (!in_core) {
      if (glucose[i] > 180.0) {
        in_core = true;
        core_start = i;
        core_end = i;
        hyper_count = 1;
      }
      continue;
    }

    if (glucose[i] > 180.0) {
      core_end = i;
      ++hyper_count;
      continue;
    }

    if (duration_met(hyper_count, 15.0, reading_minutes)) {
      core_events.push_back({core_start, core_end});
    }
    in_core = false;
    core_start = -1;
    core_end = -1;
    hyper_count = 0;
  }

  if (in_core && core_start != -1 &&
      duration_met(hyper_count, 15.0, reading_minutes)) {
    core_events.push_back({core_start, core_end});
  }

  int last_recovery_end_idx = -1;
  for (const CoreEvent& core_event : core_events) {
    const int event_start_idx = core_event.start_idx;
    const int event_end_idx = core_event.end_idx;

    bool is_new_event = true;
    if (last_recovery_end_idx != -1 && event_start_idx <= last_recovery_end_idx) {
      bool recovery_found_between = false;
      for (int i = last_recovery_end_idx + 1; i < event_start_idx; ++i) {
        if (!is_valid_glucose(glucose, i)) continue;
        if (glucose[i] <= 180.0) {
          recovery_found_between = true;
          break;
        }
      }
      is_new_event = recovery_found_between;
    }

    if (!is_new_event) continue;

    bool finalized = false;
    for (int i = event_end_idx + 1; i <= segment.end && !finalized; ++i) {
      if (!is_valid_glucose(glucose, i)) continue;
      if (glucose[i] > 180.0) continue;

      int recovery_count = 0;
      int recovery_end_idx = -1;
      for (int k = i; k <= segment.end; ++k) {
        if (!is_valid_glucose(glucose, k)) continue;
        if (glucose[k] > 180.0) break;

        ++recovery_count;
        if (duration_met(recovery_count, 15.0, reading_minutes)) {
          recovery_end_idx = k;
          break;
        }
      }

      if (recovery_end_idx != -1) {
        int reported_end_idx = event_end_idx;
        for (int r = i - 1; r >= event_start_idx; --r) {
          if (is_valid_glucose(glucose, r)) {
            reported_end_idx = r;
            break;
          }
        }
        events.push_back({event_start_idx, reported_end_idx});
        last_recovery_end_idx = recovery_end_idx;
        finalized = true;
      }
    }

    if (!finalized) {
      events.push_back({event_start_idx, event_end_idx});
      last_recovery_end_idx = segment.end;
    }
  }

  return events;
}

inline void append_rebounds_after_initial_events(
    const std::vector<LevelOneEvent>& initial_events,
    const Rcpp::NumericVector& time,
    const Rcpp::NumericVector& glucose,
    const cgmguru_events::SegmentRange& segment,
    const std::string& rebound_type,
    double rebound_minutes,
    std::vector<ReboundEvent>& out) {
  const double tolerance_minutes = 0.1;

  for (const LevelOneEvent& initial_event : initial_events) {
    const int bridge_start_idx = initial_event.end_idx;
    if (bridge_start_idx < segment.start || bridge_start_idx > segment.end) {
      continue;
    }
    if (Rcpp::NumericVector::is_na(time[bridge_start_idx])) continue;

    for (int i = bridge_start_idx + 1; i <= segment.end; ++i) {
      if (Rcpp::NumericVector::is_na(time[i]) || !is_valid_glucose(glucose, i)) {
        continue;
      }

      const double minutes_to_rebound =
        (time[i] - time[bridge_start_idx]) / 60.0;
      if (minutes_to_rebound > rebound_minutes + tolerance_minutes) {
        break;
      }
      if (minutes_to_rebound < -tolerance_minutes) {
        continue;
      }

      const bool is_rebound =
        (rebound_type == "hypo") ? glucose[i] < 70.0 : glucose[i] > 180.0;
      if (!is_rebound) continue;

      out.push_back({
        rebound_type,
        bridge_start_idx,
        i,
        initial_event.start_idx,
        initial_event.end_idx,
        minutes_to_rebound
      });
      break;
    }
  }
}

inline std::vector<ReboundEvent> detect_rebound_events(
    const cgmguru_events::PreparedIDData& prepared,
    double reading_minutes,
    double rebound_minutes,
    bool include_hypo,
    bool include_hyper) {
  std::vector<ReboundEvent> rebound_events;

  for (const cgmguru_events::SegmentRange& segment : prepared.segments) {
    if (include_hypo) {
      std::vector<LevelOneEvent> initial_hyper_events =
        detect_hyper_level1_events(prepared.glucose, segment, reading_minutes);
      append_rebounds_after_initial_events(
        initial_hyper_events, prepared.time, prepared.glucose, segment,
        "hypo", rebound_minutes, rebound_events);
    }

    if (include_hyper) {
      std::vector<LevelOneEvent> initial_hypo_events =
        detect_hypo_level1_events(prepared.glucose, segment, reading_minutes);
      append_rebounds_after_initial_events(
        initial_hypo_events, prepared.time, prepared.glucose, segment,
        "hyper", rebound_minutes, rebound_events);
    }
  }

  return rebound_events;
}

} // namespace cgmguru_rebound

#endif // CGMGURU_REBOUND_EVENTS_CORE_H
