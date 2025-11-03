## ----setup, include=FALSE-----------------------------------------------------
knitr::opts_chunk$set(collapse = TRUE, comment = "#>", fig.width = 10, fig.height = 6)
library(cgmguru)
library(iglu)
library(ggplot2)
library(dplyr)
set.seed(123)

## ----eval=FALSE---------------------------------------------------------------
# ?grid
# ?detect_all_events

## ----load-data----------------------------------------------------------------
# Load example datasets
data(example_data_5_subject)  # 5 subjects, 13,866 readings
data(example_data_hall)       # 19 subjects, 34,890 readings

# Display basic information about the datasets
cat("Dataset 1 (example_data_5_subject):\n")
cat("  Rows:", nrow(example_data_5_subject), "\n")
cat("  Subjects:", length(unique(example_data_5_subject$id)), "\n")
cat("  Time range:", as.character(range(example_data_5_subject$time)), "\n")
cat("  Glucose range:", range(example_data_5_subject$gl), "mg/dL\n\n")

cat("Dataset 2 (example_data_hall):\n")
cat("  Rows:", nrow(example_data_hall), "\n")
cat("  Subjects:", length(unique(example_data_hall$id)), "\n")
cat("  Time range:", as.character(range(example_data_hall$time)), "\n")
cat("  Glucose range:", range(example_data_hall$gl), "mg/dL\n")

# Show first few rows
head(example_data_5_subject)

## ----grid-analysis------------------------------------------------------------
# Perform GRID analysis on the smaller dataset
grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)

# Display results
cat("GRID Analysis Results:\n")
cat("  Detected grid points:", nrow(grid_result$grid_vector), "\n")
cat("  Episode counts:\n")
print(grid_result$episode_counts)

# Show first few detected grid points
cat("\nFirst few detected grid points:\n")
head(grid_result$grid_vector)

## ----hyperglycemic-events-----------------------------------------------------
# Level 1 Hyperglycemic events (≥15 consecutive minutes >180 mg/dL)
hyper_lv1 <- detect_hyperglycemic_events(
  example_data_5_subject, 
  start_gl = 180, 
  dur_length = 15, 
  end_length = 15, 
  end_gl = 180
)

# Level 2 Hyperglycemic events (≥15 consecutive minutes >250 mg/dL)
hyper_lv2 <- detect_hyperglycemic_events(
  example_data_5_subject, 
  start_gl = 250, 
  dur_length = 15, 
  end_length = 15, 
  end_gl = 250
)

# Extended Hyperglycemic events (default parameters)
hyper_extended <- detect_hyperglycemic_events(example_data_5_subject)

cat("Hyperglycemic Event Detection Results:\n")
cat("Level 1 Events (>180 mg/dL):\n")
print(hyper_lv1$events_total)

cat("\nLevel 2 Events (>250 mg/dL):\n")
print(hyper_lv2$events_total)

cat("\nExtended Events (default):\n")
print(hyper_extended$events_total)

# Show detailed events for first subject
cat("\nDetailed Level 1 Events for Subject", hyper_lv1$events_detailed$id[1], ":\n")
head(hyper_lv1$events_detailed[hyper_lv1$events_detailed$id == hyper_lv1$events_detailed$id[1], ])

## ----hypoglycemic-events------------------------------------------------------
# Level 1 Hypoglycemic events (≤70 mg/dL)
hypo_lv1 <- detect_hypoglycemic_events(
  example_data_5_subject, 
  start_gl = 70, 
  dur_length = 15, 
  end_length = 15
)

# Level 2 Hypoglycemic events (≤54 mg/dL)
hypo_lv2 <- detect_hypoglycemic_events(
  example_data_5_subject, 
  start_gl = 54, 
  dur_length = 15, 
  end_length = 15
)

cat("Hypoglycemic Event Detection Results:\n")
cat("Level 1 Events (≤70 mg/dL):\n")
print(hypo_lv1$events_total)

cat("\nLevel 2 Events (≤54 mg/dL):\n")
print(hypo_lv2$events_total)

## ----all-events---------------------------------------------------------------
# Detect all events with 5-minute reading intervals
all_events <- detect_all_events(example_data_5_subject, reading_minutes = 5)

cat("Comprehensive Event Detection Results:\n")
print(all_events)

## ----local-maxima-------------------------------------------------------------
# Find local maxima
maxima_result <- find_local_maxima(example_data_5_subject)

