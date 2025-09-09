# Test with small gap to see if gap handling works
library(Rcpp)
sourceCpp("src/detect_hyperglycemic_events.cpp")

# Test with small gap (10 minutes) - should not trigger gap handling
test_data <- data.frame(
  id = rep("test1", 26),
  time = as.POSIXct("2024-01-01 00:00:00") + c(seq(0, 24*5, by=5), 25*5 + 10) * 60, # 0-120 min, then 10 min gap
  gl = c(rep(260, 25), NA)  # 125 min above 250, then 10 min gap
)

test_data$time <- as.numeric(test_data$time)

print("Test data (last 5 rows):")
print(tail(test_data, 5))

print("Gap analysis:")
print(paste("Gap in minutes:", (test_data$time[26] - test_data$time[25]) / 60))
print(paste("Gap in seconds:", test_data$time[26] - test_data$time[25]))
print(paste("Recovery threshold (15.1 min):", 15.1 * 60, "seconds"))

result <- detect_hyperglycemic_events(test_data, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Result:")
print(result)
