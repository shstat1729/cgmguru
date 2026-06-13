# GRID Algorithm for Glycemic Event Detection

Implements the GRID (Glucose Rate Increase Detector) algorithm for
detecting rapid glucose rate increases in continuous glucose monitoring
(CGM) data. This algorithm identifies rapid glucose changes using
specific rate-based criteria, and is commonly applied for meal
detection. Meals are detected when the CGM value is \\\geq\\ 7.2 mmol/L
(\\\geq\\ 130 mg/dL) and the rate-of-change is \\\geq\\ 5.3 mmol/L/h
\[\\\geq\\ 95 mg/dL/h\] for the last two consecutive readings, or
\\\geq\\ 5.0 mmol/L/h \[\\\geq\\ 90 mg/dL/h\] for two of the last three
readings.

## Usage

``` r
grid(df, gap = 15, threshold = 130)
```

## Arguments

- df:

  A dataframe containing continuous glucose monitoring (CGM) data. Must
  include columns:

  - `id`: Subject identifier (string or factor)

  - `time`: Time of measurement (POSIXct)

  - `gl`: Glucose value (integer or numeric, mg/dL)

- gap:

  Gap threshold in minutes for event detection (default: 15). This
  parameter defines the minimum time interval between consecutive GRID
  events. For example, if gap is set to 60, only one GRID event can be
  detected within any one-hour window; subsequent events within the gap
  interval are not counted as new events.

- threshold:

  GRID slope threshold in mg/dL/hour for event classification (default:
  130)

## Value

A list containing:

- `grid_vector`: A tibble with the results of the GRID analysis.
  Contains a `grid` column (0/1 values; 1 denotes a detected GRID event)
  and all relevant input columns.

- `episode_counts`: A tibble summarizing the number of GRID events per
  subject (`id`) as `episode_counts`.

- `episode_start`: A tibble listing the start of each GRID episode, with
  columns:

  - `id`: Subject ID.

  - `time`: The timestamp (POSIXct) at which the GRID event was
    detected.

  - `gl`: The glucose value (mg/dL; integer or numeric) at the GRID
    event.

  - `index`: R-based (1-indexed) row number(s) in the original dataframe
    where the GRID event occurs. The occurrence time equals
    `df$time[index]` and glucose equals `df$gl[index]`.

## Algorithm

\- Flags points where `gl >= 130 mg/dL` and rate-of-change meets the
GRID criteria (see references). - Enforces a minimum `gap` in minutes
between detected events to avoid duplicates.

## Units and sampling

