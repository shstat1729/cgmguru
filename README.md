# 🩺 cgmguru: Advanced Continuous Glucose Monitoring Analysis

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A high-performance R package for comprehensive Continuous Glucose Monitoring (CGM) data analysis with optimized C++ implementations.

## 🎯 Overview

cgmguru provides advanced tools for CGM data analysis with two primary capabilities:

- **GRID and postprandial peak detection**: Implements GRID (Glucose Rate Increase Detector) and GRID-based algorithms to detect postprandial peak glucose events
- **Extended glycemic events (An international consensus on CGM metrics for clinical trials (Battelino et al., Lancet Diabetes Endocrinol 2023; 11: 42–57))**: Detects hypoglycemic and hyperglycemic episodes (Level 1/2) with clinically aligned duration rules

All core algorithms are implemented in optimized C++ via Rcpp for accurate and fast analysis on large datasets.

## ✨ Key Features

- **🚀 High Performance**: C++ backend with multi-ID support and memory-efficient data structures
- **📊 GRID Algorithm**: Detects rapid glucose rate increases (commonly ≥90–95 mg/dL/hour) with configurable thresholds and gaps
- **📈 Postprandial Peak Detection**: Finds peak glucose after GRID points using local maxima and configurable time windows
- **🏥 Consensus CGM metrics [1] Event Detection**: Level 1/2 hypo- and hyperglycemia detection with duration validation (default minimum 15 minutes)
- **🔧 Advanced Analysis Tools**: Local maxima finding, excursion analysis, and robust episode validation utilities
- **📋 Comprehensive Documentation**: Detailed function documentation with examples and parameter descriptions

## 📦 Installation

### From GitHub
```r
# install.packages("remotes")
remotes::install_github("shstat1729/cgmguru")
```

## 📐 Data Format Requirements

Most functions expect a dataframe with the following columns:

- **`id`**: Patient identifier (character or factor)
- **`time`**: POSIXct timestamps
- **`gl`**: Glucose values in mg/dL

All function arguments and return values are expected to be in tibble format. For convenience, single-column parameters can be passed as vectors in R, which will be automatically converted to single-column tibbles.

### Data Preprocessing
```r
library(cgmguru)

# Pre-order your data by id and time (recommended)
df <- orderfast(df)
```



## 🚀 Quick Start

### Basic GRID Detection
```r
library(cgmguru)

# Example data must contain: id, time (POSIXct), glucose
# gap: allowed gap in minutes within which readings are considered contiguous
# threshold: GRID slope threshold in mg/dL/hour

grid_points <- grid(df, gap = 15, threshold = 130)
```

### Postprandial Peak Detection (GRID-based)
```r
# Fast method: Get postprandial peaks directly
# Maps maxima to GRID points using a candidate search window
# Note: Final GRID-to-peak pairing is constrained to at most 4 hours
library(iglu)
data(example_data_5_subject)

maxima <- maxima_grid(example_data_5_subject, 
                     threshold = 130, 
                     gap = 60, 
                     hours = 2)
```

#### Example output for `maxima_grid`
```r
# Show a trimmed view for readability
maxima$episode_counts
```

```text
$results
# A tibble: 88 × 8
   id        grid_time           grid_gl maxima_time         maxima_glucose time_to_peak grid_index maxima_index
   <chr>     <dttm>                <dbl> <dttm>                       <dbl>        <dbl>      <int>        <int>
 1 Subject 1 2015-06-11 20:30:07     143 2015-06-11 21:10:07            276         2400        967          975
 2 Subject 1 2015-06-12 03:00:06     135 2015-06-12 03:50:06            209         3000       1039         1049
 3 Subject 1 2015-06-12 12:40:04     160 2015-06-12 13:20:04            210         2400       1155         1163
 4 Subject 1 2015-06-13 21:34:59     132 2015-06-13 22:34:59            202         3600       1416         1426
 5 Subject 1 2015-06-14 22:39:55     176 2015-06-14 23:24:55            227         2700       1677         1686
 6 Subject 1 2015-06-17 00:14:47     166 2015-06-17 01:19:46            208         3899       2223         2236
 7 Subject 1 2015-06-18 19:29:40     187 2015-06-18 19:49:39            212         1199       2721         2725
 8 Subject 1 2015-06-18 23:19:39     132 2015-06-18 23:54:39            183         2100       2766         2773
 9 Subject 2 2015-02-25 01:06:29     140 2015-02-25 02:31:29            222         5100       2947         2964
10 Subject 2 2015-02-26 00:26:28     173 2015-02-26 02:31:27            273         7499       3227         3252
# ℹ 78 more rows
# ℹ Use `print(n = ...)` to see more rows

$episode_counts
# A tibble: 5 × 2
  id        episode_counts
  <chr>              <int>
1 Subject 1              8
2 Subject 2             18
3 Subject 3              7
4 Subject 4             16
5 Subject 5             39
```

