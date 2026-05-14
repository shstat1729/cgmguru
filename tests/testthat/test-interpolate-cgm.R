library(testthat)
library(cgmguru)

make_interp_cgm_at <- function(minutes, gl) {
  data.frame(
    id = "A",
    time = as.POSIXct("2026-01-01 00:15:00", tz = "UTC") + minutes * 60,
    gl = gl
  )
}

iglu_episode_grid <- function(df, dt0 = 5, inter_gap = 45, tz = "UTC") {
  data_ip <- iglu::CGMS2DayByDay(df, dt0 = dt0, inter_gap = inter_gap, tz = tz)
  n_points <- length(as.vector(t(data_ip$gd2d)))
  first_time <- as.POSIXct(data_ip$actual_dates[1], tz = tz) + data_ip$dt0 * 60
  out <- data.frame(
    id = df$id[1],
    time = seq(from = first_time, by = paste(data_ip$dt0, "mins"), length.out = n_points),
    gl = as.vector(t(data_ip$gd2d))
  )
  out[!is.na(out$gl), ]
}

test_that("interpolate_cgm returns the C++ interpolated grid", {
  df <- make_interp_cgm_at(c(0, 10, 15), c(60, 80, 90))

  out <- interpolate_cgm(df, reading_minutes = 5)
  interpolated_idx <- match(df$time[1] + 5 * 60, out$time)

  expect_s3_class(out, "data.frame")
  expect_named(out, c("id", "time", "gl"))
  expect_s3_class(out$time, "POSIXct")
  expect_equal(out$time[1], df$time[1])
  expect_false(is.na(interpolated_idx))
  expect_equal(out$gl[interpolated_idx], 70)
})

test_that("detect_all_events does not return interpolated data", {
	df <- make_interp_cgm_at(c(0, 10, 15, 20), c(60, 80, 82, 84))

	event_result <- detect_all_events(
		df,
		reading_minutes = 5,
		return_interpolated = TRUE
	)

	expect_named(event_result, c("events_long_df", "summary_df"))
	expect_false("interpolated_data" %in% names(event_result))
})

test_that("interpolate_cgm uses iglu midnight-aligned episode grid", {
  skip_if_not_installed("iglu")

  df <- data.frame(
    id = "A",
    time = as.POSIXct("2026-01-01 00:17:12", tz = "UTC") + c(0, 5, 10) * 60,
    gl = c(60, 80, 82)
  )

  standalone <- interpolate_cgm(df, reading_minutes = 5)
  expected <- iglu_episode_grid(df)

  expect_equal(standalone$time, expected$time)
  expect_equal(standalone$gl, expected$gl)
  expect_false(any(is.na(standalone$gl)))
})

test_that("interpolate_cgm respects sort_time and inter_gap gaps", {
  df <- make_interp_cgm_at(c(0, 50, 55), c(60, 100, 110))
  shuffled <- df[c(1, 3, 2), ]

  expect_error(
    interpolate_cgm(shuffled, reading_minutes = 5),
    "time must be nondecreasing"
  )

  out <- interpolate_cgm(
    shuffled,
    reading_minutes = 5,
    sort_time = TRUE,
    inter_gap = 45
  )

  expect_false(any(is.na(out$gl)))
  expect_named(out, c("id", "time", "gl"))
})
