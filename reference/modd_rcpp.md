# Rcpp MODD Calculation

Calculates Mean of Daily Differences (MODD) with an Rcpp backend. The
preprocessing follows the iglu day-grid approach used by
[`iglu::modd`](https://irinagain.github.io/iglu/reference/modd.html):
CGM data are linearly interpolated to a regular midnight-aligned
full-day grid, respecting `inter_gap`, before same-time-of-day
differences are calculated.

## Usage

``` r
modd_rcpp(data, lag = 1, tz = "", inter_gap = 45)
```

## Arguments

- data:

  A dataframe containing CGM data with columns:

  - `id`: Subject identifier

  - `time`: POSIXct measurement timestamp

  - `gl`: Glucose value in mg/dL

- lag:

  Whole number of days separating paired same-time-of-day glucose
  observations. Defaults to 1.

- tz:

  Time zone used for day-grid alignment when supplied.

- inter_gap:

  Maximum gap, in minutes, over which linear interpolation is allowed.
  Defaults to 45, matching iglu's internal default.

## Value

A tibble with columns `id` and `MODD`.

## References

Service, F. John, and Roger L. Nelson. "Characteristics of glycemic
stability." Diabetes Care 3.1 (1980): 58-62.

## See also

[`iglu::modd`](https://irinagain.github.io/iglu/reference/modd.html),
[conga_rcpp](https://shstat1729.github.io/cgmguru/reference/conga_rcpp.md),
[mage_rcpp](https://shstat1729.github.io/cgmguru/reference/mage_rcpp.md)

## Examples

``` r
library(iglu)
data(example_data_5_subject)
modd_rcpp(example_data_5_subject)
#> # A tibble: 5 × 2
#>   id         MODD
#>   <chr>     <dbl>
#> 1 Subject 1  27.8
#> 2 Subject 2  44.1
#> 3 Subject 3  48.2
#> 4 Subject 4  24.9
#> 5 Subject 5  59.4
modd_rcpp(example_data_5_subject, lag = 2)
#> # A tibble: 5 × 2
#>   id         MODD
#>   <chr>     <dbl>
#> 1 Subject 1  27.7
#> 2 Subject 2  46.0
#> 3 Subject 3  47.1
#> 4 Subject 4  24.9
#> 5 Subject 5  56.0
```
