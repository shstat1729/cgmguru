# Test script to verify the recovery logic fix
library(cgmguru)

# Create test data for Level 1 Hyperglycemia Event
# start_gl = 180, end_gl = 180, dur_length = 15, end_length = 15
set.seed(123)
n <- 100
time_seq <- seq(as.POSIXct("2024-01-01 00:00:00"), by = 300, length.out = n) # 5-min intervals

# Test case 1: Event with successful recovery
glucose1 <- c(
  rep(170, 10),  # Normal levels
  rep(190, 20),  # Core definition: >180 for 15+ min (20 readings = 100 min)
  rep(175, 20),  # Recovery: ≤180 for 15+ min (20 readings = 100 min)
  rep(170, 50)   # Normal levels
)

test_data1 <- data.frame(
  id = rep("test1", n),
  time = time_seq,
  gl = glucose1
)

# Test case 2: Event with missing data (no recovery)
glucose2 <- c(
  rep(170, 10),  # Normal levels
  rep(190, 20),  # Core definition: >180 for 15+ min
  rep(NA, 30),   # Missing data - no recovery possible
  rep(170, 40)   # Normal levels
)

test_data2 <- data.frame(
  id = rep("test2", n),
  time = time_seq,
  gl = glucose2
)

# Test case 3: Core-only mode (start_gl != end_gl)
# This should use core-only mode: >250 mg/dL for 120 min
glucose3 <- c(
  rep(200, 10),  # Normal levels
  rep(280, 30),  # Core definition: >250 for 120+ min (30 readings = 150 min)
  rep(200, 60)   # Normal levels
)

test_data3 <- data.frame(
  id = rep("test3", n),
  time = time_seq,
  gl = glucose3
)

cat("Testing Level 1 Hyperglycemia Event (start_gl = end_gl = 180):\n")
result1 <- detect_hyperglycemic_events(test_data1, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
print(result1$events_detailed)

cat("\nTesting with missing data (no recovery):\n")
result2 <- detect_hyperglycemic_events(test_data2, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
print(result2$events_detailed)

cat("\nTesting Core-only mode (start_gl = 250, end_gl = 180):\n")
result3 <- detect_hyperglycemic_events(test_data3, start_gl = 250, dur_length = 120, end_length = 15, end_gl = 180)
print(result3$events_detailed)
