# Find Minimum Glucose After Specified Hours

Identifies the minimum glucose value occurring within a specified time
window after a given start point. This function is useful for analyzing
glucose patterns following specific events or time points.

## Usage

``` r
find_min_after_hours(df, start_point_df, hours)
```

## Arguments

- df:

  A dataframe containing continuous glucose monitoring (CGM) data. Must
  include columns:

  - `id`: Subject identifier (string or factor)

  - `time`: Time of measurement (POSIXct)

  - `gl`: Glucose value (integer or numeric, mg/dL)

- start_point_df:

  A dataframe with column `start_index` (R-based index into `df`)

- hours:

  Number of hours to look ahead from the start point

## Value

A list containing:

- `min_index`: Tibble with R-based (1-indexed) row numbers of minimum
  glucose (`min_index`). The corresponding occurrence time is
  `df$time[min_index]` and glucose is `df$gl[min_index]`.

- `episode_counts`: Tibble with episode counts per subject (`id`,
  `episode_counts`)

- `episode_start`: Tibble with all episode starts with columns:

  - `id`: Subject identifier

  - `time`: Timestamp at which the minimum occurs; equivalent to
    `df$time[index]`

  - `gl`: Glucose value at the minimum; equivalent to `df$gl[index]`

  - `index`: R-based (1-indexed) row number(s) in `df` denoting where
    the minimum occurs

## See also

[mod_grid](https://shstat1729.github.io/cgmguru/reference/mod_grid.md),
[find_local_maxima](https://shstat1729.github.io/cgmguru/reference/find_local_maxima.md)

Other GRID pipeline:
[`detect_between_maxima()`](https://shstat1729.github.io/cgmguru/reference/detect_between_maxima.md),
[`find_local_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_local_maxima.md),
[`find_max_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_after_hours.md),
[`find_max_before_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_before_hours.md),
[`find_min_before_hours()`](https://shstat1729.github.io/cgmguru/reference/find_min_before_hours.md),
[`find_new_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_new_maxima.md),
[`grid()`](https://shstat1729.github.io/cgmguru/reference/grid.md),
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

# Create start points for demonstration (using row index)
start_index <- seq(1, nrow(example_data_5_subject), by = 100)
start_points <- data.frame(start_index = start_index)

# Find minimum glucose in next 2 hours
min_after <- find_min_after_hours(example_data_5_subject, start_points, hours = 2)
print(paste("Found", length(min_after$min_index), "minimum points"))
#> [1] "Found 1 minimum points"

# Find minimum glucose in next 1 hour
min_after_1h <- find_min_after_hours(example_data_5_subject, start_points, hours = 1)

# Analysis on larger dataset
large_start_index <- seq(1, nrow(example_data_hall), by = 200)
large_start_points <- data.frame(start_index = large_start_index)
large_min_after <- find_min_after_hours(example_data_hall, large_start_points, hours = 2)
print(paste("Found", length(large_min_after$min_index), "minimum points in larger dataset"))
#> [1] "Found 1 minimum points in larger dataset"
```
