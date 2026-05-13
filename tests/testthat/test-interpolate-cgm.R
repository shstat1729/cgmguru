library(testthat)
library(cgmguru)
make_interp_cgm_at <- function(minutes, gl) {
  data.frame(
    id = "A",
    time = as.POSIXct("2026-01-01 00:15:00", tz = "UTC") + minutes * 60,
    gl = gl
  )
}

test_that("interpolate_cgm returns the C++ interpolated grid", {
  df <- make_interp_cgm_at(c(0, 10, 15), c(60, 80, 90))

  out <- interpolate_cgm(df, reading_minutes = 5)
  interpolated_idx <- match(df$time[1] + 5 * 60, out$time)

  expect_s3_class(out, "data.frame")
  expect_named(out, c("id", "time", "gl", "segment", "reading_minutes"))
  expect_s3_class(out$time, "POSIXct")
  expect_equal(unique(out$reading_minutes), 5)
  expect_equal(out$time[1], df$time[1])
  expect_false(is.na(interpolated_idx))
  expect_equal(out$gl[interpolated_idx], 70)
})

test_that("detect_all_events returns iglu-aligned event interpolation output", {
	df <- make_interp_cgm_at(c(0, 10, 15, 20), c(60, 80, 82, 84))

	event_result <- detect_all_events(
		df,
		reading_minutes = 5,
		return_interpolated = TRUE
	)

	expect_named(event_result$interpolated_data, c("id", "time", "gl"))
	expect_equal(event_result$interpolated_data$time[1],
							 as.POSIXct("2026-01-01 00:05:00", tz = "UTC"))
	expect_true(nrow(event_result$interpolated_data) >= nrow(df))
	expect_equal(
		event_result$interpolated_data$gl[
			match(as.POSIXct("2026-01-01 00:35:00", tz = "UTC"),
						event_result$interpolated_data$time)
		],
		84
	)
})

test_that("interpolate_cgm preserves source-aligned timestamps", {
  df <- data.frame(
    id = "A",
    time = as.POSIXct("2026-01-01 00:17:12", tz = "UTC") + c(0, 5, 10) * 60,
    gl = c(60, 80, 82)
  )

  standalone <- interpolate_cgm(df, reading_minutes = 5)

  expect_equal(standalone$time, df$time)
  expect_equal(standalone$gl, df$gl)
})

test_that("interpolate_cgm respects sort_time and inter_gap segments", {
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

  expect_true(any(is.na(out$gl)))
  expect_equal(sort(unique(out$segment[out$segment > 0])), c(1, 2))
})
