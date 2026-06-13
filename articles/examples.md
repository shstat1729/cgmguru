# cgmguru: Examples Gallery

## Overview

This vignette renders and runs the example scripts provided in the
package’s `/inst/examples/` directory, so you can see inputs, outputs,
and typical workflows in one HTML document.

If you are viewing this from RStudio Help, use the “Open in Browser”
button for the best experience.

### Notes

- Code is executed by sourcing the example scripts directly from
  `/inst/examples/`.
- Some examples may take longer on large datasets.
- Figures may be produced where applicable.

------------------------------------------------------------------------

### GRID

``` r

source(file.path(examples_dir, "grid.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > grid_result <- grid(example_data_5_subject, gap = 15, 
#> +     threshold = 130)
#> 
#> > print(grid_result$episode_counts)
#> # A tibble: 5 × 2
#>   id        episode_counts
#>   <chr>              <int>
#> 1 Subject 1             10
#> 2 Subject 2             22
#> 3 Subject 3              7
#> 4 Subject 4             18
#> 5 Subject 5             42
#> 
#> > print(grid_result$episode_start)
#> # A tibble: 99 × 4
#>    id        time                   gl index
#>    <chr>     <dttm>              <dbl> <int>
#>  1 Subject 1 2015-06-11 15:30:07   143   966
#>  2 Subject 1 2015-06-11 17:10:07   157   985
#>  3 Subject 1 2015-06-11 22:00:06   135  1038
#>  4 Subject 1 2015-06-11 22:25:06   162  1043
#>  5 Subject 1 2015-06-12 07:40:04   160  1154
#>  6 Subject 1 2015-06-13 16:34:59   132  1415
#>  7 Subject 1 2015-06-14 17:39:55   176  1676
#>  8 Subject 1 2015-06-16 19:14:47   166  2222
#>  9 Subject 1 2015-06-18 14:29:40   187  2720
#> 10 Subject 1 2015-06-18 18:19:39   132  2765
#> # ℹ 89 more rows
#> 
#> > print(grid_result$grid_vector)
#> # A tibble: 13,866 × 1
#>     grid
#>    <int>
#>  1     0
#>  2     0
#>  3     0
#>  4     0
#>  5     0
#>  6     0
#>  7     0
#>  8     0
#>  9     0
#> 10     0
#> # ℹ 13,856 more rows
#> 
#> > sensitive_result <- grid(example_data_5_subject, gap = 10, 
#> +     threshold = 120)
#> 
#> > large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
#> 
#> > print(paste("Detected", sum(large_grid$episode_counts$episode_counts), 
#> +     "episodes"))
#> [1] "Detected 79 episodes"
#> 
#> > print(large_grid$episode_start)
#> # A tibble: 79 × 4
#>    id          time                   gl index
#>    <chr>       <dttm>              <dbl> <int>
#>  1 1636-69-001 2014-02-04 07:47:05   138   336
#>  2 1636-69-001 2014-02-04 17:42:03   138   455
#>  3 1636-69-001 2014-02-05 08:41:59   137   635
#>  4 1636-69-001 2015-03-29 14:33:30   137   786
#>  5 1636-69-001 2015-03-30 10:53:25   132   979
#>  6 1636-69-001 2015-03-31 09:18:19   143  1202
#>  7 1636-69-001 2015-03-31 13:58:18   136  1258
#>  8 1636-69-001 2015-04-01 16:58:12   132  1581
#>  9 1636-69-026 2015-11-24 14:37:18   142  2011
#> 10 1636-69-026 2015-11-24 23:32:16   139  2118
#> # ℹ 69 more rows
#> 
#> > print(large_grid$grid_vector)
#> # A tibble: 34,890 × 1
#>     grid
#>    <int>
#>  1     0
#>  2     0
#>  3     0
#>  4     0
#>  5     0
#>  6     0
#>  7     0
#>  8     0
#>  9     0
#> 10     0
#> # ℹ 34,880 more rows
```

### Detect all events

