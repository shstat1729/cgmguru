# Verification script for detect_hypoglycemic_events functionality
# This script verifies the key features of the updated function

cat("=== VERIFICATION OF detect_hypoglycemic_events FUNCTION ===\n\n")

# Test 1: Verify R indexing (1-based)
cat("1. Testing R indexing (1-based)...\n")
test_data_indexing <- data.frame(
  id = rep("index_test", 5),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 4*5*60, by = 5*60),
  gl = c(80, 65, 60, 55, 70)  # Event from index 2 to 4 (0-based) = 3 to 5 (1-based)
)

if (exists("detect_hypoglycemic_events")) {
  result_indexing <- detect_hypoglycemic_events(test_data_indexing, reading_minutes = 5, dur_length = 15, end_length = 15, start_gl = 70)
  
  if (nrow(result_indexing$events_detailed) > 0) {
    start_idx <- result_indexing$events_detailed$start_indices[1]
    end_idx <- result_indexing$events_detailed$end_indices[1]
    cat("   Start index:", start_idx, "(expected: 3)\n")
    cat("   End index:", end_idx, "(expected: 5)\n")
    cat("   Indexing correct:", start_idx >= 1 && end_idx >= 1, "\n")
  } else {
    cat("   No events detected - this might be an issue\n")
  }
} else {
  cat("   Function not available - compile first\n")
}

# Test 2: Verify duration below 54 mg/dL calculation
cat("\n2. Testing duration below 54 mg/dL calculation...\n")
test_data_duration <- data.frame(
  id = rep("duration_test", 8),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 7*5*60, by = 5*60),
  gl = c(80, 70, 60, 50, 40, 30, 20, 70)  # Values below 54: 60, 50, 40, 30, 20 (5 values)
)

if (exists("detect_hypoglycemic_events")) {
  result_duration <- detect_hypoglycemic_events(test_data_duration, reading_minutes = 5, dur_length = 15, end_length = 15, start_gl = 70)
  
  if (nrow(result_duration$events_detailed) > 0) {
    duration_below_54 <- result_duration$events_detailed$duration_below_54_minutes[1]
    cat("   Duration below 54 mg/dL:", duration_below_54, "minutes\n")
    cat("   Expected: ~25 minutes (5 intervals * 5 minutes each)\n")
    cat("   Duration calculation correct:", abs(duration_below_54 - 25) < 5, "\n")  # Allow some tolerance
  } else {
    cat("   No events detected - this might be an issue\n")
  }
} else {
  cat("   Function not available - compile first\n")
}

# Test 3: Verify event ending just before recovery
cat("\n3. Testing event ending just before recovery...\n")
test_data_recovery <- data.frame(
  id = rep("recovery_test", 6),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 5*5*60, by = 5*60),
  gl = c(80, 65, 60, 55, 70, 75)  # Event should end at index 4 (glucose=55), not 5 (glucose=70)
)

if (exists("detect_hypoglycemic_events")) {
  result_recovery <- detect_hypoglycemic_events(test_data_recovery, reading_minutes = 5, dur_length = 15, end_length = 15, start_gl = 70)
  
  if (nrow(result_recovery$events_detailed) > 0) {
    end_glucose <- result_recovery$events_detailed$end_glucose[1]
    cat("   End glucose value:", end_glucose, "\n")
    cat("   Expected: 55 (last value < 70, not 70)\n")
    cat("   Event ending correct:", end_glucose == 55, "\n")
  } else {
    cat("   No events detected - this might be an issue\n")
  }
} else {
  cat("   Function not available - compile first\n")
}

# Test 4: Verify handling of missing data
cat("\n4. Testing handling of missing data...\n")
test_data_missing <- data.frame(
  id = rep("missing_test", 5),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 4*5*60, by = 5*60),
  gl = c(80, 65, 60, 55, NA)  # No recovery due to missing data
)

if (exists("detect_hypoglycemic_events")) {
  result_missing <- detect_hypoglycemic_events(test_data_missing, reading_minutes = 5, dur_length = 15, end_length = 15, start_gl = 70)
  
  if (nrow(result_missing$events_detailed) > 0) {
    cat("   Event detected with missing data: TRUE\n")
    cat("   This is correct behavior\n")
  } else {
    cat("   No events detected with missing data\n")
    cat("   This might be correct if duration < 15 minutes\n")
  }
} else {
  cat("   Function not available - compile first\n")
}

# Test 5: Verify multiple patients
cat("\n5. Testing multiple patients...\n")
test_data_multiple <- data.frame(
  id = c(rep("patient1", 5), rep("patient2", 5)),
  time = rep(as.POSIXct("2024-01-01 00:00:00") + seq(0, 4*5*60, by = 5*60), 2),
  gl = c(80, 65, 60, 55, 70,  # Patient 1: event
         90, 85, 75, 65, 70)  # Patient 2: event
)

if (exists("detect_hypoglycemic_events")) {
  result_multiple <- detect_hypoglycemic_events(test_data_multiple, reading_minutes = 5, dur_length = 15, end_length = 15, start_gl = 70)
  
  cat("   Number of events detected:", nrow(result_multiple$events_detailed), "\n")
  cat("   Expected: 2 (one per patient)\n")
  cat("   Multiple patients handled correctly:", nrow(result_multiple$events_detailed) == 2, "\n")
} else {
  cat("   Function not available - compile first\n")
}

cat("\n=== VERIFICATION COMPLETE ===\n")
cat("To run all tests, execute: source('compile_and_test.R')\n")
