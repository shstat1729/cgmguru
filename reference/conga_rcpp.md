# Rcpp CONGA Calculation

Calculates Continuous Overall Net Glycemic Action (CONGA) with an Rcpp
backend.

## Usage

``` r
conga_rcpp(data, n = 24, tz = "", inter_gap = 45)
```

## Arguments

- data:

  A dataframe containing CGM data with columns `id`, `time`, and `gl`.

- n:

  Whole number of hours separating paired glucose observations. Defaults
  to 24.

- tz:

  Time zone used for day-grid alignment when supplied.

- inter_gap:

  Maximum gap, in minutes, over which linear interpolation is allowed.
  Defaults to 45, matching iglu's internal default.

## Value

A tibble with columns `id` and `CONGA`.

## Details

The implementation follows the CONGA calculation approach used by
[`iglu::conga`](https://irinagain.github.io/iglu/reference/conga.html):
after interpolation to a regular day-aligned CGM grid, CONGA is the
sample standard deviation of glucose differences separated by `n` hours.

## References

McDonnell, C. M., et al. (2005). A novel approach to continuous glucose
analysis utilizing glycemic variation. *Diabetes Technology and
Therapeutics*, 7(2), 253-263.
[doi:10.1089/dia.2005.7.253](https://doi.org/10.1089/dia.2005.7.253)

## See also

[`iglu::conga`](https://irinagain.github.io/iglu/reference/conga.html),
[`mage_rcpp`](https://shstat1729.github.io/cgmguru/reference/mage_rcpp.md)

## Examples

``` r
library(iglu)
data(example_data_5_subject)
conga_rcpp(example_data_5_subject)
#> # A tibble: 5 × 2
#>   id        CONGA
#>   <chr>     <dbl>
#> 1 Subject 1  37.0
#> 2 Subject 2  60.6
#> 3 Subject 3  63.4
#> 4 Subject 4  33.5
#> 5 Subject 5  73.8
```
