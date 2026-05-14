# 🩺 cgmguru: Advanced Continuous Glucose Monitoring Analysis

[![R-CMD-check](https://github.com/shstat1729/cgmguru/workflows/R-CMD-check/badge.svg)](https://github.com/shstat1729/cgmguru/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Rcpp](https://img.shields.io/badge/Rcpp-enabled-blue.svg)](https://github.com/RcppCore/Rcpp)

**Advanced Continuous Glucose Monitoring Analysis with High-Performance C++ Backend**

A comprehensive R package for CGM data analysis implementing GRID algorithms and international consensus glycemic event detection with optimized C++ implementations via Rcpp.

## 🎯 Overview

cgmguru provides advanced tools for CGM data analysis with two primary capabilities:

- **GRID and postprandial peak detection**: Implements GRID (Glucose Rate Increase Detector) and GRID-based algorithms to detect postprandial peak glucose events
- **Glycemic events**: Detects hypoglycemic and hyperglycemic episodes (Level 1/2/Extended) aligned with international consensus CGM metrics [1]

All core algorithms are implemented in optimized C++ via Rcpp for accurate and fast analysis on large datasets.

## Relationship to iglu

cgmguru uses `iglu` as a formal methodological reference, source of example datasets, and comparison target. The event-detection preprocessing in cgmguru is an independent C++/Rcpp implementation of iglu-compatible semantics: an id-specific, midnight-aligned full-day grid; linear interpolation up to `inter_gap`; masking of larger gaps; removal of masked rows before event classification; and segment-wise event calculation.

cgmguru does not call `iglu` at runtime for its core algorithms. Because `iglu` is distributed under GPL-2, cgmguru documentation and tests should cite `iglu` clearly while avoiding copied GPL source code or copied explanatory prose. See the citation section for formal iglu references [3, 8].

## ✨ Key Features

- **🚀 High Performance**: C++ backend with efficient multi-subject processing and memory-optimized data structures
- **📊 GRID Algorithm**: Detects rapid glucose rate increases (commonly ≥90–95 mg/dL/hour) with configurable thresholds and gaps [2, 6]
- **📈 Postprandial Peak Detection**: Finds peak glucose after GRID points using local maxima and configurable time windows [4, 7]
- **🏥 Consensus CGM Metrics Event Detection**: Level 1/2, Level 1 excluded, and extended hypo- and hyperglycemia detection with duration validation (default minimum 15 minutes)
- **🔧 Advanced Analysis Tools**: Local maxima finding, excursion analysis, and robust episode validation utilities
- **📋 Comprehensive Documentation**: Detailed function documentation with examples and parameter descriptions

## 📦 Installation

### From CRAN
```r
install.packages("cgmguru")
```

### From GitHub (development version)
```r
# install.packages("remotes")
remotes::install_github("shstat1729/cgmguru")
```

### Dependencies
```r
# Required packages
install.packages("Rcpp")

# For examples, vignettes, tests, and comparisons
install.packages(c("iglu", "dplyr", "testthat", "microbenchmark"))
```

## 🚀 Quick Start

### Load Package and Data
```r
library(cgmguru)
library(iglu)
data(example_data_5_subject)
```

### Basic GRID Analysis
```r
# Detect rapid glucose rate increases
grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
print(grid_result$episode_counts)
```

### Comprehensive Event Detection
```r
# Detect all glycemic events (Level 1/2 hypo- and hyperglycemia)
# reading_minutes is calculated automatically from the data when omitted
all_events <- detect_all_events(example_data_5_subject)
print(all_events$summary_df)
print(all_events$events_long_df)
```

### Postprandial Peak Detection
```r
# Combined maxima and GRID analysis
maxima_result <- maxima_grid(example_data_5_subject, threshold = 130, gap = 60, hours = 2)
print(maxima_result$episode_counts)
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

### Event Preprocessing Scope

The full-day interpolation grid is used only by glycemic event functions: `detect_hypoglycemic_events()`, `detect_hyperglycemic_events()`, `detect_all_events()`, and the standalone helper `interpolate_cgm()`. If `reading_minutes` is omitted or `NULL`, these functions calculate it automatically per subject from the median positive timestamp spacing in the data. They then adjust the interval to an iglu-compatible day grid when needed, interpolate across gaps up to `inter_gap`, remove larger gap-masked rows, and split event classification by contiguous segments.

`detect_hypoglycemic_events()` and `detect_hyperglycemic_events()` can return the event grid as `interpolated_data`. `detect_all_events()` uses the grid internally for event counts and CGM summary metrics, but it returns only `events_long_df` and `summary_df`.

GRID-based functions use the rows supplied by the user. `grid()`, `maxima_grid()`, and `excursion()` do not automatically call `interpolate_cgm()` and do not use the full-day event grid unless you explicitly pass interpolated data to them.

## ⚡ Rcpp Integration

cgmguru leverages Rcpp for high-performance C++ implementations:

- **Core algorithms** implemented in C++ for speed
- **Memory-efficient** data structures for large datasets
- **Efficient multi-subject processing** with optimized data handling
- **Seamless R integration** with automatic type conversion

### Performance Benefits
```r
# Benchmark comparison
library(microbenchmark)
library(iglu)

benchmark_results <- microbenchmark(
  iglu_episodes = iglu::episode_calculation(example_data_5_subject),
  cgmguru_events = detect_all_events(example_data_5_subject),
  times = 100,
  unit = "ms"
)

print(benchmark_results)
# cgmguru event detection is substantially faster in this benchmark.
```

## 🔬 Core Functionality

### GRID Algorithm
```r
# Basic GRID analysis
grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
print(grid_result$episode_counts)

# More sensitive analysis
sensitive_result <- grid(example_data_5_subject, gap = 10, threshold = 120)
```

### Postprandial Peak Detection
```r
# Fast method: Get postprandial peaks directly
maxima <- maxima_grid(example_data_5_subject,
                     threshold = 130,
                     gap = 60,
                     hours = 2)
print(maxima$episode_counts)
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
                               mod_grid_maxima$max_index,
                               local_maxima$local_maxima_vector)

# 6) Map GRID points to maximum points (within 4 hours)
transform_maxima <- transform_df(grid_result$episode_start, final_maxima)

# 7) Redistribute overlapping maxima between GRID points
final_between_maxima <- detect_between_maxima(example_data_5_subject, transform_maxima)
```

## 🏥 Consensus CGM Glycemic Events

These functions detect hypo-/hyperglycemic episodes aligned with Consensus CGM metrics rules. They differ by type and level. The helper `detect_all_events()` aggregates results across these detectors.
Events are counted only after the required recovery condition is confirmed. In detailed outputs, `end_time`, `end_glucose`, and `end_index` report the last dysglycemic reading immediately before the confirmed recovery period starts, so recovery minutes are not included in reported event boundaries.

### Parameter Notes
- **`start_gl`**: threshold to start/qualify an episode (mg/dL). For hyper: typical `180` (lv1) or `250` (lv2). For hypo: typical `70` (lv1) or `54` (lv2).
- **`end_gl`**: glucose level indicating episode resolution (e.g., 180 mg/dL for hyper Level 1).
- **`dur_length`**: minimum episode duration in minutes (default often 15 minutes for Level 1; may be longer for extended definitions).
- **`end_length`**: grace period for termination/contiguity in minutes.
- **`reading_minutes`**: CGM device sampling interval in minutes (e.g., 5 min for Dexcom, 15 min for Libre). If omitted or `NULL`, cgmguru calculates it automatically per subject from the median positive timestamp spacing in the data. Used to place data on the event grid and convert duration criteria to whole grid-reading counts. Extended hyperglycemia uses the consensus-style 90-minute requirement within a 120-minute window.

### `detect_all_events()` Summary Metrics

`detect_all_events()` returns a list with `events_long_df` and `summary_df`. The `summary_df` contains one row per subject. Its CGM summary metrics are calculated on the internal interpolated event grid after gap masking and removal; `detect_all_events()` uses this `interpolated_data`-style grid internally but does not return it. The `sensor_wear` column is the exception: it is calculated from the original observed timestamps and glucose readings.

`summary_df` columns:

| Column | Meaning |
|--------|---------|
| `id` | Subject identifier |
| `TIR` | Percent time in range 70-180 mg/dL |
| `TITR` | Percent time in tight range 70-140 mg/dL |
| `TBR70` | Percent time below 70 mg/dL |
| `TBR54` | Percent time below 54 mg/dL |
| `TAR180` | Percent time above 180 mg/dL |
| `TAR250` | Percent time above 250 mg/dL |
| `CV` | Glucose coefficient of variation, calculated as `SD / mean_glucose` |
| `SD` | Sample standard deviation of glucose |
| `mean_glucose` | Mean glucose in mg/dL |
| `GMI` | Glucose Management Indicator, `3.31 + 0.02392 * mean_glucose` |
| `uGMI` | Unitless GMI-style metric, `1 / (15.36 / mean_glucose + 0.0425)` |
| `GRI` | Glycemia Risk Index: `3.0 * VLow + 2.4 * Low + 1.6 * VHigh + 0.8 * High` |
| `sensor_wear` | Percent of expected readings observed over the original timestamp span |
| `hypo_lv1_event_count` | Level 1 hypoglycemia event count |
| `hypo_lv2_event_count` | Level 2 hypoglycemia event count |
| `hypo_extended_event_count` | Extended hypoglycemia event count |
| `hypo_lv1_excl_event_count` | Level 1 excluded hypoglycemia event count, excluding Level 2-overlapping episodes |
| `hyper_lv1_event_count` | Level 1 hyperglycemia event count |
| `hyper_lv2_event_count` | Level 2 hyperglycemia event count |
| `hyper_extended_event_count` | Extended hyperglycemia event count |
| `hyper_lv1_excl_event_count` | Level 1 excluded hyperglycemia event count, excluding Level 2-overlapping episodes |

### `detect_all_events()` Long Event Table

`events_long_df` is the long-format event summary returned by `detect_all_events()`. It has one row per subject, event `type`, and event `level`. The `type` column is either `hypo` or `hyper`; the `level` column is one of `lv1`, `lv2`, `extended`, or `lv1_excl`. The `lv1_excl` level is the Level 1 excluded category (`lv1_excluded` in descriptive text): Level 1 episodes that do not overlap a Level 2 episode. Use `type = "lv1_excl"` in function calls.

`events_long_df` columns:

| Column | Meaning |
|--------|---------|
| `id` | Subject identifier |
| `type` | Event direction: `hypo` or `hyper` |
| `level` | Event level: `lv1`, `lv2`, `extended`, or `lv1_excl` |
| `event_count` | Number of events for that subject/type/level |
| `avg_ep_per_day` | Average episodes per day for that subject/type/level |
| `avg_episode_duration_below_54` | Average minutes below 54 mg/dL for hypoglycemia rows; zero for event levels where this does not apply |

### Hypoglycemia Usage Methods
`detect_hypoglycemic_events()` supports both the recommended `type` preset method and custom criteria supplied with `start_gl`, `dur_length`, and `end_length`.

**Preset method (recommended):**
```r
detect_hypoglycemic_events(df, type = "lv1")       # Level 1 hypoglycemia
detect_hypoglycemic_events(df, type = "lv2")       # Level 2 hypoglycemia
detect_hypoglycemic_events(df, type = "extended")  # Extended hypoglycemia
detect_hypoglycemic_events(df, type = "lv1_excl")  # Level 1 excluded hypoglycemia
```

**Custom criteria method:**
```r
detect_hypoglycemic_events(df, start_gl = 70, dur_length = 15, end_length = 15)  # Level 1
detect_hypoglycemic_events(df, start_gl = 54, dur_length = 15, end_length = 15)  # Level 2
detect_hypoglycemic_events(df, start_gl = 70, dur_length = 120, end_length = 15) # Extended
```

If an explicit `type` is supplied together with custom numeric criteria, the function returns results based on `type` and gives a warning that `dur_length`, `end_length`, and `start_gl` were ignored.

### Hyperglycemia Usage Methods
`detect_hyperglycemic_events()` supports both the recommended `type` preset method and custom criteria supplied with `start_gl`, `dur_length`, `end_length`, and `end_gl`.

**Preset method (recommended):**
```r
detect_hyperglycemic_events(df, type = "lv1")       # Level 1 hyperglycemia
detect_hyperglycemic_events(df, type = "lv2")       # Level 2 hyperglycemia
detect_hyperglycemic_events(df, type = "extended")  # Extended hyperglycemia
detect_hyperglycemic_events(df, type = "lv1_excl")  # Level 1 excluded hyperglycemia
```

**Custom criteria method:**
```r
detect_hyperglycemic_events(df, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)  # Level 1
detect_hyperglycemic_events(df, start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250)  # Level 2
detect_hyperglycemic_events(df, start_gl = 250, dur_length = 120, end_length = 15, end_gl = 180) # Extended
```

If an explicit `type` is supplied together with custom numeric criteria, the function returns results based on `type` and gives a warning that `dur_length`, `end_length`, `start_gl`, and `end_gl` were ignored.

### Event Detection Table
| Event Type | Description | Recommended Call | Custom Criteria Call / Parameters |
|------------|-------------|------------------|-------------------------------------|
| **Level 1 Hypoglycemia** | ≥15 consecutive min of <70 mg/dL, ends with ≥15 consecutive min ≥70 mg/dL | `detect_hypoglycemic_events(df, type = "lv1")` | `detect_hypoglycemic_events(df, start_gl = 70, dur_length = 15, end_length = 15)` |
| **Level 2 Hypoglycemia** | ≥15 consecutive min of <54 mg/dL, ends with ≥15 consecutive min ≥54 mg/dL | `detect_hypoglycemic_events(df, type = "lv2")` | `detect_hypoglycemic_events(df, start_gl = 54, dur_length = 15, end_length = 15)` |
| **Extended Hypoglycemia** | >120 consecutive min of <70 mg/dL, ends with ≥15 consecutive min ≥70 mg/dL | `detect_hypoglycemic_events(df, type = "extended")` | `detect_hypoglycemic_events(df, start_gl = 70, dur_length = 120, end_length = 15)` |
| **Level 1 Hypoglycemia (Excluded)** | Level 1 hypoglycemia episodes that do not overlap Level 2 hypoglycemia episodes | `detect_hypoglycemic_events(df, type = "lv1_excl")` | Use `type = "lv1_excl"`; this exclusion is overlap-based rather than a standalone custom threshold |
| **Level 1 Hyperglycemia** | ≥15 consecutive min of >180 mg/dL, ends with ≥15 consecutive min ≤180 mg/dL | `detect_hyperglycemic_events(df, type = "lv1")` | `detect_hyperglycemic_events(df, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)` |
| **Level 2 Hyperglycemia** | ≥15 consecutive min of >250 mg/dL, ends with ≥15 consecutive min ≤250 mg/dL | `detect_hyperglycemic_events(df, type = "lv2")` | `detect_hyperglycemic_events(df, start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250)` |
| **Extended Hyperglycemia** | >250 mg/dL lasting ≥90 cumulative min within a 120-min period, ends when glucose returns to ≤180 mg/dL for ≥15 consecutive min after | `detect_hyperglycemic_events(df, type = "extended")` | `detect_hyperglycemic_events(df, start_gl = 250, dur_length = 120, end_length = 15, end_gl = 180)` |
| **Level 1 Hyperglycemia (Excluded)** | Level 1 hyperglycemia episodes that do not overlap Level 2 hyperglycemia episodes | `detect_hyperglycemic_events(df, type = "lv1_excl")` | Use `type = "lv1_excl"`; this exclusion is overlap-based rather than a standalone custom threshold |

### Example Usage
```r
# Level 1 Hypoglycemia Event (≥15 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
detect_hypoglycemic_events(example_data_5_subject, type = "lv1")  # hypo, level = lv1

# Level 2 Hypoglycemia Event (≥15 consecutive min of <54 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥54 mg/dL)
detect_hypoglycemic_events(example_data_5_subject, type = "lv2")  # hypo, level = lv2

# Extended Hypoglycemia Event (>120 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
detect_hypoglycemic_events(example_data_5_subject, type = "extended")  # hypo, extended

# Level 1 excluded Hypoglycemia Event (Level 1 episodes that do not overlap Level 2 episodes)
detect_hypoglycemic_events(example_data_5_subject, type = "lv1_excl")  # hypo, level = lv1_excl

# Custom hypoglycemia criteria are also supported
detect_hypoglycemic_events(example_data_5_subject, start_gl = 70, dur_length = 15, end_length = 15)  # hypo, level = lv1
detect_hypoglycemic_events(example_data_5_subject, start_gl = 54, dur_length = 15, end_length = 15)  # hypo, level = lv2
detect_hypoglycemic_events(example_data_5_subject, start_gl = 70, dur_length = 120, end_length = 15) # hypo, extended

# Level 1 Hyperglycemia Event (≥15 consecutive min of >180 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≤180 mg/dL)
detect_hyperglycemic_events(example_data_5_subject, type = "lv1")

# Level 2 Hyperglycemia Event (≥15 consecutive min of >250 mg/dL and event ends when there is ≥15 consecutive
min with a CGM sensor value of ≤250 mg/dL)
detect_hyperglycemic_events(example_data_5_subject, type = "lv2")

