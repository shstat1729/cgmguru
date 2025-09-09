# Debug test for hyperglycemic event detection with sufficient duration
library(Rcpp)
sourceCpp("src/detect_hyperglycemic_events.cpp")

# Test data with enough readings to meet 120-minute requirement
# 25 readings * 5 minutes = 125 minutes total
test_data <- data.frame(
  id = rep("test1", 25),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 24*5, by=5) * 60, # 5-minute intervals
  gl = rep(260, 25)  # All above 250 for 125 minutes
)

# Convert time to numeric (seconds since epoch)
test_data$time <- as.numeric(test_data$time)

print("Test data (first 5 rows):")
print(head(test_data, 5))
print(paste("Total duration:", (test_data$time[25] - test_data$time[1]) / 60, "minutes"))

# Test the detection
result <- detect_hyperglycemic_events(test_data, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Result:")
print(result)