``` r

source(file.path(examples_dir, "detect_all_events.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > all_events <- detect_all_events(example_data_5_subject)
#> 
#> > print(all_events$subject_summary)
#> # A tibble: 5 × 22
#>   id          TIR  TITR TBR70 TBR54 TAR180 TAR250    CV    SD mean_glucose   GMI
#>   <chr>     <dbl> <dbl> <dbl> <dbl>  <dbl>  <dbl> <dbl> <dbl>        <dbl> <dbl>
#> 1 Subject 1  91.7 73.7   0.14  0      8.2    0.38  26.9  33.3         124.  6.27
#> 2 Subject 2  26.4  3.36  0     0     73.6   26.1   24.0  52.4         218.  8.54
#> 3 Subject 3  81.3 49.8   0.33  0     18.3    5.68  29.1  44.8         154.  6.99
#> 4 Subject 4  95.1 67.7   0.27  0.05   4.61   0     22.4  29.1         130.  6.41
#> 5 Subject 5  62.1 30.1   0.1   0     37.8   11.3   33.6  58.6         175.  7.49
#> # ℹ 11 more variables: uGMI <dbl>, GRI <dbl>, sensor_wear_percent <dbl>,
#> #   hypo_lv1_total_episodes <int>, hypo_lv2_total_episodes <int>,
#> #   hypo_extended_total_episodes <int>, hypo_lv1_excl_total_episodes <int>,
#> #   hyper_lv1_total_episodes <int>, hyper_lv2_total_episodes <int>,
#> #   hyper_extended_total_episodes <int>, hyper_lv1_excl_total_episodes <int>
#> 
#> > print(all_events$glycemic_event_summary)
#> # A tibble: 40 × 6
#>    id        type  level    total_episodes avg_ep_per_day avg_minutes_below_54…¹
#>    <chr>     <chr> <chr>             <int>          <dbl>                  <dbl>
#>  1 Subject 1 hypo  lv1                   1           0.09                      0
#>  2 Subject 1 hypo  lv2                   0           0                         0
#>  3 Subject 1 hypo  extended              0           0                         0
#>  4 Subject 1 hypo  lv1_excl              1           0.09                      0
#>  5 Subject 1 hyper lv1                  16           1.44                      0
#>  6 Subject 1 hyper lv2                   2           0.18                      0
#>  7 Subject 1 hyper extended              0           0                         0
#>  8 Subject 1 hyper lv1_excl             14           1.26                      0
#>  9 Subject 2 hypo  lv1                   0           0                         0
#> 10 Subject 2 hypo  lv2                   0           0                         0
#> # ℹ 30 more rows
#> # ℹ abbreviated name: ¹​avg_minutes_below_54_per_episode
#> 
#> > large_all_events <- detect_all_events(example_data_hall)
#> 
#> > print(paste("Total subjects analyzed:", nrow(large_all_events$subject_summary)))
#> [1] "Total subjects analyzed: 19"
#> 
#> > hyperglycemia_events <- all_events$glycemic_event_summary[all_events$glycemic_event_summary$type == 
#> +     "hyper", ]
#> 
#> > hypoglycemia_events <- all_events$glycemic_event_summary[all_events$glycemic_event_summary$type == 
#> +     "hypo", ]
#> 
#> > print("Hyperglycemia events:")
#> [1] "Hyperglycemia events:"
#> 
#> > print(hyperglycemia_events)
#> # A tibble: 20 × 6
#>    id        type  level    total_episodes avg_ep_per_day avg_minutes_below_54…¹
#>    <chr>     <chr> <chr>             <int>          <dbl>                  <dbl>
#>  1 Subject 1 hyper lv1                  16           1.44                      0
#>  2 Subject 1 hyper lv2                   2           0.18                      0
#>  3 Subject 1 hyper extended              0           0                         0
#>  4 Subject 1 hyper lv1_excl             14           1.26                      0
#>  5 Subject 2 hyper lv1                  21           2.13                      0
#>  6 Subject 2 hyper lv2                  19           1.93                      0
#>  7 Subject 2 hyper extended             10           1.02                      0
#>  8 Subject 2 hyper lv1_excl             11           1.12                      0
#>  9 Subject 3 hyper lv1                   9           1.64                      0
#> 10 Subject 3 hyper lv2                   4           0.73                      0
#> 11 Subject 3 hyper extended              2           0.36                      0
#> 12 Subject 3 hyper lv1_excl              5           0.91                      0
#> 13 Subject 4 hyper lv1                  13           1.02                      0
#> 14 Subject 4 hyper lv2                   0           0                         0
#> 15 Subject 4 hyper extended              0           0                         0
#> 16 Subject 4 hyper lv1_excl             13           1.02                      0
#> 17 Subject 5 hyper lv1                  38           3.72                      0
#> 18 Subject 5 hyper lv2                  18           1.76                      0
#> 19 Subject 5 hyper extended             10           0.98                      0
#> 20 Subject 5 hyper lv1_excl             22           2.16                      0
#> # ℹ abbreviated name: ¹​avg_minutes_below_54_per_episode
#> 
#> > print("Hypoglycemia events:")
#> [1] "Hypoglycemia events:"
#> 
#> > print(hypoglycemia_events)
#> # A tibble: 20 × 6
#>    id        type  level    total_episodes avg_ep_per_day avg_minutes_below_54…¹
#>    <chr>     <chr> <chr>             <int>          <dbl>                  <dbl>
#>  1 Subject 1 hypo  lv1                   1           0.09                    0  
#>  2 Subject 1 hypo  lv2                   0           0                       0  
#>  3 Subject 1 hypo  extended              0           0                       0  
#>  4 Subject 1 hypo  lv1_excl              1           0.09                    0  
#>  5 Subject 2 hypo  lv1                   0           0                       0  
#>  6 Subject 2 hypo  lv2                   0           0                       0  
#>  7 Subject 2 hypo  extended              0           0                       0  
#>  8 Subject 2 hypo  lv1_excl              0           0                       0  
#>  9 Subject 3 hypo  lv1                   1           0.18                    0  
#> 10 Subject 3 hypo  lv2                   0           0                       0  
#> 11 Subject 3 hypo  extended              0           0                       0  
#> 12 Subject 3 hypo  lv1_excl              1           0.18                    0  
#> 13 Subject 4 hypo  lv1                   2           0.16                    2.5
#> 14 Subject 4 hypo  lv2                   0           0                       0  
#> 15 Subject 4 hypo  extended              0           0                       0  
#> 16 Subject 4 hypo  lv1_excl              2           0.16                    0  
#> 17 Subject 5 hypo  lv1                   1           0.1                     0  
#> 18 Subject 5 hypo  lv2                   0           0                       0  
#> 19 Subject 5 hypo  extended              0           0                       0  
#> 20 Subject 5 hypo  lv1_excl              1           0.1                     0  
#> # ℹ abbreviated name: ¹​avg_minutes_below_54_per_episode
```

