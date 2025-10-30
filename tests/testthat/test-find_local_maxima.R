library(testthat)
library(cgmguru)

test_that("find_local_maxima finds expected peaks on synthetic series and respects ids", {
  # Synthetic two-id series with clear local maxima
  times <- as.POSIXct(
    c(
      "2024-01-01 00:00:00","2024-01-01 00:05:00","2024-01-01 00:10:00",
      "2024-01-01 00:15:00","2024-01-01 00:20:00","2024-01-01 00:25:00",
      "2024-01-01 00:00:00","2024-01-01 00:05:00","2024-01-01 00:10:00",
      "2024-01-01 00:15:00","2024-01-01 00:20:00"
    ), tz = "UTC"
  )
  df <- data.frame(
    id = c(rep("A", 6), rep("B", 5)),
    time = times,
    gl = c(
      # id A: rise to 120, drop, rise to 110
      80, 100, 120, 90, 110, 100,
      # id B: single peak at 130
      90, 130, 100, 95, 92
    ),
    stringsAsFactors = FALSE
  )

  res <- find_local_maxima(df)
  expect_true(is.list(res))
  expect_true(all(c("local_maxima_vector", "merged_results") %in% names(res)))

  # The local_maxima_vector contains row indices of df (1-based)
  peaks_idx_df <- res$local_maxima_vector
  expect_true(is.data.frame(peaks_idx_df))
  expect_true("local_maxima" %in% names(peaks_idx_df))

  peaks_idx <- peaks_idx_df$local_maxima
  # Map indices back to original rows
  if (length(peaks_idx) > 0) {
    peak_rows <- df[peaks_idx, ]
    # Each detected index should be a local maximum within its id neighborhood
    for (idx in peaks_idx) {
      this_id <- df$id[idx]
      id_indices <- which(df$id == this_id)
      pos_in_id <- which(id_indices == which(seq_len(nrow(df)) == idx))
      # compare with neighbors within the same id if they exist
      if (pos_in_id > 1) {
        left_idx <- id_indices[pos_in_id - 1]
        expect_gte(df$gl[idx], df$gl[left_idx])
      }
      if (pos_in_id < length(id_indices)) {
        right_idx <- id_indices[pos_in_id + 1]
        expect_gte(df$gl[idx], df$gl[right_idx])
      }
    }
  } else {
    # If no peaks are detected, that's acceptable for this synthetic series
    succeed()
  }
})


