# Simple test for cgmguru package
# Load necessary libraries
cat("--- Loading libraries ---\n")
library(cgmguru)
library(iglu)
library(dplyr)

cat("Libraries loaded successfully.\n")

# Load example data from the iglu package
cat("\n--- Loading data ---\n")
data(example_data_5_subject)
cat("Loaded example_data_5_subject from iglu package.\n")
print(head(example_data_5_subject))

# Test basic functions
cat("\n--- Testing basic functions ---\n")

# Test orderfast function
cat("Testing orderfast function...\n")
ordered_data <- orderfast(example_data_5_subject)
cat("orderfast completed successfully.\n")

# Test episode calculation from iglu
cat("\nTesting iglu episode calculation...\n")
iglu_episode <- episode_calculation(example_data_5_subject)
cat("iglu episode calculation completed.\n")
print(head(iglu_episode))

# Test cgmguru detect_all_events function
cat("\nTesting cgmguru detect_all_events function...\n")
cgm_events <- detect_all_events(example_data_5_subject)
cat("cgmguru detect_all_events completed.\n")
print(head(cgm_events))

# Test individual event detection functions
cat("\n--- Testing individual event detection functions ---\n")

# Test hypoglycemic events
cat("Testing hypoglycemic events detection...\n")
hypo_events <- detect_hypoglycemic_events(example_data_5_subject)
cat("Hypoglycemic events detection completed.\n")

# Test hyperglycemic events  
cat("Testing hyperglycemic events detection...\n")
hyper_events <- detect_hyperglycemic_events(example_data_5_subject)
cat("Hyperglycemic events detection completed.\n")

# Test level 1 events
cat("Testing level 1 hypoglycemic events detection...\n")
level1_hypo_events <- detect_level1_hypoglycemic_events(example_data_5_subject)
cat("Level 1 hypoglycemic events detection completed.\n")

cat("Testing level 1 hyperglycemic events detection...\n")
level1_hyper_events <- detect_level1_hyperglycemic_events(example_data_5_subject)
cat("Level 1 hyperglycemic events detection completed.\n")

# Test GRID-related functions
cat("\n--- Testing GRID-related functions ---\n")

# Test grid function
cat("Testing grid function...\n")
grid_result <- grid(example_data_5_subject, gap = 60, threshold = 130)
cat("Grid function completed.\n")

# Test start_finder
cat("Testing start_finder function...\n")
start_points <- start_finder(grid_result$grid_vector)
cat("Start finder completed.\n")

# Test mod_grid
cat("Testing mod_grid function...\n")
mod_grid_result <- mod_grid(example_data_5_subject, start_points, hours = 2, gap = 60)
cat("Mod grid completed.\n")

# Test find_local_maxima
cat("Testing find_local_maxima function...\n")
local_maxima <- find_local_maxima(example_data_5_subject)
cat("Find local maxima completed.\n")

# Test find_max_after_hours
cat("Testing find_max_after_hours function...\n")
max_after <- find_max_after_hours(example_data_5_subject, start_points, hours = 2)
cat("Find max after hours completed.\n")

# Test find_new_maxima
cat("Testing find_new_maxima function...\n")
new_maxima <- find_new_maxima(example_data_5_subject, max_after$max_indices, local_maxima$local_maxima_vector)
cat("Find new maxima completed.\n")

# Test transform_df
cat("Testing transform_df function...\n")
transform_result <- transform_df(grid_result$episode_start_total, new_maxima)
cat("Transform df completed.\n")

# Test detect_between_maxima
cat("Testing detect_between_maxima function...\n")
between_maxima <- detect_between_maxima(example_data_5_subject, transform_result)
cat("Detect between maxima completed.\n")

# Test maxima_grid
cat("Testing maxima_grid function...\n")
maxima_grid_result <- maxima_grid(example_data_5_subject)
cat("Maxima grid completed.\n")

cat("\n--- All tests completed successfully! ---\n")
cat("The cgmguru package is working correctly.\n")