### Step-by-Step Postprandial Peak Detection
```r
# Detailed 7-step process for postprandial peak detection
threshold <- 130
gap <- 60
hours <- 2

# 1) Find GRID points
grid_result <- grid(example_data_5_subject, gap = gap, threshold = threshold)

# 2) Find modified GRID points before 2 hours minimum
mod_grid <- mod_grid(example_data_5_subject, 
                     start_finder(grid_result$grid_vector), 
                     hours = hours, 
                     gap = gap)

# 3) Find maximum point 2 hours after mod_grid point
mod_grid_maxima <- find_max_after_hours(example_data_5_subject, 
                                       start_finder(mod_grid$mod_grid_vector), 
                                       hours = hours)

# 4) Identify local maxima around episodes/windows
local_maxima <- find_local_maxima(example_data_5_subject)

# 5) Among local maxima, find maximum point after two hours
final_maxima <- find_new_maxima(example_data_5_subject, 
                               mod_grid_maxima$max_indices, 
                               local_maxima$local_maxima_vector)

# 6) Map GRID points to maximum points (within 4 hours)
transform_maxima <- transform_df(grid_result$episode_start_total, final_maxima)

# 7) Redistribute overlapping maxima between GRID points
final_between_maxima <- detect_between_maxima(example_data_5_subject, transform_maxima)
```

#### Consensus CGM [1] glycemic events

### 🧩 Eight glycemic event functions (implemented in `detect_all_events.cpp`)
These functions detect hypo-/hyperglycemic episodes aligned with Consensus CGM metrics [1] rules. They differ by type and level. The helper `detect_all_events()` aggregates results across these detectors.

Parameter notes:
- **`start_gl`**: threshold to start/qualify an episode (mg/dL). For hyper: typical `181` (lv1) or `251` (lv2). For hypo: typical `70` (lv1) or `54` (lv2).
- **`end_gl`**: glucose level indicating episode resolution (e.g., 180 mg/dL for hyper Level 1).
- **`dur_length`**: minimum episode duration in minutes (default often 15 minutes for Level 1; may be longer for extended definitions).
- **`end_length`**: grace period for termination/contiguity in minutes.
- **`reading_minutes`** (where applicable): CGM device sampling interval in minutes (e.g., 5 min for Dexcom, 15 min for Libre). Used to calculate minimum required readings for event validation. Can be a single integer/numeric value (applied to all subjects) or a vector matching data length (different intervals per subject). The algorithm uses this to determine if an event has sufficient data points to be considered valid based on the 3/4 rule: `ceil((dur_length / reading_minutes) / 4 * 3)`.

To get a combined table across all event types, use:
```r
# reading_minutes can be integer or numeric vector.
all_events <- detect_all_events(df, reading_minutes = 5)
```

