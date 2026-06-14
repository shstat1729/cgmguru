# Detect Rebound Hypoglycemia and Hyperglycemia

Detects rebound hypoglycemia (Rhypo) and rebound hyperglycemia (Rhyper)
as derived events built from cgmguru Level 1 event starts and a later
opposite-threshold crossing within a configurable time window.

Rhypo is a Level 1 hyperglycemic event (\\\>\\180 mg/dL for at least 15
minutes) followed by any glucose value \\\<\\70 mg/dL within
`rebound_minutes`. Rhyper is a Level 1 hypoglycemic event (\\\<\\70
mg/dL for at least 15 minutes) followed by any glucose value \\\>\\180
mg/dL within `rebound_minutes`. The later rebound side only needs a
single qualifying threshold crossing.

## Usage

``` r
rebound_events(df, type = c("all", "hypo", "hyper"),
 data_source = c("raw", "preprocessed"), reading_minutes = NULL,
 sort_time = FALSE, inter_gap = 45, rebound_minutes = 120,
 return_interpolated = TRUE)
```

## Arguments

- df:

  A dataframe containing continuous glucose monitoring (CGM) data with
  columns `id`, `time`, and `gl`.

- type:

  Rebound direction to return. `"hypo"` returns Rhypo, `"hyper"` returns
  Rhyper, and `"all"` returns both.

- data_source:

  Source interpretation for `df`. `"raw"` applies cgmguru event
  preprocessing first. `"preprocessed"` treats `df` as an
  already-preprocessed event grid and does not interpolate again.

- reading_minutes:

  Time interval between readings in minutes. Can be a scalar, a vector
  matching `nrow(df)`, or `NULL`. If omitted, it is inferred per id from
  positive timestamp differences.

- sort_time:

  Logical. If `TRUE`, sort rows within each id by `time`. Defaults to
  `FALSE`.

- inter_gap:

  Maximum gap in minutes to interpolate across when
  `data_source = "raw"`. Defaults to 45.

- rebound_minutes:

  Maximum bridge interval in minutes from the initial Level 1 event end
  to the later rebound threshold crossing. Defaults to 120.

- return_interpolated:

  Logical. If `TRUE`, include the event grid used for rebound detection
  as `interpolated_data`. Defaults to `TRUE`.

## Value

A list containing:

- `events_total`: Tibble with `id`, `type`, `total_episodes`, and
  `avg_ep_per_day`.

- `events_detailed`: Tibble with bridge boundaries (`start_time`,
  `end_time`, indices, and glucose values), the initial Level 1 event
  boundaries, rebound threshold crossing fields, and
  `minutes_to_rebound`.

- `interpolated_data`: Included when `return_interpolated = TRUE`, with
  columns `id`, `time`, and `gl`.

## See also

[detect_all_events](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md),
[detect_hyperglycemic_events](https://shstat1729.github.io/cgmguru/reference/detect_hyperglycemic_events.md),
[detect_hypoglycemic_events](https://shstat1729.github.io/cgmguru/reference/detect_hypoglycemic_events.md),
[interpolate_cgm](https://shstat1729.github.io/cgmguru/reference/interpolate_cgm.md)

## Examples

``` r
df <- data.frame(
  id = "A",
  time = as.POSIXct("2026-01-01 00:00:00", tz = "UTC") + 0:8 * 5 * 60,
  gl = c(190, 195, 200, 170, 165, 160, 65, 100, 110)
)
rebound_events(df, type = "hypo", reading_minutes = 5)
#> $events_total
#> # A tibble: 1 × 4
#>   id    type  total_episodes avg_ep_per_day
#>   <chr> <chr>          <int>          <dbl>
#> 1 A     hypo               0              0
#> 
#> $events_detailed
#> # A tibble: 0 × 18
#> # ℹ 18 variables: id <chr>, type <chr>, start_time <dttm>, start_glucose <dbl>,
#> #   end_time <dttm>, end_glucose <dbl>, start_index <int>, end_index <int>,
#> #   initial_start_time <dttm>, initial_start_glucose <dbl>,
#> #   initial_end_time <dttm>, initial_end_glucose <dbl>,
#> #   initial_start_index <int>, initial_end_index <int>, rebound_time <dttm>,
#> #   rebound_glucose <dbl>, rebound_index <int>, minutes_to_rebound <dbl>
#> 
#> $interpolated_data
#> # A tibble: 8 × 3
#>   id    time                   gl
#>   <chr> <dttm>              <dbl>
#> 1 A     2026-01-01 00:05:00   195
#> 2 A     2026-01-01 00:10:00   200
#> 3 A     2026-01-01 00:15:00   170
#> 4 A     2026-01-01 00:20:00   165
#> 5 A     2026-01-01 00:25:00   160
#> 6 A     2026-01-01 00:30:00    65
#> 7 A     2026-01-01 00:35:00   100
#> 8 A     2026-01-01 00:40:00   110
#> 
```
