library(testthat)
library(cgmguru)

make_cgm <- function(gl, start = "2026-01-01 00:00:00", step_mins = 5) {
  data.frame(
    id = "A",
    time = as.POSIXct(start, tz = "UTC") + seq_along(gl) * 0 + (seq_along(gl) - 1) * step_mins * 60,
    gl = gl
  )
}

test_that("hypoglycemic detailed end is last low reading before confirmed recovery", {
  df <- make_cgm(c(60, 62, 65, 80, 82, 84, 86, 88))

  res <- detect_hypoglycemic_events(df, start_gl = 70, dur_length = 15, end_length = 15)


  expect_equal(sum(res$events_total$total_events), 1)
  expect_equal(nrow(res$events_detailed), 1)
  expect_equal(res$events_detailed$start_time[1], df$time[1])
  expect_equal(res$events_detailed$end_time[1], df$time[3])
  expect_equal(res$events_detailed$end_index[1], 3)
})

test_that("hyperglycemic detailed end is last high reading before confirmed recovery", {
  df <- make_cgm(c(190, 195, 200, 170, 165, 160, 155))

  res <- detect_hyperglycemic_events(df, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)

  expect_equal(sum(res$events_total$total_events), 1)
  expect_equal(nrow(res$events_detailed), 1)
  expect_equal(res$events_detailed$start_time[1], df$time[1])
  expect_equal(res$events_detailed$end_time[1], df$time[3])
  expect_equal(res$events_detailed$end_index[1], 3)
})

test_that("extended hyperglycemic detailed end is last high reading before confirmed recovery", {
  cases <- list(
    list(step_mins = 5, gl = c(rep(260, 18), 240, rep(170, 4)), end_index = 19),
    list(step_mins = 15, gl = c(rep(260, 6), 240, rep(170, 3)), end_index = 7)
  )

  for (case in cases) {
    df <- make_cgm(case$gl, step_mins = case$step_mins)

    res <- detect_hyperglycemic_events(df, reading_minutes = case$step_mins)

    expect_equal(sum(res$events_total$total_events), 1)
    expect_equal(nrow(res$events_detailed), 1)
    expect_equal(res$events_detailed$start_time[1], df$time[1])
    expect_equal(res$events_detailed$end_time[1], df$time[case$end_index])
    expect_equal(res$events_detailed$end_glucose[1], df$gl[case$end_index])
    expect_equal(res$events_detailed$start_index[1], 1)
    expect_equal(res$events_detailed$end_index[1], case$end_index)
  }
})

test_that("broken recovery reports the end before final successful recovery", {
  hypo_df <- make_cgm(c(50, 51, 52, 80, 82, 50, 51, 52, 80, 82, 84, 86, 88))
  hypo <- detect_hypoglycemic_events(hypo_df, start_gl = 70, dur_length = 15, end_length = 15)

  expect_equal(sum(hypo$events_total$total_events), 1)
  expect_equal(nrow(hypo$events_detailed), 1)
  expect_equal(hypo$events_detailed$end_time[1], hypo_df$time[8])
  expect_equal(hypo$events_detailed$duration_below_54_minutes[1], 30)

  hyper_df <- make_cgm(c(190, 195, 200, 170, 165, 190, 195, 200, 170, 165, 160, 155))
  hyper <- detect_hyperglycemic_events(hyper_df, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)

  expect_equal(sum(hyper$events_total$total_events), 1)
  expect_equal(nrow(hyper$events_detailed), 1)
  expect_equal(hyper$events_detailed$end_time[1], hyper_df$time[8])
})

test_that("events without confirmed recovery are not counted", {
  hypo_df <- make_cgm(c(60, 62, 65, 80, 82, 60))
  hyper_df <- make_cgm(c(190, 195, 200, 170, 165, 190))

  hypo <- detect_hypoglycemic_events(hypo_df, start_gl = 70, dur_length = 15, end_length = 15)
  hyper <- detect_hyperglycemic_events(hyper_df, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)

  expect_equal(sum(hypo$events_total$total_events), 0)
  expect_equal(nrow(hypo$events_detailed), 0)
  expect_equal(sum(hyper$events_total$total_events), 0)
  expect_equal(nrow(hyper$events_detailed), 0)
})

