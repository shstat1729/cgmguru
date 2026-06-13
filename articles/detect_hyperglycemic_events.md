# detect_hyperglycemic_events function

## detect_hyperglycemic_events

Runs the documented examples for
[`detect_hyperglycemic_events()`](https://shstat1729.github.io/cgmguru/reference/detect_hyperglycemic_events.md).

``` r

example(detect_hyperglycemic_events, package = "cgmguru", run.dontrun = FALSE)
#> 
#> dtct__> # Load sample data
#> dtct__> library(iglu)
#> 
#> dtct__> data(example_data_5_subject)
#> 
#> dtct__> data(example_data_hall)
#> 
#> dtct__> # Level 1 Hyperglycemia Event (>=15 consecutive min of >180 mg/dL and event
#> dtct__> # ends when there is >=15 consecutive min with a CGM sensor value of <=180 mg/dL)
#> dtct__> hyper_lv1 <- detect_hyperglycemic_events(example_data_5_subject, type = "lv1")
#> 
#> dtct__> print(hyper_lv1$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1             16           1.44
#> 2 Subject 2             21           2.13
#> 3 Subject 3              9           1.64
#> 4 Subject 4             13           1.02
#> 5 Subject 5             38           3.72
#> 
#> dtct__> # Level 2 Hyperglycemia Event (>=15 consecutive min of >250 mg/dL and event
#> dtct__> # ends when there is >=15 consecutive min with a CGM sensor value of <=250 mg/dL)
#> dtct__> hyper_lv2 <- detect_hyperglycemic_events(example_data_5_subject, type = "lv2")
#> 
#> dtct__> print(hyper_lv2$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1              2           0.18
#> 2 Subject 2             19           1.93
#> 3 Subject 3              4           0.73
#> 4 Subject 4              0           0   
#> 5 Subject 5             18           1.76
#> 
#> dtct__> # Extended Hyperglycemia Event (>250 mg/dL lasting >=90 cumulative min within a
#> dtct__> # 120-min period, ends when glucose returns to <=180 mg/dL for >=15 consecutive
#> dtct__> # min after)
#> dtct__> hyper_extended <- detect_hyperglycemic_events(example_data_5_subject, type = "extended")
#> 
#> dtct__> print(hyper_extended$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1              0           0   
#> 2 Subject 2             10           1.02
#> 3 Subject 3              2           0.36
#> 4 Subject 4              0           0   
#> 5 Subject 5             10           0.98
#> 
#> dtct__> # Custom criteria method for the same standard definitions
#> dtct__> hyper_lv1_custom <- detect_hyperglycemic_events(
#> dtct__+   example_data_5_subject,
#> dtct__+   start_gl = 180,
#> dtct__+   dur_length = 15,
#> dtct__+   end_length = 15,
#> dtct__+   end_gl = 180
#> dtct__+ )
#> 
#> dtct__> hyper_lv2_custom <- detect_hyperglycemic_events(
#> dtct__+   example_data_5_subject,
#> dtct__+   start_gl = 250,
#> dtct__+   dur_length = 15,
#> dtct__+   end_length = 15,
#> dtct__+   end_gl = 250
#> dtct__+ )
#> 
#> dtct__> hyper_extended_custom <- detect_hyperglycemic_events(
#> dtct__+   example_data_5_subject,
#> dtct__+   start_gl = 250,
#> dtct__+   dur_length = 120,
#> dtct__+   end_length = 15,
#> dtct__+   end_gl = 180
#> dtct__+ )
#> 
#> dtct__> # Compare event rates across levels
#> dtct__> cat("Level 1 episodes:", sum(hyper_lv1$events_total$total_episodes), "\n")
#> Level 1 episodes: 97 
#> 
#> dtct__> cat("Level 2 episodes:", sum(hyper_lv2$events_total$total_episodes), "\n")
#> Level 2 episodes: 43 
#> 
#> dtct__> cat("Extended episodes:", sum(hyper_extended$events_total$total_episodes), "\n")
#> Extended episodes: 22 
#> 
#> dtct__> # Analysis on larger dataset with Level 1 criteria
#> dtct__> large_hyper <- detect_hyperglycemic_events(example_data_hall, type = "lv1")
#> 
#> dtct__> print(large_hyper$events_total)
#> # A tibble: 19 × 3
#>    id           total_episodes avg_ep_per_day
#>    <chr>                 <int>          <dbl>
#>  1 1636-69-001               4           0.62
#>  2 1636-69-026               1           0.16
#>  3 1636-69-032               1           0.16
#>  4 1636-69-090               3           0.46
#>  5 1636-69-091               0           0   
#>  6 1636-69-114               0           0   
#>  7 1636-70-1005              3           0.46
#>  8 1636-70-1010              1           0.16
#>  9 2133-004                  5           0.81
#> 10 2133-015                  3           0.46
#> 11 2133-017                  1           0.16
#> 12 2133-018                 12           1.94
#> 13 2133-019                  0           0   
#> 14 2133-021                  9           1.44
#> 15 2133-024                  0           0   
#> 16 2133-027                  0           0   
#> 17 2133-035                  1           0.15
#> 18 2133-036                  2           0.28
#> 19 2133-039                  2           0.27
#> 
#> dtct__> # Analysis on larger dataset with Level 2 criteria
#> dtct__> large_hyper_lv2 <- detect_hyperglycemic_events(example_data_hall, type = "lv2")
#> 
#> dtct__> print(large_hyper_lv2$events_total)
#> # A tibble: 19 × 3
#>    id           total_episodes avg_ep_per_day
#>    <chr>                 <int>          <dbl>
#>  1 1636-69-001               0           0   
#>  2 1636-69-026               0           0   
#>  3 1636-69-032               0           0   
#>  4 1636-69-090               0           0   
#>  5 1636-69-091               0           0   
#>  6 1636-69-114               0           0   
#>  7 1636-70-1005              0           0   
#>  8 1636-70-1010              0           0   
#>  9 2133-004                  0           0   
#> 10 2133-015                  0           0   
#> 11 2133-017                  0           0   
#> 12 2133-018                  2           0.32
#> 13 2133-019                  0           0   
#> 14 2133-021                  0           0   
#> 15 2133-024                  0           0   
#> 16 2133-027                  0           0   
#> 17 2133-035                  0           0   
#> 18 2133-036                  0           0   
#> 19 2133-039                  0           0   
#> 
#> dtct__> # Analysis on larger dataset with Extended criteria
#> dtct__> large_hyper_extended <- detect_hyperglycemic_events(example_data_hall, type = "extended")
#> 
#> dtct__> print(large_hyper_extended$events_total)
#> # A tibble: 19 × 3
#>    id           total_episodes avg_ep_per_day
#>    <chr>                 <int>          <dbl>
#>  1 1636-69-001               0           0   
#>  2 1636-69-026               0           0   
#>  3 1636-69-032               0           0   
#>  4 1636-69-090               0           0   
#>  5 1636-69-091               0           0   
#>  6 1636-69-114               0           0   
#>  7 1636-70-1005              0           0   
#>  8 1636-70-1010              0           0   
#>  9 2133-004                  0           0   
#> 10 2133-015                  0           0   
#> 11 2133-017                  0           0   
#> 12 2133-018                  1           0.16
#> 13 2133-019                  0           0   
#> 14 2133-021                  0           0   
#> 15 2133-024                  0           0   
#> 16 2133-027                  0           0   
#> 17 2133-035                  0           0   
#> 18 2133-036                  0           0   
#> 19 2133-039                  0           0   
#> 
#> dtct__> # View detailed events for specific subject
#> dtct__> if(nrow(hyper_lv1$events_detailed) > 0) {
#> dtct__+   first_subject <- hyper_lv1$events_detailed$id[1]
#> dtct__+   subject_events <- hyper_lv1$events_detailed[hyper_lv1$events_detailed$id == first_subject, ]
#> dtct__+   head(subject_events)
#> dtct__+ }
#> # A tibble: 6 × 7
#>   id        start_time          start_glucose end_time            end_glucose
#>   <chr>     <dttm>                      <dbl> <dttm>                    <dbl>
#> 1 Subject 1 2015-06-11 15:45:00          193. 2015-06-11 16:50:00        187.
#> 2 Subject 1 2015-06-11 17:25:00          195. 2015-06-11 19:00:00        183.
#> 3 Subject 1 2015-06-11 19:20:00          181. 2015-06-11 19:45:00        187.
#> 4 Subject 1 2015-06-11 22:35:00          187. 2015-06-11 23:45:00        185.
#> 5 Subject 1 2015-06-12 07:50:00          181. 2015-06-12 09:15:00        181.
#> 6 Subject 1 2015-06-13 16:55:00          180. 2015-06-13 18:25:00        186.
#> # ℹ 2 more variables: start_index <int>, end_index <int>
```
