# 🩺 cgmguru: Advanced Continuous Glucose Monitoring Analysis

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A high-performance R package for comprehensive Continuous Glucose Monitoring (CGM) data analysis with optimized C++ implementations.

## 🎯 Overview

cgmguru provides advanced tools for CGM data analysis with two primary capabilities:

- **GRID and postprandial peak detection**: Implements GRID (Glucose Rate Increase Detector) and GRID-based algorithms to detect postprandial peak glucose events
- **Extended glycemic events (Lancet CGM consensus)**: Detects hypoglycemic and hyperglycemic episodes (Level 1/2) with clinically aligned duration rules

All core algorithms are implemented in optimized C++ via Rcpp for accurate and fast analysis on large datasets.

## ✨ Key Features

- **🚀 High Performance**: C++ backend with multi-ID support and memory-efficient data structures
- **📊 GRID Algorithm**: Detects rapid glucose rate increases (commonly ≥90–95 mg/dL/hour) with configurable thresholds and gaps
- **📈 Postprandial Peak Detection**: Finds peak glucose after GRID points using local maxima and configurable time windows
- **🏥 Lancet-Consensus Event Detection**: Level 1/2 hypo- and hyperglycemia detection with duration validation (default minimum 15 minutes)
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
- **`glucose`**: Glucose values in mg/dL

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

#### Lancet-consensus glycemic events

### 🧩 Eight glycemic event functions (implemented in `detect_all_events.cpp`)
These functions detect hypo-/hyperglycemic episodes aligned with Lancet CGM consensus rules. They differ by type and level. The helper `detect_all_events()` aggregates results across these detectors.

Parameter notes:
- **`start_gl`**: threshold to start/qualify an episode (mg/dL). For hyper: typical `181` (lv1) or `251` (lv2). For hypo: typical `70` (lv1) or `54` (lv2).
- **`end_gl`**: glucose level indicating episode resolution (e.g., 180 mg/dL for hyper Level 1).
- **`dur_length`**: minimum episode duration in minutes (default often 15 minutes for Level 1; may be longer for extended definitions).
- **`end_length`**: grace period for termination/contiguity in minutes.
- **`reading_minutes`** (where applicable): CGM data reading gap (5 min for Dexcom, 15 min for Libre)

To get a combined table across all event types, use:
```r
# reading_minutes can be integer or numeric vector.
all_events <- detect_all_events(df, reading_minutes = 5)
```

| Event Type | Description | Function Call | Parameters |
|------------|-------------|---------------|------------|
| **Level 1 Hypoglycemia** | ≥15 consecutive min of <70 mg/dL, ends with ≥15 consecutive min ≥70 mg/dL | `detect_hypoglycemic_events(dataset, start_gl = 70, dur_length = 15, end_length = 15)` | `start_gl = 70, dur_length = 15, end_length = 15` |
| **Level 2 Hypoglycemia** | ≥15 consecutive min of <54 mg/dL, ends with ≥15 consecutive min ≥54 mg/dL | `detect_hypoglycemic_events(dataset, start_gl = 54, dur_length = 15, end_length = 15)` | `start_gl = 54, dur_length = 15, end_length = 15` |
| **Extended Hypoglycemia** | >120 consecutive min of <70 mg/dL, ends with ≥15 consecutive min ≥70 mg/dL | `detect_hypoglycemic_events(dataset)` | Default parameters |
| **Level 1 Hypoglycemia (Excluded)** | 54–69 mg/dL (3·0–3·9 mmol/L) ≥15 consecutive min, ends with ≥15 consecutive min ≥70 mg/dL | `detect_level1_hypoglycemic_events(dataset)` | Default parameters |
| **Level 1 Hyperglycemia** | ≥15 consecutive min of >180 mg/dL, ends with ≥15 consecutive min ≤180 mg/dL | `detect_hyperglycemic_events(dataset, start_gl = 181, dur_length = 15, end_length = 15, end_gl = 180)` | `start_gl = 181, dur_length = 15, end_length = 15, end_gl = 180` |
| **Level 2 Hyperglycemia** | ≥15 consecutive min of >250 mg/dL, ends with ≥15 consecutive min ≤250 mg/dL | `detect_hyperglycemic_events(dataset, start_gl = 251, dur_length = 15, end_length = 15, end_gl = 250)` | `start_gl = 251, dur_length = 15, end_length = 15, end_gl = 250` |
| **Extended Hyperglycemia** | >250 mg/dL lasting ≥120 min, ends when glucose returns to ≤180 mg/dL for ≥15 min | `detect_hyperglycemic_events(dataset)` | Default parameters |
| **Level 1 Hyperglycemia (Excluded)** | 181–250 mg/dL (10·1–13·9 mmol/L) ≥15 consecutive min, ends with ≥15 consecutive min ≤180 mg/dL | `detect_level1_hyperglycemic_events(dataset)` | Default parameters |