test_that("detect_all_events keeps counts aligned with standalone confirmed events", {
  hypo_df <- make_cgm(c(50, 51, 52, 80, 82, 84, 86, 88))
  hypo <- detect_hypoglycemic_events(hypo_df, start_gl = 54, dur_length = 15, end_length = 15)
  all_hypo <- detect_all_events(hypo_df, reading_minutes = 5)

  expect_equal(all_hypo$total_episodes[all_hypo$type == "hypo" & all_hypo$level == "lv2"],
               sum(hypo$events_total$total_events))
  expect_equal(all_hypo$avg_episode_duration_below_54[all_hypo$type == "hypo" & all_hypo$level == "lv2"], 15)

  hyper_df <- make_cgm(c(190, 195, 200, 170, 165, 160, 155, 150))
  hyper <- detect_hyperglycemic_events(hyper_df, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
  all_hyper <- detect_all_events(hyper_df, reading_minutes = 5)

  expect_equal(all_hyper$total_episodes[all_hyper$type == "hyper" & all_hyper$level == "lv1"],
               sum(hyper$events_total$total_events))
})

test_that("15-minute sampling detects one-point hypoglycemic event and reports pre-recovery end", {
  df <- make_cgm(c(60, 80, 82, 84), step_mins = 15)

  res <- detect_hypoglycemic_events(df, reading_minutes = 15,
                                    start_gl = 70, dur_length = 15, end_length = 15)

  expect_equal(sum(res$events_total$total_events), 1)
  expect_equal(nrow(res$events_detailed), 1)
  expect_equal(res$events_detailed$start_time[1], df$time[1])
  expect_equal(res$events_detailed$end_time[1], df$time[1])
  expect_equal(res$events_detailed$start_index[1], 1)
  expect_equal(res$events_detailed$end_index[1], 1)
})

test_that("15-minute sampling detects one-point hyperglycemic event and reports pre-recovery end", {
  df <- make_cgm(c(190, 170, 165, 160), step_mins = 15)

  res <- detect_hyperglycemic_events(df, reading_minutes = 15,
                                     start_gl = 180, dur_length = 15,
                                     end_length = 15, end_gl = 180)

  expect_equal(sum(res$events_total$total_events), 1)
  expect_equal(nrow(res$events_detailed), 1)
  expect_equal(res$events_detailed$start_time[1], df$time[1])
  expect_equal(res$events_detailed$end_time[1], df$time[1])
  expect_equal(res$events_detailed$start_index[1], 1)
  expect_equal(res$events_detailed$end_index[1], 1)
})

test_that("15-minute sampling detect_all_events counts one-point events after recovery confirmation", {
  hypo_df <- make_cgm(c(50, 80, 82, 84), step_mins = 15)
  hypo <- detect_hypoglycemic_events(hypo_df, reading_minutes = 15,
                                    start_gl = 54, dur_length = 15, end_length = 15)
  all_hypo <- detect_all_events(hypo_df, reading_minutes = 15)
  expect_equal(all_hypo$total_episodes[all_hypo$type == "hypo" & all_hypo$level == "lv2"],
               sum(hypo$events_total$total_events))
  expect_equal(all_hypo$avg_episode_duration_below_54[all_hypo$type == "hypo" & all_hypo$level == "lv2"],
               15)

  hyper_df <- make_cgm(c(190, 170, 165, 160, 155), step_mins = 15)
  hyper <- detect_hyperglycemic_events(hyper_df, reading_minutes = 15,
                                       start_gl = 180, dur_length = 15,
                                       end_length = 15, end_gl = 180)
  all_hyper <- detect_all_events(hyper_df, reading_minutes = 15)
  expect_equal(all_hyper$total_episodes[all_hyper$type == "hyper" & all_hyper$level == "lv1"],
               sum(hyper$events_total$total_events))
})
