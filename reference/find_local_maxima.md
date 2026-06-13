# Find Local Maxima in Glucose Time Series

Identifies local maxima (peaks) in glucose concentration time series
data. Uses a difference-based algorithm to detect peaks where glucose
values increase or remain constant for two consecutive points before the
peak point, and decrease or remain constant for two consecutive points
after the peak point.

## Usage

``` r
find_local_maxima(df)
```

## Arguments

- df:

  A dataframe containing continuous glucose monitoring (CGM) data. Must
  include columns:

  - `id`: Subject identifier (string or factor)

  - `time`: Time of measurement (POSIXct)

  - `gl`: Glucose value (integer or numeric, mg/dL)

## Value

A list containing:

- `local_maxima_vector`: Tibble with R-based (1-indexed) row numbers of
  local maxima (`local_maxima`). The corresponding occurrence time is
  `df$time[local_maxima]` and glucose is `df$gl[local_maxima]`.

- `merged_results`: Tibble with local maxima details (`id`, `time`,
  `gl`)

## See also

[grid](https://shstat1729.github.io/cgmguru/reference/grid.md),
[mod_grid](https://shstat1729.github.io/cgmguru/reference/mod_grid.md),
[find_new_maxima](https://shstat1729.github.io/cgmguru/reference/find_new_maxima.md)

Other GRID pipeline:
[`detect_between_maxima()`](https://shstat1729.github.io/cgmguru/reference/detect_between_maxima.md),
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

# Find local maxima
maxima_result <- find_local_maxima(example_data_5_subject)
print(paste("Found", nrow(maxima_result$local_maxima_vector), "local maxima"))
#> [1] "Found 1602 local maxima"

# Find maxima on larger dataset
large_maxima <- find_local_maxima(example_data_hall)
print(paste("Found", nrow(large_maxima$local_maxima_vector), "local maxima in larger dataset"))
#> [1] "Found 4991 local maxima in larger dataset"

# View first few maxima
head(maxima_result$local_maxima_vector)
#> # A tibble: 6 × 1
#>   local_maxima
#>          <int>
#> 1            8
#> 2           23
#> 3           24
#> 4           65
#> 5           70
#> 6           77

# View merged results
head(maxima_result$merged_results)
#> # A tibble: 6 × 3
#>   id        time                   gl
#>   <chr>     <dttm>              <dbl>
#> 1 Subject 1 2015-06-06 18:05:27   159
#> 2 Subject 1 2015-06-06 20:15:27   174
#> 3 Subject 1 2015-06-06 20:20:26   174
#> 4 Subject 1 2015-06-07 01:20:26    88
#> 5 Subject 1 2015-06-07 01:45:25    92
#> 6 Subject 1 2015-06-07 02:20:25    92
```
