# Find New Maxima Around Grid Points

Identifies new maxima in the vicinity of previously identified grid
points, useful for refining maxima detection in GRID analysis. This
function helps improve the accuracy of peak detection by searching
around known event points.

## Usage

``` r
find_new_maxima(df, mod_grid_max_point_df, local_maxima_df)
```

## Arguments

- df:

  A dataframe containing continuous glucose monitoring (CGM) data. Must
  include columns:

  - `id`: Subject identifier (string or factor)

  - `time`: Time of measurement (POSIXct)

  - `gl`: Glucose value (integer or numeric, mg/dL)

- mod_grid_max_point_df:

  A dataframe with column `index` (candidate maxima index)

- local_maxima_df:

  A dataframe with column `local_maxima` (index of local peaks)

## Value

A tibble with updated maxima information containing columns (`id`,
`time`, `gl`, `index`) The `index` column contains R-based (1-indexed)
row number(s) in `df`; thus, `time == df$time[index]` and
`gl == df$gl[index]`.

## See also

[find_local_maxima](https://shstat1729.github.io/cgmguru/reference/find_local_maxima.md),
[find_max_after_hours](https://shstat1729.github.io/cgmguru/reference/find_max_after_hours.md),
[transform_df](https://shstat1729.github.io/cgmguru/reference/transform_df.md)

Other GRID pipeline:
[`detect_between_maxima()`](https://shstat1729.github.io/cgmguru/reference/detect_between_maxima.md),
[`find_local_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_local_maxima.md),
[`find_max_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_after_hours.md),
[`find_max_before_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_before_hours.md),
[`find_min_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_min_after_hours.md),
[`find_min_before_hours()`](https://shstat1729.github.io/cgmguru/reference/find_min_before_hours.md),
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

# First, get grid points and local maxima
grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
maxima_result <- find_local_maxima(example_data_5_subject)

# Create modified grid points (simplified for example)
mod_grid_indices <- data.frame(index = grid_result$episode_start$index[1:10])

# Find new maxima around grid points
new_maxima <- find_new_maxima(example_data_5_subject,
                              mod_grid_indices,
                              maxima_result$local_maxima_vector)
print(paste("Found", nrow(new_maxima), "new maxima"))
#> [1] "Found 9 new maxima"

# Analysis on larger dataset
large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
large_maxima <- find_local_maxima(example_data_hall)
large_mod_grid <- data.frame(index = large_grid$episode_start$index[1:20])
large_new_maxima <- find_new_maxima(example_data_hall,
                                    large_mod_grid,
                                    large_maxima$local_maxima_vector)
print(paste("Found", nrow(large_new_maxima), "new maxima in larger dataset"))
#> [1] "Found 20 new maxima in larger dataset"
```
