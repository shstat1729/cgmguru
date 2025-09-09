# Test script to verify core-only mode requires continuous glucose > 250 mg/dL
library(Rcpp)
sourceCpp("src/detect_hyperglycemic_events.cpp")

# Test Case 1: Continuous glucose > 250 mg/dL for 120+ minutes - SHOULD detect event
cat("=== Test Case 1: Continuous glucose > 250 mg/dL for 120+ minutes ===\n")
test_data1 <- data.frame(
  id = rep("test1", 25),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 24*5, by=5) * 60, # 5-minute intervals
  gl = rep(260, 25)  # All above 250 for 125 minutes (120+ minutes)
)

test_data1$time <- as.numeric(test_data1$time)

result1 <- detect_hyperglycemic_events(test_data1, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Events detailed:")
print(result1$events_detailed)
print("Events total:")
print(result1$events_total)
cat("Number of detected events:", nrow(result1$events_detailed), "\n\n")

# Test Case 2: Glucose > 250 mg/dL for 60 minutes, then drops below 250 - SHOULD NOT detect event
cat("=== Test Case 2: Glucose > 250 mg/dL for 60 minutes, then drops below 250 ===\n")
test_data2 <- data.frame(
  id = rep("test2", 15),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 14*5, by=5) * 60, # 5-minute intervals
  gl = c(rep(260, 12), 200, 210, 220)  # 60 min above 250, then drops below
)

test_data2$time <- as.numeric(test_data2$time)

result2 <- detect_hyperglycemic_events(test_data2, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Events detailed:")
print(result2$events_detailed)
print("Events total:")
print(result2$events_total)
cat("Number of detected events:", nrow(result2$events_detailed), "\n\n")

# Test Case 3: Glucose > 250 mg/dL for 120+ minutes, then drops below 250 - SHOULD detect event
cat("=== Test Case 3: Glucose > 250 mg/dL for 120+ minutes, then drops below 250 ===\n")
test_data3 <- data.frame(
  id = rep("test3", 30),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 29*5, by=5) * 60, # 5-minute intervals
  gl = c(rep(260, 25), 200, 210, 220, 230, 240)  # 125 min above 250, then drops below
)

test_data3$time <- as.numeric(test_data3$time)

result3 <- detect_hyperglycemic_events(test_data3, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Events detailed:")
print(result3$events_detailed)
print("Events total:")
print(result3$events_total)
cat("Number of detected events:", nrow(result3$events_detailed), "\n")
