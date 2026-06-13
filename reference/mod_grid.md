# Modified GRID Analysis

Constructs a modified GRID series by reapplying the GRID logic with a
designated gap (e.g., 60 minutes) and analysis window in hours (e.g., 2
hours). It reassigns GRID events under these constraints to produce a
modified grid suitable for downstream maxima mapping and episode
analysis.

## Usage

``` r
mod_grid(df, grid_point_df, hours = 2, gap = 15)
```

## Arguments

- df:

  A dataframe containing continuous glucose monitoring (CGM) data. Must
  include columns:

  - `id`: Subject identifier (string or factor)

  - `time`: Time of measurement (POSIXct)

  - `gl`: Glucose value (integer or numeric, mg/dL)

- grid_point_df:

  A dataframe with column `start_index` (start points for re-applied
  GRID)

- hours:

  Time window in hours for analysis (default: 2)

- gap:

  Gap threshold in minutes for event detection (default: 15). This
  parameter defines the minimum time interval between consecutive GRID
  events.

## Value

A list containing:

- `mod_grid_vector`: Tibble with modified GRID results (`mod_grid`)

- `episode_counts`: Tibble with episode counts per subject (`id`,
  `episode_counts`)

- `episode_start`: Tibble with all episode starts with columns:

  - `id`: Subject identifier

  - `time`: Timestamp at which the event occurs; equivalent to
    `df$time[index]`

  - `gl`: Glucose value at the event; equivalent to `df$gl[index]`

  - `index`: R-based (1-indexed) row number(s) in `df` denoting where
    the event occurs

## Units and sampling

\- `gap` is minutes; `hours` is hours; `time` is POSIXct.

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
[find_max_after_hours](https://shstat1729.github.io/cgmguru/reference/find_max_after_hours.md),
[find_new_maxima](https://shstat1729.github.io/cgmguru/reference/find_new_maxima.md)

Other GRID pipeline:
[`detect_between_maxima()`](https://shstat1729.github.io/cgmguru/reference/detect_between_maxima.md),
[`find_local_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_local_maxima.md),
[`find_max_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_after_hours.md),
[`find_max_before_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_before_hours.md),
[`find_min_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_min_after_hours.md),
[`find_min_before_hours()`](https://shstat1729.github.io/cgmguru/reference/find_min_before_hours.md),
[`find_new_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_new_maxima.md),
[`grid()`](https://shstat1729.github.io/cgmguru/reference/grid.md),
[`maxima_grid()`](https://shstat1729.github.io/cgmguru/reference/maxima_grid.md),
[`start_finder()`](https://shstat1729.github.io/cgmguru/reference/start_finder.md),
[`transform_df()`](https://shstat1729.github.io/cgmguru/reference/transform_df.md)

## Examples

``` r
# Load sample data
library(iglu)
data(example_data_5_subject)
data(example_data_hall)

# First, get grid points
grid_result <- grid(example_data_5_subject, gap = 60, threshold = 130)

# Perform modified GRID analysis
mod_result <- mod_grid(example_data_5_subject, grid_result$grid_vector, hours = 2, gap = 60)
print(paste("Modified grid points:", nrow(mod_result$mod_grid_vector)))
#> [1] "Modified grid points: 13866"

# Modified analysis with different parameters
mod_result_1h <- mod_grid(example_data_5_subject, grid_result$grid_vector, hours = 1, gap = 40)

# Analysis on larger dataset
large_grid <- grid(example_data_hall, gap = 60, threshold = 130)
large_mod_result <- mod_grid(example_data_hall, large_grid$grid_vector, hours = 2, gap = 60)
print(paste("Modified grid points in larger dataset:", nrow(large_mod_result$mod_grid_vector)))
#> [1] "Modified grid points in larger dataset: 34890"
```
