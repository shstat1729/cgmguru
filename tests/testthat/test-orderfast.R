test_that("orderfast orders by id then time", {
  df <- data.frame(
    id = c("b", "a", "a"),
    time = as.POSIXct(c("2024-01-01 01:00:00", "2024-01-01 00:00:00", "2024-01-01 01:00:00"), tz = "UTC")
  )
  res <- orderfast(df)
  expect_equal(as.character(res$id), c("a", "a", "b"))
  expect_true(res$time[1] <= res$time[2])
})

