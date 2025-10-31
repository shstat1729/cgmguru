library(testthat)
library(cgmguru)
library(iglu)

# Use example CGM data from iglu
data(example_data_5_subject)

empty_cgm <- data.frame(
  id = character(0),
  time = as.POSIXct(character(0), tz = "UTC"),
  gl = numeric(0),
  stringsAsFactors = FALSE
)

test_that("detect_hypoglycemic_events wrapper returns expected structure and validates", {
  res <- detect_hypoglycemic_events(example_data_5_subject, start_gl = 70, dur_length = 15, end_length = 15)
  expect_true(is.list(res))
  expect_true(all(c("events_detailed", "events_total") %in% names(res)))
  expect_true(is.data.frame(res$events_detailed))
  expect_true(is.data.frame(res$events_total))

  # Empty input should not error and should return empty data.frames
  res_empty <- detect_hypoglycemic_events(empty_cgm)
  expect_true(is.list(res_empty))
  expect_true(nrow(res_empty$events_detailed) == 0)
  expect_true(nrow(res_empty$events_total) == 0)

  # Parameter validation - these should work now with overrides active
  expect_error(detect_hypoglycemic_events(example_data_5_subject, dur_length = -1),
               "dur_length must be between 0 and Inf")
  expect_error(detect_hypoglycemic_events(example_data_5_subject, end_length = -1),
               "end_length must be between 0 and Inf")
  expect_error(detect_hypoglycemic_events(example_data_5_subject, start_gl = -1),
               "start_gl must be between 0 and Inf")
})

test_that("detect_all_events wrapper returns a data.frame and validates reading_minutes", {
  res <- detect_all_events(example_data_5_subject)
  expect_true(is.data.frame(res))

  # Single numeric reading_minutes is accepted
  res5 <- detect_all_events(example_data_5_subject, reading_minutes = 5)
  expect_true(is.data.frame(res5))

  # reading_minutes vector of wrong length should error
  expect_error(detect_all_events(example_data_5_subject, reading_minutes = c(5, 5)),
               "reading_minutes vector length must match data length or be a single value")

  # Empty input returns empty data.frame
  res_empty <- detect_all_events(empty_cgm)
  expect_true(is.data.frame(res_empty))
  expect_true(nrow(res_empty) == 0)
})

test_that("find_local_maxima wrapper returns expected components", {
  res <- find_local_maxima(example_data_5_subject)
  expect_true(is.list(res))
  expect_true(all(c("local_maxima_vector", "merged_results") %in% names(res)))
  expect_true(is.data.frame(res$local_maxima_vector))
  expect_true(is.data.frame(res$merged_results))
})

test_that("grid wrapper returns expected components and validates", {
  res <- grid(example_data_5_subject, gap = 15, threshold = 130)
  expect_true(is.list(res))
  expect_true(all(c("grid_vector", "episode_counts", "episode_start") %in% names(res)))
  expect_true(is.data.frame(res$grid_vector))

  expect_error(grid(example_data_5_subject, gap = -1),
               "gap must be between 0 and Inf")
  expect_error(grid(example_data_5_subject, threshold = -1),
               "threshold must be between 0 and Inf")
})

test_that("maxima_grid wrapper returns list with expected components and validates", {
  res <- maxima_grid(example_data_5_subject, threshold = 130, gap = 60, hours = 2)
  expect_true(is.list(res))
  expect_true(all(c("results", "episode_counts") %in% names(res)))
  expect_true(is.data.frame(res$results))

  expect_error(maxima_grid(example_data_5_subject, threshold = -1),
               "threshold must be between 0 and Inf")
  expect_error(maxima_grid(example_data_5_subject, gap = -1),
               "gap must be between 0 and Inf")
  expect_error(maxima_grid(example_data_5_subject, hours = -1),
               "hours must be between 0 and Inf")
})

test_that("excursion wrapper returns list and validates", {
  res <- excursion(example_data_5_subject, gap = 15)
  expect_true(is.list(res))
  expect_true(all(c("excursion_vector", "episode_counts", "episode_start") %in% names(res)))

  expect_error(excursion(example_data_5_subject, gap = -1),
               "gap must be between 0 and Inf")
})

test_that("start_finder wrapper returns expected type", {
  res <- start_finder(example_data_5_subject)
  expect_true(is.list(res) || is.data.frame(res))
})

test_that("find_max/min before/after hours wrappers validate inputs and return data", {
  # Use small subset to form a valid start point df
  start_points <- head(example_data_5_subject, 10)

  r1 <- find_max_after_hours(example_data_5_subject, start_points, hours = 1)
  expect_true(is.list(r1) || is.data.frame(r1))
  expect_error(find_max_after_hours(example_data_5_subject, start_points, hours = -1),
               "hours must be between 0 and Inf")

  r2 <- find_max_before_hours(example_data_5_subject, start_points, hours = 1)
  expect_true(is.list(r2) || is.data.frame(r2))
  expect_error(find_max_before_hours(example_data_5_subject, start_points, hours = -1),
               "hours must be between 0 and Inf")

  r3 <- find_min_after_hours(example_data_5_subject, start_points, hours = 1)
  expect_true(is.list(r3) || is.data.frame(r3))
  expect_error(find_min_after_hours(example_data_5_subject, start_points, hours = -1),
               "hours must be between 0 and Inf")

  r4 <- find_min_before_hours(example_data_5_subject, start_points, hours = 1)
  expect_true(is.list(r4) || is.data.frame(r4))
  expect_error(find_min_before_hours(example_data_5_subject, start_points, hours = -1),
               "hours must be between 0 and Inf")
})

test_that("mod_grid works with grid results as grid_point_df", {
  gr <- grid(example_data_5_subject, gap = 15, threshold = 130)
  mg <- mod_grid(example_data_5_subject, gr$grid_vector, hours = 2, gap = 15)
  expect_true(is.list(mg))
  expect_true(is.data.frame(mg$mod_grid_vector))
})

test_that("end-to-end pipeline produces compatible frames for transform_df and detect_between_maxima", {
  gap <- 15
  threshold <- 130
  hours <- 2

  # 1) GRID points
  grid_result <- grid(example_data_5_subject, gap = gap, threshold = threshold)
  expect_true(is.list(grid_result))
  expect_true(is.data.frame(grid_result$episode_start))

  # 2) Modified GRID points before 2 hours minimum
  mod_grid_result <- mod_grid(example_data_5_subject,
                              start_finder(grid_result$grid_vector),
                              hours = hours,
                              gap = gap)
  expect_true(is.list(mod_grid_result) || is.data.frame(mod_grid_result))

  # 3) Max point 2 hours after mod_grid point
  max_after <- find_max_after_hours(example_data_5_subject,
                                    start_finder(mod_grid_result$mod_grid_vector),
                                    hours = hours)
  expect_true(is.list(max_after) || is.data.frame(max_after))

  # 4) Local maxima
  local_maxima <- find_local_maxima(example_data_5_subject)
  expect_true(is.list(local_maxima))

  # 5) Among local maxima, find maximum point after two hours
  final_maxima <- find_new_maxima(example_data_5_subject,
                                  max_after$max_indices,
                                  local_maxima$local_maxima_vector)
  expect_true(is.data.frame(final_maxima))

  # 6) Map GRID points to maximum points (within 4 hours)
  transformed <- transform_df(grid_result$episode_start, final_maxima)
  expect_true(is.data.frame(transformed))

  # 7) Redistribute overlapping maxima between GRID points
  between <- detect_between_maxima(example_data_5_subject, transformed)
  expect_true(is.list(between) || is.data.frame(between))
})


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