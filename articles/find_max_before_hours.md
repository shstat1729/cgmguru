# find_max_before_hours function

## find_max_before_hours

Runs the documented examples for
[`find_max_before_hours()`](https://shstat1729.github.io/cgmguru/reference/find_max_before_hours.md).

``` r

example(find_max_before_hours, package = "cgmguru", run.dontrun = FALSE)
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
#> fnd___> # Find maximum glucose in previous 2 hours
#> fnd___> max_before <- find_max_before_hours(example_data_5_subject, start_points, hours = 2)
#> 
#> fnd___> print(paste("Found", length(max_before$max_index), "maximum points"))
#> [1] "Found 1 maximum points"
#> 
#> fnd___> # Find maximum glucose in previous 1 hour
#> fnd___> max_before_1h <- find_max_before_hours(example_data_5_subject, start_points, hours = 1)
#> 
#> fnd___> # Analysis on larger dataset
#> fnd___> large_start_index <- seq(1, nrow(example_data_hall), by = 200)
#> 
#> fnd___> large_start_points <- data.frame(start_index = large_start_index)
#> 
#> fnd___> large_max_before <- find_max_before_hours(example_data_hall, large_start_points, hours = 2)
#> 
#> fnd___> print(paste("Found", length(large_max_before$max_index), "maximum points in larger dataset"))
#> [1] "Found 1 maximum points in larger dataset"
```
