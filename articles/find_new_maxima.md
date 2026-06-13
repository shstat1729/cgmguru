# find_new_maxima function

## find_new_maxima

Runs the documented examples for
[`find_new_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_new_maxima.md).

``` r

example(find_new_maxima, package = "cgmguru", run.dontrun = FALSE)
#> 
#> fnd_n_> # Load sample data
#> fnd_n_> library(iglu)
#> 
#> fnd_n_> data(example_data_5_subject)
#> 
#> fnd_n_> data(example_data_hall)
#> 
#> fnd_n_> # First, get grid points and local maxima
#> fnd_n_> grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
#> 
#> fnd_n_> maxima_result <- find_local_maxima(example_data_5_subject)
#> 
#> fnd_n_> # Create modified grid points (simplified for example)
#> fnd_n_> mod_grid_indices <- data.frame(index = grid_result$episode_start$index[1:10])
#> 
#> fnd_n_> # Find new maxima around grid points
#> fnd_n_> new_maxima <- find_new_maxima(example_data_5_subject,
#> fnd_n_+                               mod_grid_indices,
#> fnd_n_+                               maxima_result$local_maxima_vector)
#> 
#> fnd_n_> print(paste("Found", nrow(new_maxima), "new maxima"))
#> [1] "Found 9 new maxima"
#> 
#> fnd_n_> # Analysis on larger dataset
#> fnd_n_> large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
#> 
#> fnd_n_> large_maxima <- find_local_maxima(example_data_hall)
#> 
#> fnd_n_> large_mod_grid <- data.frame(index = large_grid$episode_start$index[1:20])
#> 
#> fnd_n_> large_new_maxima <- find_new_maxima(example_data_hall,
#> fnd_n_+                                     large_mod_grid,
#> fnd_n_+                                     large_maxima$local_maxima_vector)
#> 
#> fnd_n_> print(paste("Found", nrow(large_new_maxima), "new maxima in larger dataset"))
#> [1] "Found 20 new maxima in larger dataset"
```
