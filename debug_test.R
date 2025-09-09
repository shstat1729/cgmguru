# Debug test for hyperglycemic event detection
library(Rcpp)
sourceCpp("src/detect_hyperglycemic_events.cpp")

# Simple test data - just 3 readings above 250 mg/dL
test_data <- data.frame(
  id = rep("test1", 3),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 2*5, by=5) * 60, # 5-minute intervals
  gl = c(260, 270, 280)  # All above 250
)

# Convert time to numeric (seconds since epoch)
test_data$time <- as.numeric(test_data$time)

print("Test data:")
print(test_data)

# Test the detection
result <- detect_hyperglycemic_events(test_data, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Result:")
print(result)
