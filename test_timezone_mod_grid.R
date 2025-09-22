# Test script for timezone functionality in mod_grid function
# This script demonstrates how the timezone parameter works with group-by functionality

# Create sample data with different timezones
library(cgmguru)

# Sample data with UTC timezone
set.seed(123)
sample_data <- data.frame(
  id = rep(c("subject1", "subject2"), each = 50),
  time = as.POSIXct(seq(as.POSIXct("2023-01-01 00:00:00", tz = "UTC"), 
                        as.POSIXct("2023-01-01 12:00:00", tz = "UTC"), 
                        length.out = 100), tz = "UTC"),
  gl = c(rnorm(50, mean = 120, sd = 20), rnorm(50, mean = 110, sd = 15))
)

# Create some grid points (every 10th point)
grid_points <- data.frame(grid_point = seq(1, 100, by = 10))

cat("Testing mod_grid with timezone functionality...\n")
cat("Input timezone:", attr(sample_data$time, "tzone"), "\n")

# Test 1: Default UTC output
cat("\n--- Test 1: Default UTC output ---\n")
result1 <- mod_grid(sample_data, grid_points, hours = 2, gap = 15)
cat("Output timezone (episode_start_total):", attr(result1$episode_start_total$time, "tzone"), "\n")
cat("Output timezone (episode_start):", attr(result1$episode_start$time, "tzone"), "\n")

# Test 2: Custom timezone output
cat("\n--- Test 2: Custom timezone output (America/New_York) ---\n")
result2 <- mod_grid(sample_data, grid_points, hours = 2, gap = 15, output_tz = "America/New_York")
cat("Output timezone (episode_start_total):", attr(result2$episode_start_total$time, "tzone"), "\n")
cat("Output timezone (episode_start):", attr(result2$episode_start$time, "tzone"), "\n")

# Test 3: Different input timezone
cat("\n--- Test 3: Different input timezone (Europe/London) ---\n")
sample_data_london <- sample_data
sample_data_london$time <- as.POSIXct(sample_data$time, tz = "Europe/London")
cat("Input timezone:", attr(sample_data_london$time, "tzone"), "\n")

result3 <- mod_grid(sample_data_london, grid_points, hours = 2, gap = 15, output_tz = "Asia/Tokyo")
cat("Output timezone (episode_start_total):", attr(result3$episode_start_total$time, "tzone"), "\n")
cat("Output timezone (episode_start):", attr(result3$episode_start$time, "tzone"), "\n")

# Display some results
cat("\n--- Sample Results ---\n")
if (nrow(result1$episode_start_total) > 0) {
  cat("First few episode start times (UTC output):\n")
  print(head(result1$episode_start_total[, c("id", "time", "gl")], 3))
}

if (nrow(result2$episode_start_total) > 0) {
  cat("\nFirst few episode start times (America/New_York output):\n")
  print(head(result2$episode_start_total[, c("id", "time", "gl")], 3))
}

cat("\nTimezone functionality test completed successfully!\n")
