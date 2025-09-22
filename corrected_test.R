# Corrected test script to properly display Level 1 Hyperglycemia Event detection results
library(iglu)
library(cgmguru)

# Load the data
data(example_data_5_subject)
example_data_5_subject$time <- as.POSIXct(example_data_5_subject$time, tz="UTC")

cat("=== Level 1 Hyperglycemia Event Detection Test ===\n")
cat("Criteria: ≥15 consecutive min of >180 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≤180 mg/dL\n\n")

# Level 1 Hyperglycemia Event
event <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)

cat("Number of Level 1 Hyperglycemia Events detected:", nrow(event$events_detailed), "\n\n")

if (nrow(event$events_detailed) > 0) {
  cat("Events detected:\n")
  print(event$events_detailed)
  
  # Show summary statistics
  cat("\n=== Summary Statistics ===\n")
  cat("Total events:", nrow(event$events_detailed), "\n")
  cat("Average duration:", round(mean(event$events_detailed$duration_minutes), 1), "minutes\n")
  cat("Average glucose during events:", round(mean(event$events_detailed$average_glucose), 1), "mg/dL\n")
  
  # Show events by subject
  cat("\n=== Events by Subject ===\n")
  subject_counts <- table(event$events_detailed$id)
  print(subject_counts)
  
} else {
  cat("No Level 1 Hyperglycemia Events detected!\n")
}

# Also show the events_total summary
cat("\n=== Events Summary by Subject ===\n")
print(event$events_total)

# Test other hyperglycemia levels for comparison
cat("\n=== Level 2 Hyperglycemia Event Detection Test ===\n")
event_lv2 <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250)
cat("Number of Level 2 Hyperglycemia Events detected:", nrow(event_lv2$events_detailed), "\n")

cat("\n=== Extended Hyperglycemia Event Detection Test ===\n")
event_extended <- detect_hyperglycemic_events(example_data_5_subject)
cat("Number of Extended Hyperglycemia Events detected:", nrow(event_extended$events_detailed), "\n")
