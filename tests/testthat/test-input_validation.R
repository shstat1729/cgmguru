library(testthat)
library(cgmguru)

test_that("validate_cgm_data enforces schema and converts types", {
  # Not a data.frame
  expect_error(cgmguru:::validate_cgm_data(1), "Input must be a data frame")

  # Missing required cols
  bad_df <- data.frame(a = 1:3)
  expect_error(cgmguru:::validate_cgm_data(bad_df), "Missing required columns:")

  # Proper conversion of types
  df <- data.frame(
    id = factor(c("x","y")),
    time = c("2024-01-01 00:00:00", "2024-01-01 00:05:00"),
    gl = as.character(c(100, 110)),
    stringsAsFactors = FALSE
  )
  res <- cgmguru:::validate_cgm_data(df)
  expect_true(is.character(res$id))
  expect_true(inherits(res$time, "POSIXct"))
  expect_true(is.numeric(res$gl))

  # Numeric time converts from epoch seconds
  df2 <- data.frame(
    id = c("a","b"),
    time = as.numeric(c(0, 60)),
    gl = c(1, 2),
    stringsAsFactors = FALSE
  )
  res2 <- cgmguru:::validate_cgm_data(df2)
  expect_true(inherits(res2$time, "POSIXct"))

  # Empty df returns typed empty df
  empty_in <- data.frame(id = character(0), time = as.POSIXct(character(0), tz = "UTC"), gl = numeric(0))
  empty_out <- cgmguru:::validate_cgm_data(empty_in)
  expect_equal(nrow(empty_out), 0)
  expect_true(is.character(empty_out$id))
  expect_true(inherits(empty_out$time, "POSIXct"))
  expect_true(is.numeric(empty_out$gl))

  # NA in id/time are removed with warning
  df_na <- data.frame(
    id = c("a", NA),
    time = as.POSIXct(c("2024-01-01 00:00:00", NA), tz = "UTC"),
    gl = c(100, 110),
    stringsAsFactors = FALSE
  )
  expect_warning(res_na <- cgmguru:::validate_cgm_data(df_na), "NA values in id column|NA values in time column")
  expect_equal(nrow(res_na), 1)
})

test_that("tz column coerces to character", {
  df <- data.frame(
    id = c("a","b"),
    time = as.POSIXct(c("2024-01-01 00:00:00","2024-01-01 00:05:00"), tz = "UTC"),
    gl = 1:2,
    tz = factor(c("UTC","EST")),
    stringsAsFactors = FALSE
  )
  out <- cgmguru:::validate_cgm_data(df)
  expect_true(is.character(out$tz))
})

test_that("validate_numeric_param enforces numeric scalar and bounds", {
  expect_error(cgmguru:::validate_numeric_param("a", "p"), "must be a single numeric value")
  expect_error(cgmguru:::validate_numeric_param(c(1,2), "p"), "must be a single numeric value")
  expect_error(cgmguru:::validate_numeric_param(NA_real_, "p"), "cannot be NA")
  expect_error(cgmguru:::validate_numeric_param(-1, "p", min_val = 0), "between 0 and Inf")
  # Accept bounds
  expect_equal(cgmguru:::validate_numeric_param(0, "p", min_val = 0), 0)
  expect_equal(cgmguru:::validate_numeric_param(1, "p", min_val = 0, max_val = 1), 1)
})

test_that("validate_reading_minutes handles NULL, scalar, vector, and errors", {
  # NULL passthrough
  expect_null(cgmguru:::validate_reading_minutes(NULL, 10))
  # Scalar >= 0.1 ok
  expect_equal(cgmguru:::validate_reading_minutes(5, 10), 5)
  # Too small scalar
  expect_error(cgmguru:::validate_reading_minutes(0.01, 10), "reading_minutes")
  # Length mismatch
  expect_error(cgmguru:::validate_reading_minutes(c(1,2), 3), "vector length must match")
  # Non-numeric
  expect_error(cgmguru:::validate_reading_minutes("a", 3), "must be numeric")
})

test_that("validate_intermediary_df accepts data.frame and rejects others", {
  expect_error(cgmguru:::validate_intermediary_df(1), "Input must be a data frame")
  df <- data.frame(x = 1:3)
  expect_identical(cgmguru:::validate_intermediary_df(df), df)
})


