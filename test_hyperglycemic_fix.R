# Test script to verify hyperglycemic event detection fix
library(Rcpp)
sourceCpp("src/detect_hyperglycemic_events.cpp")

# Create test data with consecutive hyperglycemic events
# Scenario 1: glucose goes above 250, stays above for 120+ minutes, then drops to <=180
# Scenario 2: glucose goes above 250, drops to intermediate range (180-250), goes back above 250, then drops to <=180
test_data <- data.frame(
  id = rep("test1", 30),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 29*5, by=5) * 60, # 5-minute intervals
  gl = c(200, 260, 270, 280, 290, 300, 310, 320, 330, 340,  # First event: 260-340 (40 min above 250)
         350, 360, 370, 380, 390, 400, 410, 420, 430, 440,  # Continue: 350-440 (40 min above 250) 
         450, 460, 470, 480, 490, 500, 510, 520, 530, 540)  # Continue: 450-540 (40 min above 250) = 120+ min total
)

# Convert time to numeric (seconds since epoch)
test_data$time <- as.numeric(test_data$time)

# Test the detection
result <- detect_hyperglycemic_events(test_data, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Events detailed:")
print(result$events_detailed)

print("Events total:")
print(result$events_total)

# Check if consecutive events are detected
cat("Number of detected events:", nrow(result$events_detailed), "\n")

# Test with intermediate range scenario
test_data2 <- data.frame(
  id = rep("test2", 35),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 34*5, by=5) * 60, # 5-minute intervals
  gl = c(200, 260, 270, 280, 290, 300, 310, 320, 330, 340,  # First part: 260-340 (40 min above 250)
         350, 360, 370, 380, 390, 400, 410, 420, 430, 440,  # Continue: 350-440 (40 min above 250)
         450, 460, 470, 480, 490, 500, 510, 520, 530, 540,  # Continue: 450-540 (40 min above 250) = 120+ min total
         200, 190, 180, 170, 160)  # Recovery: drops to <=180 for 15+ minutes
)

test_data2$time <- as.numeric(test_data2$time)

result2 <- detect_hyperglycemic_events(test_data2, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("\nTest 2 - With recovery:")
print("Events detailed:")
print(result2$events_detailed)

print("Events total:")
print(result2$events_total)

cat("Number of detected events:", nrow(result2$events_detailed), "\n")
