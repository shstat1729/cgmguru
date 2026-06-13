# Advanced Continuous Glucose Monitoring Analysis and GRID-Based Event Detection

A high-performance R package for comprehensive Continuous Glucose
Monitoring (CGM) data analysis with optimized C++ implementations. The
package provides advanced tools for CGM data analysis with two primary
capabilities: GRID and postprandial peak detection, and extended
glycemic events detection aligned with international consensus CGM
metrics.

## Details

The package implements several key algorithms for CGM analysis:

- **GRID Algorithm**: Detects rapid glucose rate increases (commonly
  \\\geq\\ 90-95 mg/dL/hour) with configurable thresholds and gaps for
  postprandial peak detection

- **Postprandial Peak Detection**: Finds peak glucose after GRID points
  using local maxima and configurable time windows

- **Consensus CGM Metrics Event Detection**: Level 1/2 hypo- and
  hyperglycemia detection with duration validation (default minimum 15
  minutes) aligned with Battelino et al. (2023) international consensus

- **Advanced Analysis Tools**: Local maxima finding, excursion analysis,
  and robust episode validation utilities

Core algorithms are implemented in optimized C++ via 'Rcpp' for accurate
and fast analysis on large datasets, making the package suitable for
both research and clinical applications.

## Relationship to iglu

cgmguru uses the iglu package as a formal methodological reference,
source of example datasets, and comparison target. Event preprocessing
is an independent C++ implementation of iglu-compatible semantics: a
midnight-aligned full-day grid, interpolation up to `inter_gap`, removal
of gap-masked rows, and segment-wise event classification. cgmguru does
not call iglu at runtime for its core algorithms.

The event preprocessing grid applies only to event functions and
[`interpolate_cgm`](https://shstat1729.github.io/cgmguru/reference/interpolate_cgm.md).
GRID-family functions, including
[`grid`](https://shstat1729.github.io/cgmguru/reference/grid.md),
[`maxima_grid`](https://shstat1729.github.io/cgmguru/reference/maxima_grid.md),
and
[`excursion`](https://shstat1729.github.io/cgmguru/reference/excursion.md),
operate on the input timestamps and glucose values supplied by the user.

## Main Functions

- [`grid`](https://shstat1729.github.io/cgmguru/reference/grid.md):

  GRID algorithm for detecting rapid glucose rate increases

- [`maxima_grid`](https://shstat1729.github.io/cgmguru/reference/maxima_grid.md):

  Combined maxima detection and GRID analysis for postprandial peaks

- [`detect_hyperglycemic_events`](https://shstat1729.github.io/cgmguru/reference/detect_hyperglycemic_events.md):

  Hyperglycemic event detection (Level 1/2/Extended)

- [`detect_hypoglycemic_events`](https://shstat1729.github.io/cgmguru/reference/detect_hypoglycemic_events.md):

  Hypoglycemic event detection (Level 1/2/Extended)

- [`detect_all_events`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md):

  Comprehensive detection of all glycemic event types

- [`find_local_maxima`](https://shstat1729.github.io/cgmguru/reference/find_local_maxima.md):

  Local maxima identification in glucose time series

- [`interpolate_cgm`](https://shstat1729.github.io/cgmguru/reference/interpolate_cgm.md):

  C++ iglu-compatible event-grid interpolation

- [`orderfast`](https://shstat1729.github.io/cgmguru/reference/orderfast.md):

  Fast dataframe ordering utility

## Data Requirements

Input dataframes should contain:

- `id`: Patient identifier (character or factor)

- `time`: POSIXct timestamps

- `gl`: Glucose values in mg/dL

All function arguments and return values are expected to be in tibble
format. For convenience, single-column parameters can be passed as
vectors in R, which will be automatically converted to single-column
tibbles.

## Examples


    # Basic GRID analysis
    result <- grid(cgm_data, gap = 15, threshold = 130)

    # Postprandial peak detection (GRID-based)
    maxima <- maxima_grid(cgm_data, threshold = 130, gap = 60, hours = 2)

    # Level 1 Hyperglycemic event detection
    events <- detect_hyperglycemic_events(cgm_data, type = "lv1")

    # Comprehensive event detection; reading_minutes is calculated automatically
    all_events <- detect_all_events(cgm_data)

## References

- Battelino, T., et al. "Continuous glucose monitoring and metrics for
  clinical trials: an international consensus statement." \*The Lancet
  Diabetes & Endocrinology\* 11.1 (2023): 42-57.

- Harvey, Rebecca A., et al. "Design of the glucose rate increase
  detector: a meal detection module for the health monitoring system."
  \*Journal of diabetes science and technology\* 8.2 (2014): 307-320.

- Adolfsson, Peter, et al. "Increased time in range and fewer missed
  bolus injections after introduction of a smart connected insulin pen."
  Diabetes Technology & Therapeutics 22.10 (2020): 709-718.

- Broll, S., Urbanek, J., Buchanan, D., Chun, E., Muschelli, J.,
  Punjabi, N., and Gaynanova, I. "Interpreting blood glucose data with R
  package iglu." PLoS One 16.4 (2021): e0248560.
  doi:10.1371/journal.pone.0248560.

- Chun, E., Fernandes, J. N., and Gaynanova, I. "An Update on the iglu
  Software Package for Interpreting Continuous Glucose Monitoring Data."
  Diabetes Technology & Therapeutics 26.12 (2024): 939-950.
  doi:10.1089/dia.2024.0154.

- Park, Sang Ho, et al. "Identification of clinically meaningful
  automatically detected postprandial glucose excursions in individuals
  with type 1 diabetes using personal continuous glucose monitoring."
  Diabetes Research and Clinical Practice (2025): 112951.

- Park, Soojin, et al. "High-Amplitude and Prolonged Glucose Excursions
  as a Key Determinant of Discordance Between Glucose Management
  Indicator and Glycated Hemoglobin in Type 1 Diabetes." Diabetes Care
  (2026): dc252820. https://doi.org/10.2337/dc25-2820

- Edwards, Stephanie, et al. "Use of connected pen as a diagnostic tool
  to evaluate missed bolus dosing behavior in people with type 1 and
  type 2 diabetes." Diabetes Technology & Therapeutics 24.1 (2022):
  61-66.

For more information about the GRID algorithm and CGM analysis
methodologies, see the package vignette:
[`vignette("intro", package = "cgmguru")`](https://shstat1729.github.io/cgmguru/articles/intro.md)

## See also

[`grid`](https://shstat1729.github.io/cgmguru/reference/grid.md),
[`maxima_grid`](https://shstat1729.github.io/cgmguru/reference/maxima_grid.md),
[`detect_hyperglycemic_events`](https://shstat1729.github.io/cgmguru/reference/detect_hyperglycemic_events.md),
[`detect_all_events`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md)

## Author

Sang Ho Park <shstat1729@gmail.com>
