# Detect Hyperglycemic Events

Identifies and segments hyperglycemic events in CGM data based on
international consensus CGM metrics (Battelino et al., 2023). Use `type`
to select one of three event definitions:

- **Level 1**: \\\geq\\ 15 consecutive min of \\\>\\ 180 mg/dL, ends
  with \\\geq\\ 15 consecutive min \\\leq\\ 180 mg/dL

- **Level 2**: \\\geq\\ 15 consecutive min of \\\>\\ 250 mg/dL, ends
  with \\\geq\\ 15 consecutive min \\\leq\\ 250 mg/dL

- **Extended**: \\\>\\ 250 mg/dL lasting \\\geq\\ 90 cumulative min
  within a 120-min period, ends when glucose returns to \\\leq\\ 180
  mg/dL for \\\geq\\ 15 consecutive min after

Events are counted only after glucose remains at or below the end
threshold for the specified end length. In `events_detailed`,
`end_time`, `end_glucose`, and `end_index` report the last hyperglycemic
reading immediately before that confirmed recovery period starts.

## Usage

``` r
detect_hyperglycemic_events(df, ..., type = "extended",
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

  - `start_gl`: Glucose threshold in mg/dL used to qualify hyperglycemic
    readings. Hyperglycemic readings are above this value.

  - `end_gl`: Glucose threshold in mg/dL used to confirm recovery.
    Hyperglycemic events end after glucose remains at or below this
    value for `end_length` minutes.

- type:

  Hyperglycemia event definition. One of `"extended"` (default),
  `"lv1"`, `"lv2"`, or `"lv1_excl"`.

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
  end_index). End fields report the last dysglycemic reading before
  confirmed recovery starts. `start_index` and `end_index` are 1-based
  row positions in the internal interpolated event grid, returned as
  `interpolated_data` when `return_interpolated = TRUE`.

- `interpolated_data`: Included when `return_interpolated = TRUE`, with
  columns `id`, `time`, and `gl`.

## Methods

Hyperglycemic events can be detected using either the recommended `type`
argument or named custom threshold and duration criteria.

**1. Preset method using `type` (recommended):** Use `type` when you
want the standard Level 1, Level 2, or Extended hyperglycemia
definitions without manually entering thresholds.

- `type = "lv1"` uses `start_gl = 180`, `dur_length = 15`,
  `end_length = 15`, and `end_gl = 180`.

- `type = "lv2"` uses `start_gl = 250`, `dur_length = 15`,
  `end_length = 15`, and `end_gl = 250`.

- `type = "extended"` uses `start_gl = 250`, `dur_length = 120`,
  `end_length = 15`, and `end_gl = 180`.

- `type = "lv1_excl"` returns Level 1 episodes that do not overlap Level
  2 episodes.

**2. Custom criteria method:** Supply `start_gl`, `dur_length`,
`end_length`, and `end_gl` directly when using a custom definition, for
example
`detect_hyperglycemic_events(df, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)`
for Level 1 hyperglycemia. If an explicit `type` is supplied together
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

# Level 1 Hyperglycemia Event (>=15 consecutive min of >180 mg/dL and event
# ends when there is >=15 consecutive min with a CGM sensor value of <=180 mg/dL)
hyper_lv1 <- detect_hyperglycemic_events(example_data_5_subject, type = "lv1")
print(hyper_lv1$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1             16           1.44
#> 2 Subject 2             21           2.13
#> 3 Subject 3              9           1.64
#> 4 Subject 4             13           1.02
#> 5 Subject 5             38           3.72

# Level 2 Hyperglycemia Event (>=15 consecutive min of >250 mg/dL and event
# ends when there is >=15 consecutive min with a CGM sensor value of <=250 mg/dL)
hyper_lv2 <- detect_hyperglycemic_events(example_data_5_subject, type = "lv2")
print(hyper_lv2$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1              2           0.18
#> 2 Subject 2             19           1.93
#> 3 Subject 3              4           0.73
#> 4 Subject 4              0           0   
#> 5 Subject 5             18           1.76

# Extended Hyperglycemia Event (>250 mg/dL lasting >=90 cumulative min within a
# 120-min period, ends when glucose returns to <=180 mg/dL for >=15 consecutive
# min after)
hyper_extended <- detect_hyperglycemic_events(example_data_5_subject, type = "extended")
print(hyper_extended$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1              0           0   
#> 2 Subject 2             10           1.02
#> 3 Subject 3              2           0.36
#> 4 Subject 4              0           0   
#> 5 Subject 5             10           0.98

# Custom criteria method for the same standard definitions
hyper_lv1_custom <- detect_hyperglycemic_events(
  example_data_5_subject,
  start_gl = 180,
  dur_length = 15,
  end_length = 15,
  end_gl = 180
)
hyper_lv2_custom <- detect_hyperglycemic_events(
  example_data_5_subject,
  start_gl = 250,
  dur_length = 15,
  end_length = 15,
  end_gl = 250
)
hyper_extended_custom <- detect_hyperglycemic_events(
  example_data_5_subject,
  start_gl = 250,
  dur_length = 120,
  end_length = 15,
  end_gl = 180
)

# Compare event rates across levels
cat("Level 1 episodes:", sum(hyper_lv1$events_total$total_episodes), "\n")
#> Level 1 episodes: 97 
cat("Level 2 episodes:", sum(hyper_lv2$events_total$total_episodes), "\n")
#> Level 2 episodes: 43 
cat("Extended episodes:", sum(hyper_extended$events_total$total_episodes), "\n")
#> Extended episodes: 22 

# Analysis on larger dataset with Level 1 criteria
large_hyper <- detect_hyperglycemic_events(example_data_hall, type = "lv1")
print(large_hyper$events_total)
#> # A tibble: 19 × 3
#>    id           total_episodes avg_ep_per_day
#>    <chr>                 <int>          <dbl>
#>  1 1636-69-001               4           0.62
#>  2 1636-69-026               1           0.16
#>  3 1636-69-032               1           0.16
#>  4 1636-69-090               3           0.46
#>  5 1636-69-091               0           0   
#>  6 1636-69-114               0           0   
#>  7 1636-70-1005              3           0.46
#>  8 1636-70-1010              1           0.16
#>  9 2133-004                  5           0.81
#> 10 2133-015                  3           0.46
#> 11 2133-017                  1           0.16
#> 12 2133-018                 12           1.94
#> 13 2133-019                  0           0   
#> 14 2133-021                  9           1.44
#> 15 2133-024                  0           0   
#> 16 2133-027                  0           0   
#> 17 2133-035                  1           0.15
#> 18 2133-036                  2           0.28
#> 19 2133-039                  2           0.27

# Analysis on larger dataset with Level 2 criteria
large_hyper_lv2 <- detect_hyperglycemic_events(example_data_hall, type = "lv2")
print(large_hyper_lv2$events_total)
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
#>  8 1636-70-1010              0           0   
#>  9 2133-004                  0           0   
#> 10 2133-015                  0           0   
#> 11 2133-017                  0           0   
#> 12 2133-018                  2           0.32
#> 13 2133-019                  0           0   
#> 14 2133-021                  0           0   
#> 15 2133-024                  0           0   
#> 16 2133-027                  0           0   
#> 17 2133-035                  0           0   
#> 18 2133-036                  0           0   
#> 19 2133-039                  0           0   

# Analysis on larger dataset with Extended criteria
large_hyper_extended <- detect_hyperglycemic_events(example_data_hall, type = "extended")
print(large_hyper_extended$events_total)
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
#>  8 1636-70-1010              0           0   
#>  9 2133-004                  0           0   
#> 10 2133-015                  0           0   
#> 11 2133-017                  0           0   
#> 12 2133-018                  1           0.16
#> 13 2133-019                  0           0   
#> 14 2133-021                  0           0   
#> 15 2133-024                  0           0   
#> 16 2133-027                  0           0   
#> 17 2133-035                  0           0   
#> 18 2133-036                  0           0   
#> 19 2133-039                  0           0   

# View detailed events for specific subject
if(nrow(hyper_lv1$events_detailed) > 0) {
  first_subject <- hyper_lv1$events_detailed$id[1]
  subject_events <- hyper_lv1$events_detailed[hyper_lv1$events_detailed$id == first_subject, ]
  head(subject_events)
}
#> # A tibble: 6 × 7
#>   id        start_time          start_glucose end_time            end_glucose
#>   <chr>     <dttm>                      <dbl> <dttm>                    <dbl>
#> 1 Subject 1 2015-06-11 15:45:00          193. 2015-06-11 16:50:00        187.
#> 2 Subject 1 2015-06-11 17:25:00          195. 2015-06-11 19:00:00        183.
#> 3 Subject 1 2015-06-11 19:20:00          181. 2015-06-11 19:45:00        187.
#> 4 Subject 1 2015-06-11 22:35:00          187. 2015-06-11 23:45:00        185.
#> 5 Subject 1 2015-06-12 07:50:00          181. 2015-06-12 09:15:00        181.
#> 6 Subject 1 2015-06-13 16:55:00          180. 2015-06-13 18:25:00        186.
#> # ℹ 2 more variables: start_index <int>, end_index <int>
```
