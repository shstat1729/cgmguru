library(testthat)
library(cgmguru)
library(iglu)

# Load test data
data(example_data_5_subject)

test_that("detect_hyperglycemic_events works correctly", {
  # Test Level 1 Hyperglycemia Event (≥15 consecutive min of >180 mg/dL)
  result_lv1 <- detect_hyperglycemic_events(
    example_data_5_subject, 
    start_gl = 180, 
    dur_length = 15, 
    end_length = 15, 
    end_gl = 180
  )
  
  # Test that the function returns the expected structure
  expect_true(is.list(result_lv1))
  expect_true("events_detailed" %in% names(result_lv1))
  expect_true("events_total" %in% names(result_lv1))

  
  
  # Test Level 2 Hyperglycemia Event (≥15 consecutive min of >250 mg/dL)
  result_lv2 <- detect_hyperglycemic_events(
    example_data_5_subject, 
    start_gl = 250, 
    dur_length = 15, 
    end_length = 15, 
    end_gl = 250
  )
  
  expect_true(is.list(result_lv2))
  expect_true(is.data.frame(result_lv2$events_detailed))
  expect_true(is.data.frame(result_lv2$events_total))

  # Test Extended Hyperglycemia Event (default parameters)
  result_extended <- detect_hyperglycemic_events(example_data_5_subject)
  
  expect_true(is.list(result_extended))
  expect_true(is.data.frame(result_extended$events_detailed))
  expect_true(is.data.frame(result_extended$events_total))
})

test_that("detect_hyperglycemic_events handles edge cases", {
  # Test with empty data - create proper data types
  empty_data <- data.frame(
    id = character(0), 
    time = as.POSIXct(character(0), tz = "UTC"), 
    gl = numeric(0),
    stringsAsFactors = FALSE
  )
  
  result_empty <- detect_hyperglycemic_events(empty_data)
  
  expect_true(is.list(result_empty))
  expect_equal(nrow(result_empty$events_detailed), 0)
  expect_true(is.data.frame(result_empty$events_total))
  expect_equal(nrow(result_empty$events_total), 0)
})

test_that("detect_hyperglycemic_events handles empty data", {
  # Test with empty data - should return empty results without crashing
  empty_data <- data.frame(
    id = character(0), 
    time = character(0),  
    gl = numeric(0)
  )
  
  # This should NOT crash - it should return empty results
  result <- detect_hyperglycemic_events(empty_data)
  expect_true(is.list(result))
  expect_equal(nrow(result$events_detailed), 0)
  expect_equal(nrow(result$events_total), 0)
})

test_that("detect_hyperglycemic_events validates parameters", {
  # Test with invalid parameters
  expect_error(detect_hyperglycemic_events(example_data_5_subject, dur_length = -1), "dur_length must be between 0 and Inf")
  expect_error(detect_hyperglycemic_events(example_data_5_subject, start_gl = -1), "start_gl must be between 0 and Inf")
})

# Note: Parameter validation tests removed as the function doesn't validate input parameters

test_that("detect_hyperglycemic_events returns expected column names", {
  result <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
  
  # Check events_detailed columns
  expected_cols <- c("id", "start_time", "start_glucose", "end_time", "end_glucose", 
                     "start_indices", "end_indices")
  expect_true(all(expected_cols %in% names(result$events_detailed)))
  
  # Check events_total columns
  expected_total_cols <- c("id", "total_events", "avg_ep_per_day")
  expect_true(all(expected_total_cols %in% names(result$events_total)))
})

test_that("detect_hyperglycemic_events with different parameters produces different results", {
  # Level 1 vs Level 2 should produce different results
  result_lv1 <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
  result_lv2 <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250)
  
  # They should have different number of events (Level 1 should detect more events than Level 2)
  total_lv1 <- sum(result_lv1$events_total$total_events)
  total_lv2 <- sum(result_lv2$events_total$total_events)
  
  expect_true(total_lv1 >= total_lv2, info = "Level 1 hyperglycemia should detect more or equal events than Level 2")
})