# Debug script to understand gap handling
library(Rcpp)
sourceCpp("src/detect_hyperglycemic_events.cpp")

# Test Case 2: Missing data gap beyond tolerance (20 minutes) - SHOULD detect event
cat("=== Debug Test Case 2: Missing data gap beyond tolerance (20 minutes) ===\n")
test_data2 <- data.frame(
  id = rep("test2", 30),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 29*5, by=5) * 60, # 5-minute intervals
  gl = c(rep(260, 25), rep(NA, 5))  # 125 min above 250, then missing data
)

# Create a gap of 20 minutes (beyond tolerance)
test_data2$time[26:30] <- test_data2$time[25] + seq(20, 40, by=5) * 60  # 20, 25, 30, 35, 40 minutes after last reading

test_data2$time <- as.numeric(test_data2$time)

print("Test data (last 10 rows):")
print(tail(test_data2, 10))

print("Gap analysis:")
print(paste("Last valid reading index:", 25))
print(paste("Last valid time:", as.POSIXct(test_data2$time[25], origin="1970-01-01")))
print(paste("First missing time:", as.POSIXct(test_data2$time[26], origin="1970-01-01")))
print(paste("Gap in minutes:", (test_data2$time[26] - test_data2$time[25]) / 60))
print(paste("Gap in seconds:", test_data2$time[26] - test_data2$time[25]))
print(paste("Recovery threshold (15.1 min):", 15.1 * 60, "seconds"))

result2 <- detect_hyperglycemic_events(test_data2, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Events detailed:")
print(result2$events_detailed)
print("Events total:")
print(result2$events_total)
cat("Number of detected events:", nrow(result2$events_detailed), "\n")
