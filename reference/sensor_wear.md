# Calculate Sensor Wear

Calculates the percent of expected CGM readings observed. By default,
the calculation uses each subject's original timestamp span from first
valid reading to last valid reading. If `ndays` is supplied, valid
readings in `[end_date - ndays, end_date]` are divided by the expected
number of readings over that fixed retrospective window.

For fixed-window calculations, if `end_date = NULL`, each subject's last
valid timestamp defines that subject's retrospective window. If
`end_date` is supplied, the same endpoint is used for all subjects,
which is useful for a common study cutoff or report date. Duplicate
timestamps within a subject are de-duplicated after sorting, and rows
with missing `time` or `gl` do not count as observed readings.

## Usage

``` r
sensor_wear(df, end_date = NULL, ndays = NULL,
 reading_minutes = NULL)
```

## Arguments

- df:

  A dataframe containing CGM data with columns:

  - `id`: Subject identifier

  - `time`: POSIXct measurement timestamp

  - `gl`: Glucose value in mg/dL

- end_date:

  End timestamp for a fixed-window calculation. Requires `ndays`. If
  `NULL`, each subject's last valid timestamp is used. `Date` values are
  converted with
  [`as.POSIXct()`](https://rdrr.io/r/base/as.POSIXlt.html), matching
  iglu's manual active-percent behavior.

- ndays:

  Number of days in the fixed retrospective window. Defaults to `NULL`,
  which uses the original timestamp span.

- reading_minutes:

  Reading interval in minutes. If `NULL`, it is inferred per id from the
  median positive difference between valid readings.

## Value

A tibble with columns `id`, `sensor_wear_percent`, `sensor_wear`,
`ndays`, `start_date`, and `end_date`. `sensor_wear` is retained as a
backward-compatible alias.

## See also

[detect_all_events](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md),
[`iglu::active_percent`](https://irinagain.github.io/iglu/reference/active_percent.html)

## Examples

``` r
library(iglu)
data(example_data_5_subject)
sensor_wear(example_data_5_subject, reading_minutes = 5)
#> # A tibble: 5 × 6
#>   id        sensor_wear_percent sensor_wear ndays start_date         
#>   <chr>                   <dbl>       <dbl> <dbl> <dttm>             
#> 1 Subject 1                79.8        79.8    NA 2015-06-06 16:50:27
#> 2 Subject 2                58.9        58.9    NA 2015-02-24 17:31:29
#> 3 Subject 3                92.1        92.1    NA 2015-03-10 15:36:26
#> 4 Subject 4                98.7        98.7    NA 2015-03-13 12:44:09
#> 5 Subject 5                95.8        95.8    NA 2015-02-28 17:40:06
#> # ℹ 1 more variable: end_date <dttm>
sensor_wear(example_data_5_subject, ndays = 90, reading_minutes = 5)
#> # A tibble: 5 × 6
#>   id        sensor_wear_percent sensor_wear ndays start_date         
#>   <chr>                   <dbl>       <dbl> <dbl> <dttm>             
#> 1 Subject 1               11.2        11.2     90 2015-03-21 08:59:36
#> 2 Subject 2               10.9        10.9     90 2014-12-13 09:38:01
#> 3 Subject 3                5.91        5.91    90 2014-12-16 10:11:05
#> 4 Subject 4               14.1        14.1     90 2014-12-26 10:01:58
#> 5 Subject 5               11.3        11.3     90 2014-12-11 08:04:28
#> # ℹ 1 more variable: end_date <dttm>
```
