# Detect Events Between Maxima

Identifies and analyzes events occurring between detected maxima points,
providing detailed episode information for GRID analysis. This function
helps characterize the glucose dynamics between identified peaks.

## Usage

``` r
detect_between_maxima(df, transform_df)
```

## Arguments

- df:

  A dataframe containing continuous glucose monitoring (CGM) data. Must
  include columns:

  - `id`: Subject identifier (string or factor)

  - `time`: Time of measurement (POSIXct)

  - `gl`: Glucose value (integer or numeric, mg/dL)

- transform_df:

  A dataframe containing summary information from previous
  transformations

## Value

A list containing:

- `results`: Tibble with events between maxima (`id`, `grid_time`,
  `grid_gl`, `maxima_time`, `maxima_glucose`, `time_to_peak`)

- `episode_counts`: Tibble with episode counts per subject (`id`,
  `episode_counts`)

## See also

[grid](https://shstat1729.github.io/cgmguru/reference/grid.md),
[mod_grid](https://shstat1729.github.io/cgmguru/reference/mod_grid.md),
[find_new_maxima](https://shstat1729.github.io/cgmguru/reference/find_new_maxima.md),
[transform_df](https://shstat1729.github.io/cgmguru/reference/transform_df.md)

Other GRID pipeline:
[`find_local_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_local_maxima.md),
[`find_max_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_after_hours.md),
[`find_max_before_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_before_hours.md),
[`find_min_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_min_after_hours.md),
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

# Complete pipeline to get transform_df
grid_result <- grid(example_data_5_subject, gap = 60, threshold = 130)
maxima_result <- find_local_maxima(example_data_5_subject)
mod_result <- mod_grid(example_data_5_subject, grid_result$grid_vector, hours = 2, gap = 60)
max_after <- find_max_after_hours(example_data_5_subject, mod_result$mod_grid_vector, hours = 2)
new_maxima <- find_new_maxima(example_data_5_subject,
                              max_after$max_index,
                              maxima_result$local_maxima_vector)
transformed <- transform_df(grid_result$episode_start, new_maxima)

# Detect events between maxima
between_events <- detect_between_maxima(example_data_5_subject, transformed)
print(paste("Events between maxima:", length(between_events)))
#> [1] "Events between maxima: 2"

# Analysis on larger dataset
large_grid <- grid(example_data_hall, gap = 60, threshold = 130)
large_maxima <- find_local_maxima(example_data_hall)
large_mod <- mod_grid(example_data_hall, large_grid$grid_vector, hours = 2, gap = 60)
large_max_after <- find_max_after_hours(example_data_hall, large_mod$mod_grid_vector, hours = 2)
large_new_maxima <- find_new_maxima(example_data_hall,
                                    large_max_after$max_index,
                                    large_maxima$local_maxima_vector)
large_transformed <- transform_df(large_grid$episode_start, large_new_maxima)
large_between <- detect_between_maxima(example_data_hall, large_transformed)
print(paste("Events between maxima in larger dataset:", length(large_between)))
#> [1] "Events between maxima in larger dataset: 2"
```
