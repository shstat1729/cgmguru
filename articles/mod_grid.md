# mod_grid function

## mod_grid

Runs the documented examples for
[`mod_grid()`](https://shstat1729.github.io/cgmguru/reference/mod_grid.md).

``` r

example(mod_grid, package = "cgmguru", run.dontrun = FALSE)
#> 
#> md_grd> # Load sample data
#> md_grd> library(iglu)
#> 
#> md_grd> data(example_data_5_subject)
#> 
#> md_grd> data(example_data_hall)
#> 
#> md_grd> # First, get grid points
#> md_grd> grid_result <- grid(example_data_5_subject, gap = 60, threshold = 130)
#> 
#> md_grd> # Perform modified GRID analysis
#> md_grd> mod_result <- mod_grid(example_data_5_subject, grid_result$grid_vector, hours = 2, gap = 60)
#> 
#> md_grd> print(paste("Modified grid points:", nrow(mod_result$mod_grid_vector)))
#> [1] "Modified grid points: 13866"
#> 
#> md_grd> # Modified analysis with different parameters
#> md_grd> mod_result_1h <- mod_grid(example_data_5_subject, grid_result$grid_vector, hours = 1, gap = 40)
#> 
#> md_grd> # Analysis on larger dataset
#> md_grd> large_grid <- grid(example_data_hall, gap = 60, threshold = 130)
#> 
#> md_grd> large_mod_result <- mod_grid(example_data_hall, large_grid$grid_vector, hours = 2, gap = 60)
#> 
#> md_grd> print(paste("Modified grid points in larger dataset:", nrow(large_mod_result$mod_grid_vector)))
#> [1] "Modified grid points in larger dataset: 34890"
```
