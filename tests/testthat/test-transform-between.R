library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

make_pipeline <- function(df) {
	gap <- 15; threshold <- 130; hours <- 2
	grid_result <- grid(df, gap = gap, threshold = threshold)
	mod_grid_result <- mod_grid(df, start_finder(grid_result$grid_vector), hours = hours, gap = gap)
	max_after <- find_max_after_hours(df, start_finder(mod_grid_result$mod_grid_vector), hours = hours)
	local_maxima <- find_local_maxima(df)
	final_maxima <- find_new_maxima(df, max_after$max_indices, local_maxima$local_maxima_vector)
	list(grid_result = grid_result, final_maxima = final_maxima)
}

test_that("transform_df returns expected columns and non-empty with typical data", {
	pl <- make_pipeline(example_data_5_subject)
	trans <- transform_df(pl$grid_result$episode_start, pl$final_maxima)
	expect_true(is.data.frame(trans))
	expect_true(all(c("id","grid_time","grid_gl","maxima_time","maxima_gl") %in% names(trans)))
	# grid_time <= maxima_time (0 to 4 hours window) if present
	if (nrow(trans) > 0) {
		dh <- as.numeric(difftime(trans$maxima_time, trans$grid_time, units = "hours"))
		expect_true(all(dh >= 0 | is.na(dh)))
		expect_true(all(dh <= 4 + 1e-9 | is.na(dh)))
	}
})

test_that("detect_between_maxima consumes transform_df and returns expected schema", {
	pl <- make_pipeline(example_data_5_subject)
	trans <- transform_df(pl$grid_result$episode_start, pl$final_maxima)
	res <- detect_between_maxima(example_data_5_subject, trans)
	expect_true(is.list(res) || is.data.frame(res))
	expect_true(all(c("results","episode_counts") %in% names(res)))
	expect_true(is.data.frame(res$results))
	# results columns per implementation
	expect_true(all(c("id","grid_time","grid_gl","maxima_time","maxima_glucose","time_to_peak") %in% names(res$results)))
	# time_to_peak should be >= 0 when not NA
	if (nrow(res$results) > 0) {
		ttp <- res$results$time_to_peak
		expect_true(all(is.na(ttp) | ttp >= 0))
	}
})
