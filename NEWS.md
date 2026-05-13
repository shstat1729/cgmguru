# cgmguru 0.2.2

# cgmguru 0.2.1

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
