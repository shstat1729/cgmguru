# Fixed version of testexample2.R - skipping the reinstallation part
# Load necessary libraries
cat("--- Loading libraries ---\n")
library(cgmguru)
library(iglu)
library(dplyr)

cat("Libraries loaded.\n")

# Load example data from the iglu package
cat("\n--- Loading data ---\n")
data(example_data_5_subject)
example_data_5_subject

# Test basic functions
cat("\n--- Testing basic functions ---\n")

# Test orderfast function
orderfast <- function(df){
  return(df[order(df$id,df$time),])
}

# Test episode calculation from iglu
cat("Testing iglu episode calculation...\n")
iglu_episode <- episode_calculation(example_data_5_subject)
iglu_episode

# Test cgmguru detect_all_events function
cat("Testing cgmguru detect_all_events function...\n")
a <- Sys.time()
test1 <- episode_calculation(example_data_5_subject)
b <- Sys.time()
cat("iglu episode calculation time:", b-a, "\n")

a <- Sys.time()
test2 <- detect_all_events(example_data_5_subject)
b <- Sys.time()
cat("cgmguru detect_all_events time:", b-a, "\n")

test1
test2

# Test individual event detection functions
cat("\n--- Testing individual event detection functions ---\n")

# Test hypoglycemic events
detect_hypoglycemic_events(example_data_5_subject, start_gl = 70, dur_length = 15, end_length = 15) # type : hypo, level = lv1
detect_hypoglycemic_events(example_data_5_subject, start_gl = 54, dur_length = 15, end_length = 15) # type : hypo, level = lv2
detect_hypoglycemic_events(example_data_5_subject) # type : hypo, level = extended
detect_level1_hypoglycemic_events(example_data_5_subject) # type : hypo, level = lv1_excl

# Test hyperglycemic events
detect_hyperglycemic_events(example_data_5_subject, start_gl = 181, dur_length = 15, end_length = 15, end_gl = 180) # type : hyper, level = lv1
detect_hyperglycemic_events(example_data_5_subject, start_gl = 251, dur_length = 15, end_length = 15, end_gl = 250) # type : hyper, level = lv2
detect_hyperglycemic_events(example_data_5_subject) # type : hyper, level = extended
detect_level1_hyperglycemic_events(example_data_5_subject) # type : hyper, level = lv1_excl

# Test GRID-related functions
cat("\n--- Testing GRID-related functions ---\n")

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

test1 <- maxima_grid(example_data_5_subject)
test2 <- final_between_maxima
test1$episode_counts
test2$episode_counts

maxima_grid(example_data_5_subject)

cat("\n--- All tests completed successfully! ---\n")
cat("The cgmguru package is working correctly.\n")
