# Simple test for detect_hypoglycemic_events function
# This is a minimal test to verify basic functionality

# Create a simple test dataset
test_data <- data.frame(
  id = rep("test_patient", 10),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 9*5*60, by = 5*60), # 5-minute intervals
  gl = c(80, 75, 65, 60, 55, 50, 45, 40, 35, 70)  # Hypoglycemic values with recovery
)

cat("Test Data:\n")
print(test_data)

# Expected behavior:
# - Event should start at index 2 (glucose = 65, < 70)
# - Event should end at index 8 (glucose = 35, last value < 70)
# - Duration should be 6 intervals * 5 minutes = 30 minutes
# - Duration below 54 mg/dL should be 4 intervals * 5 minutes = 20 minutes (values 55, 50, 45, 40, 35)

cat("\nExpected Results:\n")
cat("- Event start index: 3 (1-based R index)\n")
cat("- Event end index: 9 (1-based R index)\n")
cat("- Total duration: 30 minutes\n")
cat("- Duration below 54 mg/dL: 20 minutes\n")

# If the function is available, run it
if (exists("detect_hypoglycemic_events")) {
  cat("\nRunning detect_hypoglycemic_events...\n")
  result <- detect_hypoglycemic_events(test_data, reading_minutes = 5, dur_length = 15, end_length = 15, start_gl = 70)
  
  cat("\nResults:\n")
  print(result$events_total)
  print(result$events_detailed)
  
  # Verify indices are 1-based
  if (nrow(result$events_detailed) > 0) {
    cat("\nIndex Verification:\n")
    cat("Start index:", result$events_detailed$start_indices[1], "(should be >= 1)\n")
    cat("End index:", result$events_detailed$end_indices[1], "(should be >= 1)\n")
    cat("Duration below 54 mg/dL:", result$events_detailed$duration_below_54_minutes[1], "minutes\n")
  }
} else {
  cat("\nFunction detect_hypoglycemic_events not available. Please compile the Rcpp code first.\n")
  cat("To compile, run: sourceCpp('src/detect_hypoglycemic_events.cpp')\n")
}
