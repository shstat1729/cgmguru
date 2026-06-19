library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

make_sparse_grid_scope_data <- function() {
  data.frame(
    id = "A",
    time = as.POSIXct("2026-01-01 00:05:00", tz = "UTC") + c(0, 10, 15, 20, 25) * 60,
    gl = c(100, 120, 140, 160, 150)
  )
}

test_that("grid returns expected components and validates params", {
  res <- grid(example_data_5_subject, gap = 15, threshold = 130)
  expect_true(is.list(res))
  expect_true(all(c("grid_vector", "episode_counts", "episode_start") %in% names(res)))
  expect_true(is.data.frame(res$grid_vector))

  expect_error(grid(example_data_5_subject, gap = -1), "gap must be between 0 and Inf")
  expect_error(grid(example_data_5_subject, threshold = -1), "threshold must be between 0 and Inf")
})

test_that("maxima_grid returns expected components and validates", {
  res <- maxima_grid(example_data_5_subject, threshold = 130, gap = 60, hours = 2)
  expect_true(is.list(res))
  expect_true(all(c("results", "episode_counts") %in% names(res)))
  expect_true(is.data.frame(res$results))

  expect_error(maxima_grid(example_data_5_subject, threshold = -1), "threshold must be between 0 and Inf")
  expect_error(maxima_grid(example_data_5_subject, gap = -1), "gap must be between 0 and Inf")
  expect_error(maxima_grid(example_data_5_subject, hours = -1), "hours must be between 0 and Inf")
})

test_that("maxima_grid returns zero episode counts for subjects without episodes", {
  df <- data.frame(
    id = c(rep("short", 3), rep("flat", 4)),
    time = as.POSIXct("2026-01-01 00:00:00", tz = "UTC") +
      c(0, 5, 10, 0, 5, 10, 15) * 60,
    gl = c(100, 101, 102, 100, 101, 102, 103)
  )

  res <- maxima_grid(df, threshold = 300, gap = 15, hours = 2)

  expect_equal(nrow(res$results), 0)
  expect_setequal(res$episode_counts$id, c("flat", "short"))
  expect_equal(
    res$episode_counts$episode_counts[match(c("flat", "short"), res$episode_counts$id)],
    c(0L, 0L)
  )
})

test_that("excursion returns expected components and validates gap", {
  res <- excursion(example_data_5_subject, gap = 15)
  expect_true(is.list(res))
  expect_true(all(c("excursion_vector", "episode_counts", "episode_start") %in% names(res)))

  expect_error(excursion(example_data_5_subject, gap = -1), "gap must be between 0 and Inf")
})

test_that("excursion episode_start includes peak details within 2 hours", {
  df <- data.frame(
    id = "A",
    time = as.POSIXct("2026-01-01 00:00:00", tz = "UTC") + c(0, 30, 60, 90, 120, 150, 180) * 60,
    gl = c(100, 105, 110, 120, 150, 205, 190)
  )

  res <- excursion(df, gap = 15)

  expect_equal(nrow(res$episode_start), 1)
  expect_true(all(
    c("id", "time", "gl", "maxima_time", "maxima_glucose",
      "time_to_peak_min", "index", "maxima_index") %in%
      names(res$episode_start)
  ))
  expect_equal(res$episode_start$time, df$time[4])
  expect_equal(res$episode_start$gl, df$gl[4])
  expect_equal(res$episode_start$maxima_time, df$time[6])
  expect_equal(res$episode_start$maxima_glucose, df$gl[6])
  expect_equal(res$episode_start$time_to_peak_min, 60)
  expect_equal(res$episode_start$index, 4L)
  expect_equal(res$episode_start$maxima_index, 6L)
})

test_that("excursion requires the starting glucose value to be at least 70", {
  df <- data.frame(
    id = "A",
    time = as.POSIXct("2026-01-01 00:00:00", tz = "UTC") + c(0, 30, 60, 90, 120, 150) * 60,
    gl = c(100, 100, 100, 65, 136, 140)
  )

  res <- excursion(df, gap = 15)

  expect_equal(nrow(res$episode_start), 0)
  expect_equal(res$episode_counts$episode_counts, 0L)
})

test_that("GRID and excursion operate on supplied rows, not the event grid", {
  raw <- make_sparse_grid_scope_data()
  interpolated <- interpolate_cgm(raw, reading_minutes = 5)

  expect_gt(nrow(interpolated), nrow(raw))
  expect_equal(nrow(grid(raw, gap = 0, threshold = 105)$grid_vector), nrow(raw))
  expect_equal(nrow(excursion(raw, gap = 0)$excursion_vector), nrow(raw))
  expect_equal(nrow(grid(interpolated, gap = 0, threshold = 105)$grid_vector), nrow(interpolated))
  expect_equal(nrow(excursion(interpolated, gap = 0)$excursion_vector), nrow(interpolated))
})

test_that("maxima_grid uses supplied timestamps unless interpolated data is passed", {
  raw <- make_sparse_grid_scope_data()
  interpolated <- interpolate_cgm(raw, reading_minutes = 5)

  raw_result <- maxima_grid(raw, threshold = 105, gap = 0, hours = 2)
  interpolated_result <- maxima_grid(interpolated, threshold = 105, gap = 0, hours = 2)

  expect_gt(nrow(raw_result$results), 0)
  expect_gt(nrow(interpolated_result$results), 0)
  expect_true(all(raw_result$results$grid_time %in% raw$time))
  expect_true(all(raw_result$results$maxima_time %in% raw$time))
  expect_true(all(interpolated_result$results$grid_time %in% interpolated$time))
  expect_true(all(interpolated_result$results$maxima_time %in% interpolated$time))
  expect_false(identical(
    as.numeric(raw_result$results$grid_time),
    as.numeric(interpolated_result$results$grid_time)
  ))
})
