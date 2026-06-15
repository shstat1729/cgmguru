library(testthat)
library(cgmguru)

make_rebound_cgm <- function(gl, id = "A", start = "2026-01-01 00:15:00",
                             step_mins = 5) {
  data.frame(
    id = id,
    time = as.POSIXct(start, tz = "UTC") +
      (seq_along(gl) - 1) * step_mins * 60,
    gl = gl
  )
}

test_that("rebound_events detects Rhypo bridge after level 1 hyperglycemia", {
  df <- make_rebound_cgm(c(190, 195, 200, 170, 165, 160, 65, 100, 110))

  res <- rebound_events(df, type = "hypo", reading_minutes = 5)

  expect_named(res, c("events_total", "events_detailed", "interpolated_data"))
  expect_equal(res$events_total$total_episodes, 1)
  expect_equal(nrow(res$events_detailed), 1)
  expect_equal(res$events_detailed$type, "hypo")
  expect_equal(res$events_detailed$initial_start_time, df$time[1])
  expect_equal(res$events_detailed$initial_end_time, df$time[3])
  expect_equal(res$events_detailed$start_time, df$time[3])
  expect_equal(res$events_detailed$end_time, df$time[7])
  expect_equal(res$events_detailed$rebound_glucose, 65)
  expect_equal(res$events_detailed$minutes_to_rebound, 20)
  expect_named(res$interpolated_data, c("id", "time", "gl"))
  expect_s3_class(res$interpolated_data$time, "POSIXct")

  later_hypo <- detect_hypoglycemic_events(
    df,
    type = "lv1",
    reading_minutes = 5,
    return_interpolated = FALSE
  )
  expect_equal(sum(later_hypo$events_total$total_episodes), 0)
})

test_that("rebound_events detects Rhyper bridge after level 1 hypoglycemia", {
  df <- make_rebound_cgm(c(60, 62, 65, 80, 82, 84, 190, 120, 115))

  res <- rebound_events(df, type = "hyper", reading_minutes = 5)

  expect_equal(res$events_total$total_episodes, 1)
  expect_equal(nrow(res$events_detailed), 1)
  expect_equal(res$events_detailed$type, "hyper")
  expect_equal(res$events_detailed$initial_start_time, df$time[1])
  expect_equal(res$events_detailed$initial_end_time, df$time[3])
  expect_equal(res$events_detailed$start_time, df$time[3])
  expect_equal(res$events_detailed$end_time, df$time[7])
  expect_equal(res$events_detailed$rebound_glucose, 190)
  expect_equal(res$events_detailed$minutes_to_rebound, 20)

  later_hyper <- detect_hyperglycemic_events(
    df,
    type = "lv1",
    reading_minutes = 5,
    return_interpolated = FALSE
  )
  expect_equal(sum(later_hyper$events_total$total_episodes), 0)
})

test_that("rebound_events applies the 120 minute rebound window inclusively", {
  exact_120 <- make_rebound_cgm(c(190, 195, 200, rep(160, 23), 65))
  after_120 <- make_rebound_cgm(c(190, 195, 200, rep(160, 24), 65))

  exact <- rebound_events(
    exact_120,
    type = "hypo",
    reading_minutes = 5,
    return_interpolated = FALSE
  )
  late <- rebound_events(
    after_120,
    type = "hypo",
    reading_minutes = 5,
    return_interpolated = FALSE
  )

  expect_equal(exact$events_total$total_episodes, 1)
  expect_equal(exact$events_detailed$minutes_to_rebound, 120)
  expect_equal(late$events_total$total_episodes, 0)
  expect_equal(nrow(late$events_detailed), 0)
})

test_that("rebound_events does not bridge across gap-split segments", {
  start <- as.POSIXct("2026-01-01 00:15:00", tz = "UTC")
  df <- data.frame(
    id = "A",
    time = start + c(0, 5, 10, 15, 20, 25, 80) * 60,
    gl = c(190, 195, 200, 170, 165, 160, 65)
  )

  res <- rebound_events(
    df,
    type = "hypo",
    reading_minutes = 5,
    inter_gap = 45,
    return_interpolated = TRUE
  )

  expect_equal(res$events_total$total_episodes, 0)
  expect_equal(nrow(res$events_detailed), 0)
  grid_gaps <- as.numeric(diff(res$interpolated_data$time), units = "secs")
  expect_gt(max(grid_gaps), 45 * 60)
})

test_that("rebound_events raw and preprocessed data sources agree", {
  df <- make_rebound_cgm(c(190, 195, 200, 170, 165, 160, 65, 100, 110))
  preprocessed <- interpolate_cgm(df, reading_minutes = 5)

  raw <- rebound_events(
    df,
    type = "all",
    data_source = "raw",
    reading_minutes = 5
  )
  pre <- rebound_events(
    preprocessed,
    type = "all",
    data_source = "preprocessed",
    reading_minutes = 5
  )

  expect_equal(raw$events_total, pre$events_total)
  expect_equal(raw$events_detailed$type, pre$events_detailed$type)
  expect_equal(raw$events_detailed$start_time, pre$events_detailed$start_time)
  expect_equal(raw$events_detailed$end_time, pre$events_detailed$end_time)
})

test_that("detect_all_events rebound summaries match rebound_events", {
  rhypo <- make_rebound_cgm(
    c(190, 195, 200, 170, 165, 160, 65, 100, 110),
    id = "Rhypo"
  )
  rhyper <- make_rebound_cgm(
    c(60, 62, 65, 80, 82, 84, 190, 120, 115),
    id = "Rhyper"
  )
  df <- rbind(rhypo, rhyper)

  rebound <- rebound_events(
    df,
    type = "all",
    reading_minutes = 5,
    return_interpolated = FALSE
  )
  all_events <- detect_all_events(df, reading_minutes = 5)

  for (id in unique(df$id)) {
    hypo_count <- rebound$events_total$total_episodes[
      rebound$events_total$id == id & rebound$events_total$type == "hypo"
    ]
    hyper_count <- rebound$events_total$total_episodes[
      rebound$events_total$id == id & rebound$events_total$type == "hyper"
    ]

    subject_row <- all_events$subject_summary[all_events$subject_summary$id == id, ]
    expect_equal(subject_row$hypo_rebound_total_episodes, hypo_count)
    expect_equal(subject_row$hyper_rebound_total_episodes, hyper_count)

    event_summary <- all_events$glycemic_event_summary
    long_hypo <- event_summary[
      event_summary$id == id &
        event_summary$type == "hypo" &
        event_summary$level == "rebound",
    ]
    long_hyper <- event_summary[
      event_summary$id == id &
        event_summary$type == "hyper" &
        event_summary$level == "rebound",
    ]
    expect_equal(long_hypo$total_episodes, hypo_count)
    expect_equal(long_hyper$total_episodes, hyper_count)
  }
})

test_that("rebound_events validates inputs", {
  df <- make_rebound_cgm(c(100, 110, 120))

  expect_error(rebound_events(1), "Input must be a data frame")
  expect_error(rebound_events(df, type = "bad"), "should be one of")
  expect_error(rebound_events(df, data_source = "bad"), "should be one of")
  expect_error(rebound_events(df, rebound_minutes = -1), "rebound_minutes")
})
