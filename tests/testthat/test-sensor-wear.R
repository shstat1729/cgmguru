library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

test_that("sensor_wear defaults to original timestamp span", {
  df <- data.frame(
    id = "A",
    time = as.POSIXct("2026-01-01 00:00:00", tz = "UTC") + c(0, 10) * 60,
    gl = c(100, 120)
  )

  res <- sensor_wear(df, reading_minutes = 5)

  expect_equal(res$sensor_wear_percent, round(100 * 2 / 3, 2), tolerance = 1e-8)
  expect_equal(res$sensor_wear, res$sensor_wear_percent, tolerance = 1e-8)
  expect_true(is.na(res$ndays))
  expect_equal(as.numeric(res$start_date), as.numeric(min(df$time)))
  expect_equal(as.numeric(res$end_date), as.numeric(max(df$time)))
})

test_that("sensor_wear matches iglu manual active_percent with per-id end dates", {
  cg <- sensor_wear(
    example_data_5_subject,
    ndays = 14,
    reading_minutes = 5
  )
  ig <- iglu::active_percent(
    example_data_5_subject,
    dt0 = 5,
    range_type = "manual",
    ndays = 14
  )

  cmp <- merge(
    cg[, c("id", "sensor_wear_percent", "ndays", "start_date", "end_date")],
    ig[, c("id", "active_percent", "ndays", "start_date", "end_date")],
    by = "id",
    suffixes = c("_cg", "_ig")
  )

  expect_equal(cmp$sensor_wear_percent, round(cmp$active_percent, 2), tolerance = 1e-8)
  expect_equal(as.numeric(cmp$start_date_cg), as.numeric(cmp$start_date_ig),
               tolerance = 1e-8)
  expect_equal(as.numeric(cmp$end_date_cg), as.numeric(cmp$end_date_ig),
               tolerance = 1e-8)
})

test_that("sensor_wear matches iglu manual active_percent with a common end_date", {
  end_date <- max(example_data_5_subject$time, na.rm = TRUE)

  cg <- sensor_wear(
    example_data_5_subject,
    end_date = end_date,
    ndays = 14,
    reading_minutes = 5
  )
  ig <- iglu::active_percent(
    example_data_5_subject,
    dt0 = 5,
    range_type = "manual",
    ndays = 14,
    consistent_end_date = end_date
  )

  cmp <- merge(
    cg[, c("id", "sensor_wear_percent", "start_date", "end_date")],
    ig[, c("id", "active_percent", "start_date", "end_date")],
    by = "id",
    suffixes = c("_cg", "_ig")
  )

  expect_equal(cmp$sensor_wear_percent, round(cmp$active_percent, 2), tolerance = 1e-8)
  expect_equal(as.numeric(cmp$start_date_cg), as.numeric(cmp$start_date_ig),
               tolerance = 1e-8)
  expect_equal(as.numeric(cmp$end_date_cg), as.numeric(cmp$end_date_ig),
               tolerance = 1e-8)
})

test_that("sensor_wear validates ndays for fixed-window options", {
  expect_error(
    sensor_wear(example_data_5_subject, ndays = 0),
    "ndays must be between 0.1 and Inf"
  )
  expect_error(
    sensor_wear(example_data_5_subject, end_date = max(example_data_5_subject$time)),
    "end_date requires ndays"
  )
})
