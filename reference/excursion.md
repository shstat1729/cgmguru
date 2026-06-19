# Calculate Glucose Excursions

Calculates glucose excursions in CGM data. An excursion is defined as a
\\\>\\ 70 mg/dL (\\\>\\ 3.9 mmol/L) rise within 2 hours from a starting
glucose value \\\geq\\ 70 mg/dL (\\\geq\\ 3.9 mmol/L), not preceded by a
value \\\<\\ 70 mg/dL (\\\<\\ 3.9 mmol/L).

## Usage

``` r
excursion(df, gap = 15)
```

## Arguments

- df:

  A dataframe containing continuous glucose monitoring (CGM) data. Must
  include columns:

  - `id`: Subject identifier (string or factor)

  - `time`: Time of measurement (POSIXct)

  - `gl`: Glucose value (integer or numeric, mg/dL)

- gap:

  Gap threshold in minutes for excursion calculation (default: 15). This
  parameter defines the minimum time interval between consecutive GRID
  events.

## Value

A list containing:

- `excursion_vector`: Tibble with excursion results (`excursion`)

- `episode_counts`: Tibble with episode counts per subject (`id`,
  `episode_counts`)

- `episode_start`: Tibble with all episode starts with columns:

  - `id`: Subject identifier

  - `time`: Timestamp at which the event occurs; equivalent to
    `df$time[index]`

  - `gl`: Glucose value at the event; equivalent to `df$gl[index]`

  - `maxima_time`: Timestamp of the maximum glucose value within 2 hours
    after the event start

  - `maxima_glucose`: Maximum glucose value within 2 hours after the
    event start

  - `time_to_peak_min`: Minutes from the event start to `maxima_time`

  - `index`: R-based (1-indexed) row number(s) in `df` denoting where
    the event occurs

  - `maxima_index`: R-based (1-indexed) row number(s) in `df` denoting
    where the maximum occurs

## Notes

\- `gap` is minutes; change to enforce minimum separation between
excursions. - This function operates on the rows supplied in `df`. It
does not use
[`interpolate_cgm`](https://shstat1729.github.io/cgmguru/reference/interpolate_cgm.md)
or the full-day event preprocessing grid.

## References

Edwards, S., et al. (2022). Use of connected pen as a diagnostic tool to
evaluate missed bolus dosing behavior in people with type 1 and type 2
diabetes. Diabetes Technology & Therapeutics, 24(1), 61-66.

## See also

[grid](https://shstat1729.github.io/cgmguru/reference/grid.md)

## Examples

``` r
# Load sample data
library(iglu)
data(example_data_5_subject)
data(example_data_hall)

# Calculate glucose excursions
excursion_result <- excursion(example_data_5_subject, gap = 15)
print(paste("Excursion vector length:", length(excursion_result$excursion_vector)))
#> [1] "Excursion vector length: 1"
print(excursion_result$episode_counts)
#> # A tibble: 5 × 2
#>   id        episode_counts
#>   <chr>              <int>
#> 1 Subject 1              9
#> 2 Subject 2             14
#> 3 Subject 3             11
#> 4 Subject 4             17
#> 5 Subject 5             34

# Excursion analysis with different gap
excursion_30min <- excursion(example_data_5_subject, gap = 30)

# Analysis on larger dataset
large_excursion <- excursion(example_data_hall, gap = 15)
print(paste("Excursion vector length in larger dataset:", length(large_excursion$excursion_vector)))
#> [1] "Excursion vector length in larger dataset: 1"
print(paste("Total episodes:", sum(large_excursion$episode_counts$episode_counts)))
#> [1] "Total episodes: 103"
```
