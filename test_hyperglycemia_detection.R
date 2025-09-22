# Test script to investigate Level 1 Hyperglycemia Event detection issue
# Load required libraries
library(iglu)
library(cgmguru)

# Load the data
data(example_data_5_subject)
example_data_5_subject$time <- as.POSIXct(example_data_5_subject$time, tz="UTC")

# Print data structure and examine the specific data points mentioned
cat("=== Data Structure ===\n")
str(example_data_5_subject)
cat("\n=== First few rows ===\n")
print(head(example_data_5_subject, 20))
cat("\n=== Last few rows ===\n")
print(tail(example_data_5_subject, 20))

# Extract Subject 1 data around the time period mentioned (around row 989-1007)
cat("\n=== Subject 1 data around 2015-06-11 22:25:07 to 2015-06-12 00:00:07 ===\n")
subject1_data <- example_data_5_subject[example_data_5_subject$id == "Subject 1", ]
start_time <- as.POSIXct("2015-06-11 22:20:07", tz="UTC")
end_time <- as.POSIXct("2015-06-12 00:05:07", tz="UTC")
target_data <- subject1_data[subject1_data$time >= start_time & subject1_data$time <= end_time, ]
print(target_data)

# Check if there are any glucose values > 180 mg/dL in this period
cat("\n=== Glucose values > 180 mg/dL in target period ===\n")
hyperglycemic_points <- target_data[target_data$gl > 180, ]
print(hyperglycemic_points)

# Check the duration of consecutive values > 180
cat("\n=== Analyzing consecutive values > 180 mg/dL ===\n")
glucose_values <- target_data$gl
time_values <- target_data$time

# Find consecutive periods > 180
in_hyperglycemia <- glucose_values > 180
consecutive_periods <- rle(in_hyperglycemia)

cat("Consecutive periods analysis:\n")
for (i in 1:length(consecutive_periods$values)) {
  if (consecutive_periods$values[i]) {
    start_idx <- sum(consecutive_periods$lengths[1:(i-1)]) + 1
    end_idx <- sum(consecutive_periods$lengths[1:i])
    duration_minutes <- as.numeric(difftime(time_values[end_idx], time_values[start_idx], units = "mins"))
    cat(sprintf("Period %d: %s to %s, duration: %.1f minutes, glucose range: %.0f-%.0f mg/dL\n", 
                i, 
                format(time_values[start_idx], "%Y-%m-%d %H:%M:%S"),
                format(time_values[end_idx], "%Y-%m-%d %H:%M:%S"),
                duration_minutes,
                min(glucose_values[start_idx:end_idx]),
                max(glucose_values[start_idx:end_idx])))
  }
}

# Test the detect_hyperglycemic_events function
cat("\n=== Testing detect_hyperglycemic_events function ===\n")
cat("Testing Level 1 Hyperglycemia Event (≥15 consecutive min of >180 mg/dL)\n")

# Test with the full dataset
event_result <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
cat("Number of events detected:", nrow(event_result$events_detailed), "\n")
if (nrow(event_result$events_detailed) > 0) {
  cat("Events detected:\n")
  print(event_result$events_detailed)
} else {
  cat("No events detected!\n")
}

# Test with just Subject 1 data
cat("\n=== Testing with Subject 1 data only ===\n")
subject1_result <- detect_hyperglycemic_events(subject1_data, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
cat("Number of events detected for Subject 1:", nrow(subject1_result$events_detailed), "\n")
if (nrow(subject1_result$events_detailed) > 0) {
  cat("Events detected for Subject 1:\n")
  print(subject1_result$events_detailed)
}

# Test with the specific time period
cat("\n=== Testing with specific time period ===\n")
period_result <- detect_hyperglycemic_events(target_data, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
cat("Number of events detected in target period:", nrow(period_result$events_detailed), "\n")
if (nrow(period_result$events_detailed) > 0) {
  cat("Events detected in target period:\n")
  print(period_result$events_detailed)
}

# Let's also check what happens with different parameters
cat("\n=== Testing with different parameters ===\n")

# Test with 5-minute duration (should definitely detect)
test_5min <- detect_hyperglycemic_events(target_data, start_gl = 180, dur_length = 5, end_length = 15, end_gl = 180)
cat("With 5-minute duration:", nrow(test_5min$events_detailed), "events\n")

# Test with 10-minute duration
test_10min <- detect_hyperglycemic_events(target_data, start_gl = 180, dur_length = 10, end_length = 15, end_gl = 180)
cat("With 10-minute duration:", nrow(test_10min$events_detailed), "events\n")

# Test with 20-minute duration
test_20min <- detect_hyperglycemic_events(target_data, start_gl = 180, dur_length = 20, end_length = 15, end_gl = 180)
cat("With 20-minute duration:", nrow(test_20min$events_detailed), "events\n")

# Let's also check the raw detection without recovery requirement
cat("\n=== Testing core detection logic ===\n")
cat("Looking for periods where glucose > 180 for at least 15 minutes:\n")

# Manual calculation of consecutive periods > 180
consecutive_above_180 <- rle(glucose_values > 180)
for (i in 1:length(consecutive_above_180$values)) {
  if (consecutive_above_180$values[i]) {
    period_length <- consecutive_above_180$lengths[i]
    start_pos <- sum(consecutive_above_180$lengths[1:(i-1)]) + 1
    end_pos <- sum(consecutive_above_180$lengths[1:i])
    duration_minutes <- as.numeric(difftime(time_values[end_pos], time_values[start_pos], units = "mins"))
    
    cat(sprintf("Period %d: %d consecutive readings, %.1f minutes, glucose: %.0f-%.0f mg/dL\n",
                i, period_length, duration_minutes,
                min(glucose_values[start_pos:end_pos]),
                max(glucose_values[start_pos:end_pos])))
    
    if (duration_minutes >= 15) {
      cat("  -> This should be detected as Level 1 Hyperglycemia!\n")
    }
  }
}
