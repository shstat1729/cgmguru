library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

make_starts <- function(df) {
	gr <- grid(df, gap = 15, threshold = 130)
	start_finder(gr$grid_vector)
}

within_window_after <- function(df, start_idx, target_idx, hours) {
	t0 <- df$time[start_idx]
	t1 <- df$time[target_idx]
	!is.na(t0) && !is.na(t1) && t1 >= t0 && as.numeric(difftime(t1, t0, units = "hours")) <= hours + 1e-9
}

within_window_before <- function(df, start_idx, target_idx, hours) {
	t0 <- df$time[start_idx]
	t1 <- df$time[target_idx]
	!is.na(t0) && !is.na(t1) && t1 <= t0 && as.numeric(difftime(t0, t1, units = "hours")) <= hours + 1e-9
}

test_that("find_max_after_hours returns indices within window", {
	starts <- make_starts(example_data_5_subject)
	res <- find_max_after_hours(example_data_5_subject, starts, hours = 2)
	expect_true(is.list(res) || is.data.frame(res))
	idx <- res$max_indices$max_indices
	if (length(idx) > 0) {
		# Check window for a sample of pairs (align by length)
		k <- min(length(idx), nrow(starts))
		for (i in seq_len(k)) {
			expect_true(within_window_after(example_data_5_subject, starts$start_indices[i], idx[i], 2))
		}
	} else {
		succeed()
	}
})

test_that("find_max_before_hours returns indices within window", {
	starts <- make_starts(example_data_5_subject)
	res <- find_max_before_hours(example_data_5_subject, starts, hours = 2)
	expect_true(is.list(res) || is.data.frame(res))
	idx <- res$max_indices$max_indices
	if (length(idx) > 0) {
		k <- min(length(idx), nrow(starts))
		for (i in seq_len(k)) {
			expect_true(within_window_before(example_data_5_subject, starts$start_indices[i], idx[i], 2))
		}
	} else {
		succeed()
	}
})

test_that("find_min_after_hours returns indices within window", {
	starts <- make_starts(example_data_5_subject)
	res <- find_min_after_hours(example_data_5_subject, starts, hours = 2)
	expect_true(is.list(res) || is.data.frame(res))
	idx <- res$max_indices$max_indices
	if (length(idx) > 0) {
		k <- min(length(idx), nrow(starts))
		for (i in seq_len(k)) {
			expect_true(within_window_after(example_data_5_subject, starts$start_indices[i], idx[i], 2))
		}
	} else {
		succeed()
	}
})

test_that("find_min_before_hours returns indices within window", {
	starts <- make_starts(example_data_5_subject)
	res <- find_min_before_hours(example_data_5_subject, starts, hours = 2)
	expect_true(is.list(res) || is.data.frame(res))
	idx <- res$max_indices$max_indices
	if (length(idx) > 0) {
		k <- min(length(idx), nrow(starts))
		for (i in seq_len(k)) {
			expect_true(within_window_before(example_data_5_subject, starts$start_indices[i], idx[i], 2))
		}
	} else {
		succeed()
	}
})
