# find_local_maxima function

## find_local_maxima

Runs the documented examples for
[`find_local_maxima()`](https://shstat1729.github.io/cgmguru/reference/find_local_maxima.md).

``` r

example(find_local_maxima, package = "cgmguru", run.dontrun = FALSE)
#> 
#> fnd_l_> # Load sample data
#> fnd_l_> library(iglu)
#> 
#> fnd_l_> data(example_data_5_subject)
#> 
#> fnd_l_> data(example_data_hall)
#> 
#> fnd_l_> # Find local maxima
#> fnd_l_> maxima_result <- find_local_maxima(example_data_5_subject)
#> 
#> fnd_l_> print(paste("Found", nrow(maxima_result$local_maxima_vector), "local maxima"))
#> [1] "Found 1602 local maxima"
#> 
#> fnd_l_> # Find maxima on larger dataset
#> fnd_l_> large_maxima <- find_local_maxima(example_data_hall)
#> 
#> fnd_l_> print(paste("Found", nrow(large_maxima$local_maxima_vector), "local maxima in larger dataset"))
#> [1] "Found 4991 local maxima in larger dataset"
#> 
#> fnd_l_> # View first few maxima
#> fnd_l_> head(maxima_result$local_maxima_vector)
#> # A tibble: 6 × 1
#>   local_maxima
#>          <int>
#> 1            8
#> 2           23
#> 3           24
#> 4           65
#> 5           70
#> 6           77
#> 
#> fnd_l_> # View merged results
#> fnd_l_> head(maxima_result$merged_results)
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
