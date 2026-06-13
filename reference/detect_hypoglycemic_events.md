# Detect Hypoglycemic Events

Identifies and segments hypoglycemic events in CGM data based on
international consensus CGM metrics (Battelino et al., 2023). Use `type`
to select one of three event definitions:

- **Level 1**: \\\geq\\ 15 consecutive min of \\\<\\ 70 mg/dL, ends with
  \\\geq\\ 15 consecutive min \\\geq\\ 70 mg/dL

- **Level 2**: \\\geq\\ 15 consecutive min of \\\<\\ 54 mg/dL, ends with
  \\\geq\\ 15 consecutive min \\\geq\\ 54 mg/dL

- **Extended**: \\\>\\ 120 consecutive min of \\\<\\ 70 mg/dL, ends with
  \\\geq\\ 15 consecutive min \\\geq\\ 70 mg/dL

Events are counted only after glucose remains at or above the recovery
threshold for the specified end length. In `events_detailed`,
`end_time`, `end_glucose`, and `end_index` report the last hypoglycemic
reading immediately before that confirmed recovery period starts.

## Usage

``` r
detect_hypoglycemic_events(df, ..., type = "extended",
 reading_minutes = NULL, sort_time = FALSE, inter_gap = 45,
 return_interpolated = TRUE)
```

## Arguments

- df:

  A dataframe containing continuous glucose monitoring (CGM) data. Must
  include columns:

  - `id`: Subject identifier (string or factor)

  - `time`: Time of measurement (POSIXct)

  - `gl`: Glucose value (integer or numeric, mg/dL)

- ...:

  Custom event criteria supplied by name. Prefer `type` for standard
  Level 1, Level 2, and Extended events. Supported custom criteria are:

  - `dur_length`: Minimum event duration in minutes required for an
    event to qualify.

  - `end_length`: Required recovery duration in minutes before event
    termination is confirmed.

  - `start_gl`: Glucose threshold in mg/dL used to qualify hypoglycemic
    readings. Hypoglycemic readings are below this value, and recovery
    is confirmed after glucose remains at or above this value for
    `end_length` minutes.

- type:

  Hypoglycemia event definition. One of `"extended"` (default), `"lv1"`,
  `"lv2"`, or `"lv1_excl"`.

- reading_minutes:

  Time interval between readings in minutes (optional). If omitted or
  `NULL`, the interval is calculated automatically per id as the median
  positive time difference in the data.

- sort_time:

  Logical. If `TRUE`, sort rows within each id by `time` in C++ before
  interpolation. Defaults to `FALSE`.

- inter_gap:

  Maximum gap in minutes to interpolate across. Defaults to 45; larger
  gaps split event-detection segments.

- return_interpolated:

  Logical. If `TRUE`, include the interpolated grid data used for event
  detection in the returned list. Defaults to `TRUE`.

## Value

A list containing:

- `events_total`: Tibble with summary statistics per subject (id,
  total_episodes, avg_ep_per_day)

- `events_detailed`: Tibble with detailed event information (id,
  start_time, start_glucose, end_time, end_glucose, start_index,
  end_index, duration_below_54_minutes). End fields report the last
  dysglycemic reading before confirmed recovery starts. `start_index`
  and `end_index` are 1-based row positions in the internal interpolated
  event grid, returned as `interpolated_data` when
  `return_interpolated = TRUE`.

- `interpolated_data`: Included when `return_interpolated = TRUE`, with
  columns `id`, `time`, and `gl`.

## Methods

Hypoglycemic events can be detected using either the recommended `type`
argument or named custom threshold and duration criteria.

**1. Preset method using `type` (recommended):** Use `type` when you
want the standard Level 1, Level 2, or Extended hypoglycemia definitions
without manually entering thresholds.

- `type = "lv1"` uses `start_gl = 70`, `dur_length = 15`, and
  `end_length = 15`.

- `type = "lv2"` uses `start_gl = 54`, `dur_length = 15`, and
  `end_length = 15`.

- `type = "extended"` uses `start_gl = 70`, `dur_length = 120`, and
  `end_length = 15`.

- `type = "lv1_excl"` returns Level 1 episodes that do not overlap Level
  2 episodes.

**2. Custom criteria method:** Supply `start_gl`, `dur_length`, and
`end_length` directly when using a custom definition, for example
`detect_hypoglycemic_events(df, start_gl = 70, dur_length = 15, end_length = 15)`
for Level 1 hypoglycemia. If an explicit `type` is supplied together
with custom numeric criteria, the function returns results based on
`type`; the custom criteria are ignored and a warning is issued.

## Units and sampling

