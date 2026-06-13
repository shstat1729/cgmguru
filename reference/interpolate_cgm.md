# Interpolate CGM Data

Interpolates continuous glucose monitoring (CGM) data onto the same
iglu-compatible, midnight-aligned full-day grid used internally by
cgmguru's event-detection functions. The interpolation is implemented in
C++ and is intended for users who want to inspect or reuse the
preprocessed grid behind
[`detect_all_events`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md),
[`detect_hyperglycemic_events`](https://shstat1729.github.io/cgmguru/reference/detect_hyperglycemic_events.md),
and
[`detect_hypoglycemic_events`](https://shstat1729.github.io/cgmguru/reference/detect_hypoglycemic_events.md).

For each subject, `interpolate_cgm()` builds an equally spaced grid at
`reading_minutes` intervals. If `reading_minutes` is omitted, it is
inferred per subject from the median positive timestamp difference.
Glucose values are linearly interpolated only across gaps up to
`inter_gap`; larger gaps are treated as missing and removed from the
returned data, preserving segment boundaries used by event calculation.

The GRID-family functions
[`grid`](https://shstat1729.github.io/cgmguru/reference/grid.md),
[`maxima_grid`](https://shstat1729.github.io/cgmguru/reference/maxima_grid.md),
and
[`excursion`](https://shstat1729.github.io/cgmguru/reference/excursion.md)
do not call this helper automatically; they operate on the rows supplied
by the user unless the caller explicitly passes an interpolated dataset.

## Usage

``` r
interpolate_cgm(df, reading_minutes = NULL, sort_time = FALSE,
 inter_gap = 45)
```

## Arguments

- df:

  A dataframe containing CGM data with columns:

  - `id`: Subject identifier

  - `time`: POSIXct measurement timestamp

  - `gl`: Glucose value in mg/dL

- reading_minutes:

  Time interval for the interpolation grid in minutes. If omitted or
  `NULL`, it is calculated automatically per id as the median positive
  time difference in the data.

- sort_time:

  Logical. If `TRUE`, sort rows within each id by `time` in C++ before
  interpolation. Defaults to `FALSE`.

- inter_gap:

  Maximum gap in minutes to interpolate across. Defaults to 45; larger
  gaps split the event-detection grid.

## Value

A tibble with columns `id`, interpolated `time`, and interpolated `gl`.
Rows inside gaps larger than `inter_gap` are omitted.

## See also

[detect_all_events](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md),
[detect_hyperglycemic_events](https://shstat1729.github.io/cgmguru/reference/detect_hyperglycemic_events.md),
[detect_hypoglycemic_events](https://shstat1729.github.io/cgmguru/reference/detect_hypoglycemic_events.md)

## Examples

``` r
df <- data.frame(
  id = "A",
  time = as.POSIXct(c("2026-01-01 00:15:00", "2026-01-01 00:25:00"),
                    tz = "UTC"),
  gl = c(100, 120)
)
interpolate_cgm(df)
#> # A tibble: 1 × 3
#>   id    time                   gl
#>   <chr> <dttm>              <dbl>
#> 1 A     2026-01-01 00:20:00   110
```