# Extended Hyperglycemia Event (>250 mg/dL lasting ≥90 cumulative min within a 120-min period, ends when glucose returns to ≤180 mg/dL for ≥15 consecutive min after)
detect_hyperglycemic_events(example_data_5_subject, type = "extended")

# Level 1 excluded Hyperglycemia Event (Level 1 episodes that do not overlap Level 2 episodes)
detect_hyperglycemic_events(example_data_5_subject, type = "lv1_excl")

# Custom hyperglycemia criteria are also supported
detect_hyperglycemic_events(example_data_5_subject, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)  # hyper, level = lv1
detect_hyperglycemic_events(example_data_5_subject, start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250)  # hyper, level = lv2
detect_hyperglycemic_events(example_data_5_subject, start_gl = 250, dur_length = 120, end_length = 15, end_gl = 180) # hyper, extended

# Comprehensive event detection
all_events <- detect_all_events(example_data_5_subject)
print(all_events$summary_df)
print(all_events$events_long_df)
```

## 📚 Function Reference

### Core Analysis Functions
| Function | Description | C++ Implementation |
|----------|-------------|-------------------|
| `grid()` | GRID algorithm for rapid glucose increase detection | ✅ |
| `maxima_grid()` | Combined maxima detection and GRID analysis | ✅ |
| `detect_hyperglycemic_events()` | Hyperglycemic event detection (Level 1/2/Extended/Level 1 excluded) | ✅ |
| `detect_hypoglycemic_events()` | Hypoglycemic event detection (Level 1/2/Extended/Level 1 excluded) | ✅ |
| `detect_all_events()` | Comprehensive event detection across all types | ✅ |

### Advanced Analysis Functions
| Function | Description | C++ Implementation |
|----------|-------------|-------------------|
| `find_local_maxima()` | Local maxima identification in glucose time series | ✅ |
| `excursion()` | Glucose excursion calculation [5] | ✅ |
| `mod_grid()` | Modified GRID analysis with custom parameters | ✅ |
| `detect_between_maxima()` | Event detection between maxima | ✅ |
| `find_new_maxima()` | New maxima detection around grid points | ✅ |

#### Excursion definition

An excursion is defined as a >70 mg/dL (>3.9 mmol/L) rise within 2 hours, not preceded by a value <70 mg/dL (<3.9 mmol/L) [5].

### Time-Based Analysis Functions
| Function | Description | C++ Implementation |
|----------|-------------|-------------------|
| `find_max_after_hours()` | Find maximum glucose after specified hours | ✅ |
| `find_max_before_hours()` | Find maximum glucose before specified hours | ✅ |
| `find_min_after_hours()` | Find minimum glucose after specified hours | ✅ |
| `find_min_before_hours()` | Find minimum glucose before specified hours | ✅ |

### Utility Functions
| Function | Description | C++ Implementation |
|----------|-------------|-------------------|
| `orderfast()` | Fast dataframe ordering by id and time | ✅ |
| `start_finder()` | Find episode start index from binary vectors | ✅ |
| `transform_df()` | Data transformation for downstream analysis | ✅ |

## 📊 Performance

```r
library(microbenchmark)
library(iglu)
data(example_data_hall)
# Perform microbenchmark comparison
benchmark_results <- microbenchmark(
  episode_calculation = iglu::episode_calculation(example_data_hall),
  detect_all_events = cgmguru::detect_all_events(example_data_hall),
  times = 100,
  unit = "ms"
)