| Event Type | Description | Function Call | Parameters |
|------------|-------------|---------------|------------|
| **Level 1 Hypoglycemia** | ≥15 consecutive min of <70 mg/dL, ends with ≥15 consecutive min ≥70 mg/dL | `detect_hypoglycemic_events(df, start_gl = 70, dur_length = 15, end_length = 15)` | `start_gl = 70, dur_length = 15, end_length = 15` |
| **Level 2 Hypoglycemia** | ≥15 consecutive min of <54 mg/dL, ends with ≥15 consecutive min ≥54 mg/dL | `detect_hypoglycemic_events(df, start_gl = 54, dur_length = 15, end_length = 15)` | `start_gl = 54, dur_length = 15, end_length = 15` |
| **Extended Hypoglycemia** | >120 consecutive min of <70 mg/dL, ends with ≥15 consecutive min ≥70 mg/dL | `detect_hypoglycemic_events(df)` | Default parameters |
| **Level 1 Hypoglycemia (Excluded)** | 54–69 mg/dL (3·0–3·9 mmol/L) ≥15 consecutive min, ends with ≥15 consecutive min ≥70 mg/dL | `detect_all_events(df)` | Default parameters |
| **Level 1 Hyperglycemia** | ≥15 consecutive min of >180 mg/dL, ends with ≥15 consecutive min ≤180 mg/dL | `detect_hyperglycemic_events(df, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)` | `start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180` |
| **Level 2 Hyperglycemia** | ≥15 consecutive min of >250 mg/dL, ends with ≥15 consecutive min ≤250 mg/dL | `detect_hyperglycemic_events(df, start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250)` | `start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250` |
| **Extended Hyperglycemia** | >250 mg/dL lasting ≥120 min, ends when glucose returns to ≤180 mg/dL for ≥15 min | `detect_hyperglycemic_events(df)` | Default parameters |
| **Level 1 Hyperglycemia (Excluded)** | 181–250 mg/dL (10·1–13·9 mmol/L) ≥15 consecutive min, ends with ≥15 consecutive min ≤180 mg/dL | `detect_all_events(df)` | Default parameters |

```r
# Equivalent examples using camelCase naming for readability
# Level 1 Hypoglycemia Event (≥15 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
detect_hypoglycemic_events(example_data_5_subject, start_gl = 70, dur_length = 15, end_length = 15)  # hypo, level = lv1

# Level 2 Hypoglycemia Event (≥15 consecutive min of <54 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥54 mg/dL)
detect_hypoglycemic_events(example_data_5_subject, start_gl = 54, dur_length = 15, end_length = 15)  # hypo, level = lv2

# Extended Hypoglycemia Event (>120 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
detect_hypoglycemic_events(example_data_5_subject)                                                    # hypo, extended

# Hypoglycaemia alert value (54–69 mg/dL (3·0–3·9 mmol/L) ≥15 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL
# It is implemented in detect_all_events function.                                                     # hypo, lv1_excl

# Level 1 Hyperglycemia Event (≥15 consecutive min of >180 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≤180 mg/dL)
detect_hyperglycemic_events(example_data_5_subject, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)  # hyper, lv1

# Level 2 Hyperglycemia Event (≥15 consecutive min of >250 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≤250 mg/dL)
detect_hyperglycemic_events(example_data_5_subject, start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250)  # hyper, lv2

# Extended Hyperglycemia Event (Number of events with sensor glucose >250 mg/dL (>13·9 mmol/L) lasting at least 120 min; event ends when glucose returns to ≤180 mg/dL (≤10·0 mmol/L) for ≥15 min)
detect_hyperglycemic_events(example_data_5_subject)                                                                 # hyper, extended

# High glucose (Level 1, excluded) (181–250 mg/dL (10·1–13·9 mmol/L) ≥15 consecutive min and Event ends when there is ≥15 consecutive min with a CGM sensor value of ≤180 mg/dL) 
# It is implemented in detect_all_events function.                                                     # hyper, lv1_excl
```

#### Example output for `detect_hypoglycemic_events` (Level 1)
```r
res_hypo_lv1 <- detect_hypoglycemic_events(example_data_5_subject, start_gl = 70, dur_length = 15, end_length = 15)
print(res_hypo_lv1$events_total)
print(res_hypo_lv1$events_detailed)
```

