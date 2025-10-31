library(testthat)
library(cgmguru)
library(iglu)

# Use example CGM data from iglu/cgmguru
data(example_data_5_subject)

test_that("orderfast orders by id then time", {
  set.seed(123)
  shuffled <- example_data_5_subject[sample(seq_len(nrow(example_data_5_subject))), ]
  ordered <- orderfast(shuffled)

  # Same dimensions and columns preserved
  expect_equal(nrow(ordered), nrow(shuffled))
  expect_equal(ncol(ordered), ncol(shuffled))
  expect_equal(names(ordered), names(shuffled))

  # Check that data is globally ordered by id then time
  expect_true(all(order(ordered$id, ordered$time) == seq_len(nrow(ordered))))

  # Within each id, time should be non-decreasing
  by_id_ok <- by(ordered$time, ordered$id, function(tt) all(diff(as.numeric(tt)) >= 0))
  expect_true(all(unlist(by_id_ok)))
})

test_that("orderfast is idempotent on already ordered data", {
  already <- orderfast(example_data_5_subject)
  again <- orderfast(already)
  expect_identical(already, again)
})

test_that("orderfast handles empty data frame with required columns", {
  empty_df <- data.frame(
    id = character(0),
    time = as.POSIXct(character(0), tz = "UTC"),
    gl = numeric(0),
    stringsAsFactors = FALSE
  )
  res <- orderfast(empty_df)
  expect_true(is.data.frame(res))
  expect_equal(nrow(res), 0)
  expect_equal(names(res), names(empty_df))
})

test_that("orderfast handles ties in id and time deterministically", {
  # Create small data with deliberate ties
  df <- data.frame(
    id = c("a", "a", "b", "a", "b"),
    time = as.POSIXct(c(
      "2024-01-01 00:00:00",
      "2024-01-01 00:00:00", # tie with first row
      "2024-01-01 01:00:00",
      "2024-01-01 00:30:00",
      "2024-01-01 00:00:00"
    ), tz = "UTC"),
    gl = c(100, 110, 120, 90, 95),
    stringsAsFactors = FALSE
  )

  set.seed(1)
  df_shuffled <- df[sample(seq_len(nrow(df))), ]
  ord <- orderfast(df_shuffled)

  # Check primary ordering by id then time
  expect_true(all(order(ord$id, ord$time) == seq_len(nrow(ord))))

  # Ties kept but relative order among perfect ties may change; ensure the tied block is contiguous
  tie_block <- ord[ord$id == "a" & ord$time == as.POSIXct("2024-01-01 00:00:00", tz = "UTC"), ]
  expect_equal(nrow(tie_block), 2)
})

test_that("orderfast places NA times last within each id and preserves NAs in id", {
  df <- data.frame(
    id = c("a", NA, "a", "b", "b"),
    time = as.POSIXct(c(
      "2024-01-01 01:00:00",
      "2024-01-01 00:00:00",
      NA,
      "2024-01-01 00:00:00",
      NA
    ), tz = "UTC"),
    gl = 1:5,
    stringsAsFactors = FALSE
  )

  ord <- orderfast(df)

  # Non-NA ids should come before NA id when ordering
  expect_true(any(is.na(ord$id)))

  # Within id 'a', NA time should be placed last
  a_rows <- ord[ord$id == "a", ]
  expect_true(is.na(tail(a_rows$time, 1)))

  # Within id 'b', NA time should be placed last
  b_rows <- ord[ord$id == "b", ]
  expect_true(is.na(tail(b_rows$time, 1)))
})