cat("Local Maxima Detection Results:\n")
cat("  Total local maxima found:", nrow(maxima_result$local_maxima_vector), "\n")
cat("  Merged results:", nrow(maxima_result$merged_results), "\n")

# Show first few maxima
head(maxima_result$local_maxima_vector)

## ----maxima-grid--------------------------------------------------------------
# Combined maxima and GRID analysis
maxima_grid_result <- maxima_grid(
  example_data_5_subject, 
  threshold = 130, 
  gap = 60, 
  hours = 2
)

cat("Maxima-GRID Combined Analysis Results:\n")
cat("  Detected maxima:", nrow(maxima_grid_result$results), "\n")
cat("  Episode counts:\n")
print(maxima_grid_result$episode_counts)

# Show first few results
head(maxima_grid_result$results)

## ----excursion-analysis-------------------------------------------------------
# Excursion analysis
excursion_result <- excursion(example_data_5_subject, gap = 15)

cat("Excursion Analysis Results:\n")
cat("  Excursion vector length:", length(excursion_result$excursion_vector), "\n")
cat("  Episode counts:\n")
print(excursion_result$episode_counts)

# Show episode start information
head(excursion_result$episode_start)

## ----complete-pipeline--------------------------------------------------------
# Use the larger dataset for comprehensive analysis
cat("Running complete analysis pipeline on example_data_hall...\n")

# Step 1: GRID analysis
cat("Step 1: GRID Analysis\n")
grid_pipeline <- grid(example_data_hall, gap = 15, threshold = 130)
cat("  Detected", nrow(grid_pipeline$grid_vector), "grid points\n")

# Step 2: Local maxima detection
cat("Step 2: Local Maxima Detection\n")
maxima_pipeline <- find_local_maxima(example_data_hall)
cat("  Found", nrow(maxima_pipeline$local_maxima_vector), "local maxima\n")

# Step 3: Modified GRID analysis
cat("Step 3: Modified GRID Analysis\n")
mod_grid_pipeline <- mod_grid(
  example_data_hall, 
  grid_pipeline$grid_vector, 
  hours = 2, 
  gap = 15
)
cat("  Modified grid points:", nrow(mod_grid_pipeline$mod_grid_vector), "\n")

# Step 4: Find maximum points after modified GRID points
cat("Step 4: Finding Maximum Points After GRID Points\n")
max_after_pipeline <- find_max_after_hours(
  example_data_hall,
  mod_grid_pipeline$mod_grid_vector,
  hours = 2
)
cat("  Maximum points found:", length(max_after_pipeline$max_indices), "\n")

# Step 5: Find new maxima
cat("Step 5: Finding New Maxima\n")
new_maxima_pipeline <- find_new_maxima(
  example_data_hall,
  max_after_pipeline$max_indices,
  maxima_pipeline$local_maxima_vector
)
cat("  New maxima identified:", nrow(new_maxima_pipeline), "\n")

# Step 6: Transform dataframes
cat("Step 6: Transforming Dataframes\n")
transformed_pipeline <- transform_df(
  grid_pipeline$episode_start, 
  new_maxima_pipeline
)
cat("  Transformed dataframe rows:", nrow(transformed_pipeline), "\n")

# Step 7: Detect between maxima
cat("Step 7: Detecting Between Maxima\n")
between_maxima_pipeline <- detect_between_maxima(
  example_data_hall, 
  transformed_pipeline
)
cat("  Between maxima analysis completed\n")

cat("\nComplete pipeline executed successfully!\n")

## ----time-based-analysis------------------------------------------------------
# Create a subset for demonstration
subset_data <- example_data_5_subject[example_data_5_subject$id == unique(example_data_5_subject$id)[1], ][1:100, ]

# Create start points for time-based analysis
start_points <- subset_data[seq(1, nrow(subset_data), by = 20), ]

cat("Time-Based Analysis Functions:\n")

# Find maximum after 1 hour
max_after <- find_max_after_hours(subset_data, start_points, hours = 1)
cat("  Max after 1 hour:", length(max_after$max_indices), "points\n")

# Find maximum before 1 hour
max_before <- find_max_before_hours(subset_data, start_points, hours = 1)
cat("  Max before 1 hour:", length(max_before$max_indices), "points\n")