### Detect hyperglycemic events

``` r

source(file.path(examples_dir, "detect_hyperglycemic_events.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > hyper_lv1 <- detect_hyperglycemic_events(example_data_5_subject, 
#> +     type = "lv1")
#> 
#> > print(hyper_lv1$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1             16           1.44
#> 2 Subject 2             21           2.13
#> 3 Subject 3              9           1.64
#> 4 Subject 4             13           1.02
#> 5 Subject 5             38           3.72
#> 
#> > hyper_lv2 <- detect_hyperglycemic_events(example_data_5_subject, 
#> +     type = "lv2")
#> 
#> > print(hyper_lv2$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1              2           0.18
#> 2 Subject 2             19           1.93
#> 3 Subject 3              4           0.73
#> 4 Subject 4              0           0   
#> 5 Subject 5             18           1.76
#> 
#> > hyper_extended <- detect_hyperglycemic_events(example_data_5_subject, 
#> +     type = "extended")
#> 
#> > large_hyper <- detect_hyperglycemic_events(example_data_hall, 
#> +     type = "lv1")
#> 
#> > print(paste("Total hyperglycemic episodes:", sum(large_hyper$events_total$total_episodes)))
#> [1] "Total hyperglycemic episodes: 48"
#> 
#> > if (nrow(hyper_lv1$events_detailed) > 0) {
#> +     first_subject <- hyper_lv1$events_detailed$id[1]
#> +     subject_events <- hyper_lv1$events_detailed[hyper_lv1$events_detailed$id == 
#> +         first_subject, ]
#> +     head(subject_events)
#> + }
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

### Detect hypoglycemic events

``` r

