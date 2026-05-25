library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

test_that("per-id tzone mapping attaches and propagates", {
  df <- head(example_data_5_subject, 60)
  df$tz <- rep(c("UTC","EST","PST"), length.out = nrow(df))
  gr <- grid(df, gap = 15, threshold = 130)
  maxa <- find_max_after_hours(df, start_finder(gr$grid_vector), hours = 2)
  expect_true(!is.null(attr(maxa$episode_start, "tzone_by_id")))
  expect_true(!is.null(attr(maxa$episode_counts, "tzone_by_id")))
})

test_that("episode counting runs across multiple ids and boundaries", {
  df <- example_data_5_subject
  gr <- grid(df, gap = 15, threshold = 130)
  mg <- mod_grid(df, gr$grid_vector, hours = 2, gap = 15)
  res <- find_max_after_hours(df, start_finder(mg$mod_grid_vector), hours = 1)
  expect_true(is.data.frame(res$episode_counts))
})


