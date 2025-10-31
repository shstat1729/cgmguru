library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

test_that("detect_all_events returns data.frame and validates reading_minutes", {
	res <- detect_all_events(example_data_5_subject)
	expect_true(is.data.frame(res))

	res5 <- detect_all_events(example_data_5_subject, reading_minutes = 5)
	expect_true(is.data.frame(res5))

	expect_error(detect_all_events(example_data_5_subject, reading_minutes = c(5, 5)),
		"reading_minutes vector length must match data length or be a single value")

	# Empty input returns empty data.frame
	empty_cgm <- data.frame(
		id = character(0),
		time = as.POSIXct(character(0), tz = "UTC"),
		gl = numeric(0),
		stringsAsFactors = FALSE
	)
	res_empty <- detect_all_events(empty_cgm)
	expect_true(is.data.frame(res_empty))
	expect_equal(nrow(res_empty), 0)
})

test_that("detect_hypoglycemic_events structure and parameter validation", {
	res <- detect_hypoglycemic_events(example_data_5_subject, start_gl = 70, dur_length = 15, end_length = 15)
	expect_true(is.list(res))
	expect_true(all(c("events_detailed", "events_total") %in% names(res)))
	expect_true(is.data.frame(res$events_detailed))
	expect_true(is.data.frame(res$events_total))

	expect_error(detect_hypoglycemic_events(example_data_5_subject, dur_length = -1), "dur_length must be between 0 and Inf")
	expect_error(detect_hypoglycemic_events(example_data_5_subject, end_length = -1), "end_length must be between 0 and Inf")
	expect_error(detect_hypoglycemic_events(example_data_5_subject, start_gl = -1), "start_gl must be between 0 and Inf")
})

test_that("detect_hyperglycemic_events structure, params and boundary thresholds", {
	# Level 1 boundary at 180
	res_lv1 <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
	expect_true(is.list(res_lv1))
	expect_true(is.data.frame(res_lv1$events_detailed))
	expect_true(is.data.frame(res_lv1$events_total))

	# Level 2 boundary at 250
	res_lv2 <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250)
	expect_true(is.list(res_lv2))
	expect_true(is.data.frame(res_lv2$events_detailed))
	expect_true(is.data.frame(res_lv2$events_total))

	# Parameter validation
	expect_error(detect_hyperglycemic_events(example_data_5_subject, dur_length = -1), "dur_length must be between 0 and Inf")
	expect_error(detect_hyperglycemic_events(example_data_5_subject, start_gl = -1), "start_gl must be between 0 and Inf")

	# Sanity: level 1 should detect at least as many events as stricter level 2
	total_lv1 <- if (nrow(res_lv1$events_total)) sum(res_lv1$events_total$total_events) else 0
	total_lv2 <- if (nrow(res_lv2$events_total)) sum(res_lv2$events_total$total_events) else 0
	expect_true(total_lv1 >= total_lv2)
})
