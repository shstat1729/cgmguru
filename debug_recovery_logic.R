# Debug the exact recovery detection logic
library(iglu)
library(cgmguru)

# Load the data
data(example_data_5_subject)
example_data_5_subject$time <- as.POSIXct(example_data_5_subject$time, tz="UTC")

cat("=== Debugging Recovery Detection Logic ===\n")

# Extract Subject 1 data
subject1_data <- example_data_5_subject[example_data_5_subject$id == "Subject 1", ]

# Focus on the problematic area
start_idx <- 960
end_idx <- 1020
debug_data <- subject1_data[start_idx:end_idx, ]

cat("Data around the recovery period:\n")
for (i in 1:nrow(debug_data)) {
  cat(sprintf("Row %d: %s, glucose: %.0f mg/dL\n",
              start_idx + i - 1,
              format(debug_data$time[i], "%Y-%m-%d %H:%M:%S"),
              debug_data$gl[i]))
}

# Manual recovery detection simulation
cat("\n=== Manual Recovery Detection Simulation ===\n")

# First event core: indices 970-982 (in the subset, this would be indices 11-23)
# Recovery should start from index 983 (subset index 24)
glucose_values <- debug_data$gl
time_values <- debug_data$time
end_gl <- 180
end_length <- 15
reading_minutes <- 5.0

# Simulate the algorithm's logic
event_end_idx <- 23  # This is index 982 in the full dataset (970 + 23 - 1 = 992, wait that's wrong)
# Let me recalculate: subset index 11 corresponds to full index 970, so subset index 23 corresponds to full index 982
# So event_end_idx in subset = 23, and we look for recovery starting from subset index 24

cat(sprintf("Event core ends at subset index: %d (full index: %d)\n", 23, 970 + 23 - 1))
cat("Looking for recovery starting from subset index 24...\n")

recovery_found <- FALSE
recovery_needed_secs <- end_length * 60.0

for (i in 24:nrow(debug_data)) {  # Start from subset index 24
  if (is.na(glucose_values[i])) next
  
  if (glucose_values[i] <= end_gl) {
    cat(sprintf("Found recovery candidate at subset index %d: %s, glucose: %.0f mg/dL\n",
                i,
                format(time_values[i], "%Y-%m-%d %H:%M:%S"),
                glucose_values[i]))
    
    # Check if sustained for end_length minutes
    recovery_start_time <- time_values[i]
    k <- i
    last_recovery_idx <- i
    recovery_broken <- FALSE
    
    while (k + 1 <= nrow(debug_data) && (time_values[k + 1] - recovery_start_time) <= recovery_needed_secs) {
      if (!is.na(glucose_values[k + 1]) && glucose_values[k + 1] > end_gl) {
        recovery_broken <- TRUE
        cat(sprintf("  Recovery broken at subset index %d: %s, glucose: %.0f mg/dL\n",
                    k + 1,
                    format(time_values[k + 1], "%Y-%m-%d %H:%M:%S"),
                    glucose_values[k + 1]))
        break
      }
      last_recovery_idx <- k + 1
      k <- k + 1
    }
    
    sustained_secs <- time_values[last_recovery_idx] - recovery_start_time
    total_recovery_minutes <- sustained_secs / 60.0
    
    cat(sprintf("  Recovery duration: %.1f minutes (needed: %.0f minutes)\n",
                total_recovery_minutes, end_length))
    cat(sprintf("  Recovery broken: %s\n", recovery_broken))
    
    if (!recovery_broken && total_recovery_minutes >= end_length) {
      cat("  *** VALID RECOVERY FOUND! ***\n")
      recovery_found <- TRUE
      break
    } else {
      cat("  Recovery not valid (too short or broken)\n")
    }
  }
}

if (!recovery_found) {
  cat("No valid recovery found - this explains why the second event is not detected!\n")
}

# Let's also check what the actual algorithm does
cat("\n=== Testing Actual Algorithm ===\n")
result <- detect_hyperglycemic_events(debug_data, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
cat(sprintf("Events detected: %d\n", nrow(result$events_detailed)))

if (nrow(result$events_detailed) > 0) {
  for (i in 1:nrow(result$events_detailed)) {
    event <- result$events_detailed[i, ]
    cat(sprintf("Event %d: %s to %s, start_idx: %d, end_idx: %d\n",
                i,
                format(event$start_time, "%Y-%m-%d %H:%M:%S"),
                format(event$end_time, "%Y-%m-%d %H:%M:%S"),
                event$start_indices, event$end_indices))
  }
}
