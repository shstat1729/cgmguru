# start_finder function

## start_finder

### Overview

Finds R-based (1-indexed) positions where 1s begin episodes in a 0/1
vector (1 after 0 or at start). Useful for identifying episode starts.

### Inputs

- **df**: Dataframe whose first column is an integer vector of 0/1

### Returns

- **start_index**: Tibble of episode start positions (R index)

### Run documented examples

``` r

example(start_finder, package = "cgmguru", run.dontrun = FALSE)
#> 
#> strt_f> # Load sample data
#> strt_f> library(iglu)
#> 
#> strt_f> data(example_data_5_subject)
#> 
#> strt_f> data(example_data_hall)
#> 
#> strt_f> # Create a binary vector indicating episode starts
#> strt_f> binary_vector <- c(0, 0, 1, 1, 0, 1, 0, 0, 1, 1)
#> 
#> strt_f> df <- data.frame(episode_starts = binary_vector)
#> 
#> strt_f> # Find R-based index where episodes start
#> strt_f> start_points <- start_finder(df)
#> 
#> strt_f> print(paste("Start index:", paste(start_points$start_index, collapse = ", ")))
#> [1] "Start index: 3, 6, 9"
#> 
#> strt_f> # Use with actual GRID results
#> strt_f> grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
#> 
#> strt_f> grid_starts <- start_finder(grid_result$grid_vector)
#> 
#> strt_f> print(paste("GRID episode starts:", length(grid_starts$start_index)))
#> [1] "GRID episode starts: 99"
#> 
#> strt_f> # Analysis on larger dataset
#> strt_f> large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
#> 
#> strt_f> large_starts <- start_finder(large_grid$grid_vector)
#> 
#> strt_f> print(paste("GRID episode starts in larger dataset:", length(large_starts$start_index)))
#> [1] "GRID episode starts in larger dataset: 79"
```
