# detect_between_maxima function

## detect_between_maxima

### Overview

Identifies and analyzes events occurring between detected maxima points
to characterize glucose dynamics between peaks. Often used downstream of
[`grid()`](https://shstat1729.github.io/cgmguru/reference/grid.md),
[`mod_grid()`](https://shstat1729.github.io/cgmguru/reference/mod_grid.md),
[`find_max_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_after_hours.md),
and
[`find_new_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_new_maxima.md).

### Inputs

- **df**: CGM dataframe with `id`, `time` (POSIXct), `gl` (mg/dL)
- **transform_df**: Output from
  [`transform_df()`](https://shstat1729.github.io/cgmguru/reference/transform_df.md)
  mapping GRID starts to maxima

### Returns

- **results**: Tibble with `id`, `grid_time`, `grid_gl`, `maxima_time`,
  `maxima_glucose`, `time_to_peak`
- **episode_counts**: Counts per `id`

### Run documented examples

``` r

example(detect_between_maxima, package = "cgmguru", run.dontrun = FALSE)
#> 
#> dtct__> # Load sample data
#> dtct__> library(iglu)
#> 
#> dtct__> data(example_data_5_subject)
#> 
#> dtct__> data(example_data_hall)
#> 
#> dtct__> # Complete pipeline to get transform_df
#> dtct__> grid_result <- grid(example_data_5_subject, gap = 60, threshold = 130)
#> 
#> dtct__> maxima_result <- find_local_maxima(example_data_5_subject)
#> 
#> dtct__> mod_result <- mod_grid(example_data_5_subject, grid_result$grid_vector, hours = 2, gap = 60)
#> 
#> dtct__> max_after <- find_max_after_hours(example_data_5_subject, mod_result$mod_grid_vector, hours = 2)
#> 
#> dtct__> new_maxima <- find_new_maxima(example_data_5_subject,
#> dtct__+                               max_after$max_index,
#> dtct__+                               maxima_result$local_maxima_vector)
#> 
#> dtct__> transformed <- transform_df(grid_result$episode_start, new_maxima)
#> 
#> dtct__> # Detect events between maxima
#> dtct__> between_events <- detect_between_maxima(example_data_5_subject, transformed)
#> 
#> dtct__> print(paste("Events between maxima:", length(between_events)))
#> [1] "Events between maxima: 2"
#> 
#> dtct__> # Analysis on larger dataset
#> dtct__> large_grid <- grid(example_data_hall, gap = 60, threshold = 130)
#> 
#> dtct__> large_maxima <- find_local_maxima(example_data_hall)
#> 
#> dtct__> large_mod <- mod_grid(example_data_hall, large_grid$grid_vector, hours = 2, gap = 60)
#> 
#> dtct__> large_max_after <- find_max_after_hours(example_data_hall, large_mod$mod_grid_vector, hours = 2)
#> 
#> dtct__> large_new_maxima <- find_new_maxima(example_data_hall,
#> dtct__+                                     large_max_after$max_index,
#> dtct__+                                     large_maxima$local_maxima_vector)
#> 
#> dtct__> large_transformed <- transform_df(large_grid$episode_start, large_new_maxima)
#> 
#> dtct__> large_between <- detect_between_maxima(example_data_hall, large_transformed)
#> 
#> dtct__> print(paste("Events between maxima in larger dataset:", length(large_between)))
#> [1] "Events between maxima in larger dataset: 2"
```