\- `reading_minutes` can be a scalar (all rows) or a vector per-row. -
If `reading_minutes` is omitted or `NULL`, it is calculated
automatically per id from timestamp spacing. - Event classification uses
cgmguru's independent C++ implementation of an iglu-compatible,
midnight-aligned full-day grid. Data are linearly interpolated at the
id-specific interval up to `inter_gap`; larger gaps are masked, removed
from the event-classification data, and split segments. - This
preprocessing is specific to event calculation and does not affect
[`grid`](https://shstat1729.github.io/cgmguru/reference/grid.md),
[`maxima_grid`](https://shstat1729.github.io/cgmguru/reference/maxima_grid.md),
or
[`excursion`](https://shstat1729.github.io/cgmguru/reference/excursion.md).

## References

Battelino, T., et al. (2023). Continuous glucose monitoring and metrics
for clinical trials: an international consensus statement. The Lancet
Diabetes & Endocrinology, 11(1), 42-57.

## See also

[detect_all_events](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md)

## Examples

``` r
# Load sample data
library(iglu)
data(example_data_5_subject)
data(example_data_hall)

# Level 1 Hypoglycemia Event (>=15 consecutive min of <70 mg/dL and event
# ends when there is >=15 consecutive min with a CGM sensor value of >=70 mg/dL)
hypo_lv1 <- detect_hypoglycemic_events(example_data_5_subject, type = "lv1")
print(hypo_lv1$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1              1           0.09
#> 2 Subject 2              0           0   
#> 3 Subject 3              1           0.18
#> 4 Subject 4              2           0.16
#> 5 Subject 5              1           0.1 

# Level 2 Hypoglycemia Event (>=15 consecutive min of <54 mg/dL and event
# ends when there is >=15 consecutive min with a CGM sensor value of >=54 mg/dL)
hypo_lv2 <- detect_hypoglycemic_events(example_data_5_subject, type = "lv2")

# Extended Hypoglycemia Event (>120 consecutive min of <70 mg/dL and event
# ends when there is >=15 consecutive min with a CGM sensor value of >=70 mg/dL)
hypo_extended <- detect_hypoglycemic_events(example_data_5_subject, type = "extended")
print(hypo_extended$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1              0              0
#> 2 Subject 2              0              0
#> 3 Subject 3              0              0
#> 4 Subject 4              0              0
#> 5 Subject 5              0              0

# Custom criteria method for the same standard definitions
hypo_lv1_custom <- detect_hypoglycemic_events(
  example_data_5_subject,
  start_gl = 70,
  dur_length = 15,
  end_length = 15
)
hypo_lv2_custom <- detect_hypoglycemic_events(
  example_data_5_subject,
  start_gl = 54,
  dur_length = 15,
  end_length = 15
)
hypo_extended_custom <- detect_hypoglycemic_events(
  example_data_5_subject,
  start_gl = 70,
  dur_length = 120,
  end_length = 15
)

# Compare event rates across levels
cat("Level 1 episodes:", sum(hypo_lv1$events_total$total_episodes), "\n")
#> Level 1 episodes: 5 
cat("Level 2 episodes:", sum(hypo_lv2$events_total$total_episodes), "\n")
#> Level 2 episodes: 0 
cat("Extended episodes:", sum(hypo_extended$events_total$total_episodes), "\n")
#> Extended episodes: 0 

# Analysis on larger dataset with Level 1 criteria
large_hypo <- detect_hypoglycemic_events(example_data_hall, type = "lv1")
print(large_hypo$events_total)
#> # A tibble: 19 × 3
#>    id           total_episodes avg_ep_per_day
#>    <chr>                 <int>          <dbl>
#>  1 1636-69-001               3           0.47
#>  2 1636-69-026               0           0   
#>  3 1636-69-032               0           0   
#>  4 1636-69-090               4           0.61
#>  5 1636-69-091               0           0   
#>  6 1636-69-114               0           0   
#>  7 1636-70-1005              2           0.31
#>  8 1636-70-1010              5           0.78
#>  9 2133-004                  2           0.32
#> 10 2133-015                  2           0.31
#> 11 2133-017                  0           0   
#> 12 2133-018                  0           0   
#> 13 2133-019                  3           0.47
#> 14 2133-021                  1           0.16
#> 15 2133-024                  8           1.26
#> 16 2133-027                  3           0.44
#> 17 2133-035                  1           0.15
#> 18 2133-036                  8           1.1 
#> 19 2133-039                 10           1.33

# Analysis on larger dataset with Level 2 criteria
large_hypo_lv2 <- detect_hypoglycemic_events(example_data_hall, type = "lv2")
print(large_hypo_lv2$events_total)
#> # A tibble: 19 × 3
#>    id           total_episodes avg_ep_per_day
#>    <chr>                 <int>          <dbl>
#>  1 1636-69-001               0           0   
#>  2 1636-69-026               0           0   
#>  3 1636-69-032               0           0   
#>  4 1636-69-090               0           0   
#>  5 1636-69-091               0           0   
#>  6 1636-69-114               0           0   
#>  7 1636-70-1005              1           0.15
#>  8 1636-70-1010              0           0   
#>  9 2133-004                  0           0   
#> 10 2133-015                  0           0   
#> 11 2133-017                  0           0   
#> 12 2133-018                  0           0   
#> 13 2133-019                  0           0   
#> 14 2133-021                  0           0   
#> 15 2133-024                  1           0.16
#> 16 2133-027                  0           0   
#> 17 2133-035                  0           0   
#> 18 2133-036                  0           0   
#> 19 2133-039                  1           0.13

# Analysis on larger dataset with Extended criteria
large_hypo_extended <- detect_hypoglycemic_events(example_data_hall, type = "extended")
print(large_hypo_extended$events_total)
#> # A tibble: 19 × 3
#>    id           total_episodes avg_ep_per_day
#>    <chr>                 <int>          <dbl>
#>  1 1636-69-001               0           0   
#>  2 1636-69-026               0           0   
#>  3 1636-69-032               0           0   
#>  4 1636-69-090               0           0   
#>  5 1636-69-091               0           0   
#>  6 1636-69-114               0           0   
#>  7 1636-70-1005              0           0   
#>  8 1636-70-1010              1           0.16
#>  9 2133-004                  0           0   
#> 10 2133-015                  0           0   
#> 11 2133-017                  0           0   
#> 12 2133-018                  0           0   
#> 13 2133-019                  0           0   
#> 14 2133-021                  0           0   
#> 15 2133-024                  1           0.16
#> 16 2133-027                  1           0.15
#> 17 2133-035                  0           0   
#> 18 2133-036                  1           0.14
#> 19 2133-039                  0           0   
```