\- `gl` is mg/dL; `time` is POSIXct; `gap` is minutes. - The effective
sampling interval is derived from `time` deltas. - This function
operates on the rows supplied in `df`. It does not use
[`interpolate_cgm`](https://shstat1729.github.io/cgmguru/reference/interpolate_cgm.md)
or the full-day event preprocessing grid.

## References

Harvey, R. A., et al. (2014). Design of the glucose rate increase
detector: a meal detection module for the health monitoring system.
Journal of Diabetes Science and Technology, 8(2), 307-320.

Adolfsson, Peter, et al. "Increased time in range and fewer missed bolus
injections after introduction of a smart connected insulin pen."
Diabetes technology & therapeutics 22.10 (2020): 709-718.

## See also

[mod_grid](https://shstat1729.github.io/cgmguru/reference/mod_grid.md),
[maxima_grid](https://shstat1729.github.io/cgmguru/reference/maxima_grid.md),
[find_local_maxima](https://shstat1729.github.io/cgmguru/reference/find_local_maxima.md),
[detect_between_maxima](https://shstat1729.github.io/cgmguru/reference/detect_between_maxima.md)

Other GRID pipeline:
[`detect_between_maxima()`](https://shstat1729.github.io/cgmguru/reference/detect_between_maxima.md),
[`find_local_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_local_maxima.md),
[`find_max_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_after_hours.md),
[`find_max_before_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_before_hours.md),
[`find_min_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_min_after_hours.md),
[`find_min_before_hours()`](https://shstat1729.github.io/cgmguru/reference/find_min_before_hours.md),
[`find_new_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_new_maxima.md),
[`maxima_grid()`](https://shstat1729.github.io/cgmguru/reference/maxima_grid.md),
[`mod_grid()`](https://shstat1729.github.io/cgmguru/reference/mod_grid.md),
[`start_finder()`](https://shstat1729.github.io/cgmguru/reference/start_finder.md),
[`transform_df()`](https://shstat1729.github.io/cgmguru/reference/transform_df.md)

## Examples

``` r
# Load sample data
library(iglu)
data(example_data_5_subject)
data(example_data_hall)

# Basic GRID analysis on smaller dataset
grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
print(grid_result$episode_counts)
#> # A tibble: 5 × 2
#>   id        episode_counts
#>   <chr>              <int>
#> 1 Subject 1             10
#> 2 Subject 2             22
#> 3 Subject 3              7
#> 4 Subject 4             18
#> 5 Subject 5             42
print(grid_result$episode_start)
#> # A tibble: 99 × 4
#>    id        time                   gl index
#>    <chr>     <dttm>              <dbl> <int>
#>  1 Subject 1 2015-06-11 15:30:07   143   966
#>  2 Subject 1 2015-06-11 17:10:07   157   985
#>  3 Subject 1 2015-06-11 22:00:06   135  1038
#>  4 Subject 1 2015-06-11 22:25:06   162  1043
#>  5 Subject 1 2015-06-12 07:40:04   160  1154
#>  6 Subject 1 2015-06-13 16:34:59   132  1415
#>  7 Subject 1 2015-06-14 17:39:55   176  1676
#>  8 Subject 1 2015-06-16 19:14:47   166  2222
#>  9 Subject 1 2015-06-18 14:29:40   187  2720
#> 10 Subject 1 2015-06-18 18:19:39   132  2765
#> # ℹ 89 more rows
print(grid_result$grid_vector)
#> # A tibble: 13,866 × 1
#>     grid
#>    <int>
#>  1     0
#>  2     0
#>  3     0
#>  4     0
#>  5     0
#>  6     0
#>  7     0
#>  8     0
#>  9     0
#> 10     0
#> # ℹ 13,856 more rows

# More sensitive GRID analysis
sensitive_result <- grid(example_data_5_subject, gap = 10, threshold = 120)

# GRID analysis on larger dataset
large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
print(paste("Detected", sum(large_grid$episode_counts$episode_counts), "episodes"))
#> [1] "Detected 79 episodes"
print(large_grid$episode_start)
#> # A tibble: 79 × 4
#>    id          time                   gl index
#>    <chr>       <dttm>              <dbl> <int>
#>  1 1636-69-001 2014-02-04 07:47:05   138   336
#>  2 1636-69-001 2014-02-04 17:42:03   138   455
#>  3 1636-69-001 2014-02-05 08:41:59   137   635
#>  4 1636-69-001 2015-03-29 14:33:30   137   786
#>  5 1636-69-001 2015-03-30 10:53:25   132   979
#>  6 1636-69-001 2015-03-31 09:18:19   143  1202
#>  7 1636-69-001 2015-03-31 13:58:18   136  1258
#>  8 1636-69-001 2015-04-01 16:58:12   132  1581
#>  9 1636-69-026 2015-11-24 14:37:18   142  2011
#> 10 1636-69-026 2015-11-24 23:32:16   139  2118
#> # ℹ 69 more rows
print(large_grid$grid_vector)
#> # A tibble: 34,890 × 1
#>     grid
#>    <int>
#>  1     0
#>  2     0
#>  3     0
#>  4     0
#>  5     0
#>  6     0
#>  7     0
#>  8     0
#>  9     0
#> 10     0
#> # ℹ 34,880 more rows
```