# Find minimum after 1 hour
min_after <- find_min_after_hours(subset_data, start_points, hours = 1)
cat("  Min after 1 hour:", length(min_after$min_indices), "points\n")

# Find minimum before 1 hour
min_before <- find_min_before_hours(subset_data, start_points, hours = 1)
cat("  Min before 1 hour:", length(min_before$min_indices), "points\n")

## ----ordering-utility---------------------------------------------------------
# Create sample data with mixed order
sample_data <- data.frame(
  id = c("b", "a", "c", "a", "b"),
  time = as.POSIXct(c("2023-01-01 10:00:00", "2023-01-01 09:00:00", 
                      "2023-01-01 11:00:00", "2023-01-01 08:00:00", 
                      "2023-01-01 12:00:00"), tz = "UTC"),
  gl = c(120, 100, 140, 90, 130)
)

cat("Original data (unordered):\n")
print(sample_data)

# Order the data
ordered_data <- orderfast(sample_data)

cat("\nOrdered data:\n")
print(ordered_data)

## ----visualization, fig.width=12, fig.height=8--------------------------------
# Select one subject for visualization
subject_id <- unique(example_data_5_subject$id)[1]
subject_data <- example_data_5_subject[example_data_5_subject$id == subject_id, ]

# Create a comprehensive plot
p1 <- ggplot(subject_data, aes(x = time, y = gl)) +
  geom_line(color = "blue", alpha = 0.7, size = 0.5) +
  geom_hline(yintercept = 180, color = "red", linetype = "dashed", alpha = 0.8) +
  geom_hline(yintercept = 250, color = "darkred", linetype = "dashed", alpha = 0.8) +
  geom_hline(yintercept = 70, color = "orange", linetype = "dashed", alpha = 0.8) +
  geom_hline(yintercept = 54, color = "darkorange", linetype = "dashed", alpha = 0.8) +
  labs(title = paste("CGM Data for Subject", subject_id, "with Clinical Thresholds"),
       subtitle = "Red lines: Hyperglycemia thresholds (180, 250 mg/dL)\nOrange lines: Hypoglycemia thresholds (70, 54 mg/dL)",
       x = "Time", 
       y = "Glucose (mg/dL)") +
  theme_minimal() +
  theme(plot.title = element_text(size = 14, face = "bold"),
        plot.subtitle = element_text(size = 10))

print(p1)

# Create a summary plot showing event counts across subjects
event_summary <- hyper_lv1$events_total
event_summary$subject <- paste("Subject", event_summary$id)

p2 <- ggplot(event_summary, aes(x = subject, y = total_events)) +
  geom_col(fill = "steelblue", alpha = 0.7) +
  geom_text(aes(label = total_events), vjust = -0.5) +
  labs(title = "Level 1 Hyperglycemic Events by Subject",
       x = "Subject",
       y = "Number of Events") +
  theme_minimal() +
  theme(axis.text.x = element_text(angle = 45, hjust = 1))

print(p2)

## ----performance-comparison---------------------------------------------------
# Function to measure execution time
measure_time <- function(expr) {
  start_time <- Sys.time()
  result <- eval(expr)
  end_time <- Sys.time()
  return(list(result = result, time = as.numeric(end_time - start_time, units = "secs")))
}

cat("Performance Comparison:\n")

# Test on smaller dataset
cat("Small dataset (5 subjects, 13,866 readings):\n")
small_time <- measure_time(grid(example_data_5_subject, gap = 15, threshold = 130))
cat("  GRID analysis time:", round(small_time$time, 3), "seconds\n")

small_maxima_time <- measure_time(find_local_maxima(example_data_5_subject))
cat("  Local maxima time:", round(small_maxima_time$time, 3), "seconds\n")

# Test on larger dataset
cat("\nLarge dataset (19 subjects, 34,890 readings):\n")
large_time <- measure_time(grid(example_data_hall, gap = 15, threshold = 130))
cat("  GRID analysis time:", round(large_time$time, 3), "seconds\n")

large_maxima_time <- measure_time(find_local_maxima(example_data_hall))
cat("  Local maxima time:", round(large_maxima_time$time, 3), "seconds\n")

# Calculate efficiency
efficiency_ratio <- (large_time$time / large_time$result$episode_counts$total_episodes) / 
                    (small_time$time / small_time$result$episode_counts$total_episodes)
cat("\nEfficiency ratio (large/small):", round(efficiency_ratio, 2))

