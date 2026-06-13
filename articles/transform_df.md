# transform_df function

## transform_df

### Overview

Prepares data for downstream analysis by mapping GRID episode starts to
maxima within a 4-hour window and merging the information.

### Inputs

- **grid_df**: From
  [`grid()`](https://shstat1729.github.io/cgmguru/reference/grid.md)
  (typically `episode_start`)
- **maxima_df**: From
  [`find_new_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_new_maxima.md)
  or local maxima results

### Returns

- Tibble with columns: `id`, `grid_time`, `grid_gl`, `maxima_time`,
  `maxima_gl`

### Run documented examples

``` r

example(transform_df, package = "cgmguru", run.dontrun = FALSE)
#> 
#> trnsf_> # Load sample data
#> trnsf_> library(iglu)
#> 
#> trnsf_> data(example_data_5_subject)
#> 
#> trnsf_> data(example_data_hall)
#> 
#> trnsf_> # Complete pipeline example with smaller dataset
#> trnsf_> threshold <- 130
#> 
#> trnsf_> gap <- 60
#> 
#> trnsf_> hours <- 2
#> 
#> trnsf_> # 1) Find GRID points
#> trnsf_> grid_result <- grid(example_data_5_subject, gap = gap, threshold = threshold)
#> 
#> trnsf_> # 2) Find modified GRID points before 2 hours minimum
#> trnsf_> mod_grid <- mod_grid(example_data_5_subject,
#> trnsf_+                      start_finder(grid_result$grid_vector),
#> trnsf_+                      hours = hours,
#> trnsf_+                      gap = gap)
#> 
#> trnsf_> # 3) Find maximum point 2 hours after mod_grid point
#> trnsf_> mod_grid_maxima <- find_max_after_hours(example_data_5_subject,
#> trnsf_+                                         start_finder(mod_grid$mod_grid_vector),
#> trnsf_+                                         hours = hours)
#> 
#> trnsf_> # 4) Identify local maxima around episodes/windows
#> trnsf_> local_maxima <- find_local_maxima(example_data_5_subject)
#> 
#> trnsf_> # 5) Among local maxima, find maximum point after two hours
#> trnsf_> final_maxima <- find_new_maxima(example_data_5_subject,
#> trnsf_+                                 mod_grid_maxima$max_index,
#> trnsf_+                                 local_maxima$local_maxima_vector)
#> 
#> trnsf_> # 6) Map GRID points to maximum points (within 4 hours)
#> trnsf_> transform_maxima <- transform_df(grid_result$episode_start, final_maxima)
#> 
#> trnsf_> # 7) Redistribute overlapping maxima between GRID points
#> trnsf_> final_between_maxima <- detect_between_maxima(example_data_5_subject, transform_maxima)
#> 
#> trnsf_> # Complete pipeline example with larger dataset (example_data_hall)
#> trnsf_> # This demonstrates the same workflow on a more comprehensive dataset
#> trnsf_> hall_threshold <- 130
#> 
#> trnsf_> hall_gap <- 60
#> 
#> trnsf_> hall_hours <- 2
#> 
#> trnsf_> # 1) Find GRID points on larger dataset
#> trnsf_> hall_grid_result <- grid(example_data_hall, gap = hall_gap, threshold = hall_threshold)
#> 
#> trnsf_> # 2) Find modified GRID points
#> trnsf_> hall_mod_grid <- mod_grid(example_data_hall,
#> trnsf_+                          start_finder(hall_grid_result$grid_vector),
#> trnsf_+                          hours = hall_hours,
#> trnsf_+                          gap = hall_gap)
#> 
#> trnsf_> # 3) Find maximum points after mod_grid
#> trnsf_> hall_mod_grid_maxima <- find_max_after_hours(example_data_hall,
#> trnsf_+                                             start_finder(hall_mod_grid$mod_grid_vector),
#> trnsf_+                                             hours = hall_hours)
#> 
#> trnsf_> # 4) Identify local maxima
#> trnsf_> hall_local_maxima <- find_local_maxima(example_data_hall)
#> 
#> trnsf_> # 5) Find new maxima
#> trnsf_> hall_final_maxima <- find_new_maxima(example_data_hall,
#> trnsf_+                                     hall_mod_grid_maxima$max_index,
#> trnsf_+                                     hall_local_maxima$local_maxima_vector)
#> 
#> trnsf_> # 6) Transform data
#> trnsf_> hall_transform_maxima <- transform_df(hall_grid_result$episode_start, hall_final_maxima)
#> 
#> trnsf_> # 7) Detect between maxima
#> trnsf_> hall_final_between_maxima <- detect_between_maxima(example_data_hall, hall_transform_maxima)
```
