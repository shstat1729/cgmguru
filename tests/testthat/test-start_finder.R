library(testthat)
library(cgmguru)

test_that("start_finder works with grid_vector intermediary and handles empty start flags", {
  # Build intermediary via grid
  data(example_data_5_subject, package = "iglu")
  gr <- grid(example_data_5_subject, gap = 15, threshold = 130)
  starts <- start_finder(gr$grid_vector)
  expect_true(is.data.frame(starts))
  expect_true("start_indices" %in% names(starts))

  # Empty start flags -> no indices
  empty_intermediate <- data.frame(flag = integer(0))
  res_empty <- start_finder(empty_intermediate)
  expect_true(is.data.frame(res_empty))
  expect_equal(nrow(res_empty), 0)
})


