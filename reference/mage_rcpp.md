# Rcpp MAGE Calculation

Calculates Mean Amplitude of Glycemic Excursions (MAGE) with an Rcpp
backend.

## Usage

``` r
mage_rcpp(
  data,
  version = c("ma", "naive"),
  sd_multiplier = 1,
  short_ma = 5,
  long_ma = 32,
  return_type = c("num", "df"),
  direction = c("avg", "service", "max", "plus", "minus"),
  tz = "",
  inter_gap = 45,
  max_gap = 180
)
```

## Arguments

- data:

  A dataframe containing CGM data with columns `id`, `time`, and `gl`.

- version:

  Either `"ma"` or `"naive"`. Defaults to `"ma"`.

- sd_multiplier:

  Multiplier for the standard deviation threshold in
  `version = "naive"`.

- short_ma:

  Short moving-average window length. Defaults to 5.

- long_ma:

  Long moving-average window length. Defaults to 32.

- return_type:

  Either `"num"` for one metric per id or `"df"` for a list-column of
  segment-level MAGE rows.

- direction:

  One of `"avg"`, `"service"`, `"max"`, `"plus"`, or `"minus"`.

- tz:

  Time zone used for day-grid alignment when supplied.

- inter_gap:

  Maximum gap, in minutes, over which linear interpolation is allowed.
  Defaults to 45.

- max_gap:

  Gap length, in minutes, above which MAGE is calculated on separate
  trace segments. Defaults to 180.

## Value

A tibble with columns `id` and `MAGE`. With `return_type = "df"`, `MAGE`
is a list-column of tibbles with `start`, `end`, `mage`,
`plus_or_minus`, and `first_excursion`.

## Details

The default `version = "ma"` follows iglu's moving-average approach: CGM
is interpolated to 5-minute intervals, short and long moving-average
crossings identify candidate peak/nadir intervals, and countable
excursions are those whose peak-to-nadir or nadir-to-peak amplitude is
at least one glucose standard deviation.

The `version = "naive"` option matches iglu's older standard-deviation
based calculation. This function is calculation-only and does not
implement iglu's plotting options.

## References

Service, F. J., et al. (1970). Mean amplitude of glycemic excursions, a
measure of diabetic instability. *Diabetes*, 19, 644-655.
[doi:10.2337/diab.19.9.644](https://doi.org/10.2337/diab.19.9.644)

Fernandes, N. J., et al. (2022). Open-source algorithm to calculate mean
amplitude of glycemic excursions using short and long moving averages.
*Journal of Diabetes Science and Technology*, 16, 576-577.
[doi:10.1177/19322968211061165](https://doi.org/10.1177/19322968211061165)

## See also

[`iglu::mage`](https://irinagain.github.io/iglu/reference/mage.html),
[`conga_rcpp`](https://shstat1729.github.io/cgmguru/reference/conga_rcpp.md)

## Examples

``` r
library(iglu)
data(example_data_5_subject)
mage_rcpp(example_data_5_subject)
#> # A tibble: 5 × 2
#>   id         MAGE
#>   <chr>     <dbl>
#> 1 Subject 1  72.4
#> 2 Subject 2 118. 
#> 3 Subject 3 116. 
#> 4 Subject 4  70.9
#> 5 Subject 5 142. 
mage_rcpp(example_data_5_subject, version = "naive")
#> # A tibble: 5 × 2
#>   id         MAGE
#>   <chr>     <dbl>
#> 1 Subject 1  53.4
#> 2 Subject 2  78.2
#> 3 Subject 3  76.6
#> 4 Subject 4  42.9
#> 5 Subject 5  90.0
```
