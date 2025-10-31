library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

test_that("find_new_maxima handles no-maxima gracefully", {
  df <- example_data_5_subject
  df <- df[order(df$id, df$time), ]
  df$gl <- ave(df$gl, df$id, FUN = function(x) seq_along(x))
  loc <- find_local_maxima(df)$local_maxima_vector
  expect_equal(nrow(loc), 0)

  gr <- grid(df, gap = 15, threshold = 500)
  mg <- mod_grid(df, gr$grid_vector, hours = 1, gap = 15)
  max_after <- find_max_after_hours(df, start_finder(mg$mod_grid_vector), hours = 1)
  fnm <- find_new_maxima(df, max_after$max_indices, loc)
  expect_true(is.data.frame(fnm))
})

test_that("find_new_maxima accepts shuffled maxima input without cross-id issues", {
  df <- example_data_5_subject
  loc <- find_local_maxima(df)$local_maxima_vector
  gr <- grid(df, gap = 15, threshold = 130)
  mg <- mod_grid(df, gr$grid_vector, hours = 2, gap = 15)
  max_after <- find_max_after_hours(df, start_finder(mg$mod_grid_vector), hours = 2)
  loc_shuffled <- loc[sample(seq_len(nrow(loc))), , drop = FALSE]
  out <- find_new_maxima(df, max_after$max_indices, loc_shuffled)
  expect_true(is.data.frame(out))
})


