library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

test_that("detect_all_events returns named tables and validates reading_minutes", {
	res <- detect_all_events(example_data_5_subject)
	expect_true(is.list(res))
	expect_named(res, c("events_long_df", "summary_df"))
	expect_true(is.data.frame(res$events_long_df))
	expect_true(is.data.frame(res$summary_df))
	expect_true(all(c(
		"id", "TIR", "TITR", "TBR70", "TBR54", "TAR180", "TAR250",
		"CV", "SD", "mean_glucose", "GMI", "uGMI", "GRI", "sensor_wear",
		"hypo_lv1_event_count", "hypo_lv2_event_count",
		"hypo_extended_event_count", "hypo_lv1_excl_event_count",
		"hyper_lv1_event_count", "hyper_lv2_event_count",
		"hyper_extended_event_count", "hyper_lv1_excl_event_count"
	) %in% names(res$summary_df)))
	expect_named(res$events_long_df, c(
		"id", "type", "level", "event_count",
		"avg_ep_per_day", "avg_episode_duration_below_54"
	))

	res5 <- detect_all_events(example_data_5_subject, reading_minutes = 5)
	expect_true(is.list(res5))

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
	expect_true(is.list(res_empty))
	expect_named(res_empty, c("events_long_df", "summary_df"))
	expect_equal(nrow(res_empty$events_long_df), 0)
	expect_equal(nrow(res_empty$summary_df), 0)
})

test_that("detect_all_events CGM summary metrics use interpolated data", {
	df <- data.frame(
		id = "A",
		time = as.POSIXct("2026-01-01 00:05:00", tz = "UTC") + c(0, 10) * 60,
		gl = c(100, 200)
	)

	res <- detect_all_events(df, reading_minutes = 5)$summary_df

	expect_equal(nrow(res), 1)
	expect_equal(res$TIR, 100 * 2 / 3, tolerance = 1e-8)
	expect_equal(res$TITR, 100 / 3, tolerance = 1e-8)
	expect_equal(res$TAR180, 100 / 3, tolerance = 1e-8)
	expect_equal(res$mean_glucose, 150, tolerance = 1e-8)
	expect_equal(res$SD, stats::sd(c(100, 150, 200)), tolerance = 1e-8)
	expect_equal(res$CV, stats::sd(c(100, 150, 200)) / 150, tolerance = 1e-8)
	expect_equal(res$GMI, 3.31 + 0.02392 * 150, tolerance = 1e-8)
	expect_equal(res$uGMI, 1 / (15.36 / 150 + 0.0425), tolerance = 1e-8)
	expect_equal(res$GRI, 0.8 * (100 / 3), tolerance = 1e-8)
	expect_equal(res$sensor_wear, 100 * 2 / 3, tolerance = 1e-8)
})

test_that("detect_all_events calculates Glycemia Risk Index components", {
	df <- data.frame(
		id = "A",
		time = as.POSIXct("2026-01-01 00:05:00", tz = "UTC") + 0:4 * 5 * 60,
		gl = c(50, 60, 100, 200, 260)
	)

	res <- detect_all_events(df, reading_minutes = 5)$summary_df

	expect_equal(res$TBR54, 20, tolerance = 1e-8)
	expect_equal(res$TBR70, 40, tolerance = 1e-8)
	expect_equal(res$TAR180, 40, tolerance = 1e-8)
	expect_equal(res$TAR250, 20, tolerance = 1e-8)
	expect_equal(res$GRI, 3.0 * 20 + 2.4 * 20 + 1.6 * 20 + 0.8 * 20,
							 tolerance = 1e-8)
	expect_equal(res$sensor_wear, 100, tolerance = 1e-8)
})

test_that("detect_hypoglycemic_events structure and parameter validation", {
	res <- detect_hypoglycemic_events(example_data_5_subject, start_gl = 70, dur_length = 15, end_length = 15)
	res_lv1_type <- detect_hypoglycemic_events(example_data_5_subject, type = "lv1")
	expect_true(is.list(res))
	expect_true(all(c("events_detailed", "events_total") %in% names(res)))
	expect_true(is.data.frame(res$events_detailed))
	expect_true(is.data.frame(res$events_total))
	expect_equal(res_lv1_type, res)

	res_lv2 <- detect_hypoglycemic_events(example_data_5_subject, start_gl = 54, dur_length = 15, end_length = 15)
	res_lv2_type <- detect_hypoglycemic_events(example_data_5_subject, type = "lv2")
	expect_equal(res_lv2_type, res_lv2)

	res_extended <- detect_hypoglycemic_events(example_data_5_subject)
	res_extended_type <- detect_hypoglycemic_events(example_data_5_subject, type = "extended")
	expect_equal(res_extended_type, res_extended)

	expect_error(detect_hypoglycemic_events(example_data_5_subject, dur_length = -1), "dur_length must be between 0 and Inf")
	expect_error(detect_hypoglycemic_events(example_data_5_subject, end_length = -1), "end_length must be between 0 and Inf")
	expect_error(detect_hypoglycemic_events(example_data_5_subject, start_gl = -1), "start_gl must be between 0 and Inf")
	expect_error(detect_hypoglycemic_events(example_data_5_subject, type = "bad"), "should be one of")
	expect_warning(
		res_type_wins <- detect_hypoglycemic_events(example_data_5_subject, type = "lv1", start_gl = 54),
		"using type and ignoring"
	)
	expect_equal(res_type_wins, res_lv1_type)
})

test_that("detect_hyperglycemic_events structure, params and boundary thresholds", {
	# Level 1 boundary at 180
	res_lv1 <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)
	res_lv1_type <- detect_hyperglycemic_events(example_data_5_subject, type = "lv1")
	expect_true(is.list(res_lv1))
	expect_true(is.data.frame(res_lv1$events_detailed))
	expect_true(is.data.frame(res_lv1$events_total))
	expect_equal(res_lv1_type, res_lv1)

	# Level 2 boundary at 250
	res_lv2 <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250)
	res_lv2_type <- detect_hyperglycemic_events(example_data_5_subject, type = "lv2")
	expect_true(is.list(res_lv2))
	expect_true(is.data.frame(res_lv2$events_detailed))
	expect_true(is.data.frame(res_lv2$events_total))
	expect_equal(res_lv2_type, res_lv2)

	# Default and explicit extended type preserve existing behavior
	res_extended <- detect_hyperglycemic_events(example_data_5_subject)
	res_extended_type <- detect_hyperglycemic_events(example_data_5_subject, type = "extended")
	expect_equal(res_extended_type, res_extended)

	# Parameter validation
	expect_error(detect_hyperglycemic_events(example_data_5_subject, dur_length = -1), "dur_length must be between 0 and Inf")
	expect_error(detect_hyperglycemic_events(example_data_5_subject, start_gl = -1), "start_gl must be between 0 and Inf")
	expect_error(detect_hyperglycemic_events(example_data_5_subject, type = "bad"), "should be one of")
	expect_warning(
		res_type_wins <- detect_hyperglycemic_events(example_data_5_subject, type = "lv1", start_gl = 250),
		"using type and ignoring"
	)
	expect_equal(res_type_wins, res_lv1_type)

	# Sanity: level 1 should detect at least as many events as stricter level 2
	total_lv1 <- if (nrow(res_lv1$events_total)) sum(res_lv1$events_total$total_events) else 0
	total_lv2 <- if (nrow(res_lv2$events_total)) sum(res_lv2$events_total$total_events) else 0
	expect_true(total_lv1 >= total_lv2)
})
