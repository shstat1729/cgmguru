library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

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

test_that("excursion returns expected components and validates gap", {
  res <- excursion(example_data_5_subject, gap = 15)
  expect_true(is.list(res))
  expect_true(all(c("excursion_vector", "episode_counts", "episode_start") %in% names(res)))

  expect_error(excursion(example_data_5_subject, gap = -1), "gap must be between 0 and Inf")
})


