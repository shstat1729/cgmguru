# detect_all_events function: cgmguru and iglu-compatible event preprocessing

## Relationship to iglu

[`cgmguru::detect_all_events()`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md)
is an independent C++/Rcpp implementation of event calculation and CGM
summary output. Its preprocessing is designed to be compatible with the
event grid semantics used by `iglu`: subject-specific reading intervals,
a midnight-aligned full-day grid, interpolation up to `inter_gap`,
removal of larger gap-masked rows, and segment-wise event
classification.

When `reading_minutes` is omitted or `NULL`, cgmguru calculates it
automatically per subject from the median positive timestamp spacing in
the input data.

The `iglu` package is used here as a formal reference, source of public
example datasets, and comparison target. cgmguru does not call `iglu` at
runtime for its core algorithms.

## Datasets

We use two CGM example datasets shipped with `iglu`:

- `example_data_5_subject`: 5 subjects
- `example_data_hall`: 19 subjects

``` r

data(example_data_5_subject, package = "iglu")
data(example_data_hall, package = "iglu")

# Base-R summaries (no external dependencies)
summary_5 <- data.frame(
  rows = nrow(example_data_5_subject),
  subjects = length(unique(example_data_5_subject$id)),
  time_min = min(example_data_5_subject$time),
  time_max = max(example_data_5_subject$time),
  gl_min = min(example_data_5_subject$gl, na.rm = TRUE),
  gl_max = max(example_data_5_subject$gl, na.rm = TRUE)
)

summary_5
#>    rows subjects            time_min            time_max gl_min gl_max
#> 1 13866        5 2015-02-24 17:31:29 2015-06-19 08:59:36     50    400
```

``` r

cat("Note: The 'iglu' package is not available; vignette examples are skipped.\n")
```

## iglu: episode_calculation

