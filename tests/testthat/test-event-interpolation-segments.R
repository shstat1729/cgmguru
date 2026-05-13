library(testthat)
library(cgmguru)

make_cgm_at <- function(minutes, gl) {
  data.frame(
    id = "A",
    time = as.POSIXct("2026-01-01 00:15:00", tz = "UTC") + minutes * 60,
    gl = gl
  )
}

test_that("reading_minutes is inferred per id when omitted", {
  df <- make_cgm_at(c(0, 15, 30, 45), c(60, 80, 82, 84))

  res <- detect_hypoglycemic_events(
    df,
    start_gl = 70,
    dur_length = 15,
    end_length = 15,
    return_interpolated = TRUE
  )

  expect_equal(sum(res$events_total$total_events), 1)
  expect_equal(unique(res$interpolated_data$reading_minutes), 15)
  expect_equal(res$events_detailed$start_index[1], 1)
  expect_equal(res$events_detailed$end_index[1], 1)
})

test_that("sort_time is optional and uses C++ sorting when enabled", {
  df <- make_cgm_at(c(0, 5, 10, 15, 20, 25), c(60, 62, 65, 80, 82, 84))
  shuffled <- df[c(1, 3, 2, 4, 5, 6), ]

  expect_error(
    detect_hypoglycemic_events(
      shuffled,
      start_gl = 70,
      dur_length = 15,
      end_length = 15
    ),
    "time must be nondecreasing"
  )

  sorted_res <- detect_hypoglycemic_events(
    shuffled,
    start_gl = 70,
    dur_length = 15,
    end_length = 15,
    sort_time = TRUE
  )
  baseline <- detect_hypoglycemic_events(
    df,
    start_gl = 70,
    dur_length = 15,
    end_length = 15
  )

  expect_equal(sorted_res$events_total$total_events, baseline$events_total$total_events)
  expect_equal(sorted_res$events_detailed$start_time, baseline$events_detailed$start_time)
})

test_that("gaps within inter_gap are linearly interpolated before event detection", {
  df <- make_cgm_at(c(0, 10, 15, 20, 25), c(60, 60, 80, 82, 84))

  res <- detect_hypoglycemic_events(
    df,
    start_gl = 70,
    dur_length = 15,
    end_length = 15,
    return_interpolated = TRUE
  )

  expect_equal(sum(res$events_total$total_events), 1)
  expect_equal(res$events_detailed$end_time[1], df$time[2])
  interpolated_idx <- match(df$time[1] + 5 * 60, res$interpolated_data$time)
  expect_false(is.na(interpolated_idx))
  expect_equal(res$interpolated_data$gl[interpolated_idx], 60)
})

test_that("gaps above inter_gap split segments and do not finalize events", {
  df <- make_cgm_at(c(0, 50, 55, 60, 65), c(60, 60, 80, 82, 84))

  res <- detect_hypoglycemic_events(
    df,
    start_gl = 70,
    dur_length = 15,
    end_length = 15,
    return_interpolated = TRUE
  )

  expect_equal(sum(res$events_total$total_events), 0)
  expect_equal(nrow(res$events_detailed), 0)
  expect_true(any(is.na(res$interpolated_data$gl)))
  expect_equal(sort(unique(res$interpolated_data$segment[res$interpolated_data$segment > 0])), c(1, 2))
})

test_that("detect_all_events can return interpolated data on request", {
  df <- make_cgm_at(c(0, 15, 30, 45), c(50, 80, 82, 84))

  res <- detect_all_events(df, return_interpolated = TRUE)

  expect_true(is.list(res))
  expect_true(is.data.frame(res$CGM_summary_metrics))
  expect_true(is.data.frame(res$events))
  expect_true(is.data.frame(res$events_long))
  expect_true(is.data.frame(res$interpolated_data))
  expect_named(res$interpolated_data, c("id", "time", "gl"))
})

test_that("standalone lv1_excl excludes lv1 episodes overlapping lv2", {
  hyper_df <- make_cgm_at(
    c(0, 5, 10, 15, 20, 25, 30, 35, 40, 45),
    c(190, 195, 200, 170, 165, 160, 260, 265, 270, 170)
  )
  hyper <- detect_hyperglycemic_events(
    hyper_df,
    type = "lv1_excl",
    reading_minutes = 5
  )
  expect_equal(sum(hyper$events_total$total_events), 1)
  expect_equal(nrow(hyper$events_detailed), 1)
  expect_equal(hyper$events_detailed$start_time[1], hyper_df$time[1])

  hypo_df <- make_cgm_at(
    c(0, 5, 10, 15, 20, 25, 30, 35, 40, 45),
    c(60, 62, 65, 80, 82, 84, 50, 51, 52, 80)
  )
  hypo <- detect_hypoglycemic_events(
    hypo_df,
    type = "lv1_excl",
    reading_minutes = 5
  )
  expect_equal(sum(hypo$events_total$total_events), 1)
  expect_equal(nrow(hypo$events_detailed), 1)
  expect_equal(hypo$events_detailed$start_time[1], hypo_df$time[1])
})
