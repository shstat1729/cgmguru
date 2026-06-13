library(testthat)
library(cgmguru)

data(example_data_5_subject, package = "iglu")

# Compare cgmguru's fixed ndays * 24-hour window directly. iglu's manual
# start_date uses calendar-day arithmetic, which shifts across DST in some TZs.
fixed_window_sensor_wear_expected <- function(data, ndays, reading_minutes,
                                             end_date = NULL) {
  expected_count <- ndays * 24 * (60 / reading_minutes)
  by_id <- split(data, data$id, drop = TRUE)

  expected <- lapply(by_id, function(sub_data) {
    sub_data <- sub_data[!is.na(sub_data$time) & !is.na(sub_data$gl), , drop = FALSE]
    sub_data <- sub_data[order(sub_data$time), , drop = FALSE]

    id_end_date <- if (is.null(end_date)) tail(sub_data$time, 1) else end_date
    id_start_date <- id_end_date - ndays * 24 * 60 * 60
    valid_times <- unique(as.numeric(sub_data$time))
    observed_count <- sum(
      valid_times >= as.numeric(id_start_date) &
        valid_times <= as.numeric(id_end_date)
    )

    data.frame(
      id = as.character(sub_data$id[1]),
      sensor_wear_percent = round(100 * observed_count / expected_count, 2),
      ndays = ndays,
      end_date = id_end_date,
      stringsAsFactors = FALSE
    )
  })

  do.call(rbind, expected)
}

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

test_that("sensor_wear calculates fixed-window percent with per-id end dates", {
  cg <- sensor_wear(
    example_data_5_subject,
    ndays = 14,
    reading_minutes = 5
  )

  expected <- fixed_window_sensor_wear_expected(
    example_data_5_subject,
    ndays = 14,
    reading_minutes = 5
  )

  cmp <- merge(
    cg[, c("id", "sensor_wear_percent", "sensor_wear", "ndays", "end_date")],
    expected[, c("id", "sensor_wear_percent", "ndays", "end_date")],
    by = "id",
    suffixes = c("_cg", "_expected")
  )

  expect_equal(cmp$sensor_wear_percent_cg, cmp$sensor_wear_percent_expected,
               tolerance = 1e-8)
  expect_equal(cmp$sensor_wear, cmp$sensor_wear_percent_cg, tolerance = 1e-8)
  expect_equal(cmp$ndays_cg, cmp$ndays_expected, tolerance = 1e-8)
  expect_equal(as.numeric(cmp$end_date_cg), as.numeric(cmp$end_date_expected),
               tolerance = 1e-8)
})

test_that("sensor_wear calculates fixed-window percent with a common end_date", {
  end_date <- max(example_data_5_subject$time, na.rm = TRUE)

  cg <- sensor_wear(
    example_data_5_subject,
    end_date = end_date,
    ndays = 14,
    reading_minutes = 5
  )

  expected <- fixed_window_sensor_wear_expected(
    example_data_5_subject,
    ndays = 14,
    reading_minutes = 5,
    end_date = end_date
  )

  cmp <- merge(
    cg[, c("id", "sensor_wear_percent", "sensor_wear", "end_date")],
    expected[, c("id", "sensor_wear_percent", "end_date")],
    by = "id",
    suffixes = c("_cg", "_expected")
  )

  expect_equal(cmp$sensor_wear_percent_cg, cmp$sensor_wear_percent_expected,
               tolerance = 1e-8)
  expect_equal(cmp$sensor_wear, cmp$sensor_wear_percent_cg, tolerance = 1e-8)
  expect_equal(as.numeric(cmp$end_date_cg), as.numeric(cmp$end_date_expected),
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
