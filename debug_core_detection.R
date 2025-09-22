# Debug the core detection logic to see why the second event is missed
library(iglu)
library(cgmguru)

# Load the data
data(example_data_5_subject)
example_data_5_subject$time <- as.POSIXct(example_data_5_subject$time, tz="UTC")

cat("=== Debugging Core Detection Logic ===\n")

# Extract Subject 1 data
subject1_data <- example_data_5_subject[example_data_5_subject$id == "Subject 1", ]

# Focus on the problematic area (indices 960-1020)
start_idx <- 960
end_idx <- 1020
debug_data <- subject1_data[start_idx:end_idx, ]

cat(sprintf("Analyzing data from index %d to %d\n", start_idx, end_idx))
cat("Data points:\n")
for (i in 1:nrow(debug_data)) {
  cat(sprintf("Row %d: %s, glucose: %.0f mg/dL\n",
              start_idx + i - 1,
              format(debug_data$time[i], "%Y-%m-%d %H:%M:%S"),
              debug_data$gl[i]))
}

# Manual core detection simulation
cat("\n=== Manual Core Detection Simulation ===\n")
glucose_values <- debug_data$gl
time_values <- debug_data$time
start_gl <- 180
dur_length <- 15

# Find consecutive periods > 180 mg/dL
consecutive_above_180 <- rle(glucose_values > start_gl)

cat("Consecutive periods analysis:\n")
current_pos <- 1
for (i in 1:length(consecutive_above_180$values)) {
  if (consecutive_above_180$values[i]) {
    period_start <- current_pos
    period_end <- current_pos + consecutive_above_180$lengths[i] - 1
    duration_minutes <- as.numeric(difftime(time_values[period_end], time_values[period_start], units = "mins"))
    
    cat(sprintf("Period %d: %s to %s, %d readings, %.1f minutes, glucose: %.0f-%.0f mg/dL\n",
                i,
                format(time_values[period_start], "%Y-%m-%d %H:%M:%S"),
                format(time_values[period_end], "%Y-%m-%d %H:%M:%S"),
                consecutive_above_180$lengths[i],
                duration_minutes,
                min(glucose_values[period_start:period_end]),
                max(glucose_values[period_start:period_end])))
    
    if (duration_minutes >= dur_length) {
      cat("  -> This should be detected as a core event!\n")
      cat(sprintf("  -> Core start index: %d, Core end index: %d\n", 
                  start_idx + period_start - 1, start_idx + period_end - 1))
    } else {
      cat(sprintf("  -> Duration too short (%.1f < %.0f minutes)\n", duration_minutes, dur_length))
    }
  }
  current_pos <- current_pos + consecutive_above_180$lengths[i]
}

# Test the actual detection on this subset
cat("\n=== Testing Detection on Debug Subset ===\n")
debug_result <- detect_hyperglycemic_events(debug_data, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
cat(sprintf("Events detected in debug subset: %d\n", nrow(debug_result$events_detailed)))

if (nrow(debug_result$events_detailed) > 0) {
  cat("Detected events:\n")
  for (i in 1:nrow(debug_result$events_detailed)) {
    event <- debug_result$events_detailed[i, ]
    cat(sprintf("Event %d: %s to %s, start_idx: %d, end_idx: %d\n",
                i,
                format(event$start_time, "%Y-%m-%d %H:%M:%S"),
                format(event$end_time, "%Y-%m-%d %H:%M:%S"),
                event$start_indices, event$end_indices))
  }
}

# The issue might be that the algorithm is working correctly on subsets
# but there's a problem when processing the full dataset
# Let's check if there's an issue with the index mapping

cat("\n=== Checking Index Mapping Issue ===\n")
cat("When we test on the full Subject 1 dataset, the indices should be:\n")
cat("- Event 1: start at 970, end around 982\n")
cat("- Event 2: start at 989, end around 1007\n")

# Get the actual result from full dataset
full_result <- detect_hyperglycemic_events(subject1_data, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)

# Look for events that should be around our target indices
events_near_970 <- full_result$events_detailed[
  full_result$events_detailed$start_indices >= 965 & 
  full_result$events_detailed$start_indices <= 975, ]

events_near_989 <- full_result$events_detailed[
  full_result$events_detailed$start_indices >= 985 & 
  full_result$events_detailed$start_indices <= 995, ]

cat(sprintf("Events near index 970: %d\n", nrow(events_near_970)))
cat(sprintf("Events near index 989: %d\n", nrow(events_near_989)))

if (nrow(events_near_970) > 0) {
  cat("Event near 970:\n")
  print(events_near_970)
}

if (nrow(events_near_989) > 0) {
  cat("Event near 989:\n")
  print(events_near_989)
} else {
  cat("No event detected near index 989 - this confirms the issue!\n")
}

# Let's check if there's a problem with the recovery detection between events
cat("\n=== Checking Recovery Detection Between Events ===\n")
cat("The algorithm should detect recovery between index 982 and 989\n")
cat("Recovery period: 2015-06-11 21:50:07 to 2015-06-11 22:25:07 (35 minutes)\n")
cat("Glucose values: 187, 173, 164, 158, 157, 166, 180, 195 mg/dL\n")
cat("All values except the last are <= 180 mg/dL\n")
cat("This should be sufficient recovery to separate the two events.\n")

# The problem might be in the recovery detection logic
# Let's check what happens if we test with different parameters

cat("\n=== Testing with Different Parameters ===\n")
for (end_length in c(5, 10, 15, 20)) {
  test_result <- detect_hyperglycemic_events(subject1_data, start_gl = 180, dur_length = 15, end_length = end_length, end_gl = 180)
  
  events_near_989 <- test_result$events_detailed[
    test_result$events_detailed$start_indices >= 985 & 
    test_result$events_detailed$start_indices <= 995, ]
  
  cat(sprintf("End length %d: %d total events, %d events near index 989\n",
              end_length, nrow(test_result$events_detailed), nrow(events_near_989)))
}
