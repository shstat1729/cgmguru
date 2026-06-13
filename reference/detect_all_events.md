# Detect All Glycemic Events

Comprehensive function to detect all types of glycemic events aligned
with international consensus CGM metrics (Battelino et al., 2023). This
function provides a unified interface for detecting multiple event types
including Level 1/2/Extended hypo- and hyperglycemia, and Level 1
excluded events. Events are counted only after the required recovery
condition is confirmed; duration summaries use the event boundary
immediately before recovery starts. Event preprocessing uses cgmguru's
independent C++ implementation of an iglu-compatible day-based grid:
each subject is interpolated from the first observed day's midnight plus
one reading interval, rather than from the first observed timestamp.
Larger gaps are masked and removed before event classification,
preserving gap-based segment boundaries. This preprocessing is specific
to event calculation and does not affect
[`grid`](https://shstat1729.github.io/cgmguru/reference/grid.md),
[`maxima_grid`](https://shstat1729.github.io/cgmguru/reference/maxima_grid.md),
or
[`excursion`](https://shstat1729.github.io/cgmguru/reference/excursion.md).
CGM summary metrics in `subject_summary` are calculated from the
original raw glucose values by default. Set
`summary_metrics_source = "preprocessed"` to calculate them from the
internal event-preprocessed grid.

## Usage

``` r
detect_all_events(df, reading_minutes = NULL, sort_time = FALSE,
 inter_gap = 45, return_interpolated = FALSE,
 summary_metrics_source = c("raw", "preprocessed"),
 sensor_wear_ndays = NULL)
```

## Arguments

- df:

  A dataframe containing continuous glucose monitoring (CGM) data. Must
  include columns:

  - `id`: Subject identifier (string or factor)

  - `time`: Time of measurement (POSIXct)

  - `gl`: Glucose value (integer or numeric, mg/dL)

- reading_minutes:

  Time interval between readings in minutes (optional). Can be a single
  integer/numeric value (applied to all subjects), a vector matching
  data length, or `NULL`. If omitted or `NULL`, the interval is
  calculated automatically per id as the median positive time difference
  in the data.

- sort_time:

  Logical. If `TRUE`, sort rows within each id by `time` in C++ before
  interpolation. Defaults to `FALSE`.

- inter_gap:

  Maximum gap in minutes to interpolate across. Defaults to 45; larger
  gaps split event-detection segments.

- return_interpolated:

  Logical. If `TRUE`, include the internal event-preprocessed grid as
  `interpolated_data`. Defaults to `FALSE`.

- summary_metrics_source:

  Character. Source glucose values for CGM summary metrics. Defaults to
  `"raw"` for original observed data; use `"preprocessed"` for the
  internal event-preprocessed grid after interpolation and gap masking.
  `sensor_wear_percent` is always calculated from the original
  timestamps and glucose readings.

- sensor_wear_ndays:

  Number of days for fixed-window `sensor_wear_percent` calculation.
  Defaults to `NULL`, which uses the original timestamp span. Set to a
  positive number such as `90` to calculate observed readings over the
  last 90 days for each subject divided by the expected number of
  readings in 90 days.

## Value

A list containing:

- `subject_summary`: One row per subject. CGM summary metric columns are
  calculated on the original raw glucose values by default; set
  `summary_metrics_source = "preprocessed"` to use the
  event-preprocessed glucose grid. Event summaries are included as wide
  `*_total_episodes` columns only.

- `glycemic_event_summary`: One row per subject, event type, and event
  level. Contains the full event summary: `id`, `type`, `level`,
  `total_episodes`, `avg_ep_per_day`, and
  `avg_minutes_below_54_per_episode`.

- `interpolated_data`: Included when `return_interpolated = TRUE`, with
  columns `id`, `time`, and `gl`.

`subject_summary` includes:

- `id`: Subject identifier

- `TIR`: Percent of glucose readings in range 70-180 mg/dL

- `TITR`: Percent of glucose readings in tight range 70-140 mg/dL

- `TBR70`: Percent of glucose readings below 70 mg/dL

- `TBR54`: Percent of glucose readings below 54 mg/dL

- `TAR180`: Percent of glucose readings above 180 mg/dL

- `TAR250`: Percent of glucose readings above 250 mg/dL

- `CV`: Coefficient of variation in percent, \\100 \* SD /
  mean_glucose\\

- `SD`: Sample standard deviation of glucose, mg/dL

- `mean_glucose`: Mean glucose, mg/dL

- `GMI`: Glucose Management Indicator, \\3.31 + 0.02392 \*
  mean_glucose\\

- `uGMI`: Unitless GMI-style metric, \\1 / (15.36 / mean_glucose +
  0.0425)\\

- `GRI`: Glycemia Risk Index, \\3.0 \* VLow + 2.4 \* Low + 1.6 \*
  VHigh + 0.8 \* High\\, where `VLow` is percent time \\\<\\54 mg/dL,
  `Low` is 54-\\\<\\70 mg/dL, `VHigh` is \\\>\\250 mg/dL, and `High` is
  \\\>\\180-\\\leq\\250 mg/dL

- `sensor_wear_percent`: Percent of expected CGM readings observed,
  calculated from the original timestamps using the same automatic range
  method as
  [`iglu::active_percent()`](https://irinagain.github.io/iglu/reference/active_percent.html)
  by default. If `sensor_wear_ndays` is supplied, this is calculated
  over the last N days for each subject.

- `hypo_lv1_total_episodes`: Number of Level 1 hypoglycemia episodes

- `hypo_lv2_total_episodes`: Number of Level 2 hypoglycemia episodes

- `hypo_extended_total_episodes`: Number of extended hypoglycemia
  episodes

- `hypo_lv1_excl_total_episodes`: Number of Level 1 hypoglycemia
  episodes that do not overlap a Level 2 episode

- `hyper_lv1_total_episodes`: Number of Level 1 hyperglycemia episodes

- `hyper_lv2_total_episodes`: Number of Level 2 hyperglycemia episodes

- `hyper_extended_total_episodes`: Number of extended hyperglycemia
  episodes

- `hyper_lv1_excl_total_episodes`: Number of Level 1 hyperglycemia
  episodes that do not overlap a Level 2 episode

`glycemic_event_summary` includes:

- `id`: Subject identifier

- `type`: Event direction, either `"hypo"` or `"hyper"`

- `level`: Event level, one of `"lv1"`, `"lv2"`, `"extended"`, or
  `"lv1_excl"`

- `total_episodes`: Number of episodes for the subject, event direction,
  and event level

- `avg_ep_per_day`: Average episodes per day for the subject, event
  direction, and event level, rounded to two decimals

- `avg_minutes_below_54_per_episode`: For hypoglycemia rows, average
  minutes below 54 mg/dL per episode, rounded to two decimals; for
  hyperglycemia rows, 0

## Event types

\- Hypoglycemia: lv1 (\\\<\\ 70 mg/dL, \\\geq\\ 15 min), lv2 (\\\<\\ 54
mg/dL, \\\geq\\ 15 min), extended (\\\<\\ 70 mg/dL, \\\geq\\ 120 min). -
Hyperglycemia: lv1 (\\\>\\ 180 mg/dL, \\\geq\\ 15 min), lv2 (\\\>\\ 250
mg/dL, \\\geq\\ 15 min), extended (\\\>\\ 250 mg/dL, \\\geq\\ 90 min in
120 min, end \\\leq\\ 180 mg/dL for \\\geq\\ 15 min).

## References

Battelino, T., et al. (2023). Continuous glucose monitoring and metrics
for clinical trials: an international consensus statement. The Lancet
Diabetes & Endocrinology, 11(1), 42-57.

## See also

[detect_hyperglycemic_events](https://shstat1729.github.io/cgmguru/reference/detect_hyperglycemic_events.md),
[detect_hypoglycemic_events](https://shstat1729.github.io/cgmguru/reference/detect_hypoglycemic_events.md)

## Examples

``` r
# Load sample data
library(iglu)
data(example_data_5_subject)
data(example_data_hall)

# Detect all glycemic events; reading_minutes is calculated automatically
# from the timestamp spacing when omitted
all_outputs <- detect_all_events(example_data_5_subject)
print(all_outputs$subject_summary)
#> # A tibble: 5 × 22
#>   id          TIR  TITR TBR70 TBR54 TAR180 TAR250    CV    SD mean_glucose   GMI
#>   <chr>     <dbl> <dbl> <dbl> <dbl>  <dbl>  <dbl> <dbl> <dbl>        <dbl> <dbl>
#> 1 Subject 1  91.7 73.7   0.14  0      8.2    0.38  26.9  33.3         124.  6.27
#> 2 Subject 2  26.4  3.36  0     0     73.6   26.1   24.0  52.4         218.  8.54
#> 3 Subject 3  81.3 49.8   0.33  0     18.3    5.68  29.1  44.8         154.  6.99
#> 4 Subject 4  95.1 67.7   0.27  0.05   4.61   0     22.4  29.1         130.  6.41
#> 5 Subject 5  62.1 30.1   0.1   0     37.8   11.3   33.6  58.6         175.  7.49
#> # ℹ 11 more variables: uGMI <dbl>, GRI <dbl>, sensor_wear_percent <dbl>,
#> #   hypo_lv1_total_episodes <int>, hypo_lv2_total_episodes <int>,
#> #   hypo_extended_total_episodes <int>, hypo_lv1_excl_total_episodes <int>,
#> #   hyper_lv1_total_episodes <int>, hyper_lv2_total_episodes <int>,
#> #   hyper_extended_total_episodes <int>, hyper_lv1_excl_total_episodes <int>
print(all_outputs$glycemic_event_summary)
#> # A tibble: 40 × 6
#>    id        type  level    total_episodes avg_ep_per_day avg_minutes_below_54…¹
#>    <chr>     <chr> <chr>             <int>          <dbl>                  <dbl>
#>  1 Subject 1 hypo  lv1                   1           0.09                      0
#>  2 Subject 1 hypo  lv2                   0           0                         0
#>  3 Subject 1 hypo  extended              0           0                         0
#>  4 Subject 1 hypo  lv1_excl              1           0.09                      0
#>  5 Subject 1 hyper lv1                  16           1.44                      0
#>  6 Subject 1 hyper lv2                   2           0.18                      0
#>  7 Subject 1 hyper extended              0           0                         0
#>  8 Subject 1 hyper lv1_excl             14           1.26                      0
#>  9 Subject 2 hypo  lv1                   0           0                         0
#> 10 Subject 2 hypo  lv2                   0           0                         0
#> # ℹ 30 more rows
#> # ℹ abbreviated name: ¹​avg_minutes_below_54_per_episode

# Detect all events on larger dataset
large_outputs <- detect_all_events(example_data_hall)
print(paste("Total subjects analyzed:", nrow(large_outputs$subject_summary)))
#> [1] "Total subjects analyzed: 19"
```
