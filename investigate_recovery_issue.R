# Investigate the recovery detection issue
library(iglu)
library(cgmguru)

# Load the data
data(example_data_5_subject)
example_data_5_subject$time <- as.POSIXct(example_data_5_subject$time, tz="UTC")

cat("=== Investigating Recovery Detection Issue ===\n")

# The issue: The event is detected in subsets but not in the full dataset
# This suggests a problem with the recovery detection logic

subject1_data <- example_data_5_subject[example_data_5_subject$id == "Subject 1", ]

# Test with the full Subject 1 dataset
cat("=== Testing with full Subject 1 dataset ===\n")
full_result <- detect_hyperglycemic_events(subject1_data, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
cat(sprintf("Events detected in full Subject 1 dataset: %d\n", nrow(full_result$events_detailed)))

# Look at the specific time period in the full results
target_start <- as.POSIXct("2015-06-11 22:25:07", tz="UTC")
target_end <- as.POSIXct("2015-06-12 00:00:07", tz="UTC")

overlapping_events <- full_result$events_detailed[
  full_result$events_detailed$start_time <= target_end & 
  full_result$events_detailed$end_time >= target_start, ]

cat(sprintf("Events overlapping with target period in full dataset: %d\n", nrow(overlapping_events)))

if (nrow(overlapping_events) > 0) {
  cat("Overlapping events:\n")
  print(overlapping_events)
}

# The problem might be that the recovery detection is looking for 15 consecutive minutes <= 180 mg/dL
# But the data shows glucose drops to 179 mg/dL at 00:05:07, then goes back up to 181 mg/dL at 00:20:07
# This might not meet the 15-minute consecutive requirement

cat("\n=== Analyzing Recovery Requirements ===\n")
cat("Looking for 15 consecutive minutes with glucose <= 180 mg/dL after the hyperglycemic period:\n")

# Find the end of the hyperglycemic period (00:00:07, glucose: 183 mg/dL)
end_hyper_idx <- which(subject1_data$time == target_end)
if (length(end_hyper_idx) == 0) {
  end_hyper_idx <- which.min(abs(subject1_data$time - target_end))
}

cat(sprintf("End of hyperglycemic period: %s (glucose: %.0f mg/dL)\n",
            format(subject1_data$time[end_hyper_idx], "%Y-%m-%d %H:%M:%S"),
            subject1_data$gl[end_hyper_idx]))

# Look at the next 20 readings to see the recovery pattern
recovery_start_idx <- end_hyper_idx + 1
recovery_end_idx <- min(nrow(subject1_data), end_hyper_idx + 20)

cat("\nRecovery period analysis (next 20 readings):\n")
if (recovery_start_idx <= nrow(subject1_data)) {
  recovery_data <- subject1_data[recovery_start_idx:recovery_end_idx, ]
  
  # Find consecutive periods <= 180
  recovery_runs <- rle(recovery_data$gl <= 180)
  
  current_pos <- 1
  for (i in 1:length(recovery_runs$values)) {
    if (recovery_runs$values[i]) {
      start_pos <- current_pos
      end_pos <- current_pos + recovery_runs$lengths[i] - 1
      duration_minutes <- as.numeric(difftime(recovery_data$time[end_pos], recovery_data$time[start_pos], units = "mins"))
      
      cat(sprintf("Recovery run %d: %s to %s, %d readings, %.1f minutes, glucose: %.0f-%.0f mg/dL\n",
                  i,
                  format(recovery_data$time[start_pos], "%Y-%m-%d %H:%M:%S"),
                  format(recovery_data$time[end_pos], "%Y-%m-%d %H:%M:%S"),
                  recovery_runs$lengths[i],
                  duration_minutes,
                  min(recovery_data$gl[start_pos:end_pos]),
                  max(recovery_data$gl[start_pos:end_pos])))
      
      if (duration_minutes >= 15) {
        cat("  -> This meets the 15-minute recovery requirement!\n")
      } else {
        cat(sprintf("  -> This does NOT meet the 15-minute requirement (needs %.1f more minutes)\n", 15 - duration_minutes))
      }
    }
    current_pos <- current_pos + recovery_runs$lengths[i]
  }
}

# Test with different end_length parameters to see if that's the issue
cat("\n=== Testing with different end_length parameters ===\n")
for (end_length in c(5, 10, 15, 20)) {
  test_result <- detect_hyperglycemic_events(subject1_data, start_gl = 180, dur_length = 15, end_length = end_length, end_gl = 180)
  
  overlapping_test <- test_result$events_detailed[
    test_result$events_detailed$start_time <= target_end & 
    test_result$events_detailed$end_time >= target_start, ]
  
  cat(sprintf("End length %d minutes: %d events, %d overlapping with target period\n",
              end_length, nrow(test_result$events_detailed), nrow(overlapping_test)))
}

# Let's also check if the issue is with the core detection vs recovery detection
cat("\n=== Testing core vs recovery detection ===\n")

# Test with start_gl != end_gl (core-only mode)
core_test <- detect_hyperglycemic_events(subject1_data, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 181)
core_overlapping <- core_test$events_detailed[
  core_test$events_detailed$start_time <= target_end & 
  core_test$events_detailed$end_time >= target_start, ]

cat(sprintf("Core-only mode (start_gl=180, end_gl=181): %d events, %d overlapping\n",
            nrow(core_test$events_detailed), nrow(core_overlapping)))

if (nrow(core_overlapping) > 0) {
  cat("Core-only overlapping events:\n")
  print(core_overlapping)
}

# The issue seems to be that the recovery detection is too strict
# The algorithm requires 15 consecutive minutes <= 180 mg/dL, but the data shows:
# 00:05:07 - 179 mg/dL (start of potential recovery)
# 00:10:07 - 177 mg/dL  
# 00:15:07 - 180 mg/dL
# 00:20:07 - 181 mg/dL (recovery broken!)

# This gives only 10 minutes of consecutive recovery, not 15 minutes

cat("\n=== Conclusion ===\n")
cat("The issue is that the recovery detection requires 15 consecutive minutes with glucose <= 180 mg/dL,\n")
cat("but the actual data shows only ~10 minutes of consecutive recovery before glucose rises again.\n")
cat("This is why the event is not detected in the full dataset - it fails the recovery requirement.\n")
cat("\nHowever, this might be too strict. The clinical definition typically allows for brief interruptions.\n")
