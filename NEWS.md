# cgmguru 1.0.1

* Renamed event count output columns to `total_episodes` for standalone
  hypo-/hyperglycemic event summaries and `detect_all_events()` long-format
  event output.
* Changed `detect_all_events()` summary glucose metrics to use original raw
  CGM values by default, with `summary_metrics_source = "preprocessed"` for
  the previous internal event-grid behavior.
* Renamed `detect_all_events()` return tables to `subject_summary` and
  `glycemic_event_summary`.
* Renamed `detect_all_events()` summary columns for clarity:
  `sensor_wear_percent`, `*_total_episodes`, and
  `avg_minutes_below_54_per_episode`; `CV` is now reported as a percent.
* Updated documentation, examples, vignettes, and tests to use
  `total_episodes` consistently.

# cgmguru 1.0.0

* Added iglu-compatible event-grid interpolation to the event detection
  pipeline, including automatic reading interval detection, linear
  interpolation up to `inter_gap`, gap masking, and segment-wise event
  classification.
* Added `interpolate_cgm()` as a standalone helper for inspecting the
  interpolated event grid used by glycemic event functions.
* Added `sensor_wear()` and included observed-data sensor wear in
  `detect_all_events()` summary output.
* Updated `detect_all_events()` to calculate CGM summary metrics on the
  interpolated event grid while returning event and summary tables only.
* Added iglu parity and interpolation-focused tests for glycemic event
  detection.

# cgmguru 0.2.0

* Added preset event definitions with `type = "lv1"`, `"lv2"`, and
  `"extended"` to `detect_hyperglycemic_events()` and
  `detect_hypoglycemic_events()`.
* Updated event boundary reporting so `end_glucose` and `end_index` identify
  the final dysglycemic reading immediately before the confirmed recovery
  period begins.
* Renamed public output index columns to singular forms: `indices` to `index`,
  `start_indices` to `start_index`, `end_indices` to `end_index`,
  `max_indices` to `max_index`, and `min_indices` to `min_index`.
* Added tests for pre-recovery event boundaries in extended hyperglycemic events
  with 5-minute and 15-minute sampling intervals.

# cgmguru 0.1.0

* Initial CRAN submission.
