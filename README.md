# 🩺 cgmguru: Advanced Continuous Glucose Monitoring Analysis

[![R-CMD-check](https://github.com/shstat1729/cgmguru/workflows/R-CMD-check/badge.svg)](https://github.com/shstat1729/cgmguru/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Rcpp](https://img.shields.io/badge/Rcpp-enabled-blue.svg)](https://github.com/RcppCore/Rcpp)

**Advanced Continuous Glucose Monitoring Analysis with High-Performance C++ Backend**

A comprehensive R package for CGM data analysis implementing GRID algorithms and international consensus glycemic event detection with optimized C++ implementations via Rcpp.

## 🎯 Overview

cgmguru provides advanced tools for CGM data analysis with two primary capabilities:

- **GRID and postprandial peak detection**: Implements GRID (Glucose Rate Increase Detector) and GRID-based algorithms to detect postprandial peak glucose events
- **Glycemic events**: Detects hypoglycemic and hyperglycemic episodes (Level 1/2) aligned with international consensus CGM metrics [1]

All core algorithms are implemented in optimized C++ via Rcpp for accurate and fast analysis on large datasets.

## ✨ Key Features

- **🚀 High Performance**: C++ backend with efficient multi-subject processing and memory-optimized data structures
- **📊 GRID Algorithm**: Detects rapid glucose rate increases (commonly ≥90–95 mg/dL/hour) with configurable thresholds and gaps [2]
- **📈 Postprandial Peak Detection**: Finds peak glucose after GRID points using local maxima and configurable time windows [4]
- **🏥 Consensus CGM Metrics Event Detection**: Level 1/2 hypo- and hyperglycemia detection with duration validation (default minimum 15 minutes)
- **🔧 Advanced Analysis Tools**: Local maxima finding, excursion analysis, and robust episode validation utilities
- **📋 Comprehensive Documentation**: Detailed function documentation with examples and parameter descriptions

## 📦 Installation

### From GitHub (development version)
```r
# install.packages("remotes")
remotes::install_github("shstat1729/cgmguru")
```

### Dependencies
```r
# Required packages
install.packages(c("Rcpp", "iglu"))

# For examples and vignettes
install.packages(c("ggplot2", "dplyr", "testthat"))
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
all_events <- detect_all_events(example_data_5_subject, reading_minutes = 5)
print(all_events)
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
  iglu_episodes = episode_calculation(example_data_5_subject),
  cgmguru_events = detect_all_events(example_data_5_subject),
  times = 100,
  unit = "ms"
)

print(benchmark_results)
# cgmguru is ~300x faster than pure R implementations
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
                               mod_grid_maxima$max_indices, 
                               local_maxima$local_maxima_vector)

# 6) Map GRID points to maximum points (within 4 hours)
transform_maxima <- transform_df(grid_result$episode_start_total, final_maxima)

# 7) Redistribute overlapping maxima between GRID points
final_between_maxima <- detect_between_maxima(example_data_5_subject, transform_maxima)
```

## 🏥 Consensus CGM Glycemic Events

These functions detect hypo-/hyperglycemic episodes aligned with Consensus CGM metrics rules. They differ by type and level. The helper `detect_all_events()` aggregates results across these detectors.

### Parameter Notes
- **`start_gl`**: threshold to start/qualify an episode (mg/dL). For hyper: typical `180` (lv1) or `250` (lv2). For hypo: typical `70` (lv1) or `54` (lv2).
- **`end_gl`**: glucose level indicating episode resolution (e.g., 180 mg/dL for hyper Level 1).
- **`dur_length`**: minimum episode duration in minutes (default often 15 minutes for Level 1; may be longer for extended definitions).
- **`end_length`**: grace period for termination/contiguity in minutes.
- **`reading_minutes`**: CGM device sampling interval in minutes (e.g., 5 min for Dexcom, 15 min for Libre). Used to calculate minimum required readings for event validation based on the 3/4 rule: `ceil((dur_length / reading_minutes) / 4 * 3)`.

### Event Detection Table
| Event Type | Description | Function Call | Parameters |
|------------|-------------|---------------|------------|
| **Level 1 Hypoglycemia** | ≥15 consecutive min of <70 mg/dL, ends with ≥15 consecutive min ≥70 mg/dL | `detect_hypoglycemic_events(df, start_gl = 70, dur_length = 15, end_length = 15)` | `start_gl = 70, dur_length = 15, end_length = 15` |
| **Level 2 Hypoglycemia** | ≥15 consecutive min of <54 mg/dL, ends with ≥15 consecutive min ≥54 mg/dL | `detect_hypoglycemic_events(df, start_gl = 54, dur_length = 15, end_length = 15)` | `start_gl = 54, dur_length = 15, end_length = 15` |
| **Extended Hypoglycemia** | >120 consecutive min of <70 mg/dL, ends with ≥15 consecutive min ≥70 mg/dL | `detect_hypoglycemic_events(df)` | Default parameters |
| **Level 1 Hypoglycemia (Excluded)** | 54–69 mg/dL (3·0–3·9 mmol/L) ≥15 consecutive min, ends with ≥15 consecutive min ≥70 mg/dL | `detect_all_events(df)` | Default parameters |
| **Level 1 Hyperglycemia** | ≥15 consecutive min of >180 mg/dL, ends with ≥15 consecutive min ≤180 mg/dL | `detect_hyperglycemic_events(df, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)` | `start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180` |
| **Level 2 Hyperglycemia** | ≥15 consecutive min of >250 mg/dL, ends with ≥15 consecutive min ≤250 mg/dL | `detect_hyperglycemic_events(df, start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250)` | `start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250` |
| **Extended Hyperglycemia** | >250 mg/dL lasting ≥120 min, ends when glucose returns to ≤180 mg/dL for ≥15 min ≥90 cumulative min within a 120-min period | `detect_hyperglycemic_events(df)` | Default parameters |
| **Level 1 Hyperglycemia (Excluded)** | 181–250 mg/dL (10·1–13·9 mmol/L) ≥15 consecutive min, ends with ≥15 consecutive min ≤180 mg/dL | `detect_all_events(df)` | Default parameters |

### Example Usage
```r
# Level 1 Hypoglycemia Event (≥15 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
detect_hypoglycemic_events(example_data_5_subject, start_gl = 70, dur_length = 15, end_length = 15)  # hypo, level = lv1

# Level 2 Hypoglycemia Event (≥15 consecutive min of <54 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥54 mg/dL)
detect_hypoglycemic_events(example_data_5_subject, start_gl = 54, dur_length = 15, end_length = 15)  # hypo, level = lv2

# Extended Hypoglycemia Event (>120 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
detect_hypoglycemic_events(example_data_5_subject)                                                    # hypo, extended

# Level 1 Hyperglycemia Event (≥15 consecutive min of >180 mg/dL and event ends when there is ≥15 consecutive 
min with a CGM sensor value of ≤180 mg/dL)
detect_hyperglycemic_events(example_data_5_subject, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)

# Level 2 Hyperglycemia Event (≥15 consecutive min of >250 mg/dL and event ends when there is ≥15 consecutive 
min with a CGM sensor value of ≤250 mg/dL)
detect_hyperglycemic_events(example_data_5_subject, start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250)

# Extended Hyperglycemia Event (>250 mg/dL lasting ≥120 min, ends when glucose returns to ≤180 mg/dL for ≥15 min ≥90 cumulative min within a 120-min period)
detect_hyperglycemic_events(example_data_5_subject)

# Comprehensive event detection
all_events <- detect_all_events(example_data_5_subject, reading_minutes = 5)
print(all_events)
```

## 📚 Function Reference

### Core Analysis Functions
| Function | Description | C++ Implementation |
|----------|-------------|-------------------|
| `grid()` | GRID algorithm for rapid glucose increase detection | ✅ |
| `maxima_grid()` | Combined maxima detection and GRID analysis | ✅ |
| `detect_hyperglycemic_events()` | Hyperglycemic event detection (Level 1/2/Extended) | ✅ |
| `detect_hypoglycemic_events()` | Hypoglycemic event detection (Level 1/2/Extended) | ✅ |
| `detect_all_events()` | Comprehensive event detection across all types | ✅ |

### Advanced Analysis Functions
| Function | Description | C++ Implementation |
|----------|-------------|-------------------|
| `find_local_maxima()` | Local maxima identification in glucose time series | ✅ |
| `excursion()` | Glucose excursion calculation | ✅ |
| `mod_grid()` | Modified GRID analysis with custom parameters | ✅ |
| `detect_between_maxima()` | Event detection between maxima | ✅ |
| `find_new_maxima()` | New maxima detection around grid points | ✅ |

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
| `start_finder()` | Find episode start indices from binary vectors | ✅ |
| `transform_df()` | Data transformation for downstream analysis | ✅ |

## 📊 Performance

```r
library(microbenchmark)
library(iglu)
data(example_data_hall)
# Perform microbenchmark comparison
benchmark_results <- microbenchmark(
  episode_calculation = episode_calculation(example_data_hall), # iglu package 
  detect_all_events = detect_all_events(example_data_hall), # cgmguru package
  times = 100,
  unit = "ms"
)

print(benchmark_results)
```

**Results:**
```
Unit: milliseconds
                expr        min         lq       mean     median         uq        max neval cld
 episode_calculation 767.365512 775.952347 791.590803 780.188242 788.746479 894.089132   100  a 
   detect_all_events   2.668485   2.753642   2.789381   2.792592   2.828323   2.908253   100   b
```

**Performance:** cgmguru is ~270x faster than episode_calculation in iglu.

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
  author = {Park, Sang Ho and Jin, Sang-Man and Oh, Rosa},
  year = {2025},
  url = {https://github.com/shstat1729/cgmguru},
  note = {R package version 0.1.0}
}
```

### Related Publications
[1] Battelino, T., et al. (2023). Continuous glucose monitoring and metrics for clinical trials: an international consensus statement. *The Lancet Diabetes & Endocrinology*, 11(1), 42-57.

[2] Harvey, R. A., et al. (2014). Design of the glucose rate increase detector: a meal detection module for the health monitoring system. *Journal of Diabetes Science and Technology*, 8(2), 307-320.

[3] Chun, E., et al. (2023). iglu: interpreting glucose data from continuous glucose monitors. R package version 3.0.

[4] Park, Sang Ho, et al. "Identification of clinically meaningful automatically detected postprandial glucose excursions in individuals with type 1 diabetes using personal continuous glucose monitoring." Diabetes Research and Clinical Practice (2025): 112951.

## 🔗 Links

- [GitHub Repository](https://github.com/shstat1729/cgmguru)
- [Issue Tracker](https://github.com/shstat1729/cgmguru/issues)
- [Documentation](https://shstat1729.github.io/cgmguru/)

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
  detect_all_events(subset_data, reading_minutes = 5)
})
```

---

**Note**: This package is designed for research and clinical analysis of CGM data. Always consult with healthcare professionals for clinical decision-making.