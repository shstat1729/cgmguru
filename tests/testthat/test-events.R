library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject)

test_that("detect_all_events returns named tables and validates reading_minutes", {
	res <- detect_all_events(example_data_5_subject)
	expect_true(is.list(res))
	expect_named(res, c("subject_summary", "glycemic_event_summary"))
	expect_true(is.data.frame(res$glycemic_event_summary))
	expect_true(is.data.frame(res$subject_summary))
	expect_true(all(c(
		"id", "TIR", "TITR", "TBR70", "TBR54", "TAR180", "TAR250",
		"CV", "SD", "mean_glucose", "GMI", "uGMI", "GRI", "sensor_wear_percent",
		"hypo_lv1_total_episodes", "hypo_lv2_total_episodes",
		"hypo_extended_total_episodes", "hypo_lv1_excl_total_episodes",
		"hyper_lv1_total_episodes", "hyper_lv2_total_episodes",
		"hyper_extended_total_episodes", "hyper_lv1_excl_total_episodes"
	) %in% names(res$subject_summary)))
	expect_named(res$glycemic_event_summary, c(
		"id", "type", "level", "total_episodes",
		"avg_ep_per_day", "avg_minutes_below_54_per_episode"
	))

	res5 <- detect_all_events(example_data_5_subject, reading_minutes = 5)
	expect_true(is.list(res5))

	expect_error(detect_all_events(example_data_5_subject, reading_minutes = c(5, 5)),
		"reading_minutes vector length must match data length or be a single value")
	expect_error(detect_all_events(example_data_5_subject, summary_metrics_source = "bad"),
		"'arg' should be one of")
	expect_error(detect_all_events(example_data_5_subject, sensor_wear_ndays = 0),
		"sensor_wear_ndays must be between 0.1 and Inf")

	# Empty input returns empty data.frame
	empty_cgm <- data.frame(
		id = character(0),
		time = as.POSIXct(character(0), tz = "UTC"),
		gl = numeric(0),
		stringsAsFactors = FALSE
	)
	res_empty <- detect_all_events(empty_cgm)
	expect_true(is.list(res_empty))
	expect_named(res_empty, c("subject_summary", "glycemic_event_summary"))
	expect_equal(nrow(res_empty$glycemic_event_summary), 0)
	expect_equal(nrow(res_empty$subject_summary), 0)
})

test_that("detect_all_events can calculate sensor wear over fixed ndays", {
	df <- data.frame(
		id = "A",
		time = as.POSIXct("2026-01-01 00:00:00", tz = "UTC") + c(0, 5) * 60,
		gl = c(100, 120)
	)

	res <- detect_all_events(
		df,
		reading_minutes = 5,
		sensor_wear_ndays = 90
	)$subject_summary

	expected_count <- 90 * 24 * (60 / 5)
	expect_equal(res$sensor_wear_percent, 100 * 2 / expected_count, tolerance = 1e-8)
})

test_that("detect_all_events CGM summary metrics default to raw data", {
	df <- data.frame(
		id = "A",
		time = as.POSIXct("2026-01-01 00:05:00", tz = "UTC") + c(0, 10) * 60,
		gl = c(100, 200)
	)

	res <- detect_all_events(df, reading_minutes = 5)$subject_summary

	expect_equal(nrow(res), 1)
	expect_equal(res$TIR, 50, tolerance = 1e-8)
	expect_equal(res$TITR, 50, tolerance = 1e-8)
	expect_equal(res$TAR180, 50, tolerance = 1e-8)
	expect_equal(res$mean_glucose, 150, tolerance = 1e-8)
	expect_equal(res$SD, stats::sd(c(100, 200)), tolerance = 1e-8)
	expect_equal(res$CV, 100 * stats::sd(c(100, 200)) / 150, tolerance = 1e-8)
	expect_equal(res$GMI, 3.31 + 0.02392 * 150, tolerance = 1e-8)
	expect_equal(res$uGMI, 1 / (15.36 / 150 + 0.0425), tolerance = 1e-8)
	expect_equal(res$GRI, 0.8 * 50, tolerance = 1e-8)
	expect_equal(res$sensor_wear_percent, 100 * 2 / 3, tolerance = 1e-8)
})

test_that("detect_all_events can calculate CGM summary metrics on preprocessed data", {
	df <- data.frame(
		id = "A",
		time = as.POSIXct("2026-01-01 00:05:00", tz = "UTC") + c(0, 10) * 60,
		gl = c(100, 200)
	)

	res <- detect_all_events(
		df,
		reading_minutes = 5,
		summary_metrics_source = "preprocessed"
	)$subject_summary

	expect_equal(nrow(res), 1)
	expect_equal(res$TIR, 100 * 2 / 3, tolerance = 1e-8)
	expect_equal(res$TITR, 100 / 3, tolerance = 1e-8)
	expect_equal(res$TAR180, 100 / 3, tolerance = 1e-8)
	expect_equal(res$mean_glucose, 150, tolerance = 1e-8)
	expect_equal(res$SD, stats::sd(c(100, 150, 200)), tolerance = 1e-8)
	expect_equal(res$CV, 100 * stats::sd(c(100, 150, 200)) / 150, tolerance = 1e-8)
	expect_equal(res$GMI, 3.31 + 0.02392 * 150, tolerance = 1e-8)
	expect_equal(res$uGMI, 1 / (15.36 / 150 + 0.0425), tolerance = 1e-8)
	expect_equal(res$GRI, 0.8 * (100 / 3), tolerance = 1e-8)
	expect_equal(res$sensor_wear_percent, 100 * 2 / 3, tolerance = 1e-8)
})

test_that("detect_all_events calculates Glycemia Risk Index components", {
	df <- data.frame(
		id = "A",
		time = as.POSIXct("2026-01-01 00:05:00", tz = "UTC") + 0:4 * 5 * 60,
		gl = c(50, 60, 100, 200, 260)
	)

	res <- detect_all_events(df, reading_minutes = 5)$subject_summary

	expect_equal(res$TBR54, 20, tolerance = 1e-8)
	expect_equal(res$TBR70, 40, tolerance = 1e-8)
	expect_equal(res$TAR180, 40, tolerance = 1e-8)
	expect_equal(res$TAR250, 20, tolerance = 1e-8)
	expect_equal(res$GRI, 3.0 * 20 + 2.4 * 20 + 1.6 * 20 + 0.8 * 20,
							 tolerance = 1e-8)
	expect_equal(res$sensor_wear_percent, 100, tolerance = 1e-8)
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
	total_lv1 <- if (nrow(res_lv1$events_total)) sum(res_lv1$events_total$total_episodes) else 0
	total_lv2 <- if (nrow(res_lv2$events_total)) sum(res_lv2$events_total$total_episodes) else 0
	expect_true(total_lv1 >= total_lv2)
})
