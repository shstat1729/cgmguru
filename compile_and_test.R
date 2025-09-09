# Compile and test script for detect_hypoglycemic_events
# This script compiles the Rcpp code and runs tests

library(Rcpp)

# Compile the Rcpp function
cat("Compiling detect_hypoglycemic_events.cpp...\n")
sourceCpp("src/detect_hypoglycemic_events.cpp")

# Run the simple test
cat("\nRunning simple test...\n")
source("simple_test_hypo.R")

# Run the comprehensive test
cat("\nRunning comprehensive test...\n")
source("test_hypoglycemic_events.R")

cat("\nAll tests completed!\n")
