# Detailed analysis of events_detailed output to identify issues
library(iglu)
library(cgmguru)

# Load the data
data(example_data_5_subject)
example_data_5_subject$time <- as.POSIXct(example_data_5_subject$time, tz="UTC")

cat("=== Detailed Analysis of events_detailed ===\n")

# Get the events
event <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
events_detailed <- event$events_detailed

cat("Total events detected:", nrow(events_detailed), "\n\n")

# Check the first few events in detail
cat("=== First 5 Events Analysis ===\n")
for (i in 1:min(5, nrow(events_detailed))) {
  cat(sprintf("Event %d:\n", i))
  cat(sprintf("  Subject: %s\n", events_detailed$id[i]))
  cat(sprintf("  Start time: %s (glucose: %.0f mg/dL)\n", 
              format(events_detailed$start_time[i], "%Y-%m-%d %H:%M:%S"), 
              events_detailed$start_glucose[i]))
  cat(sprintf("  End time: %s (glucose: %.0f mg/dL)\n", 
              format(events_detailed$end_time[i], "%Y-%m-%d %H:%M:%S"), 
              events_detailed$end_glucose[i]))
  cat(sprintf("  Duration: %.1f minutes\n", events_detailed$duration_minutes[i]))
  cat(sprintf("  Average glucose: %.1f mg/dL\n", events_detailed$average_glucose[i]))
  cat(sprintf("  Start index: %d, End index: %d\n", 
              events_detailed$start_indices[i], events_detailed$end_indices[i]))
  
  # Verify the event by looking at the actual data
  subject_data <- example_data_5_subject[example_data_5_subject$id == events_detailed$id[i], ]
  start_idx <- events_detailed$start_indices[i]
  end_idx <- events_detailed$end_indices[i]
  
  if (start_idx <= nrow(subject_data) && end_idx <= nrow(subject_data)) {
    actual_start_time <- subject_data$time[start_idx]
    actual_end_time <- subject_data$time[end_idx]
    actual_start_glucose <- subject_data$gl[start_idx]
    actual_end_glucose <- subject_data$gl[end_idx]
    
    cat(sprintf("  VERIFICATION:\n"))
    cat(sprintf("    Actual start: %s (glucose: %.0f mg/dL)\n", 
                format(actual_start_time, "%Y-%m-%d %H:%M:%S"), actual_start_glucose))
    cat(sprintf("    Actual end: %s (glucose: %.0f mg/dL)\n", 
                format(actual_end_time, "%Y-%m-%d %H:%M:%S"), actual_end_glucose))
    
    # Check if there's a mismatch
    time_diff_start <- abs(as.numeric(difftime(events_detailed$start_time[i], actual_start_time, units = "secs")))
    time_diff_end <- abs(as.numeric(difftime(events_detailed$end_time[i], actual_end_time, units = "secs")))
    
    if (time_diff_start > 60 || time_diff_end > 60) {
      cat("    *** TIME MISMATCH DETECTED! ***\n")
    }
    if (abs(events_detailed$start_glucose[i] - actual_start_glucose) > 0.1) {
      cat("    *** GLUCOSE MISMATCH DETECTED! ***\n")
    }
  }
  cat("\n")
}

# Check for potential issues in the data
cat("=== Checking for Data Issues ===\n")

# Check if any events have negative duration
negative_duration <- sum(events_detailed$duration_minutes < 0)
cat(sprintf("Events with negative duration: %d\n", negative_duration))

# Check if any events have very short duration
short_duration <- sum(events_detailed$duration_minutes < 15)
cat(sprintf("Events with duration < 15 minutes: %d\n", short_duration))

# Check if any events have very long duration (potential issue)
very_long_duration <- sum(events_detailed$duration_minutes > 1000)
cat(sprintf("Events with duration > 1000 minutes: %d\n", very_long_duration))

# Check for NA values
na_values <- sum(is.na(events_detailed$duration_minutes))
cat(sprintf("Events with NA duration: %d\n", na_values))

# Check average glucose values
avg_gl_issues <- sum(events_detailed$average_glucose < 180, na.rm = TRUE)
cat(sprintf("Events with average glucose < 180 mg/dL: %d\n", avg_gl_issues))

# Check for duplicate events (same start time and subject)
duplicates <- sum(duplicated(events_detailed[, c("id", "start_time")]))
cat(sprintf("Duplicate events (same subject and start time): %d\n", duplicates))

# Look at the specific event mentioned by the user
cat("\n=== Analysis of the Specific Event Mentioned ===\n")
target_time_start <- as.POSIXct("2015-06-11 22:25:07", tz="UTC")
target_time_end <- as.POSIXct("2015-06-12 00:00:07", tz="UTC")

# Find events that overlap with this time period
overlapping_events <- events_detailed[
  events_detailed$start_time <= target_time_end & 
  events_detailed$end_time >= target_time_start & 
  events_detailed$id == "Subject 1", ]

if (nrow(overlapping_events) > 0) {
  cat(sprintf("Found %d overlapping events:\n", nrow(overlapping_events)))
  for (i in 1:nrow(overlapping_events)) {
    cat(sprintf("  Event %d: %s to %s, duration: %.1f min, avg glucose: %.1f mg/dL\n",
                i,
                format(overlapping_events$start_time[i], "%Y-%m-%d %H:%M:%S"),
                format(overlapping_events$end_time[i], "%Y-%m-%d %H:%M:%S"),
                overlapping_events$duration_minutes[i],
                overlapping_events$average_glucose[i]))
  }
} else {
  cat("No overlapping events found for the specific time period mentioned.\n")
}

# Manual verification of the specific period
cat("\n=== Manual Verification of Specific Period ===\n")
subject1_data <- example_data_5_subject[example_data_5_subject$id == "Subject 1", ]
period_data <- subject1_data[
  subject1_data$time >= target_time_start & 
  subject1_data$time <= target_time_end, ]

cat(sprintf("Data points in period: %d\n", nrow(period_data)))
cat("Glucose values in period:\n")
print(period_data$gl)

# Calculate manual duration
if (nrow(period_data) > 1) {
  manual_duration <- as.numeric(difftime(period_data$time[nrow(period_data)], period_data$time[1], units = "mins"))
  cat(sprintf("Manual duration calculation: %.1f minutes\n", manual_duration))
  manual_avg_glucose <- mean(period_data$gl)
  cat(sprintf("Manual average glucose: %.1f mg/dL\n", manual_avg_glucose))
}
