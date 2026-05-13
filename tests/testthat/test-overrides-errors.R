library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

test_that("overrides forward clear messages for bad df", {
  expect_error(detect_all_events(1), "Error in detect_all_events(): Input must be a data frame", fixed = TRUE)
  expect_error(grid(data.frame(a=1), 15, 130), "Error in grid(): Missing required columns", fixed = TRUE)
})

test_that("numeric param validators triggered in overrides", {
  df <- example_data_5_subject
  expect_error(grid(df, gap = -1), "gap must be between 0 and Inf")
  expect_error(maxima_grid(df, hours = -1), "hours must be between 0 and Inf")
  expect_error(detect_hyperglycemic_events(df, start_gl = -1), "start_gl must be between 0 and Inf")
  expect_error(detect_all_events(df, reading_minutes = c(1,2,3)), "vector length must match")
})