source(file.path(examples_dir, "detect_hypoglycemic_events.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > hypo_lv1 <- detect_hypoglycemic_events(example_data_5_subject, 
#> +     type = "lv1")
#> 
#> > print(hypo_lv1$events_total)
#> # A tibble: 5 × 3
#>   id        total_episodes avg_ep_per_day
#>   <chr>              <int>          <dbl>
#> 1 Subject 1              1           0.09
#> 2 Subject 2              0           0   
#> 3 Subject 3              1           0.18
#> 4 Subject 4              2           0.16
#> 5 Subject 5              1           0.1 
#> 
#> > hypo_lv2 <- detect_hypoglycemic_events(example_data_5_subject, 
#> +     type = "lv2")
#> 
#> > hypo_extended <- detect_hypoglycemic_events(example_data_5_subject, 
#> +     type = "extended")
#> 
#> > large_hypo <- detect_hypoglycemic_events(example_data_hall, 
#> +     type = "lv1")
#> 
#> > print(paste("Total hypoglycemic episodes:", sum(large_hypo$events_total$total_episodes)))
#> [1] "Total hypoglycemic episodes: 52"
#> 
#> > cat("Level 1 episodes:", sum(hypo_lv1$events_total$total_episodes), 
#> +     "\n")
#> Level 1 episodes: 5 
#> 
#> > cat("Level 2 episodes:", sum(hypo_lv2$events_total$total_episodes), 
#> +     "\n")
#> Level 2 episodes: 0
```

### Excursion

``` r

source(file.path(examples_dir, "excursion.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > excursion_result <- excursion(example_data_5_subject, 
#> +     gap = 15)
#> 
#> > print(paste("Excursion vector length:", length(excursion_result$excursion_vector)))
#> [1] "Excursion vector length: 1"
#> 
#> > print(excursion_result$episode_counts)
#> # A tibble: 5 × 2
#>   id        episode_counts
#>   <chr>              <int>
#> 1 Subject 1              9
#> 2 Subject 2             14
#> 3 Subject 3             11
#> 4 Subject 4             17
#> 5 Subject 5             34
#> 
#> > excursion_30min <- excursion(example_data_5_subject, 
#> +     gap = 30)
#> 
#> > large_excursion <- excursion(example_data_hall, gap = 15)
#> 
#> > print(paste("Excursion vector length in larger dataset:", 
#> +     length(large_excursion$excursion_vector)))
#> [1] "Excursion vector length in larger dataset: 1"
#> 
#> > print(paste("Total episodes:", sum(large_excursion$episode_counts$episode_counts)))
#> [1] "Total episodes: 111"
```

### Local maxima

``` r

source(file.path(examples_dir, "find_local_maxima.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > maxima_result <- find_local_maxima(example_data_5_subject)
#> 
#> > print(paste("Found", nrow(maxima_result$local_maxima_vector), 
#> +     "local maxima"))
#> [1] "Found 1602 local maxima"
#> 
#> > large_maxima <- find_local_maxima(example_data_hall)
#> 
#> > print(paste("Found", nrow(large_maxima$local_maxima_vector), 
#> +     "local maxima in larger dataset"))
#> [1] "Found 4991 local maxima in larger dataset"
#> 
#> > head(maxima_result$local_maxima_vector)
#> # A tibble: 6 × 1
#>   local_maxima
#>          <int>
#> 1            8
#> 2           23
#> 3           24
#> 4           65
#> 5           70
#> 6           77
#> 
#> > head(maxima_result$merged_results)
#> # A tibble: 6 × 3
#>   id        time                   gl
#>   <chr>     <dttm>              <dbl>
#> 1 Subject 1 2015-06-06 18:05:27   159
#> 2 Subject 1 2015-06-06 20:15:27   174
#> 3 Subject 1 2015-06-06 20:20:26   174
#> 4 Subject 1 2015-06-07 01:20:26    88
#> 5 Subject 1 2015-06-07 01:45:25    92
#> 6 Subject 1 2015-06-07 02:20:25    92
```

### Max after hours

``` r

source(file.path(examples_dir, "find_max_after_hours.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > start_index <- seq(1, nrow(example_data_5_subject), 
#> +     by = 100)
#> 
#> > start_points <- data.frame(start_index = start_index)
#> 
#> > max_after <- find_max_after_hours(example_data_5_subject, 
#> +     start_points, hours = 2)
#> 
#> > print(paste("Found", length(max_after$max_index), 
#> +     "maximum points"))
#> [1] "Found 1 maximum points"
#> 
#> > max_after_1h <- find_max_after_hours(example_data_5_subject, 
#> +     start_points, hours = 1)
#> 
#> > large_start_index <- seq(1, nrow(example_data_hall), 
#> +     by = 200)
#> 
#> > large_start_points <- data.frame(start_index = large_start_index)
#> 
#> > large_max_after <- find_max_after_hours(example_data_hall, 
#> +     large_start_points, hours = 2)
#> 
#> > print(paste("Found", length(large_max_after$max_index), 
#> +     "maximum points in larger dataset"))
#> [1] "Found 1 maximum points in larger dataset"
```

### Max before hours

``` r

source(file.path(examples_dir, "find_max_before_hours.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > start_index <- seq(1, nrow(example_data_5_subject), 
#> +     by = 100)
#> 
#> > start_points <- data.frame(start_index = start_index)
#> 
#> > max_before <- find_max_before_hours(example_data_5_subject, 
#> +     start_points, hours = 2)
#> 
#> > print(paste("Found", length(max_before$max_index), 
#> +     "maximum points"))
#> [1] "Found 1 maximum points"
#> 
#> > max_before_1h <- find_max_before_hours(example_data_5_subject, 
#> +     start_points, hours = 1)
#> 
#> > large_start_index <- seq(1, nrow(example_data_hall), 
#> +     by = 200)
#> 
#> > large_start_points <- data.frame(start_index = large_start_index)
#> 
#> > large_max_before <- find_max_before_hours(example_data_hall, 
#> +     large_start_points, hours = 2)
#> 
#> > print(paste("Found", length(large_max_before$max_index), 
#> +     "maximum points in larger dataset"))
#> [1] "Found 1 maximum points in larger dataset"
```

### Min after hours

``` r

source(file.path(examples_dir, "find_min_after_hours.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > start_index <- seq(1, nrow(example_data_5_subject), 
#> +     by = 100)
#> 
#> > start_points <- data.frame(start_index = start_index)
#> 
#> > min_after <- find_min_after_hours(example_data_5_subject, 
#> +     start_points, hours = 2)
#> 
#> > print(paste("Found", length(min_after$min_index), 
#> +     "minimum points"))
#> [1] "Found 1 minimum points"
#> 
#> > min_after_1h <- find_min_after_hours(example_data_5_subject, 
#> +     start_points, hours = 1)
#> 
#> > large_start_index <- seq(1, nrow(example_data_hall), 
#> +     by = 200)
#> 
#> > large_start_points <- data.frame(start_index = large_start_index)
#> 
#> > large_min_after <- find_min_after_hours(example_data_hall, 
#> +     large_start_points, hours = 2)
#> 
#> > print(paste("Found", length(large_min_after$min_index), 
#> +     "minimum points in larger dataset"))
#> [1] "Found 1 minimum points in larger dataset"
```

### Min before hours

``` r

source(file.path(examples_dir, "find_min_before_hours.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > start_index <- seq(1, nrow(example_data_5_subject), 
#> +     by = 100)
#> 
#> > start_points <- data.frame(start_index = start_index)
#> 
#> > min_before <- find_min_before_hours(example_data_5_subject, 
#> +     start_points, hours = 2)
#> 
#> > print(paste("Found", length(min_before$min_index), 
#> +     "minimum points"))
#> [1] "Found 1 minimum points"
#> 
#> > min_before_1h <- find_min_before_hours(example_data_5_subject, 
#> +     start_points, hours = 1)
#> 
#> > large_start_index <- seq(1, nrow(example_data_hall), 
#> +     by = 200)
#> 
#> > large_start_points <- data.frame(start_index = large_start_index)
#> 
#> > large_min_before <- find_min_before_hours(example_data_hall, 
#> +     large_start_points, hours = 2)
#> 
#> > print(paste("Found", length(large_min_before$min_index), 
#> +     "minimum points in larger dataset"))
#> [1] "Found 1 minimum points in larger dataset"
```

### Maxima + GRID combined

``` r

source(file.path(examples_dir, "maxima_grid.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > maxima_result <- maxima_grid(example_data_5_subject, 
#> +     threshold = 130, gap = 60, hours = 2)
#> 
#> > print(maxima_result$episode_counts)
#> # A tibble: 5 × 2
#>   id        episode_counts
#>   <chr>              <int>
#> 1 Subject 1              8
#> 2 Subject 2             18
#> 3 Subject 3              7
#> 4 Subject 4             16
#> 5 Subject 5             39
#> 
#> > print(maxima_result$results)
#> # A tibble: 88 × 8
#>    id        grid_time       grid_gl maxima_time maxima_glucose time_to_peak_min
#>    <chr>     <dttm>            <dbl> <dttm>               <dbl>            <dbl>
#>  1 Subject 1 2015-06-11 15:…     143 2015-06-11…            276               40
#>  2 Subject 1 2015-06-11 22:…     135 2015-06-11…            209               50
#>  3 Subject 1 2015-06-12 07:…     160 2015-06-12…            210               40
#>  4 Subject 1 2015-06-13 16:…     132 2015-06-13…            202               60
#>  5 Subject 1 2015-06-14 17:…     176 2015-06-14…            227               45
#>  6 Subject 1 2015-06-16 19:…     166 2015-06-16…            208               65
#>  7 Subject 1 2015-06-18 14:…     187 2015-06-18…            212               20
#>  8 Subject 1 2015-06-18 18:…     132 2015-06-18…            183               35
#>  9 Subject 2 2015-02-24 20:…     140 2015-02-24…            222               85
#> 10 Subject 2 2015-02-25 19:…     173 2015-02-25…            273              125
#> # ℹ 78 more rows
#> # ℹ 2 more variables: grid_index <int>, maxima_index <int>
#> 
#> > sensitive_maxima <- maxima_grid(example_data_5_subject, 
#> +     threshold = 120, gap = 30, hours = 1)
#> 
#> > print(sensitive_maxima$episode_counts)
#> # A tibble: 5 × 2
#>   id        episode_counts
#>   <chr>              <int>
#> 1 Subject 1             10
#> 2 Subject 2             19
#> 3 Subject 3             10
#> 4 Subject 4             20
#> 5 Subject 5             40
#> 
#> > print(sensitive_maxima$results)
#> # A tibble: 99 × 8
#>    id        grid_time       grid_gl maxima_time maxima_glucose time_to_peak_min
#>    <chr>     <dttm>            <dbl> <dttm>               <dbl>            <dbl>
#>  1 Subject 1 2015-06-11 15:…     143 2015-06-11…            276               40
#>  2 Subject 1 2015-06-11 17:…     157 2015-06-11…            267               55
#>  3 Subject 1 2015-06-11 21:…     125 2015-06-11…            209               60
#>  4 Subject 1 2015-06-12 07:…     160 2015-06-12…            210               40
#>  5 Subject 1 2015-06-13 16:…     124 2015-06-13…            202               65
#>  6 Subject 1 2015-06-14 17:…     176 2015-06-14…            228               95
#>  7 Subject 1 2015-06-16 19:…     166 2015-06-16…            208               65
#>  8 Subject 1 2015-06-18 13:…     126 2015-06-18…            183               55
#>  9 Subject 1 2015-06-18 14:…     187 2015-06-18…            212               20
#> 10 Subject 1 2015-06-18 18:…     132 2015-06-18…            183               35
#> # ℹ 89 more rows
#> # ℹ 2 more variables: grid_index <int>, maxima_index <int>
#> 
#> > large_maxima <- maxima_grid(example_data_hall, threshold = 130, 
#> +     gap = 60, hours = 2)
#> 
#> > print(large_maxima$episode_counts)
#> # A tibble: 19 × 2
#>    id           episode_counts
#>    <chr>                 <int>
#>  1 1636-69-001               8
#>  2 1636-69-026               7
#>  3 1636-69-032               2
#>  4 1636-69-090               3
#>  5 1636-69-091               1
#>  6 1636-69-114               0
#>  7 1636-70-1005              8
#>  8 1636-70-1010              2
#>  9 2133-004                  5
#> 10 2133-015                  4
#> 11 2133-017                  2
#> 12 2133-018                 12
#> 13 2133-019                  2
#> 14 2133-021                 10
#> 15 2133-024                  1
#> 16 2133-027                  1
#> 17 2133-035                  1
#> 18 2133-036                  2
#> 19 2133-039                  5
#> 
#> > print(large_maxima$results)
#> # A tibble: 76 × 8
#>    id          grid_time     grid_gl maxima_time maxima_glucose time_to_peak_min
#>    <chr>       <dttm>          <dbl> <dttm>               <dbl>            <dbl>
#>  1 1636-69-001 2014-02-04 0…     138 2014-02-04…            194               30
#>  2 1636-69-001 2014-02-04 1…     138 2014-02-04…            225               60
#>  3 1636-69-001 2014-02-05 0…     137 2014-02-05…            196               45
#>  4 1636-69-001 2015-03-29 1…     137 2015-03-29…            250               65
#>  5 1636-69-001 2015-03-30 1…     132 2015-03-30…            181               45
#>  6 1636-69-001 2015-03-31 0…     143 2015-03-31…            177               20
#>  7 1636-69-001 2015-03-31 1…     136 2015-03-31…            169               30
#>  8 1636-69-001 2015-04-01 1…     132 2015-04-01…            165               50
#>  9 1636-69-026 2015-11-24 1…     142 2015-11-24…            182               40
#> 10 1636-69-026 2015-11-24 2…     139 2015-11-25…            171               35
#> # ℹ 66 more rows
#> # ℹ 2 more variables: grid_index <int>, maxima_index <int>
```

### Ordering utility

``` r

source(file.path(examples_dir, "orderfast.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > set.seed(123)
#> 
#> > shuffled <- example_data_5_subject[sample(seq_len(nrow(example_data_5_subject)), 
#> +     replace = FALSE), ]
#> 
#> > baseline <- orderfast(example_data_5_subject)
#> 
#> > ordered_shuffled <- orderfast(shuffled)
#> 
#> > print(paste("Identical after ordering:", identical(baseline, 
#> +     ordered_shuffled)))
#> [1] "Identical after ordering: TRUE"
#> 
#> > head(baseline[, c("id", "time", "gl")])
#>          id                time  gl
#> 1 Subject 1 2015-06-06 16:50:27 153
#> 2 Subject 1 2015-06-06 17:05:27 137
#> 3 Subject 1 2015-06-06 17:10:27 128
#> 4 Subject 1 2015-06-06 17:15:28 121
#> 5 Subject 1 2015-06-06 17:25:27 120
#> 6 Subject 1 2015-06-06 17:45:27 138
#> 
#> > head(ordered_shuffled[, c("id", "time", "gl")])
#>          id                time  gl
#> 1 Subject 1 2015-06-06 16:50:27 153
#> 2 Subject 1 2015-06-06 17:05:27 137
#> 3 Subject 1 2015-06-06 17:10:27 128
#> 4 Subject 1 2015-06-06 17:15:28 121
#> 5 Subject 1 2015-06-06 17:25:27 120
#> 6 Subject 1 2015-06-06 17:45:27 138
#> 
#> > ordered_large <- orderfast(example_data_hall)
#> 
#> > print(paste("Ordered", nrow(ordered_large), "rows in larger dataset"))
#> [1] "Ordered 34890 rows in larger dataset"
```

### Start finder

``` r

source(file.path(examples_dir, "start_finder.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > binary_vector <- c(0, 0, 1, 1, 0, 1, 0, 0, 1, 1)
#> 
#> > df <- data.frame(episode_starts = binary_vector)
#> 
#> > start_points <- start_finder(df)
#> 
#> > print(paste("Start index:", paste(start_points$start_index, 
#> +     collapse = ", ")))
#> [1] "Start index: 3, 6, 9"
#> 
#> > grid_result <- grid(example_data_5_subject, gap = 15, 
#> +     threshold = 130)
#> 
#> > grid_starts <- start_finder(grid_result$grid_vector)
#> 
#> > print(paste("GRID episode starts:", length(grid_starts$start_index)))
#> [1] "GRID episode starts: 99"
#> 
#> > large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
#> 
#> > large_starts <- start_finder(large_grid$grid_vector)
#> 
#> > print(paste("GRID episode starts in larger dataset:", 
#> +     length(large_starts$start_index)))
#> [1] "GRID episode starts in larger dataset: 79"
```

### Transform dataframe

``` r

source(file.path(examples_dir, "transform_df.R"), echo = TRUE, print.eval = TRUE, max.deparse.length = Inf)
#> 
#> > library(cgmguru)
#> 
#> > library(iglu)
#> 
#> > data(example_data_5_subject)
#> 
#> > data(example_data_hall)
#> 
#> > threshold <- 130
#> 
#> > gap <- 60
#> 
#> > hours <- 2
#> 
#> > grid_result <- grid(example_data_5_subject, gap = gap, 
#> +     threshold = threshold)
#> 
#> > mod_grid <- mod_grid(example_data_5_subject, start_finder(grid_result$grid_vector), 
#> +     hours = hours, gap = gap)
#> 
#> > mod_grid_maxima <- find_max_after_hours(example_data_5_subject, 
#> +     start_finder(mod_grid$mod_grid_vector), hours = hours)
#> 
#> > local_maxima <- find_local_maxima(example_data_5_subject)
#> 
#> > final_maxima <- find_new_maxima(example_data_5_subject, 
#> +     mod_grid_maxima$max_index, local_maxima$local_maxima_vector)
#> 
#> > transform_maxima <- transform_df(grid_result$episode_start, 
#> +     final_maxima)
#> 
#> > final_between_maxima <- detect_between_maxima(example_data_5_subject, 
#> +     transform_maxima)
#> 
#> > hall_threshold <- 130
#> 
#> > hall_gap <- 60
#> 
#> > hall_hours <- 2
#> 
#> > hall_grid_result <- grid(example_data_hall, gap = hall_gap, 
#> +     threshold = hall_threshold)
#> 
#> > hall_mod_grid <- mod_grid(example_data_hall, start_finder(hall_grid_result$grid_vector), 
#> +     hours = hall_hours, gap = hall_gap)
#> 
#> > hall_mod_grid_maxima <- find_max_after_hours(example_data_hall, 
#> +     start_finder(hall_mod_grid$mod_grid_vector), hours = hall_hours)
#> 
#> > hall_local_maxima <- find_local_maxima(example_data_hall)
#> 
#> > hall_final_maxima <- find_new_maxima(example_data_hall, 
#> +     hall_mod_grid_maxima$max_index, hall_local_maxima$local_maxima_vector)
#> 
#> > hall_transform_maxima <- transform_df(hall_grid_result$episode_start, 
#> +     hall_final_maxima)
#> 
#> > hall_final_between_maxima <- detect_between_maxima(example_data_hall, 
#> +     hall_transform_maxima)
```

------------------------------------------------------------------------

### Function reference and help

For detailed function documentation with additional runnable
`@examples`, see the package reference:

``` r

help(package = "cgmguru")
```

You can also open the package vignettes index:

``` r

browseVignettes("cgmguru")
```
