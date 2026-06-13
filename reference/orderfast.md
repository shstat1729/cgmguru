# Fast Ordering Function

Orders a dataframe by `id` and `time` columns using a C++ `std::sort`
backend. Optimized for large CGM datasets, it returns the input with
rows sorted by subject then timestamp while preserving all columns.

Orders a dataframe by id and time columns using the C++ backend

## Usage

``` r
orderfast(df)
```

## Arguments

- df:

  A dataframe with 'id' and 'time' columns

## Value

A dataframe ordered by id and time

A dataframe ordered by id and time

## Examples

``` r
# Load sample data
library(iglu)
data(example_data_5_subject)
data(example_data_hall)

# Shuffle without replacement, then order and compare to baseline
set.seed(123)
shuffled <- example_data_5_subject[sample(seq_len(nrow(example_data_5_subject)),
                                          replace = FALSE), ]
baseline <- orderfast(example_data_5_subject)
ordered_shuffled <- orderfast(shuffled)

# Compare results
print(paste("Identical after ordering:", identical(baseline, ordered_shuffled)))
#> [1] "Identical after ordering: TRUE"
head(baseline[, c("id", "time", "gl")])
#>          id                time  gl
#> 1 Subject 1 2015-06-06 16:50:27 153
#> 2 Subject 1 2015-06-06 17:05:27 137
#> 3 Subject 1 2015-06-06 17:10:27 128
#> 4 Subject 1 2015-06-06 17:15:28 121
#> 5 Subject 1 2015-06-06 17:25:27 120
#> 6 Subject 1 2015-06-06 17:45:27 138
head(ordered_shuffled[, c("id", "time", "gl")])
#>          id                time  gl
#> 1 Subject 1 2015-06-06 16:50:27 153
#> 2 Subject 1 2015-06-06 17:05:27 137
#> 3 Subject 1 2015-06-06 17:10:27 128
#> 4 Subject 1 2015-06-06 17:15:28 121
#> 5 Subject 1 2015-06-06 17:25:27 120
#> 6 Subject 1 2015-06-06 17:45:27 138

# Order larger dataset
ordered_large <- orderfast(example_data_hall)
print(paste("Ordered", nrow(ordered_large), "rows in larger dataset"))
#> [1] "Ordered 34890 rows in larger dataset"
df <- data.frame(id = c("b", "a", "a"), time = as.POSIXct(
  c("2024-01-01 01:00:00", "2024-01-01 00:00:00", "2024-01-01 01:00:00"), tz = "UTC"
))
orderfast(df)
#>   id                time
#> 2  a 2024-01-01 00:00:00
#> 3  a 2024-01-01 01:00:00
#> 1  b 2024-01-01 01:00:00
```