[`iglu::episode_calculation()`](https://irinagain.github.io/iglu/reference/episode_calculation.html)
identifies hypo/hyperglycemia episodes.

``` r

iglu_episodes_5 <- iglu::episode_calculation(
  data = example_data_5_subject
)
print(iglu_episodes_5)
#> # A tibble: 35 × 7
#>    id        type  level avg_ep_per_day avg_ep_duration avg_ep_gl total_episodes
#>    <fct>     <chr> <chr>          <dbl>           <dbl>     <dbl>          <dbl>
#>  1 Subject 1 hypo  lv1           0.0899            35        68.6              1
#>  2 Subject 1 hypo  lv2           0                  0        NA                0
#>  3 Subject 1 hypo  exte…         0                  0        NA                0
#>  4 Subject 1 hyper lv1           1.44              80.3     200.              16
#>  5 Subject 1 hyper lv2           0.180             30       264.               2
#>  6 Subject 1 hypo  lv1_…         0.0899            35        68.6              1
#>  7 Subject 1 hyper lv1_…         1.26              79.6     195.              14
#>  8 Subject 2 hypo  lv1           0                  0        NA                0
#>  9 Subject 2 hypo  lv2           0                  0        NA                0
#> 10 Subject 2 hypo  exte…         0                  0        NA                0
#> # ℹ 25 more rows
```

``` r

iglu_episodes_hall <- iglu::episode_calculation(
  data = example_data_hall
)
print(iglu_episodes_hall)
#> # A tibble: 133 × 7
#>    id        type  level avg_ep_per_day avg_ep_duration avg_ep_gl total_episodes
#>    <chr>     <chr> <chr>          <dbl>           <dbl>     <dbl>          <dbl>
#>  1 1636-69-… hypo  lv1            0.468            15        68.1              3
#>  2 1636-69-… hypo  lv2            0                 0        NA                0
#>  3 1636-69-… hypo  exte…          0                 0        NA                0
#>  4 1636-69-… hyper lv1            0.623            57.5     201.               4
#>  5 1636-69-… hyper lv2            0                 0        NA                0
#>  6 1636-69-… hypo  lv1_…          0.468            15        68.1              3
#>  7 1636-69-… hyper lv1_…          0.623            57.5     201.               4
#>  8 1636-69-… hypo  lv1            0                 0        NA                0
#>  9 1636-69-… hypo  lv2            0                 0        NA                0
#> 10 1636-69-… hypo  exte…          0                 0        NA                0
#> # ℹ 123 more rows
```

## cgmguru: detect_all_events

``` r

all_events_5 <- detect_all_events(example_data_5_subject)
print(all_events_5)
#> $subject_summary
#> # A tibble: 5 × 24
#>   id          TIR  TITR TBR70 TBR54 TAR180 TAR250    CV    SD mean_glucose   GMI
#>   <chr>     <dbl> <dbl> <dbl> <dbl>  <dbl>  <dbl> <dbl> <dbl>        <dbl> <dbl>
#> 1 Subject 1  91.7 73.7   0.14  0      8.2    0.38  26.9  33.3         124.  6.27
#> 2 Subject 2  26.4  3.36  0     0     73.6   26.1   24.0  52.4         218.  8.54
#> 3 Subject 3  81.3 49.8   0.33  0     18.3    5.68  29.1  44.8         154.  6.99
#> 4 Subject 4  95.1 67.7   0.27  0.05   4.61   0     22.4  29.1         130.  6.41
#> 5 Subject 5  62.1 30.1   0.1   0     37.8   11.3   33.6  58.6         175.  7.49
#> # ℹ 13 more variables: uGMI <dbl>, GRI <dbl>, sensor_wear_percent <dbl>,
#> #   hypo_lv1_total_episodes <int>, hypo_lv2_total_episodes <int>,
#> #   hypo_extended_total_episodes <int>, hypo_lv1_excl_total_episodes <int>,
#> #   hypo_rebound_total_episodes <int>, hyper_lv1_total_episodes <int>,
#> #   hyper_lv2_total_episodes <int>, hyper_extended_total_episodes <int>,
#> #   hyper_lv1_excl_total_episodes <int>, hyper_rebound_total_episodes <int>
#> 
#> $glycemic_event_summary
#> # A tibble: 50 × 6
#>    id        type  level    total_episodes avg_ep_per_day avg_minutes_below_54…¹
#>    <chr>     <chr> <chr>             <int>          <dbl>                  <dbl>
#>  1 Subject 1 hypo  lv1                   1           0.09                      0
#>  2 Subject 1 hypo  lv2                   0           0                         0
#>  3 Subject 1 hypo  extended              0           0                         0
#>  4 Subject 1 hypo  lv1_excl              1           0.09                      0
#>  5 Subject 1 hypo  rebound               0           0                         0
#>  6 Subject 1 hyper lv1                  16           1.44                      0
#>  7 Subject 1 hyper lv2                   2           0.18                      0
#>  8 Subject 1 hyper extended              0           0                         0
#>  9 Subject 1 hyper lv1_excl             14           1.26                      0
#> 10 Subject 1 hyper rebound               0           0                         0
#> # ℹ 40 more rows
#> # ℹ abbreviated name: ¹​avg_minutes_below_54_per_episode
```

``` r

all_events_hall <- detect_all_events(example_data_hall)
print(all_events_hall)
#> $subject_summary
#> # A tibble: 19 × 24
#>    id         TIR  TITR TBR70 TBR54 TAR180 TAR250    CV    SD mean_glucose   GMI
#>    <chr>    <dbl> <dbl> <dbl> <dbl>  <dbl>  <dbl> <dbl> <dbl>        <dbl> <dbl>
#>  1 1636-69…  96.9  88.0  0.54  0      2.55   0     25.2  27.3        108.   5.9 
#>  2 1636-69…  99.6  86.5  0.17  0      0.28   0     17.5  20.1        115.   6.06
#>  3 1636-69…  99.8  97.0  0.06  0      0.17   0     14.1  15.2        108.   5.9 
#>  4 1636-69…  98.1  89.8  0.91  0      1.02   0     22.0  24.0        109.   5.91
#>  5 1636-69… 100    96.6  0     0      0      0     14.3  14.7        103.   5.78
#>  6 1636-69… 100    94.3  0     0      0      0     14.9  16.8        113.   6.02
#>  7 1636-70…  97.1  89.8  1.46  0.22   1.41   0     19.7  22.3        113.   6.01
#>  8 1636-70…  97.1  85.8  2.64  0      0.27   0     19.7  22.5        114.   6.04
#>  9 2133-004  94.3  74.7  0.73  0      5.01   0     22.6  28.7        127.   6.34
#> 10 2133-015  97.8  94.3  1.2   0      0.98   0     17.4  18.9        109.   5.91
#> 11 2133-017  99.8  91.3  0.06  0      0.11   0     18.8  20.6        110.   5.93
#> 12 2133-018  88.3  80.4  0     0     11.7    1.86  31.1  39.4        127.   6.34
#> 13 2133-019  98.4  89.7  1.44  0.06   0.11   0     21.1  22.5        107.   5.86
#> 14 2133-021  91.3  70.7  0.61  0      8.07   0     24.7  32.1        130.   6.42
#> 15 2133-024  93.8  91.0  6.15  0.55   0      0     20.1  20.0         99.4  5.69
#> 16 2133-027  94.5  93.6  5.48  0      0      0     14.7  13.4         91.1  5.49
#> 17 2133-035  99.2  95.0  0.55  0.05   0.27   0     16.6  16.9        102.   5.74
#> 18 2133-036  93.5  82.6  5.07  0      1.43   0     24.7  26.6        108.   5.88
#> 19 2133-039  95.1  87.3  4.22  0.15   0.7    0     22.8  23.7        104.   5.8 
#> # ℹ 13 more variables: uGMI <dbl>, GRI <dbl>, sensor_wear_percent <dbl>,
#> #   hypo_lv1_total_episodes <int>, hypo_lv2_total_episodes <int>,
#> #   hypo_extended_total_episodes <int>, hypo_lv1_excl_total_episodes <int>,
#> #   hypo_rebound_total_episodes <int>, hyper_lv1_total_episodes <int>,
#> #   hyper_lv2_total_episodes <int>, hyper_extended_total_episodes <int>,
#> #   hyper_lv1_excl_total_episodes <int>, hyper_rebound_total_episodes <int>
#> 
#> $glycemic_event_summary
#> # A tibble: 190 × 6
#>    id          type  level  total_episodes avg_ep_per_day avg_minutes_below_54…¹
#>    <chr>       <chr> <chr>           <int>          <dbl>                  <dbl>
#>  1 1636-69-001 hypo  lv1                 3           0.47                      0
#>  2 1636-69-001 hypo  lv2                 0           0                         0
#>  3 1636-69-001 hypo  exten…              0           0                         0
#>  4 1636-69-001 hypo  lv1_e…              3           0.47                      0
#>  5 1636-69-001 hypo  rebou…              0           0                         0
#>  6 1636-69-001 hyper lv1                 4           0.62                      0
#>  7 1636-69-001 hyper lv2                 0           0                         0
#>  8 1636-69-001 hyper exten…              0           0                         0
#>  9 1636-69-001 hyper lv1_e…              4           0.62                      0
#> 10 1636-69-001 hyper rebou…              0           0                         0
#> # ℹ 180 more rows
#> # ℹ abbreviated name: ¹​avg_minutes_below_54_per_episode
```

## Speed comparison

We compare performance using `microbenchmark` on both datasets. Each
benchmark contrasts
[`iglu::episode_calculation()`](https://irinagain.github.io/iglu/reference/episode_calculation.html)
with
[`cgmguru::detect_all_events()`](https://shstat1729.github.io/cgmguru/reference/detect_all_events.md).

``` r

library(microbenchmark)
library(iglu)

# example_data_5_subject
bench_5 <- microbenchmark(
  episode_calculation = iglu::episode_calculation(example_data_5_subject),
  detect_all_events   = cgmguru::detect_all_events(example_data_5_subject),
  times = 10,
  unit = "ms"
)
print(bench_5)
#> Unit: milliseconds
#>                 expr        min         lq       mean     median         uq
#>  episode_calculation 949.679023 952.857108 976.415117 963.119588 969.866449
#>    detect_all_events   8.341683   8.518285   8.602283   8.624979   8.674618
#>          max neval
#>  1115.871968    10
#>     8.788928    10

# example_data_hall (all subjects)
bench_hall <- microbenchmark(
  episode_calculation = iglu::episode_calculation(example_data_hall),
  detect_all_events   = cgmguru::detect_all_events(example_data_hall),
  times = 10,
  unit = "ms"
)
print(bench_hall)
#> Unit: milliseconds
#>                 expr        min         lq       mean     median        uq
#>  episode_calculation 2710.15782 2737.93817 2775.70051 2760.59695 2826.1545
#>    detect_all_events   21.78925   22.29859   22.97372   22.44381   22.8919
#>         max neval
#>  2858.04653    10
#>    27.24435    10
```

``` r

cat("Note: Installed 'iglu' version has a different 'episode_calculation' API; iglu examples are skipped.\n")
```

## References

- Broll S, Urbanek J, Buchanan D, Chun E, Muschelli J, Punjabi N, and
  Gaynanova I. Interpreting blood glucose data with R package iglu.
  *PLoS One*. 2021;16(4):e0248560. <doi:10.1371/journal.pone.0248560>.
- Chun E, Fernandes JN, and Gaynanova I. An Update on the iglu Software
  Package for Interpreting Continuous Glucose Monitoring Data. *Diabetes
  Technology & Therapeutics*. 2024;26(12):939-950.
  <doi:10.1089/dia.2024.0154>.
- iglu package site and citation guidance:
  <https://irinagain.github.io/iglu/> and
  <https://irinagain.github.io/iglu/authors.html>.
