test_that("interval_down averages clock-aligned 15-minute intervals", {
  input <- data.frame(
    id = rep("a", 7),
    time = as.POSIXct(
      "2024-01-01 00:00:00", tz = "America/New_York"
    ) + 5 * 60 * 0:6,
    gl = c(90, 105, 120, 135, 150, 165, 180)
  )

  default_result <- interval_down(input)
  expect_named(default_result, c("id", "time", "gl"))

  result <- interval_down(input, n_observed = TRUE)

  expect_equal(result$id, rep("a", 3))
  expect_equal(
    result$time,
    input$time[1] + c(15, 30, 45) * 60
  )
  expect_equal(result$gl, c(105, 150, 180))
  expect_equal(result$n_observed, c(3L, 3L, 1L))
  expect_s3_class(result$time, "POSIXct")
  expect_identical(attr(result$time, "tzone"), "America/New_York")
})

test_that("interval_down averages the available values when one glucose is missing", {
  input <- data.frame(
    id = rep("a", 6),
    time = as.POSIXct("2024-01-01 00:00:00", tz = "UTC") + 5 * 60 * 0:5,
    gl = c(90, NA_real_, 120, 135, 150, 165)
  )

  result <- interval_down(input, n_observed = TRUE)

  expect_equal(result$id, c("a", "a"))
  expect_equal(result$time, input$time[1] + c(15, 30) * 60)
  expect_equal(result$gl, c(105, 150))
  expect_equal(result$n_observed, c(2L, 3L))
})

test_that("interval_down retains a single glucose value at the interval end", {
  input <- data.frame(
    id = rep("a", 6),
    time = as.POSIXct("2024-01-01 00:00:00", tz = "UTC") + 5 * 60 * 0:5,
    gl = c(NA_real_, NA_real_, 120, 135, 150, 165)
  )

  result <- interval_down(input, n_observed = TRUE)

  expect_equal(result$id, c("a", "a"))
  expect_equal(result$time, input$time[1] + c(15, 30) * 60)
  expect_equal(result$gl, c(120, 150))
  expect_equal(result$n_observed, c(1L, 3L))
})

test_that("interval_down omits a triplet with no glucose values", {
  input <- data.frame(
    id = rep("a", 6),
    time = as.POSIXct("2024-01-01 00:00:00", tz = "UTC") + 5 * 60 * 0:5,
    gl = c(NA_real_, NA_real_, NA_real_, 135, 150, 165)
  )

  result <- interval_down(input, n_observed = TRUE)

  expect_equal(result$id, "a")
  expect_equal(result$time, input$time[1] + 30 * 60)
  expect_equal(result$gl, 150)
  expect_equal(result$n_observed, 3L)
})

test_that("interval_down preserves time bins when a 5-minute row is absent", {
  input <- data.frame(
    id = rep("a", 5),
    time = as.POSIXct("2024-01-01 00:00:00", tz = "UTC") +
      c(0, 5, 15, 20, 25) * 60,
    gl = c(90, 105, 135, 150, 165)
  )

  result <- interval_down(input, n_observed = TRUE)

  expect_equal(result$time, input$time[1] + c(15, 30) * 60)
  expect_equal(result$gl, c(97.5, 150))
  expect_equal(result$n_observed, c(2L, 3L))
})

test_that("interval_down starts a new set of time bins for each subject", {
  input <- data.frame(
    id = c(rep("a", 3), rep("b", 3)),
    time = as.POSIXct("2024-01-01 00:00:00", tz = "UTC") +
      c(0, 5, 10, 0, 5, 10) * 60,
    gl = c(90, 105, 120, 150, 165, 180)
  )

  result <- interval_down(input, n_observed = TRUE)

  expect_equal(result$id, c("a", "b"))
  expect_equal(result$gl, c(105, 165))
  expect_equal(result$n_observed, c(3L, 3L))
})
