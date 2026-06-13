# find_min_after_hours function

## find_min_after_hours

Runs the documented examples for
[`find_min_after_hours()`](https://shstat1729.github.io/cgmguru/reference/find_min_after_hours.md).

``` r

example(find_min_after_hours, package = "cgmguru", run.dontrun = FALSE)
#> 
#> fnd___> # Load sample data
#> fnd___> library(iglu)
#> 
#> fnd___> data(example_data_5_subject)
#> 
#> fnd___> data(example_data_hall)
#> 
#> fnd___> # Create start points for demonstration (using row index)
#> fnd___> start_index <- seq(1, nrow(example_data_5_subject), by = 100)
#> 
#> fnd___> start_points <- data.frame(start_index = start_index)
#> 
#> fnd___> # Find minimum glucose in next 2 hours
#> fnd___> min_after <- find_min_after_hours(example_data_5_subject, start_points, hours = 2)
#> 
#> fnd___> print(paste("Found", length(min_after$min_index), "minimum points"))
#> [1] "Found 1 minimum points"
#> 
#> fnd___> # Find minimum glucose in next 1 hour
#> fnd___> min_after_1h <- find_min_after_hours(example_data_5_subject, start_points, hours = 1)
#> 
#> fnd___> # Analysis on larger dataset
#> fnd___> large_start_index <- seq(1, nrow(example_data_hall), by = 200)
#> 
#> fnd___> large_start_points <- data.frame(start_index = large_start_index)
#> 
#> fnd___> large_min_after <- find_min_after_hours(example_data_hall, large_start_points, hours = 2)
#> 
#> fnd___> print(paste("Found", length(large_min_after$min_index), "minimum points in larger dataset"))
#> [1] "Found 1 minimum points in larger dataset"
```