print(benchmark_results)
```

**Results:**
```
Unit: milliseconds
                expr        min         lq      mean     median        uq       max neval cld
 episode_calculation 842.527081 857.136099 871.03685 864.288712 882.31248 917.91845   100  a 
   detect_all_events   8.219721   8.606904   8.85185   8.769675   8.91711  13.05834   100   b
```

**Performance:** In this benchmark, cgmguru is substantially faster than `iglu::episode_calculation()`.

*Tested on: Mac OS, Apple M4 Max (16-core CPU), 64 GB RAM.*

## 📖 Vignettes

For comprehensive examples and detailed workflows, see the package vignettes:

```r
# View vignettes
vignette("intro", package = "cgmguru")
```

## 🔧 Development

### Building from Source
```bash
# Clone repository
git clone https://github.com/shstat1729/cgmguru.git
cd cgmguru

# Install dependencies
R -e "install.packages(c('Rcpp', 'testthat', 'knitr', 'rmarkdown'))"

# Build package
R CMD build .
R CMD INSTALL cgmguru_*.tar.gz
```

### Testing
```r
# Run tests
devtools::test()

# Build vignettes
devtools::build_vignettes()

# Check package
devtools::check()
```

## 🚀 Future Plans

We have development plans for cgmguru:

### 📦 Distribution
- **CRAN Submission**: Preparing for CRAN distribution to make cgmguru easily accessible via `install.packages("cgmguru")`

### ⚡ Performance Enhancements
- **Parallel Computing**: Implementation of multi-threaded processing using OpenMP for even faster analysis of large datasets

### 🐍 Cross-Platform Support
- **Python Implementation**: Python package with identical functionality

### 📝 Academic Publication
- **Package Documentation Paper**: Comprehensive paper describing cgmguru's algorithms, performance benchmarks, and clinical applications for broader scientific community awareness

### 📚 Documentation & Examples
- **Extended Vignettes**: Additional comprehensive tutorials covering advanced use cases, clinical interpretation guidelines, and integration with other CGM analysis tools
- **Clinical Case Studies**: Detailed examples using real-world CGM datasets with step-by-step analysis workflows and interpretation guidelines

*Timeline: These features will be rolled out incrementally over the next 3 months. Follow our [GitHub repository](https://github.com/shstat1729/cgmguru) for updates and release announcements.*

## 🤝 Contributing

We welcome contributions! Please feel free to submit issues, feature requests, or pull requests.

### Development Guidelines
- Follow R package conventions
- Add tests for new functions
- Update documentation
- Ensure C++ code is optimized

### Areas for Contribution
- **Algorithm Improvements**: Enhance existing algorithms or propose new ones
- **Documentation**: Improve examples, tutorials, and documentation
- **Testing**: Add comprehensive test cases and edge case handling
- **Performance**: Optimize C++ implementations and memory usage
- **Features**: Implement planned features from the roadmap above

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE.md) file for details.

iglu is distributed under GPL-2. cgmguru treats iglu as a cited methodological reference and comparison target; cgmguru should not copy GPL-licensed iglu implementation code or prose into the MIT-licensed package.

## 👨‍💻 Authors

**Sang Ho Park, M.S.** - [shstat1729@gmail.com](mailto:shstat1729@gmail.com)
- Package development and C++ implementation

**Sang-Man Jin, MD, PhD** - [sjin772@gmail.com](mailto:sjin772@gmail.com)
- GRID-based algorithm consultation and CGM consensus

**Rosa Oh, MD** - [wlscodl123@naver.com](mailto:wlscodl123@naver.com)
- GRID-based algorithm consultation and CGM consensus

## 📖 Citation

If you use cgmguru in your research, please cite:

```bibtex
@software{cgmguru,
  title = {cgmguru: Advanced Continuous Glucose Monitoring Analysis with High-Performance C++ Backend},
  author = {Sang Ho Park, Rosa Oh, Sang-Man Jin},
  year = {2026},
  url = {https://github.com/shstat1729/cgmguru},
  note = {R package version 1.0.0}
}
```

### Related Publications
[1] Battelino, T., et al. (2023). Continuous glucose monitoring and metrics for clinical trials: an international consensus statement. *The Lancet Diabetes & Endocrinology*, 11(1), 42-57.

[2] Harvey, R. A., et al. (2014). Design of the glucose rate increase detector: a meal detection module for the health monitoring system. *Journal of Diabetes Science and Technology*, 8(2), 307-320.

[3] Broll, S., Urbanek, J., Buchanan, D., Chun, E., Muschelli, J., Punjabi, N., & Gaynanova, I. (2021). Interpreting blood glucose data with R package iglu. *PLoS One*, 16(4), e0248560. https://doi.org/10.1371/journal.pone.0248560

[4] Park, Sang Ho, et al. "Identification of clinically meaningful automatically detected postprandial glucose excursions in individuals with type 1 diabetes using personal continuous glucose monitoring." Diabetes Research and Clinical Practice (2025): 112951.

[5] Edwards, Stephanie, et al. "Use of connected pen as a diagnostic tool to evaluate missed bolus dosing behavior in people with type 1 and type 2 diabetes." Diabetes Technology & Therapeutics 24.1 (2022): 61-66.

[6] Adolfsson, Peter, et al. "Increased time in range and fewer missed bolus injections after introduction of a smart connected insulin pen." Diabetes Technology & Therapeutics 22.10 (2020): 709-718.

[7] Park, Soojin, et al. "High-Amplitude and Prolonged Glucose Excursions as a Key Determinant of Discordance Between Glucose Management Indicator and Glycated Hemoglobin in Type 1 Diabetes." Diabetes Care (2026): dc252820. https://doi.org/10.2337/dc25-2820

[8] Chun, E., Fernandes, J. N., & Gaynanova, I. (2024). An Update on the iglu Software Package for Interpreting Continuous Glucose Monitoring Data. *Diabetes Technology & Therapeutics*, 26(12), 939-950. https://doi.org/10.1089/dia.2024.0154

[9] iglu package site and citation guidance: https://irinagain.github.io/iglu/ and https://irinagain.github.io/iglu/authors.html

## 🔗 Links

- [GitHub Repository](https://github.com/shstat1729/cgmguru)
- [Issue Tracker](https://github.com/shstat1729/cgmguru/issues)
- [Documentation](https://github.com/shstat1729/cgmguru)

## 🛠️ Troubleshooting

### Common Issues

**Installation Problems:**
```r
# Ensure Rcpp is properly installed
install.packages("Rcpp")
Rcpp::evalCpp("2 + 2")  # Should return 4

# Reinstall cgmguru
remove.packages("cgmguru")
remotes::install_github("shstat1729/cgmguru")
```

**Data Format Issues:**
```r
# Check data format
str(your_data)
# Should have: id (character), time (POSIXct), gl (numeric)

# Order data properly
your_data <- orderfast(your_data)
```

**Performance Issues:**
```r
# For large datasets, consider chunking
# Process subjects individually if memory is limited
unique_ids <- unique(your_data$id)
results <- lapply(unique_ids, function(id) {
  subset_data <- your_data[your_data$id == id, ]
  detect_all_events(subset_data)
})
```

---

**Note**: This package is designed for research and clinical analysis of CGM data. Always consult with healthcare professionals for clinical decision-making.