```text
$events_total
# A tibble: 5 × 5
  id        total_events avg_ep_per_day avg_ep_duration avg_ep_gl
  <chr>            <int>          <dbl>           <dbl>     <dbl>
1 Subject 1            1           0.08              20      67  
2 Subject 2            0           0                  0       0  
3 Subject 3            1           0.17              25      62.8
4 Subject 4            2           0.16              30      65.4
5 Subject 5            1           0.09              15      66.7

$events_detailed
# A tibble: 5 × 10
  id       start_time start_glucose end_time end_glucose start_indices end_indices duration_minutes duration_below_54_mi…¹
  <chr>    <dttm>             <dbl> <dttm>         <dbl>         <int>       <int>            <dbl>                  <dbl>
1 Subject… 2015-06-0…            69 2015-06…          66           360         363             20.0                      0
2 Subject… 2015-03-1…            67 2015-03…          63          5999        6003             25                        0
3 Subject… 2015-03-1…            60 2015-03…          54          7280        7287             40.0                     10
4 Subject… 2015-03-2…            69 2015-03…          69         10093       10096             20                        0
5 Subject… 2015-03-0…            67 2015-03…          67         13218       13220             15                        0
# ℹ abbreviated name: ¹​duration_below_54_minutes
# ℹ 1 more variable: average_glucose <dbl>
```

#### Example output for `detect_hyperglycemic_events`
```r
res_hyper <- detect_hyperglycemic_events(example_data_5_subject)
print(res_hyper$events_total)
print(res_hyper$events_detailed)
```

```text
$events_total
# A tibble: 5 × 5
  id        total_events avg_ep_per_day avg_ep_duration avg_ep_gl
  <chr>            <int>          <dbl>           <dbl>     <dbl>
1 Subject 1            0           0                 0         0 
2 Subject 2            7           0.42            317.      291.
3 Subject 3            1           0.17            185       276.
4 Subject 4            0           0                 0         0 
5 Subject 5            4           0.38            166.      302.

$events_detailed
# A tibble: 12 × 9
   id        start_time      start_glucose end_time end_glucose start_indices end_indices duration_minutes average_glucose
   <chr>     <dttm>                  <dbl> <dttm>         <dbl>         <int>       <int>            <dbl>           <dbl>
 1 Subject 2 2015-02-27 20:…           254 2015-02…         254          3757        3786             150             274.
 2 Subject 2 2015-03-01 04:…           254 2015-03…         275          4136        4184             245             296.
 3 Subject 2 2015-03-01 23:…           252 2015-03…         251          4372        4448             385             279.
 4 Subject 2 2015-03-03 02:…           254 2015-03…         256          4661        4726             330.            290.
 5 Subject 2 2015-03-10 23:…           333 2015-03…         252          5004        5110             625.            341.
 6 Subject 2 2015-03-11 23:…           259 2015-03…         252          5269        5315             235             293.
 7 Subject 2 2015-03-12 12:…           265 2015-03…         252          5425        5474             250.            265.
 8 Subject 3 2015-03-11 02:…           251 2015-03…         251          5811        5847             185             276.
 9 Subject 5 2015-03-02 14:…           261 2015-03…         251         11412       11442             155             304.
10 Subject 5 2015-03-03 00:…           259 2015-03…         269         11530       11566             185             346.
11 Subject 5 2015-03-03 21:…           254 2015-03…         251         11737       11761             125             281.
12 Subject 5 2015-03-04 04:…           258 2015-03…         255         11825       11864             200.            277.
```

#### Example output for `detect_all_events`
```r
all_events <- detect_all_events(example_data_5_subject)
all_events
```

```text
# A tibble: 40 × 7
   id        type  level    avg_ep_per_day avg_ep_duration avg_ep_gl total_episodes
   <chr>     <chr> <chr>             <dbl>           <dbl>     <dbl>          <int>
 1 Subject 1 hypo  lv1                0.08            15        67.3              1
 2 Subject 1 hypo  lv2                0                0         0                0
 3 Subject 1 hypo  extended           0                0         0                0
 4 Subject 1 hypo  lv1_excl           0.08            15        67.3              1
 5 Subject 1 hyper lv1                1.1             74.6     201.              14
 6 Subject 1 hyper lv2                0.16            22.5     266.               2
 7 Subject 1 hyper extended           0                0         0                0
 8 Subject 1 hyper lv1_excl           0.95            83.3     190.              12
 9 Subject 2 hypo  lv1                0                0         0                0
10 Subject 2 hypo  lv2                0                0         0                0
# ℹ 30 more rows
# ℹ Use `print(n = ...)` to see more rows
```



