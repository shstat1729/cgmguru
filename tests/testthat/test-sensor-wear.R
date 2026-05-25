library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

test_that("sensor_wear matches iglu manual active_percent with per-id end dates", {
  cg <- sensor_wear(example_data_5_subject, ndays = 14, reading_minutes = 5)
  ig <- iglu::active_percent(
    example_data_5_subject,
    dt0 = 5,
    range_type = "manual",
    ndays = 14
  )

  cmp <- merge(
    cg[, c("id", "sensor_wear", "ndays", "start_date", "end_date")],
    ig[, c("id", "active_percent", "ndays", "start_date", "end_date")],
    by = "id",
    suffixes = c("_cg", "_ig")
  )

  expect_equal(cmp$sensor_wear, cmp$active_percent, tolerance = 1e-8)
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
    cg[, c("id", "sensor_wear", "start_date", "end_date")],
    ig[, c("id", "active_percent", "start_date", "end_date")],
    by = "id",
    suffixes = c("_cg", "_ig")
  )

  expect_equal(cmp$sensor_wear, cmp$active_percent, tolerance = 1e-8)
  expect_equal(as.numeric(cmp$start_date_cg), as.numeric(cmp$start_date_ig),
               tolerance = 1e-8)
  expect_equal(as.numeric(cmp$end_date_cg), as.numeric(cmp$end_date_ig),
               tolerance = 1e-8)
})

