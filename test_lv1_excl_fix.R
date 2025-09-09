# Test script to verify the lv1_excl fix
library(cgmguru)

# Create test data with overlapping events
set.seed(123)
n_points <- 1000
time_seq <- seq(0, 24*60*60, length.out = n_points)  # 24 hours in seconds

# Create test data with some hyperglycemic events
test_data <- data.frame(
  id = rep("test_patient", n_points),
  time = time_seq,
  gl = rep(100, n_points)  # Start with normal glucose
)

# Add some hyperglycemic events
# Event 1: 181-200 mg/dL (should be hyper lv1 but not lv2)
test_data$gl[100:150] <- 190

# Event 2: 280 mg/dL (should be both hyper lv1 and lv2)
test_data$gl[300:350] <- 280

# Event 3: 200 mg/dL (should be hyper lv1 but not lv2)
test_data$gl[500:550] <- 200

# Event 4: 300 mg/dL (should be both hyper lv1 and lv2)
test_data$gl[700:750] <- 300

# Event 5: 220 mg/dL (should be hyper lv1 but not lv2)
test_data$gl[800:850] <- 220

# Add some hypoglycemic events
# Event 1: 60 mg/dL (should be hypo lv1 but not lv2)
test_data$gl[200:250] <- 60

# Event 2: 50 mg/dL (should be both hypo lv1 and lv2)
test_data$gl[400:450] <- 50

# Event 3: 65 mg/dL (should be hypo lv1 but not lv2)
test_data$gl[600:650] <- 65

# Event 4: 45 mg/dL (should be both hypo lv1 and lv2)
test_data$gl[900:950] <- 45

# Test the detect_all_events function
cat("Testing detect_all_events with lv1_excl fix...\n")
result <- detect_all_events(test_data)

# Print results
print(result)

# Check specific metrics
cat("\n=== HYPERGLYCEMIC EVENTS ===\n")
hyper_results <- result[result$type == "hyper", ]
print(hyper_results)

cat("\n=== HYPOGLYCEMIC EVENTS ===\n")
hypo_results <- result[result$type == "hypo", ]
print(hypo_results)

# Verify lv1_excl calculations
cat("\n=== VERIFICATION ===\n")
hyper_lv1 <- hyper_results[hyper_results$level == "lv1", "total_episodes"]
hyper_lv2 <- hyper_results[hyper_results$level == "lv2", "total_episodes"]
hyper_lv1_excl <- hyper_results[hyper_results$level == "lv1_excl", "total_episodes"]

hypo_lv1 <- hypo_results[hypo_results$level == "lv1", "total_episodes"]
hypo_lv2 <- hypo_results[hypo_results$level == "lv2", "total_episodes"]
hypo_lv1_excl <- hypo_results[hypo_results$level == "lv1_excl", "total_episodes"]

cat("Hyper lv1 events:", hyper_lv1, "\n")
cat("Hyper lv2 events:", hyper_lv2, "\n")
cat("Hyper lv1_excl events:", hyper_lv1_excl, "\n")
cat("Expected hyper lv1_excl (lv1 - lv2):", hyper_lv1 - hyper_lv2, "\n")

cat("\nHypo lv1 events:", hypo_lv1, "\n")
cat("Hypo lv2 events:", hypo_lv2, "\n")
cat("Hypo lv1_excl events:", hypo_lv1_excl, "\n")
cat("Expected hypo lv1_excl (lv1 - lv2):", hypo_lv1 - hypo_lv2, "\n")
