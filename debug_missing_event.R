# Debug why the specific event (2015-06-11 22:25:07 to 2015-06-12 00:00:07) is missing
library(iglu)
library(cgmguru)

# Load the data
data(example_data_5_subject)
example_data_5_subject$time <- as.POSIXct(example_data_5_subject$time, tz="UTC")

cat("=== Debugging Missing Event ===\n")

# Extract Subject 1 data
subject1_data <- example_data_5_subject[example_data_5_subject$id == "Subject 1", ]

# Find the specific period
target_start <- as.POSIXct("2015-06-11 22:25:07", tz="UTC")
target_end <- as.POSIXct("2015-06-12 00:00:07", tz="UTC")

cat(sprintf("Looking for data between %s and %s\n", 
            format(target_start, "%Y-%m-%d %H:%M:%S"),
            format(target_end, "%Y-%m-%d %H:%M:%S")))

# Find the actual data points
start_idx <- which.min(abs(subject1_data$time - target_start))
end_idx <- which.min(abs(subject1_data$time - target_end))

cat(sprintf("Closest start index: %d, time: %s, glucose: %.0f mg/dL\n",
            start_idx,
            format(subject1_data$time[start_idx], "%Y-%m-%d %H:%M:%S"),
            subject1_data$gl[start_idx]))

cat(sprintf("Closest end index: %d, time: %s, glucose: %.0f mg/dL\n",
            end_idx,
            format(subject1_data$time[end_idx], "%Y-%m-%d %H:%M:%S"),
            subject1_data$gl[end_idx]))

# Get a wider range to see what's happening
wider_start <- max(1, start_idx - 5)
wider_end <- min(nrow(subject1_data), end_idx + 5)

cat("\n=== Data around the target period ===\n")
period_data <- subject1_data[wider_start:wider_end, ]
for (i in 1:nrow(period_data)) {
  cat(sprintf("Row %d: %s, glucose: %.0f mg/dL\n",
              wider_start + i - 1,
              format(period_data$time[i], "%Y-%m-%d %H:%M:%S"),
              period_data$gl[i]))
}

# Check if there's a recovery period after the hyperglycemia
cat("\n=== Checking for recovery period ===\n")
recovery_start_idx <- end_idx + 1
recovery_end_idx <- min(nrow(subject1_data), end_idx + 10)

if (recovery_start_idx <= nrow(subject1_data)) {
  cat("Checking for recovery (glucose <= 180) after the hyperglycemic period:\n")
  recovery_data <- subject1_data[recovery_start_idx:recovery_end_idx, ]
  for (i in 1:nrow(recovery_data)) {
    cat(sprintf("Row %d: %s, glucose: %.0f mg/dL\n",
                recovery_start_idx + i - 1,
                format(recovery_data$time[i], "%Y-%m-%d %H:%M:%S"),
                recovery_data$gl[i]))
  }
}

# Test the detection algorithm step by step
cat("\n=== Testing detection algorithm step by step ===\n")

# Test with just the specific time period
period_subset <- subject1_data[start_idx:end_idx, ]
cat(sprintf("Testing with %d data points from %s to %s\n",
            nrow(period_subset),
            format(period_subset$time[1], "%Y-%m-%d %H:%M:%S"),
            format(period_subset$time[nrow(period_subset)], "%Y-%m-%d %H:%M:%S")))

# Check if all values are > 180
all_above_180 <- all(period_subset$gl > 180)
cat(sprintf("All values > 180 mg/dL: %s\n", all_above_180))

# Calculate duration
duration_minutes <- as.numeric(difftime(period_subset$time[nrow(period_subset)], period_subset$time[1], units = "mins"))
cat(sprintf("Duration: %.1f minutes\n", duration_minutes))

# Check if duration >= 15 minutes
duration_sufficient <- duration_minutes >= 15
cat(sprintf("Duration >= 15 minutes: %s\n", duration_sufficient))

# Test detection on this subset
cat("\n=== Testing detection on subset ===\n")
subset_result <- detect_hyperglycemic_events(period_subset, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
cat(sprintf("Events detected in subset: %d\n", nrow(subset_result$events_detailed)))

# Test with a larger window that includes potential recovery
cat("\n=== Testing with larger window including recovery ===\n")
extended_start_idx <- max(1, start_idx - 2)
extended_end_idx <- min(nrow(subject1_data), end_idx + 10)
extended_subset <- subject1_data[extended_start_idx:extended_end_idx, ]

extended_result <- detect_hyperglycemic_events(extended_subset, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
cat(sprintf("Events detected in extended subset: %d\n", nrow(extended_result$events_detailed)))

if (nrow(extended_result$events_detailed) > 0) {
  cat("Extended subset events:\n")
  print(extended_result$events_detailed)
}

# Check what happens if we test without recovery requirement (core-only mode)
cat("\n=== Testing core-only mode (start_gl != end_gl) ===\n")
core_result <- detect_hyperglycemic_events(extended_subset, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 181)
cat(sprintf("Events detected in core-only mode: %d\n", nrow(core_result$events_detailed)))

if (nrow(core_result$events_detailed) > 0) {
  cat("Core-only mode events:\n")
  print(core_result$events_detailed)
}

# Let's also check the raw detection markers
cat("\n=== Checking raw detection markers ===\n")
# We need to look at the internal detection logic
# Let's create a simple test to see what the algorithm is doing

# Manual check of consecutive values > 180
glucose_values <- extended_subset$gl
time_values <- extended_subset$time

# Find runs of consecutive values > 180
runs <- rle(glucose_values > 180)
cat("Consecutive runs analysis:\n")
current_pos <- 1
for (i in 1:length(runs$values)) {
  if (runs$values[i]) {
    start_pos <- current_pos
    end_pos <- current_pos + runs$lengths[i] - 1
    duration_minutes <- as.numeric(difftime(time_values[end_pos], time_values[start_pos], units = "mins"))
    cat(sprintf("Run %d: %s to %s, %d readings, %.1f minutes, glucose: %.0f-%.0f mg/dL\n",
                i,
                format(time_values[start_pos], "%Y-%m-%d %H:%M:%S"),
                format(time_values[end_pos], "%Y-%m-%d %H:%M:%S"),
                runs$lengths[i],
                duration_minutes,
                min(glucose_values[start_pos:end_pos]),
                max(glucose_values[start_pos:end_pos])))
    
    if (duration_minutes >= 15) {
      cat("  -> This should be detected as a hyperglycemic event!\n")
      
      # Check for recovery after this run
      if (end_pos < length(glucose_values)) {
        recovery_start <- end_pos + 1
        recovery_candidates <- which(glucose_values[recovery_start:length(glucose_values)] <= 180)
        if (length(recovery_candidates) > 0) {
          recovery_pos <- recovery_start + recovery_candidates[1] - 1
          cat(sprintf("  -> Recovery candidate at %s (glucose: %.0f mg/dL)\n",
                      format(time_values[recovery_pos], "%Y-%m-%d %H:%M:%S"),
                      glucose_values[recovery_pos]))
        } else {
          cat("  -> No recovery found in remaining data\n")
        }
      }
    }
  }
  current_pos <- current_pos + runs$lengths[i]
}
