# Test script to verify hyperglycemic event detection in both modes
library(Rcpp)
sourceCpp("src/detect_hyperglycemic_events.cpp")

# Test Case 1: Core-only mode (start_gl != end_gl + 1)
# This should only count time when glucose > 250 mg/dL
cat("=== Test Case 1: Core-only mode (start_gl=250, end_gl=180) ===\n")
test_data1 <- data.frame(
  id = rep("test1", 30),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 29*5, by=5) * 60, # 5-minute intervals
  gl = c(260, 270, 280, 290, 300, 310, 320, 330, 340, 350,  # 50 min above 250
         200, 210, 220, 230, 240, 250, 260, 270, 280, 290,  # 10 min below 250, then 50 min above
         300, 310, 320, 330, 340, 350, 360, 370, 380, 390)  # 50 min above 250 = 150 min total core time
)

test_data1$time <- as.numeric(test_data1$time)

result1 <- detect_hyperglycemic_events(test_data1, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Events detailed:")
print(result1$events_detailed)
print("Events total:")
print(result1$events_total)
cat("Number of detected events:", nrow(result1$events_detailed), "\n\n")

# Test Case 2: Full event mode (start_gl == end_gl + 1)
# This should count time from start to recovery
cat("=== Test Case 2: Full event mode (start_gl=250, end_gl=249) ===\n")
test_data2 <- data.frame(
  id = rep("test2", 30),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 29*5, by=5) * 60, # 5-minute intervals
  gl = c(260, 270, 280, 290, 300, 310, 320, 330, 340, 350,  # 50 min above 250
         200, 210, 220, 230, 240, 250, 260, 270, 280, 290,  # 10 min below 250, then 50 min above
         300, 310, 320, 330, 340, 350, 360, 370, 380, 390)  # 50 min above 250 = 150 min total time
)

test_data2$time <- as.numeric(test_data2$time)

result2 <- detect_hyperglycemic_events(test_data2, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 249)

print("Events detailed:")
print(result2$events_detailed)
print("Events total:")
print(result2$events_total)
cat("Number of detected events:", nrow(result2$events_detailed), "\n\n")

# Test Case 3: Core-only mode with recovery
cat("=== Test Case 3: Core-only mode with recovery ===\n")
test_data3 <- data.frame(
  id = rep("test3", 35),
  time = as.POSIXct("2024-01-01 00:00:00") + seq(0, 34*5, by=5) * 60, # 5-minute intervals
  gl = c(260, 270, 280, 290, 300, 310, 320, 330, 340, 350,  # 50 min above 250
         360, 370, 380, 390, 400, 410, 420, 430, 440, 450,  # 50 min above 250 = 100 min total core time
         460, 470, 480, 490, 500, 510, 520, 530, 540, 550,  # 50 min above 250 = 150 min total core time
         200, 190, 180, 170, 160)  # Recovery: drops to <=180 for 15+ minutes
)

test_data3$time <- as.numeric(test_data3$time)

result3 <- detect_hyperglycemic_events(test_data3, reading_minutes = 5, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180)

print("Events detailed:")
print(result3$events_detailed)
print("Events total:")
print(result3$events_total)
cat("Number of detected events:", nrow(result3$events_detailed), "\n")
