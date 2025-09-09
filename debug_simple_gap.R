# Simple debug test for gap handling
library(Rcpp)
sourceCpp("src/detect_hyperglycemic_events.cpp")

# Very simple test: 3 readings with a gap
test_data <- data.frame(
  id = rep("test1", 3),
  time = as.POSIXct("2024-01-01 00:00:00") + c(0, 5, 25) * 60, # 0, 5, 25 minutes
  gl = c(260, 260, NA)  # 5 min above 250, then 20 min gap
)

test_data$time <- as.numeric(test_data$time)

print("Test data:")
print(test_data)

print("Gap analysis:")
print(paste("Gap in minutes:", (test_data$time[3] - test_data$time[2]) / 60))
print(paste("Gap in seconds:", test_data$time[3] - test_data$time[2]))
print(paste("Recovery threshold (15.1 min):", 15.1 * 60, "seconds"))

result <- detect_hyperglycemic_events(test_data, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Result:")
print(result)
