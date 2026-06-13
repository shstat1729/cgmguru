# maxima_grid function

## maxima_grid

Runs the documented examples for
[`maxima_grid()`](https://shstat1729.github.io/cgmguru/reference/maxima_grid.md).

[`maxima_grid()`](https://shstat1729.github.io/cgmguru/reference/maxima_grid.md)
runs the GRID and peak-mapping pipeline on the rows supplied in `df`. It
does not automatically use
[`interpolate_cgm()`](https://shstat1729.github.io/cgmguru/reference/interpolate_cgm.md)
or the full-day event preprocessing grid unless you explicitly pass
interpolated data to the function.

``` r

example(maxima_grid, package = "cgmguru", run.dontrun = FALSE)
#> 
#> mxm_gr> # Load sample data
#> mxm_gr> library(iglu)
#> 
#> mxm_gr> data(example_data_5_subject)
#> 
#> mxm_gr> data(example_data_hall)
#> 
#> mxm_gr> # Combined analysis on smaller dataset
#> mxm_gr> maxima_result <- maxima_grid(example_data_5_subject, threshold = 130, gap = 60, hours = 2)
#> 
#> mxm_gr> print(maxima_result$episode_counts)
#> # A tibble: 5 × 2
#>   id        episode_counts
#>   <chr>              <int>
#> 1 Subject 1              8
#> 2 Subject 2             18
#> 3 Subject 3              7
#> 4 Subject 4             16
#> 5 Subject 5             39
#> 
#> mxm_gr> print(maxima_result$results)
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
#> mxm_gr> # More sensitive analysis
#> mxm_gr> sensitive_maxima <- maxima_grid(example_data_5_subject, threshold = 120, gap = 30, hours = 1)
#> 
#> mxm_gr> print(sensitive_maxima$episode_counts)
#> # A tibble: 5 × 2
#>   id        episode_counts
#>   <chr>              <int>
#> 1 Subject 1             10
#> 2 Subject 2             19
#> 3 Subject 3             10
#> 4 Subject 4             20
#> 5 Subject 5             40
#> 
#> mxm_gr> print(sensitive_maxima$results)
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
#> mxm_gr> # Analysis on larger dataset
#> mxm_gr> large_maxima <- maxima_grid(example_data_hall, threshold = 130, gap = 60, hours = 2)
#> 
#> mxm_gr> print(large_maxima$episode_counts)
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
#> mxm_gr> print(large_maxima$results)
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
