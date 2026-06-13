# orderfast function

## orderfast

### Overview

Fast ordering of a dataframe by `id` and `time`, optimized for large CGM
datasets. Returns the input sorted while preserving columns.

### Inputs

- **df**: Dataframe containing `id` and `time`

### Returns

- Dataframe ordered by `id`, then `time`

### Run documented examples

``` r

example(orderfast, package = "cgmguru", run.dontrun = FALSE)
#> 
#> ordrfs> # Load sample data
#> ordrfs> library(iglu)
#> 
#> ordrfs> data(example_data_5_subject)
#> 
#> ordrfs> data(example_data_hall)
#> 
#> ordrfs> # Shuffle without replacement, then order and compare to baseline
#> ordrfs> set.seed(123)
#> 
#> ordrfs> shuffled <- example_data_5_subject[sample(seq_len(nrow(example_data_5_subject)),
#> ordrfs+                                           replace = FALSE), ]
#> 
#> ordrfs> baseline <- orderfast(example_data_5_subject)
#> 
#> ordrfs> ordered_shuffled <- orderfast(shuffled)
#> 
#> ordrfs> # Compare results
#> ordrfs> print(paste("Identical after ordering:", identical(baseline, ordered_shuffled)))
#> [1] "Identical after ordering: TRUE"
#> 
#> ordrfs> head(baseline[, c("id", "time", "gl")])
#>          id                time  gl
#> 1 Subject 1 2015-06-06 16:50:27 153
#> 2 Subject 1 2015-06-06 17:05:27 137
#> 3 Subject 1 2015-06-06 17:10:27 128
#> 4 Subject 1 2015-06-06 17:15:28 121
#> 5 Subject 1 2015-06-06 17:25:27 120
#> 6 Subject 1 2015-06-06 17:45:27 138
#> 
#> ordrfs> head(ordered_shuffled[, c("id", "time", "gl")])
#>          id                time  gl
#> 1 Subject 1 2015-06-06 16:50:27 153
#> 2 Subject 1 2015-06-06 17:05:27 137
#> 3 Subject 1 2015-06-06 17:10:27 128
#> 4 Subject 1 2015-06-06 17:15:28 121
#> 5 Subject 1 2015-06-06 17:25:27 120
#> 6 Subject 1 2015-06-06 17:45:27 138
#> 
#> ordrfs> # Order larger dataset
#> ordrfs> ordered_large <- orderfast(example_data_hall)
#> 
#> ordrfs> print(paste("Ordered", nrow(ordered_large), "rows in larger dataset"))
#> [1] "Ordered 34890 rows in larger dataset"
#> 
#> ordrfs> df <- data.frame(id = c("b", "a", "a"), time = as.POSIXct(
#> ordrfs+   c("2024-01-01 01:00:00", "2024-01-01 00:00:00", "2024-01-01 01:00:00"), tz = "UTC"
#> ordrfs+ ))
#> 
#> ordrfs> orderfast(df)
#>   id                time
#> 2  a 2024-01-01 00:00:00
#> 3  a 2024-01-01 01:00:00
#> 1  b 2024-01-01 01:00:00
```
