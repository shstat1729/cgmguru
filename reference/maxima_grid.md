# Combined Maxima Detection and GRID Analysis

Fast method for postprandial glucose peak detection combining GRID
algorithm with local maxima analysis. Detects meal-induced glucose peaks
by identifying GRID events (rapid glucose increases) and mapping them to
corresponding local maxima within a search window. Local maxima are
defined as points where glucose values increase or remain constant for
two consecutive points before the peak, and decrease or remain constant
for two consecutive points after the peak.

The 7-step algorithm: (1) finds GRID points indicating meal starts (2)
identifies modified GRID points after minimum duration (3) locates
maximum glucose within the subsequent time window (4) detects all local
maxima using the two-consecutive-point criteria (5) refines peaks from
local maxima candidates (6) maps GRID points to peaks within 4-hour
constraint (7) redistributes overlapping peaks.

## Usage

``` r
maxima_grid(df, threshold = 130, gap = 60, hours = 2)
```

## Arguments

- df:

  A dataframe containing continuous glucose monitoring (CGM) data. Must
  include columns:

  - `id`: Subject identifier (string or factor)

  - `time`: Time of measurement (POSIXct)

  - `gl`: Glucose value (integer or numeric, mg/dL)

- threshold:

  GRID slope threshold in mg/dL/hour for event classification (default:
  130)

- gap:

  Gap threshold in minutes for event detection (default: 60). This
  parameter defines the minimum time interval between consecutive GRID
  events.

- hours:

  Time window in hours for maxima analysis (default: 2)

## Value

A list containing:

- `results`: Tibble with combined maxima and GRID analysis results, with
  columns:

  - `id`: Subject identifier

  - `grid_time`: Timestamp of GRID event detection (POSIXct)

  - `grid_gl`: Glucose value at GRID event (mg/dL)

  - `maxima_time`: Timestamp of peak glucose (POSIXct)

  - `maxima_glucose`: Peak glucose value (mg/dL)

  - `time_to_peak_min`: Time from GRID event to peak in minutes

  - `grid_index`: R-based (1-indexed) row number of GRID event;
    `grid_time == df$time[grid_index]`, `grid_gl == df$gl[grid_index]`

  - `maxima_index`: R-based (1-indexed) row number of peak;
    `maxima_time == df$time[maxima_index]`,
    `maxima_glucose == df$gl[maxima_index]`

- `episode_counts`: Tibble with episode counts per subject (`id`,
  `episode_counts`)

## Algorithm (7 steps)

1\) GRID -\> 2) modified GRID -\> 3) window maxima -\> 4) local maxima
-\> 5) refine peaks -\> 6) map GRID to peaks (\\\leq\\ 4h) -\> 7)
redistribute overlapping peaks.

## Input grid

