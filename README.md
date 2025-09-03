## 🩺 cgmguru: Advanced Continuous Glucose Monitoring Analysis
A high-performance R package for comprehensive CGM data analysis with optimized C++ implementations

### 🎯 Overview
cgmguru provides two primary capabilities:
- **GRID and postprandial peak detection**: Implements GRID (Glucose Rate Increase Detector) and a GRID-based algorithm to detect postprandial peak glucose.
- **Extended glycemic events (Lancet CGM consensus)**: Detects hypoglycemic and hyperglycemic episodes (Level 1/2) with clinically aligned duration rules.

All core algorithms are implemented in optimized C++ via Rcpp for accurate and fast analysis on large datasets.

### ✨ Key Features
- **GRID algorithm**: Detects rapid glucose rate increases (commonly ≥90–95 mg/dL/hour). Thresholds and gaps are configurable.
- **Postprandial peak detection**: Finds peak glucose after GRID points using local maxima and configurable windows.
- **Lancet-consensus event detection**: Level 1/2 hypo- and hyperglycemia with duration validation (default minimum 15 minutes).
- **High performance**: C++ backend, multi-ID support, and memory-efficient data structures.
- **Advanced tools**: Local maxima finding, excursion analysis, and robust episode validation utilities.

### 📦 Installation
From source (this repository):
```r
# install.packages("remotes")
remotes::install_local(".")
```

### 📐 Expected data format
Most functions expect a dataframe with at least:
- **id**: patient identifier (character or factor)
- **time**: POSIXct timestamps
- **gl**: glucose value (mg/dL)

You can pre-order your data by id and time with `orderfast(df)` if needed.

### 🚀 Usage

#### GRID detection
```r
library(cgmguru)

# df must contain columns: id, time (POSIXct), gl
# gap: allowed gap (minutes) within which readings are considered contiguous
# threshold: GRID slope threshold (mg/dL/hour)

grid_points <- GRID(df, gap = 15, threshold = 130)
```

#### Postprandial peak detection (GRID-based)
```r
# 1) Find GRID points
grid_points <- GRID(df, gap = 15, threshold = 130)

# 2) Identify local maxima around episodes/windows
local_maxima <- find_local_maxima(df)

# 3) Map maxima to GRID points using a candidate search window (`hours`)
#    Note: final GRID-to-peak pairing is constrained to at most 4 hours.
#    Using hours = 2 searches within 2 hours post-GRID, but any
#    mapping beyond 4 hours is excluded downstream.
maxima <- maxima_grid(df, threshold = 130, gap = 60, hours = 2)

# 4) Optional: refine/transform for episode-level summaries
summary_df <- transform_df(grid_points, maxima)
```

#### Lancet-consensus glycemic events
```r
# Canonical (exported) function names
l1_hypo <- detect_level1_hypoglycemic_events(df, reading_minutes = NULL,
  dur_length = 15, end_length = 15, start_gl_min = 54, start_gl_max = 69, end_gl = 70)

l2_hypo <- detect_hypoglycemic_events(df, reading_minutes = NULL,
  dur_length = 120, end_length = 15, start_gl = 70)

l1_hyper <- detect_level1_hyperglycemic_events(df, reading_minutes = NULL,
  dur_length = 15, end_length = 15, start_gl_min = 181, start_gl_max = 250, end_gl = 180)

l2_hyper <- detect_hyperglycemic_events(df, reading_minutes = NULL,
  dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

# All events helper
all_events <- detect_all_events(df, reading_minutes = NULL)
```

```r
# Equivalent examples using camelCase naming for readability
# (map to exported snake_case functions shown above)

detect_hypoglycemic_events(dataset, start_gl = 70, dur_length = 15, end_length = 15)  # hypo, level = lv1

detect_hypoglycemic_events(dataset, start_gl = 54, dur_length = 15, end_length = 15)  # hypo, level = lv2

detect_hypoglycemic_events(dataset)                                                    # hypo, extended

detect_level1_hypoglycemic_events(dataset)                                              # hypo, lv1_excl

detect_hyperglycemic_events(dataset, start_gl = 181, dur_length = 15, end_length = 15, end_gl = 180)  # hyper, lv1

detect_hyperglycemic_events(dataset, start_gl = 251, dur_length = 15, end_length = 15, end_gl = 250)  # hyper, lv2

detect_hyperglycemic_events(dataset)                                                                 # hyper, extended

detect_level1_hyperglycemic_events(dataset)                                                           # hyper, lv1_excl
```

### 🧩 Eight glycemic event functions (implemented in `detect_all_events.cpp`)
These functions detect hypo-/hyperglycemic episodes aligned with Lancet CGM consensus rules. They differ by type and level. The helper `detect_all_events()` aggregates results across these detectors.

- **`detect_hypoglycemic_events(dataset, start_gl = 70, dur_length = 15, end_length = 15)`**: hypo, level = lv1 (≤70 mg/dL), minimum duration `dur_length`, episode termination grace `end_length`.
- **`detect_hypoglycemic_events(dataset, start_gl = 54, dur_length = 15, end_length = 15)`**: hypo, level = lv2 (≤54 mg/dL).
- **`detect_hypoglycemic_events(dataset)`**: hypo, level = extended (broader hypo detection using defaults/extended criteria).
- **`detect_level1_hypoglycemic_events(dataset)`**: hypo, level = lv1_excl (Level 1 excluding Level 2 periods).
- **`detect_hyperglycemic_events(dataset, start_gl = 181, dur_length = 15, end_length = 15, end_gl = 180)`**: hyper, level = lv1 (181–250 mg/dL), with return-to-`end_gl` criterion.
- **`detect_hyperglycemic_events(dataset, start_gl = 251, dur_length = 15, end_length = 15, end_gl = 250)`**: hyper, level = lv2 (>250 mg/dL).
- **`detect_hyperglycemic_events(dataset)`**: hyper, level = extended (broader hyper detection using defaults/extended criteria).
- **`detect_level1_hyperglycemic_events(dataset)`**: hyper, level = lv1_excl (Level 1 excluding Level 2 periods).

Parameter notes:
- **`start_gl`**: threshold to start/qualify an episode (mg/dL). For hyper: typical `181` (lv1) or `251` (lv2). For hypo: typical `70` (lv1) or `54` (lv2).
- **`end_gl`**: glucose level indicating episode resolution (e.g., 180 mg/dL for hyper Level 1).
- **`dur_length`**: minimum episode duration in minutes (default often 15 minutes for Level 1; may be longer for extended definitions).
- **`end_length`**: grace period for termination/contiguity in minutes.
- **`reading_minutes`** (where applicable): sampling interval override; if `NULL`, inferred from data.

To get a combined table across all event types, use:
```r
all_events <- detect_all_events(df, reading_minutes = NULL)
```

### 🔧 Advanced analysis helpers
- **`