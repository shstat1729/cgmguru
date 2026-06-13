# Changelog

## cgmguru 1.1.1

- Updated
  [`sensor_wear()`](https://shstat1729.github.io/cgmguru/reference/sensor_wear.md)
  tests to avoid timezone-dependent one-to-one `start_date` comparisons
  against
  [`iglu::active_percent()`](https://irinagain.github.io/iglu/reference/active_percent.html)
  manual windows. Fixed-window sensor wear tests now compare the
  calculated observed/expected reading counts directly, making the
  checks stable across DST-sensitive timezones.
- Expanded the package-level `cgmguru` vignette into a practical CGM
  analysis guide covering data requirements, sensor wear, event
  summaries, event-grid inspection, GRID analysis, postprandial maxima
  workflows, excursions, visualization, and scaling to larger datasets.

## cgmguru 1.1.0

CRAN release: 2026-06-09

- Updated
  [`maxima_grid()`](https://shstat1729.github.io/cgmguru/reference/maxima_grid.md)
  and
  [`detect_between_maxima()`](https://shstat1729.github.io/cgmguru/reference/detect_between_maxima.md)
  to include all subject IDs in `episode_counts`, returning `0` for
  subjects with no detected episodes or between-maxima results.
- Fixed
  [`detect_all_events()`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md)
  to summarize event counts segment-by-segment after interpolation gaps,
  preventing events that end at a gap boundary from being merged into
  the next segment.
- Updated extended hypoglycemia event detection to match iglu by
  requiring duration strictly greater than 120 minutes below 70 mg/dL,
  rather than greater than or equal to 120 minutes.
- Avoided materializing the standalone hypo-/hyperglycemic event grid
  when `return_interpolated = FALSE`, improving speed and memory use for
  calls that do not request the interpolated data.
- Optimized returned interpolated event grids by preallocating storage,
  avoiding repeated ID strings in C++ storage, and skipping unused grid
  metadata.
- Changed
  [`detect_all_events()`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md)
  summary glucose metrics to use original raw CGM values by default,
  with `summary_metrics_source = "preprocessed"` for the previous
  internal event-grid behavior.
- Rounded
  [`detect_all_events()`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md)
  CGM summary metrics and sensor wear outputs to two decimal places.
- Added `sensor_wear_ndays` to
  [`detect_all_events()`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md)
  to calculate `sensor_wear_percent` over a fixed retrospective window,
  such as the last 90 days; when omitted, `sensor_wear_percent`
  continues to use the original timestamp span.
- Updated
  [`sensor_wear()`](https://shstat1729.github.io/cgmguru/reference/sensor_wear.md)
  so the default calculation uses each subject’s original timestamp
  span. Supplying `ndays` now switches to the fixed-window calculation.
- Renamed
  [`detect_all_events()`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md)
  return tables to `subject_summary` and `glycemic_event_summary`.
- Renamed
  [`detect_all_events()`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md)
  summary columns for clarity: `sensor_wear_percent`,
  `*_total_episodes`, and `avg_minutes_below_54_per_episode`; `CV` is
  now reported as a percent.

## cgmguru 1.0.1

CRAN release: 2026-05-14

- Renamed event count output columns to `total_episodes` for standalone
  hypo-/hyperglycemic event summaries and
  [`detect_all_events()`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md)
  long-format event output.
- Updated documentation, examples, vignettes, and tests to use
  `total_episodes` consistently.

## cgmguru 1.0.0

CRAN release: 2026-05-14

- Added iglu-compatible event-grid interpolation to the event detection
  pipeline, including automatic reading interval detection, linear
  interpolation up to `inter_gap`, gap masking, and segment-wise event
  classification.
- Added
  [`interpolate_cgm()`](https://shstat1729.github.io/cgmguru/reference/interpolate_cgm.md)
  as a standalone helper for inspecting the interpolated event grid used
  by glycemic event functions.
- Added
  [`sensor_wear()`](https://shstat1729.github.io/cgmguru/reference/sensor_wear.md)
  and included observed-data sensor wear in
  [`detect_all_events()`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md)
  summary output.
- Updated
  [`detect_all_events()`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md)
  to calculate CGM summary metrics on the interpolated event grid while
  returning event and summary tables only.
- Added iglu parity and interpolation-focused tests for glycemic event
  detection.

## cgmguru 0.2.0

CRAN release: 2026-05-07

- Added preset event definitions with `type = "lv1"`, `"lv2"`, and
  `"extended"` to
  [`detect_hyperglycemic_events()`](https://shstat1729.github.io/cgmguru/reference/detect_hyperglycemic_events.md)
  and
  [`detect_hypoglycemic_events()`](https://shstat1729.github.io/cgmguru/reference/detect_hypoglycemic_events.md).
- Updated event boundary reporting so `end_glucose` and `end_index`
  identify the final dysglycemic reading immediately before the
  confirmed recovery period begins.
- Renamed public output index columns to singular forms: `indices` to
  `index`, `start_indices` to `start_index`, `end_indices` to
  `end_index`, `max_indices` to `max_index`, and `min_indices` to
  `min_index`.
- Added tests for pre-recovery event boundaries in extended
  hyperglycemic events with 5-minute and 15-minute sampling intervals.

## cgmguru 0.1.0

CRAN release: 2025-11-05

- Initial CRAN submission.
