library(testthat)
library(cgmguru)

test_that("conga_rcpp matches iglu conga on example data", {
  skip_if_not_installed("iglu")
  data(example_data_5_subject, package = "iglu")

  cg <- conga_rcpp(example_data_5_subject)
  ig <- iglu::conga(example_data_5_subject)

  expect_equal(as.character(cg$id), as.character(ig$id))
  expect_equal(cg$CONGA, ig$CONGA, tolerance = 1e-8)
})

test_that("modd_rcpp matches iglu modd on example data", {
  skip_if_not_installed("iglu")
  data(example_data_5_subject, package = "iglu")

  for (day_lag in c(1, 2)) {
    cg <- modd_rcpp(example_data_5_subject, lag = day_lag)
    ig <- iglu::modd(example_data_5_subject, lag = day_lag)

    expect_equal(as.character(cg$id), as.character(ig$id), info = day_lag)
    expect_equal(cg$MODD, ig$MODD, tolerance = 1e-8, info = day_lag)
  }
})

test_that("mage_rcpp naive matches iglu mage naive on example data", {
  skip_if_not_installed("iglu")
  data(example_data_5_subject, package = "iglu")

  cg <- mage_rcpp(example_data_5_subject, version = "naive")
  ig <- suppressWarnings(iglu::mage(example_data_5_subject, version = "naive"))

  expect_equal(as.character(cg$id), as.character(ig$id))
  expect_equal(cg$MAGE, ig$MAGE, tolerance = 1e-8)
})

test_that("mage_rcpp moving-average directions match iglu mage", {
  skip_if_not_installed("iglu")
  data(example_data_5_subject, package = "iglu")

  for (direction in c("avg", "service", "max", "plus", "minus")) {
    cg <- mage_rcpp(example_data_5_subject, direction = direction)
    ig <- suppressMessages(iglu::mage(example_data_5_subject, direction = direction))

    expect_equal(as.character(cg$id), as.character(ig$id), info = direction)
    expect_equal(cg$MAGE, ig$MAGE, tolerance = 1e-8, info = direction)
  }
})

test_that("mage_rcpp segment return matches iglu mage segment output", {
  skip_if_not_installed("iglu")
  data(example_data_5_subject, package = "iglu")

  cg <- mage_rcpp(example_data_5_subject, return_type = "df")
  ig <- suppressMessages(iglu::mage(example_data_5_subject, return_type = "df"))

  expect_equal(as.character(cg$id), as.character(ig$id))
  expect_equal(length(cg$MAGE), length(ig$MAGE))

  for (i in seq_along(cg$MAGE)) {
    cg_segment <- as.data.frame(cg$MAGE[[i]])
    ig_segment <- as.data.frame(ig$MAGE[[i]])

    expect_equal(format(cg_segment$start, "%Y-%m-%d %H:%M:%S"),
                 format(ig_segment$start, "%Y-%m-%d %H:%M:%S"))
    expect_equal(format(cg_segment$end, "%Y-%m-%d %H:%M:%S"),
                 format(ig_segment$end, "%Y-%m-%d %H:%M:%S"))
    expect_equal(cg_segment$mage, ig_segment$mage, tolerance = 1e-8)
    expect_equal(cg_segment$plus_or_minus, ig_segment$plus_or_minus)
    expect_equal(cg_segment$first_excursion, ig_segment$first_excursion)
  }
})