### 🔧 Advanced analysis helpers
- **start_finder** : find R-based index of 1 (1-indexed) from 0 and 1 vector.



## 📚 Function Reference

### Core Analysis Functions
- `grid()` - GRID algorithm for glycemic event detection
- `maxima_grid()` - Combined maxima detection and GRID analysis
- `detect_hyperglycemic_events()` - Hyperglycemic event detection
- `detect_hypoglycemic_events()` - Hypoglycemic event detection
- `detect_all_events()` - Comprehensive event detection

### Utility Functions
- `find_local_maxima()` - Local maxima identification
- `find_max_after_hours()` / `find_max_before_hours()` - Maximum glucose search
- `find_min_after_hours()` / `find_min_before_hours()` - Minimum glucose search
- `find_new_maxima()` - New maxima detection around grid points
- `mod_grid()` - Modified GRID analysis
- `detect_between_maxima()` - Event detection between maxima
- `excursion()` - Glucose excursion calculation
- `start_finder()` - Start point identification
- `transform_df()` - Data transformation utilities
- `orderfast()` - Fast dataframe ordering

## ⚡ Performance

```r
library(microbenchmark)

# Perform microbenchmark comparison
# detection of glycemic events 
benchmark_results <- microbenchmark(
  episode_calculation = episode_calculation(example_data_5_subject), # iglu package 
  detect_all_events = detect_all_events(example_data_5_subject), # cgmguru package
  times = 100,
  unit = "ms"  # You can change to "s", "us", "ns" as needed
)

# Print summary statistics
benchmark_results
```

```text
Unit: milliseconds
expr                 min        lq       mean     median        uq       max neval cld
episode_calculation 392.456633 399.879949 410.764202 403.986756 412.516211 493.902236   100  a 
detect_all_events     1.266531   1.290475   1.325581   1.326678   1.344554   1.491949   100   b
```

Tested on: Mac OS, Apple M2 Pro (10-core CPU), 16 GB RAM.

## 🤝 Contributing

We welcome contributions! Please feel free to submit issues, feature requests, or pull requests.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE.md) file for details.

## 👨‍💻 Author

**Sang Ho Park, M.S.** - [shstat1729@gmail.com](mailto:shstat1729@gmail.com)
  - He writes code for the package. 

**Sang-Man Jin, MD, PhD** - [sjin772@gmail.com](mailto:sjin772@gmail.com)
  - He consults GRID-based algorithm (post-prandial peak glucose) and CGM consensus.

**Rosa Oh, MD** - [wlscodl123@naver.com](mailto:wlscodl123@naver.com)
  - She consults GRID-based algorithm (post-prandial peak glucose) and CGM consensus.

## 📖 Citation

If you use cgmguru in your research, please cite:

```bibtex
@software{cgmguru,
  title = {cgmguru: Advanced Continuous Glucose Monitoring Analysis},
  author = {Sang Ho Park},
  year = {2025},
  url = {https://github.com/shstat1729/cgmguru}
}
```

## 🔗 Links

- [GitHub Repository](https://github.com/shstat1729/cgmguru)
- [Issue Tracker](https://github.com/shstat1729/cgmguru/issues)
- [Documentation](https://shstat1729.github.io/cgmguru/)

## References

[1] Battelino, T., et al. "Continuous glucose monitoring and metrics for clinical trials: an international consensus statement." *The Lancet Diabetes & Endocrinology* 11.1 (2023): 42-57.

[2] Chun, E., et al. "iglu: interpreting glucose data from continuous glucose monitors." R package version 3.0 (2023).

[3] Harvey, Rebecca A., et al. "Design of the glucose rate increase detector: a meal detection module for the health monitoring system." Journal of diabetes science and technology 8.2 (2014): 307-320.

---

**Note**: This package is designed for research and clinical analysis of CGM data. Always consult with healthcare professionals for clinical decision-making.