```r
# Equivalent examples using camelCase naming for readability
# Level 1 Hypoglycemia Event (≥15 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
detect_hypoglycemic_events(dataset, start_gl = 70, dur_length = 15, end_length = 15)  # hypo, level = lv1

# Level 2 Hypoglycemia Event (≥15 consecutive min of <54 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥54 mg/dL)
detect_hypoglycemic_events(dataset, start_gl = 54, dur_length = 15, end_length = 15)  # hypo, level = lv2

# Extended Hypoglycemia Event (>120 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
detect_hypoglycemic_events(dataset)                                                    # hypo, extended

# Hypoglycaemia alert value (54–69 mg/dL (3·0–3·9 mmol/L) ≥15 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL
detect_level1_hypoglycemic_events(dataset)                                              # hypo, lv1_excl

# Level 1 Hyperglycemia Event (≥15 consecutive min of >180 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≤180 mg/dL)
detect_hyperglycemic_events(dataset, start_gl = 181, dur_length = 15, end_length = 15, end_gl = 180)  # hyper, lv1

# Level 2 Hyperglycemia Event (≥15 consecutive min of >250 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≤250 mg/dL)
detect_hyperglycemic_events(dataset, start_gl = 251, dur_length = 15, end_length = 15, end_gl = 250)  # hyper, lv2

# Extended Hyperglycemia Event (Number of events with sensor glucose >250 mg/dL (>13·9 mmol/L) lasting at least 120 min; event ends when glucose returns to ≤180 mg/dL (≤10·0 mmol/L) for ≥15 min)
detect_hyperglycemic_events(dataset)                                                                 # hyper, extended

# High glucose (Level 1, excluded) (181–250 mg/dL (10·1–13·9 mmol/L) ≥15 consecutive min and Event ends when there is ≥15 consecutive min with a CGM sensor value of ≤180 mg/dL) 
detect_level1_hyperglycemic_events(dataset)                                                           # hyper, lv1_excl
```



### 🔧 Advanced analysis helpers
- **start_finder** : find R-based index (1-indexed) from 0 and 1 vector.



## 📚 Function Reference

### Core Analysis Functions
- `grid()` - GRID algorithm for glycemic event detection
- `maxima_grid()` - Combined maxima detection and GRID analysis
- `detect_hyperglycemic_events()` - Hyperglycemic event detection
- `detect_hypoglycemic_events()` - Hypoglycemic event detection
- `detect_level1_hyperglycemic_events()` - Level 1 hyperglycemic events
- `detect_level1_hypoglycemic_events()` - Level 1 hypoglycemic events
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

## 🤝 Contributing

We welcome contributions! Please feel free to submit issues, feature requests, or pull requests.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👨‍💻 Author

**Sang Ho Park, M.S.** - [shstat1729@gmail.com](mailto:shstat1729@gmail.com)
  - He writes code for the package. 
**Sang-Man Jin, MD, PhD** - [sjin772@gmail.com](mailto:sjin772@gmail.com)
  - He consults GRID-based algorithm and CGM consensus (post-prandial peak glucose).
**Rosa Oh, MD** - [wlscodl123@naver.com](mailto:wlscodl123@naver.com)
  - She consults GRID-based algorithm and CGM consensus (post-prandial peak glucose).

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


