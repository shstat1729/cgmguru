# Find Start Points for Event Analysis

Finds R-based (1-indexed) positions where the value is 1 in an integer
vector of 0s and 1s, specifically identifying episode start points. This
function looks for positions where a 1 follows a 0 or is at the
beginning of the vector, which is useful for identifying the start of
glycemic events or episodes.

## Usage

``` r
start_finder(df)
```

## Arguments

- df:

  A dataframe with the first column containing an integer vector of 0s
  and 1s

## Value

A tibble containing start_index with R-based (1-indexed) positions where
episodes start Note: These index refer to positions in the provided
input vector/dataframe, not necessarily rows of the original CGM `df`
unless that vector was derived directly from `df` in row order.

## Notes

\- Returns R-based `start_index` positions relative to the provided
input vector/dataframe. - If used on vectors derived from a CGM `df`,
index map directly to `df` rows.

## See also

[grid](https://shstat1729.github.io/cgmguru/reference/grid.md),
[mod_grid](https://shstat1729.github.io/cgmguru/reference/mod_grid.md)

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
[`mod_grid()`](https://shstat1729.github.io/cgmguru/reference/mod_grid.md),
[`transform_df()`](https://shstat1729.github.io/cgmguru/reference/transform_df.md)

## Examples

``` r
# Load sample data
library(iglu)
data(example_data_5_subject)
data(example_data_hall)

# Create a binary vector indicating episode starts
binary_vector <- c(0, 0, 1, 1, 0, 1, 0, 0, 1, 1)
df <- data.frame(episode_starts = binary_vector)

# Find R-based index where episodes start
start_points <- start_finder(df)
print(paste("Start index:", paste(start_points$start_index, collapse = ", ")))
#> [1] "Start index: 3, 6, 9"

# Use with actual GRID results
grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
grid_starts <- start_finder(grid_result$grid_vector)
print(paste("GRID episode starts:", length(grid_starts$start_index)))
#> [1] "GRID episode starts: 99"

# Analysis on larger dataset
large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
large_starts <- start_finder(large_grid$grid_vector)
print(paste("GRID episode starts in larger dataset:", length(large_starts$start_index)))
#> [1] "GRID episode starts in larger dataset: 79"
```
