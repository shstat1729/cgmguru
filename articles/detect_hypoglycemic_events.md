# detect_hypoglycemic_events function

## detect_hypoglycemic_events

Runs the documented examples for
[`detect_hypoglycemic_events()`](https://shstat1729.github.io/cgmguru/reference/detect_hypoglycemic_events.md).

``` r

example(detect_hypoglycemic_events, package = "cgmguru", run.dontrun = FALSE)
#> 
#> dtct__> # Load sample data
#> dtct__> library(iglu)
#> 
#> dtct__> data(example_data_5_subject)
#> 
#> dtct__> data(example_data_hall)
#> 
#> dtct__> # Level 1 Hypoglycemia Event (>=15 consecutive min of <70 mg/dL and event
#> dtct__> # ends when there is >=15 consecutive min with a CGM sensor value of >=70 mg/dL)
#> dtct__> hypo_lv1 <- detect_hypoglycemic_events(example_data_5_subject, type = "lv1")
#> 
#> dtct__> print(hypo_lv1$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1              1           0.09
#> 2 Subject 2              0           0   
#> 3 Subject 3              1           0.18
#> 4 Subject 4              2           0.16
#> 5 Subject 5              1           0.1 
#> 
#> dtct__> # Level 2 Hypoglycemia Event (>=15 consecutive min of <54 mg/dL and event
#> dtct__> # ends when there is >=15 consecutive min with a CGM sensor value of >=54 mg/dL)
#> dtct__> hypo_lv2 <- detect_hypoglycemic_events(example_data_5_subject, type = "lv2")
#> 
#> dtct__> # Extended Hypoglycemia Event (>120 consecutive min of <70 mg/dL and event
#> dtct__> # ends when there is >=15 consecutive min with a CGM sensor value of >=70 mg/dL)
#> dtct__> hypo_extended <- detect_hypoglycemic_events(example_data_5_subject, type = "extended")
#> 
#> dtct__> print(hypo_extended$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1              0              0
#> 2 Subject 2              0              0
#> 3 Subject 3              0              0
#> 4 Subject 4              0              0
#> 5 Subject 5              0              0
#> 
#> dtct__> # Custom criteria method for the same standard definitions
#> dtct__> hypo_lv1_custom <- detect_hypoglycemic_events(
#> dtct__+   example_data_5_subject,
#> dtct__+   start_gl = 70,
#> dtct__+   dur_length = 15,
#> dtct__+   end_length = 15
#> dtct__+ )
#> 
#> dtct__> hypo_lv2_custom <- detect_hypoglycemic_events(
#> dtct__+   example_data_5_subject,
#> dtct__+   start_gl = 54,
#> dtct__+   dur_length = 15,
#> dtct__+   end_length = 15
#> dtct__+ )
#> 
#> dtct__> hypo_extended_custom <- detect_hypoglycemic_events(
#> dtct__+   example_data_5_subject,
#> dtct__+   start_gl = 70,
#> dtct__+   dur_length = 120,
#> dtct__+   end_length = 15
#> dtct__+ )
#> 
#> dtct__> # Compare event rates across levels
#> dtct__> cat("Level 1 episodes:", sum(hypo_lv1$events_total$total_episodes), "\n")
#> Level 1 episodes: 5 
#> 
#> dtct__> cat("Level 2 episodes:", sum(hypo_lv2$events_total$total_episodes), "\n")
#> Level 2 episodes: 0 
#> 
#> dtct__> cat("Extended episodes:", sum(hypo_extended$events_total$total_episodes), "\n")
#> Extended episodes: 0 
#> 
#> dtct__> # Analysis on larger dataset with Level 1 criteria
#> dtct__> large_hypo <- detect_hypoglycemic_events(example_data_hall, type = "lv1")
#> 
#> dtct__> print(large_hypo$events_total)
#> # A tibble: 19 × 3
#>    id           total_episodes avg_ep_per_day
#>    <chr>                 <int>          <dbl>
#>  1 1636-69-001               3           0.47
#>  2 1636-69-026               0           0   
#>  3 1636-69-032               0           0   
#>  4 1636-69-090               4           0.61
#>  5 1636-69-091               0           0   
#>  6 1636-69-114               0           0   
#>  7 1636-70-1005              2           0.31
#>  8 1636-70-1010              5           0.78
#>  9 2133-004                  2           0.32
#> 10 2133-015                  2           0.31
#> 11 2133-017                  0           0   
#> 12 2133-018                  0           0   
#> 13 2133-019                  3           0.47
#> 14 2133-021                  1           0.16
#> 15 2133-024                  8           1.26
#> 16 2133-027                  3           0.44
#> 17 2133-035                  1           0.15
#> 18 2133-036                  8           1.1 
#> 19 2133-039                 10           1.33
#> 
#> dtct__> # Analysis on larger dataset with Level 2 criteria
#> dtct__> large_hypo_lv2 <- detect_hypoglycemic_events(example_data_hall, type = "lv2")
#> 
#> dtct__> print(large_hypo_lv2$events_total)
#> # A tibble: 19 × 3
#>    id           total_episodes avg_ep_per_day
#>    <chr>                 <int>          <dbl>
#>  1 1636-69-001               0           0   
#>  2 1636-69-026               0           0   
#>  3 1636-69-032               0           0   
#>  4 1636-69-090               0           0   
#>  5 1636-69-091               0           0   
#>  6 1636-69-114               0           0   
#>  7 1636-70-1005              1           0.15
#>  8 1636-70-1010              0           0   
#>  9 2133-004                  0           0   
#> 10 2133-015                  0           0   
#> 11 2133-017                  0           0   
#> 12 2133-018                  0           0   
#> 13 2133-019                  0           0   
#> 14 2133-021                  0           0   
#> 15 2133-024                  1           0.16
#> 16 2133-027                  0           0   
#> 17 2133-035                  0           0   
#> 18 2133-036                  0           0   
#> 19 2133-039                  1           0.13
#> 
#> dtct__> # Analysis on larger dataset with Extended criteria
#> dtct__> large_hypo_extended <- detect_hypoglycemic_events(example_data_hall, type = "extended")
#> 
#> dtct__> print(large_hypo_extended$events_total)
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
#>  8 1636-70-1010              1           0.16
#>  9 2133-004                  0           0   
#> 10 2133-015                  0           0   
#> 11 2133-017                  0           0   
#> 12 2133-018                  0           0   
#> 13 2133-019                  0           0   
#> 14 2133-021                  0           0   
#> 15 2133-024                  1           0.16
#> 16 2133-027                  1           0.15
#> 17 2133-035                  0           0   
#> 18 2133-036                  1           0.14
#> 19 2133-039                  0           0
```
