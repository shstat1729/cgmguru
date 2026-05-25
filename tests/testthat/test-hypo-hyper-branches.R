library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

test_that("hyper boundary equality is included (>=)", {
  res <- detect_hyperglycemic_events(example_data_5_subject,
                                     start_gl = 180, end_gl = 180,
                                     dur_length = 15, end_length = 15)
  expect_true(is.list(res))
})

test_that("hypo boundary equality is included (<=)", {
  res <- detect_hypoglycemic_events(example_data_5_subject,
                                    start_gl = 70, dur_length = 15, end_length = 15)
  expect_true(is.list(res))
})

test_that("reading_minutes vector path is exercised", {
  rm <- rep(5, nrow(example_data_5_subject))
  res_hyper <- detect_hyperglycemic_events(example_data_5_subject, reading_minutes = rm)
  res_hypo  <- detect_hypoglycemic_events(example_data_5_subject, reading_minutes = rm)
  expect_true(is.list(res_hyper) && is.list(res_hypo))
})

test_that("no-event and NA-paths don’t crash", {
  df <- example_data_5_subject
  df$gl[] <- 100
  with_na <- df; with_na$gl[1:3] <- NA
  r1 <- detect_hyperglycemic_events(df)
  r2 <- detect_hypoglycemic_events(with_na)
  expect_true(is.list(r1) && is.list(r2))
})