This function operates on the rows supplied in `df`. It does not use
[`interpolate_cgm`](https://shstat1729.github.io/cgmguru/reference/interpolate_cgm.md)
or the full-day event preprocessing grid unless the caller explicitly
supplies interpolated data.

## References

Park, Sang Ho, et al. "Identification of clinically meaningful
automatically detected postprandial glucose excursions in individuals
with type 1 diabetes using personal continuous glucose monitoring."
Diabetes Research and Clinical Practice (2025): 112951.

Park, Soojin, et al. "High-Amplitude and Prolonged Glucose Excursions as
a Key Determinant of Discordance Between Glucose Management Indicator
and Glycated Hemoglobin in Type 1 Diabetes." Diabetes Care (2026):
dc252820. https://doi.org/10.2337/dc25-2820

## See also

[grid](https://shstat1729.github.io/cgmguru/reference/grid.md),
[mod_grid](https://shstat1729.github.io/cgmguru/reference/mod_grid.md),
[find_local_maxima](https://shstat1729.github.io/cgmguru/reference/find_local_maxima.md),
[find_new_maxima](https://shstat1729.github.io/cgmguru/reference/find_new_maxima.md),
[transform_df](https://shstat1729.github.io/cgmguru/reference/transform_df.md)

Other GRID pipeline:
[`detect_between_maxima()`](https://shstat1729.github.io/cgmguru/reference/detect_between_maxima.md),
[`find_local_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_local_maxima.md),
[`find_max_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_after_hours.md),
[`find_max_before_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_before_hours.md),
[`find_min_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_min_after_hours.md),
[`find_min_before_hours()`](https://shstat1729.github.io/cgmguru/reference/find_min_before_hours.md),
[`find_new_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_new_maxima.md),
[`grid()`](https://shstat1729.github.io/cgmguru/reference/grid.md),
[`mod_grid()`](https://shstat1729.github.io/cgmguru/reference/mod_grid.md),
[`start_finder()`](https://shstat1729.github.io/cgmguru/reference/start_finder.md),
[`transform_df()`](https://shstat1729.github.io/cgmguru/reference/transform_df.md)

## Examples

``` r
# Load sample data
library(iglu)
data(example_data_5_subject)
data(example_data_hall)

# Combined analysis on smaller dataset
maxima_result <- maxima_grid(example_data_5_subject, threshold = 130, gap = 60, hours = 2)
print(maxima_result$episode_counts)
#> # A tibble: 5 × 2
#>   id        episode_counts
#>   <chr>              <int>
#> 1 Subject 1              8
#> 2 Subject 2             18
#> 3 Subject 3              7
#> 4 Subject 4             16
#> 5 Subject 5             39
print(maxima_result$results)
#> # A tibble: 88 × 8
#>    id        grid_time       grid_gl maxima_time maxima_glucose time_to_peak_min
#>    <chr>     <dttm>            <dbl> <dttm>               <dbl>            <dbl>
#>  1 Subject 1 2015-06-11 15:…     143 2015-06-11…            276               40
#>  2 Subject 1 2015-06-11 22:…     135 2015-06-11…            209               50
#>  3 Subject 1 2015-06-12 07:…     160 2015-06-12…            210               40
#>  4 Subject 1 2015-06-13 16:…     132 2015-06-13…            202               60
#>  5 Subject 1 2015-06-14 17:…     176 2015-06-14…            227               45
#>  6 Subject 1 2015-06-16 19:…     166 2015-06-16…            208               65
#>  7 Subject 1 2015-06-18 14:…     187 2015-06-18…            212               20
#>  8 Subject 1 2015-06-18 18:…     132 2015-06-18…            183               35
#>  9 Subject 2 2015-02-24 20:…     140 2015-02-24…            222               85
#> 10 Subject 2 2015-02-25 19:…     173 2015-02-25…            273              125
#> # ℹ 78 more rows
#> # ℹ 2 more variables: grid_index <int>, maxima_index <int>

# More sensitive analysis
sensitive_maxima <- maxima_grid(example_data_5_subject, threshold = 120, gap = 30, hours = 1)
print(sensitive_maxima$episode_counts)
#> # A tibble: 5 × 2
#>   id        episode_counts
#>   <chr>              <int>
#> 1 Subject 1             10
#> 2 Subject 2             19
#> 3 Subject 3             10
#> 4 Subject 4             20
#> 5 Subject 5             40
print(sensitive_maxima$results)
#> # A tibble: 99 × 8
#>    id        grid_time       grid_gl maxima_time maxima_glucose time_to_peak_min
#>    <chr>     <dttm>            <dbl> <dttm>               <dbl>            <dbl>
#>  1 Subject 1 2015-06-11 15:…     143 2015-06-11…            276               40
#>  2 Subject 1 2015-06-11 17:…     157 2015-06-11…            267               55
#>  3 Subject 1 2015-06-11 21:…     125 2015-06-11…            209               60
#>  4 Subject 1 2015-06-12 07:…     160 2015-06-12…            210               40
#>  5 Subject 1 2015-06-13 16:…     124 2015-06-13…            202               65
#>  6 Subject 1 2015-06-14 17:…     176 2015-06-14…            228               95
#>  7 Subject 1 2015-06-16 19:…     166 2015-06-16…            208               65
#>  8 Subject 1 2015-06-18 13:…     126 2015-06-18…            183               55
#>  9 Subject 1 2015-06-18 14:…     187 2015-06-18…            212               20
#> 10 Subject 1 2015-06-18 18:…     132 2015-06-18…            183               35
#> # ℹ 89 more rows
#> # ℹ 2 more variables: grid_index <int>, maxima_index <int>

# Analysis on larger dataset
large_maxima <- maxima_grid(example_data_hall, threshold = 130, gap = 60, hours = 2)
print(large_maxima$episode_counts)
#> # A tibble: 19 × 2
#>    id           episode_counts
#>    <chr>                 <int>
#>  1 1636-69-001               8
#>  2 1636-69-026               7
#>  3 1636-69-032               2
#>  4 1636-69-090               3
#>  5 1636-69-091               1
#>  6 1636-69-114               0
#>  7 1636-70-1005              8
#>  8 1636-70-1010              2
#>  9 2133-004                  5
#> 10 2133-015                  4
#> 11 2133-017                  2
#> 12 2133-018                 12
#> 13 2133-019                  2
#> 14 2133-021                 10
#> 15 2133-024                  1
#> 16 2133-027                  1
#> 17 2133-035                  1
#> 18 2133-036                  2
#> 19 2133-039                  5
print(large_maxima$results)
#> # A tibble: 76 × 8
#>    id          grid_time     grid_gl maxima_time maxima_glucose time_to_peak_min
#>    <chr>       <dttm>          <dbl> <dttm>               <dbl>            <dbl>
#>  1 1636-69-001 2014-02-04 0…     138 2014-02-04…            194               30
#>  2 1636-69-001 2014-02-04 1…     138 2014-02-04…            225               60
#>  3 1636-69-001 2014-02-05 0…     137 2014-02-05…            196               45
#>  4 1636-69-001 2015-03-29 1…     137 2015-03-29…            250               65
#>  5 1636-69-001 2015-03-30 1…     132 2015-03-30…            181               45
#>  6 1636-69-001 2015-03-31 0…     143 2015-03-31…            177               20
#>  7 1636-69-001 2015-03-31 1…     136 2015-03-31…            169               30
#>  8 1636-69-001 2015-04-01 1…     132 2015-04-01…            165               50
#>  9 1636-69-026 2015-11-24 1…     142 2015-11-24…            182               40
#> 10 1636-69-026 2015-11-24 2…     139 2015-11-25…            171               35
#> # ℹ 66 more rows
#> # ℹ 2 more variables: grid_index <int>, maxima_index <int>
```
