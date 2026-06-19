# excursion function

## excursion

### Overview

Calculates glucose excursions: \>70 mg/dL rise within 2 hours, not
preceded by a value \<70 mg/dL.

[`excursion()`](https://shstat1729.github.io/cgmguru/reference/excursion.md)
evaluates the rows supplied in `df`. It does not automatically call
[`interpolate_cgm()`](https://shstat1729.github.io/cgmguru/reference/interpolate_cgm.md)
and does not use the full-day event preprocessing grid unless you
explicitly pass interpolated data to
[`excursion()`](https://shstat1729.github.io/cgmguru/reference/excursion.md).

### Inputs

- **df**: CGM dataframe with `id`, `time` (POSIXct), `gl` (mg/dL)
- **gap**: Minimum minutes separating excursions (default 15)

### Returns

- **excursion_vector**: Logical/int vector marking excursions
- **episode_counts**: Counts per `id`
- **episode_start**: Starts with `id`, `time`, `gl`, `index`

### Run documented examples

``` r

example(excursion, package = "cgmguru", run.dontrun = FALSE)
#> 
#> excrsn> # Load sample data
#> excrsn> library(iglu)
#> 
#> excrsn> data(example_data_5_subject)
#> 
#> excrsn> data(example_data_hall)
#> 
#> excrsn> # Calculate glucose excursions
#> excrsn> excursion_result <- excursion(example_data_5_subject, gap = 15)
#> 
#> excrsn> print(paste("Excursion vector length:", length(excursion_result$excursion_vector)))
#> [1] "Excursion vector length: 1"
#> 
#> excrsn> print(excursion_result$episode_counts)
#> # A tibble: 5 × 2
#>   id        episode_counts
#>   <chr>              <int>
#> 1 Subject 1              9
#> 2 Subject 2             14
#> 3 Subject 3             11
#> 4 Subject 4             17
#> 5 Subject 5             34
#> 
#> excrsn> # Excursion analysis with different gap
#> excrsn> excursion_30min <- excursion(example_data_5_subject, gap = 30)
#> 
#> excrsn> # Analysis on larger dataset
#> excrsn> large_excursion <- excursion(example_data_hall, gap = 15)
#> 
#> excrsn> print(paste("Excursion vector length in larger dataset:", length(large_excursion$excursion_vector)))
#> [1] "Excursion vector length in larger dataset: 1"
#> 
#> excrsn> print(paste("Total episodes:", sum(large_excursion$episode_counts$episode_counts)))
#> [1] "Total episodes: 103"
```
