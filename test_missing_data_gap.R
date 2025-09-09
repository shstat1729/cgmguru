# Test script to verify event detection with missing data gaps
library(Rcpp)
sourceCpp("src/detect_hyperglycemic_events.cpp")

# Test Case 1: Missing data gap within tolerance (15.1 minutes) - SHOULD detect event
cat("=== Test Case 1: Missing data gap within tolerance (15.1 minutes) ===\n")
test_data1 <- data.frame(
  id = rep("test1", 30),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 29*5, by=5) * 60, # 5-minute intervals
  gl = c(rep(260, 25), rep(NA, 5))  # 125 min above 250, then missing data
)

# Create a gap of 15.1 minutes (within tolerance)
test_data1$time[26:30] <- test_data1$time[25] + seq(5, 25, by=5) * 60  # 5, 10, 15, 20, 25 minutes after last reading

test_data1$time <- as.numeric(test_data1$time)

print("Test data (first 10 and last 10 rows):")
print(head(test_data1, 10))
print(tail(test_data1, 10))

result1 <- detect_hyperglycemic_events(test_data1, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Events detailed:")
print(result1$events_detailed)
print("Events total:")
print(result1$events_total)
cat("Number of detected events:", nrow(result1$events_detailed), "\n\n")

# Test Case 2: Missing data gap beyond tolerance (20 minutes) - SHOULD NOT detect event
cat("=== Test Case 2: Missing data gap beyond tolerance (20 minutes) ===\n")
test_data2 <- data.frame(
  id = rep("test2", 30),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 29*5, by=5) * 60, # 5-minute intervals
  gl = c(rep(260, 25), rep(NA, 5))  # 125 min above 250, then missing data
)

# Create a gap of 20 minutes (beyond tolerance)
test_data2$time[26:30] <- test_data2$time[25] + seq(5, 25, by=5) * 60  # 5, 10, 15, 20, 25 minutes after last reading

test_data2$time <- as.numeric(test_data2$time)

result2 <- detect_hyperglycemic_events(test_data2, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Events detailed:")
print(result2$events_detailed)
print("Events total:")
print(result2$events_total)
cat("Number of detected events:", nrow(result2$events_detailed), "\n\n")

# Test Case 3: Missing data gap within tolerance but insufficient core duration - SHOULD NOT detect event
cat("=== Test Case 3: Missing data gap within tolerance but insufficient core duration ===\n")
test_data3 <- data.frame(
  id = rep("test3", 20),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 19*5, by=5) * 60, # 5-minute intervals
  gl = c(rep(260, 15), rep(NA, 5))  # 75 min above 250, then missing data
)

# Create a gap of 15.1 minutes (within tolerance)
test_data3$time[16:20] <- test_data3$time[15] + seq(5, 25, by=5) * 60  # 5, 10, 15, 20, 25 minutes after last reading

test_data3$time <- as.numeric(test_data3$time)

result3 <- detect_hyperglycemic_events(test_data3, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Events detailed:")
print(result3$events_detailed)
print("Events total:")
print(result3$events_total)
cat("Number of detected events:", nrow(result3$events_detailed), "\n")
