#' @title GRID Algorithm for Glycemic Event Detection
#' @name grid
#' @description
#' Implements the GRID (Glucose Rate Increase Detector) algorithm for detecting rapid glucose rate increases in continuous glucose monitoring (CGM) data.
#' This algorithm identifies rapid glucose changes using specific rate-based criteria, and is commonly applied for meal detection.
#' Meals are detected when the CGM value is \eqn{\geq} 7.2 mmol/L (\eqn{\geq} 130 mg/dL) and the rate-of-change is \eqn{\geq} 5.3 mmol/L/h [\eqn{\geq} 95 mg/dL/h] for the last two consecutive readings, or \eqn{\geq} 5.0 mmol/L/h [\eqn{\geq} 90 mg/dL/h] for two of the last three readings.
#'
#'
#' @references
#' Harvey, R. A., et al. (2014). Design of the glucose rate increase detector: a meal detection module for the health monitoring system. Journal of Diabetes Science and Technology, 8(2), 307-320.
#'
#' Adolfsson, Peter, et al. "Increased time in range and fewer missed bolus injections after introduction of a smart connected insulin pen." Diabetes technology & therapeutics 22.10 (2020): 709-718.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param gap Gap threshold in minutes for event detection (default: 15).
#'   This parameter defines the minimum time interval between consecutive GRID events. For example, if gap is set to 60, only one GRID event can be detected within any one-hour window; subsequent events within the gap interval are not counted as new events.
#' @param threshold GRID slope threshold in mg/dL/hour for event classification (default: 130)
#' @usage grid(df, gap = 15, threshold = 130)
#' @section Algorithm:
#' - Flags points where \code{gl >= 130 mg/dL} and rate-of-change meets the GRID criteria (see references).
#' - Enforces a minimum \code{gap} in minutes between detected events to avoid duplicates.
#' @section Units and sampling:
#' - \code{gl} is mg/dL; \code{time} is POSIXct; \code{gap} is minutes.
#' - The effective sampling interval is derived from \code{time} deltas.
#' - This function operates on the rows supplied in \code{df}. It does not use
#'   \code{\link{interpolate_cgm}} or the full-day event preprocessing grid.
#' @seealso \link{mod_grid}, \link{maxima_grid}, \link{find_local_maxima}, \link{detect_between_maxima}
#' @family GRID pipeline
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{grid_vector}: A tibble with the results of the GRID analysis. Contains a \code{grid} column (0/1 values; 1 denotes a detected GRID event) and all relevant input columns.
#'   \item \code{episode_counts}: A tibble summarizing the number of GRID events per subject (\code{id}) as \code{episode_counts}.
#'   \item \code{episode_start}: A tibble listing the start of each GRID episode, with columns:
#'     \itemize{
#'       \item \code{id}: Subject ID.
#'       \item \code{time}: The timestamp (POSIXct) at which the GRID event was detected.
#'       \item \code{gl}: The glucose value (mg/dL; integer or numeric) at the GRID event.
#'       \item \code{index}: R-based (1-indexed) row number(s) in the original dataframe where the GRID event occurs. The occurrence time equals \code{df$time[index]} and glucose equals \code{df$gl[index]}.
#'     }
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Basic GRID analysis on smaller dataset
#' grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
#' print(grid_result$episode_counts)
#' print(grid_result$episode_start)
#' print(grid_result$grid_vector)
#'
#' # More sensitive GRID analysis
#' sensitive_result <- grid(example_data_5_subject, gap = 10, threshold = 120)
#'
#' # GRID analysis on larger dataset
#' large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
#' print(paste("Detected", sum(large_grid$episode_counts$episode_counts), "episodes"))
#' print(large_grid$episode_start)
#' print(large_grid$grid_vector)
#'

NULL

#' @title Combined Maxima Detection and GRID Analysis
#' @name maxima_grid
#' @description
#' Fast method for postprandial glucose peak detection combining GRID algorithm with local maxima analysis.
#' Detects meal-induced glucose peaks by identifying GRID events (rapid glucose increases) and mapping
#' them to corresponding local maxima within a search window. Local maxima are defined as points where
#' glucose values increase or remain constant for two consecutive points before the peak, and decrease
#' or remain constant for two consecutive points after the peak.
#'
#' The 7-step algorithm:
#' (1) finds GRID points indicating meal starts
#' (2) identifies modified GRID points after minimum duration
#' (3) locates maximum glucose within the subsequent time window
#' (4) detects all local maxima using the two-consecutive-point criteria
#' (5) refines peaks from local maxima candidates
#' (6) maps GRID points to peaks within 4-hour constraint
#' (7) redistributes overlapping peaks.
#'
#' @references
#' Park, Sang Ho, et al. "Identification of clinically meaningful automatically detected postprandial glucose excursions in individuals with type 1 diabetes using personal continuous glucose monitoring." Diabetes Research and Clinical Practice (2025): 112951.
#'
#' Park, Soojin, et al. "High-Amplitude and Prolonged Glucose Excursions as a Key Determinant of Discordance Between Glucose Management Indicator and Glycated Hemoglobin in Type 1 Diabetes." Diabetes Care (2026): dc252820. https://doi.org/10.2337/dc25-2820
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param threshold GRID slope threshold in mg/dL/hour for event classification (default: 130)
#' @param gap Gap threshold in minutes for event detection (default: 60).
#'   This parameter defines the minimum time interval between consecutive GRID events.
#' @param hours Time window in hours for maxima analysis (default: 2)
#' @usage maxima_grid(df, threshold = 130, gap = 60, hours = 2)
#' @section Algorithm (7 steps):
#' 1) GRID -> 2) modified GRID -> 3) window maxima -> 4) local maxima -> 5) refine peaks ->
#' 6) map GRID to peaks (\eqn{\leq} 4h) -> 7) redistribute overlapping peaks.
#' @section Input grid:
#' This function operates on the rows supplied in \code{df}. It does not use
#' \code{\link{interpolate_cgm}} or the full-day event preprocessing grid unless
#' the caller explicitly supplies interpolated data.
#' @seealso \link{grid}, \link{mod_grid}, \link{find_local_maxima}, \link{find_new_maxima}, \link{transform_df}
#' @family GRID pipeline
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{results}: Tibble with combined maxima and GRID analysis results, with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{grid_time}: Timestamp of GRID event detection (POSIXct)
#'       \item \code{grid_gl}: Glucose value at GRID event (mg/dL)
#'       \item \code{maxima_time}: Timestamp of peak glucose (POSIXct)
#'       \item \code{maxima_glucose}: Peak glucose value (mg/dL)
#'       \item \code{time_to_peak_min}: Time from GRID event to peak in minutes
#'       \item \code{grid_index}: R-based (1-indexed) row number of GRID event; \code{grid_time == df$time[grid_index]}, \code{grid_gl == df$gl[grid_index]}
#'       \item \code{maxima_index}: R-based (1-indexed) row number of peak; \code{maxima_time == df$time[maxima_index]}, \code{maxima_glucose == df$gl[maxima_index]}
#'     }
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Combined analysis on smaller dataset
#' maxima_result <- maxima_grid(example_data_5_subject, threshold = 130, gap = 60, hours = 2)
#' print(maxima_result$episode_counts)
#' print(maxima_result$results)
#'
#' # More sensitive analysis
#' sensitive_maxima <- maxima_grid(example_data_5_subject, threshold = 120, gap = 30, hours = 1)
#' print(sensitive_maxima$episode_counts)
#' print(sensitive_maxima$results)
#'
#' # Analysis on larger dataset
#' large_maxima <- maxima_grid(example_data_hall, threshold = 130, gap = 60, hours = 2)
#' print(large_maxima$episode_counts)
#' print(large_maxima$results)
NULL

#' @title Detect Hyperglycemic Events
#' @name detect_hyperglycemic_events
#' @encoding UTF-8
#' @description
#' Identifies and segments hyperglycemic events in CGM data based on international consensus
#' CGM metrics (Battelino et al., 2023). Use \code{type} to select one of
#' three event definitions:
#' \itemize{
#'   \item \strong{Level 1}: \eqn{\geq} 15 consecutive min of \eqn{>} 180 mg/dL, ends with \eqn{\geq} 15 consecutive min \eqn{\leq} 180 mg/dL
#'   \item \strong{Level 2}: \eqn{\geq} 15 consecutive min of \eqn{>} 250 mg/dL, ends with \eqn{\geq} 15 consecutive min \eqn{\leq} 250 mg/dL
#'   \item \strong{Extended}: \eqn{>} 250 mg/dL lasting \eqn{\geq} 90 cumulative min within a 120-min period, ends when glucose returns to \eqn{\leq} 180 mg/dL
#'     for \eqn{\geq} 15 consecutive min after
#' }
#' Events are counted only after glucose remains at or below the end threshold
#' for the specified end length. In \code{events_detailed}, \code{end_time},
#' \code{end_glucose}, and \code{end_index} report the last hyperglycemic
#' reading immediately before that confirmed recovery period starts.
#'
#' @references
#' Battelino, T., et al. (2023). Continuous glucose monitoring and metrics for clinical trials: an international consensus statement. The Lancet Diabetes & Endocrinology, 11(1), 42-57.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param ... Custom event criteria supplied by name. Prefer \code{type} for
#'   standard Level 1, Level 2, and Extended events. Supported custom criteria
#'   are:
#'   \itemize{
#'     \item \code{dur_length}: Minimum event duration in minutes required for
#'       an event to qualify.
#'     \item \code{end_length}: Required recovery duration in minutes before
#'       event termination is confirmed.
#'     \item \code{start_gl}: Glucose threshold in mg/dL used to qualify
#'       hyperglycemic readings. Hyperglycemic readings are above this value.
#'     \item \code{end_gl}: Glucose threshold in mg/dL used to confirm
#'       recovery. Hyperglycemic events end after glucose remains at or below
#'       this value for \code{end_length} minutes.
#'   }
#' @param type Hyperglycemia event definition. One of \code{"extended"}
#'   (default), \code{"lv1"}, \code{"lv2"}, or \code{"lv1_excl"}.
#' @param reading_minutes Time interval between readings in minutes (optional).
#'   If omitted or \code{NULL}, the interval is calculated automatically per id
#'   as the median positive time difference in the data.
#' @param sort_time Logical. If \code{TRUE}, sort rows within each id by
#'   \code{time} in C++ before interpolation. Defaults to \code{FALSE}.
#' @param inter_gap Maximum gap in minutes to interpolate across. Defaults to
#'   45; larger gaps split event-detection segments.
#' @param return_interpolated Logical. If \code{TRUE}, include the interpolated
#'   grid data used for event detection in the returned list. Defaults to
#'   \code{TRUE}.
#' @usage detect_hyperglycemic_events(df, ..., type = "extended",
#'  reading_minutes = NULL, sort_time = FALSE, inter_gap = 45,
#'  return_interpolated = TRUE)
#' @section Methods:
#' Hyperglycemic events can be detected using either the recommended
#' \code{type} argument or named custom threshold and duration criteria.
#'
#' \strong{1. Preset method using \code{type} (recommended):}
#' Use \code{type} when you want the standard Level 1, Level 2, or Extended
#' hyperglycemia definitions without manually entering thresholds.
#' \itemize{
#'   \item \code{type = "lv1"} uses \code{start_gl = 180}, \code{dur_length = 15},
#'     \code{end_length = 15}, and \code{end_gl = 180}.
#'   \item \code{type = "lv2"} uses \code{start_gl = 250}, \code{dur_length = 15},
#'     \code{end_length = 15}, and \code{end_gl = 250}.
#'   \item \code{type = "extended"} uses \code{start_gl = 250},
#'     \code{dur_length = 120}, \code{end_length = 15}, and \code{end_gl = 180}.
#'   \item \code{type = "lv1_excl"} returns Level 1 episodes that do not
#'     overlap Level 2 episodes.
#' }
#'
#' \strong{2. Custom criteria method:}
#' Supply \code{start_gl}, \code{dur_length}, \code{end_length}, and
#' \code{end_gl} directly when using a custom definition, for example
#' \code{detect_hyperglycemic_events(df, start_gl = 180, dur_length = 15,
#' end_length = 15, end_gl = 180)} for Level 1 hyperglycemia. If an explicit
#' \code{type} is supplied together with custom numeric criteria, the function
#' returns results based on \code{type}; the custom criteria are ignored and a
#' warning is issued.
#' @section Units and sampling:
#' - \code{reading_minutes} can be a scalar (all rows) or a vector per-row.
#' - If \code{reading_minutes} is omitted or \code{NULL}, it is calculated
#'   automatically per id from timestamp spacing.
#' - Event classification uses cgmguru's independent C++ implementation of an
#'   iglu-compatible, midnight-aligned full-day grid. Data are linearly
#'   interpolated at the id-specific interval up to \code{inter_gap}; larger
#'   gaps are masked, removed from the event-classification data, and split
#'   segments.
#' - This preprocessing is specific to event calculation and does not affect
#'   \code{\link{grid}}, \code{\link{maxima_grid}}, or \code{\link{excursion}}.
#' @seealso \link{detect_all_events}
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{events_total}: Tibble with summary statistics per subject (id, total_episodes, avg_ep_per_day)
#'   \item \code{events_detailed}: Tibble with detailed event information (id, start_time, start_glucose, end_time, end_glucose, start_index, end_index). End fields report the last dysglycemic reading before confirmed recovery starts. \code{start_index} and \code{end_index} are 1-based row positions in the internal interpolated event grid, returned as \code{interpolated_data} when \code{return_interpolated = TRUE}.
#'   \item \code{interpolated_data}: Included when
#'     \code{return_interpolated = TRUE}, with columns \code{id}, \code{time},
#'     and \code{gl}.
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Level 1 Hyperglycemia Event (>=15 consecutive min of >180 mg/dL and event
#' # ends when there is >=15 consecutive min with a CGM sensor value of <=180 mg/dL)
#' hyper_lv1 <- detect_hyperglycemic_events(example_data_5_subject, type = "lv1")
#' print(hyper_lv1$events_total)
#'
#' # Level 2 Hyperglycemia Event (>=15 consecutive min of >250 mg/dL and event
#' # ends when there is >=15 consecutive min with a CGM sensor value of <=250 mg/dL)
#' hyper_lv2 <- detect_hyperglycemic_events(example_data_5_subject, type = "lv2")
#' print(hyper_lv2$events_total)
#'
#' # Extended Hyperglycemia Event (>250 mg/dL lasting >=90 cumulative min within a
#' # 120-min period, ends when glucose returns to <=180 mg/dL for >=15 consecutive
#' # min after)
#' hyper_extended <- detect_hyperglycemic_events(example_data_5_subject, type = "extended")
#' print(hyper_extended$events_total)
#'
#' # Custom criteria method for the same standard definitions
#' hyper_lv1_custom <- detect_hyperglycemic_events(
#'   example_data_5_subject,
#'   start_gl = 180,
#'   dur_length = 15,
#'   end_length = 15,
#'   end_gl = 180
#' )
#' hyper_lv2_custom <- detect_hyperglycemic_events(
#'   example_data_5_subject,
#'   start_gl = 250,
#'   dur_length = 15,
#'   end_length = 15,
#'   end_gl = 250
#' )
#' hyper_extended_custom <- detect_hyperglycemic_events(
#'   example_data_5_subject,
#'   start_gl = 250,
#'   dur_length = 120,
#'   end_length = 15,
#'   end_gl = 180
#' )
#'
#' # Compare event rates across levels
#' cat("Level 1 episodes:", sum(hyper_lv1$events_total$total_episodes), "\n")
#' cat("Level 2 episodes:", sum(hyper_lv2$events_total$total_episodes), "\n")
#' cat("Extended episodes:", sum(hyper_extended$events_total$total_episodes), "\n")
#'
#' # Analysis on larger dataset with Level 1 criteria
#' large_hyper <- detect_hyperglycemic_events(example_data_hall, type = "lv1")
#' print(large_hyper$events_total)
#'
#' # Analysis on larger dataset with Level 2 criteria
#' large_hyper_lv2 <- detect_hyperglycemic_events(example_data_hall, type = "lv2")
#' print(large_hyper_lv2$events_total)
#'
#' # Analysis on larger dataset with Extended criteria
#' large_hyper_extended <- detect_hyperglycemic_events(example_data_hall, type = "extended")
#' print(large_hyper_extended$events_total)
#'
#' # View detailed events for specific subject
#' if(nrow(hyper_lv1$events_detailed) > 0) {
#'   first_subject <- hyper_lv1$events_detailed$id[1]
#'   subject_events <- hyper_lv1$events_detailed[hyper_lv1$events_detailed$id == first_subject, ]
#'   head(subject_events)
#' }
NULL

#' @title Detect Hypoglycemic Events
#' @name detect_hypoglycemic_events
#' @encoding UTF-8
#' @description
#' Identifies and segments hypoglycemic events in CGM data based on international consensus
#' CGM metrics (Battelino et al., 2023). Use \code{type} to select one of
#' three event definitions:
#' \itemize{
#'   \item \strong{Level 1}: \eqn{\geq} 15 consecutive min of \eqn{<} 70 mg/dL, ends with \eqn{\geq} 15 consecutive min \eqn{\geq} 70 mg/dL
#'   \item \strong{Level 2}: \eqn{\geq} 15 consecutive min of \eqn{<} 54 mg/dL, ends with \eqn{\geq} 15 consecutive min \eqn{\geq} 54 mg/dL
#'   \item \strong{Extended}: \eqn{>} 120 consecutive min of \eqn{<} 70 mg/dL, ends with \eqn{\geq} 15 consecutive min \eqn{\geq} 70 mg/dL
#' }
#' Events are counted only after glucose remains at or above the recovery
#' threshold for the specified end length. In \code{events_detailed},
#' \code{end_time}, \code{end_glucose}, and \code{end_index} report the last
#' hypoglycemic reading immediately before that confirmed recovery period starts.
#'
#' @references
#' Battelino, T., et al. (2023). Continuous glucose monitoring and metrics for clinical trials: an international consensus statement. The Lancet Diabetes & Endocrinology, 11(1), 42-57.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param ... Custom event criteria supplied by name. Prefer \code{type} for
#'   standard Level 1, Level 2, and Extended events. Supported custom criteria
#'   are:
#'   \itemize{
#'     \item \code{dur_length}: Minimum event duration in minutes required for
#'       an event to qualify.
#'     \item \code{end_length}: Required recovery duration in minutes before
#'       event termination is confirmed.
#'     \item \code{start_gl}: Glucose threshold in mg/dL used to qualify
#'       hypoglycemic readings. Hypoglycemic readings are below this value, and
#'       recovery is confirmed after glucose remains at or above this value for
#'       \code{end_length} minutes.
#'   }
#' @param type Hypoglycemia event definition. One of \code{"extended"}
#'   (default), \code{"lv1"}, \code{"lv2"}, or \code{"lv1_excl"}.
#' @param reading_minutes Time interval between readings in minutes (optional).
#'   If omitted or \code{NULL}, the interval is calculated automatically per id
#'   as the median positive time difference in the data.
#' @param sort_time Logical. If \code{TRUE}, sort rows within each id by
#'   \code{time} in C++ before interpolation. Defaults to \code{FALSE}.
#' @param inter_gap Maximum gap in minutes to interpolate across. Defaults to
#'   45; larger gaps split event-detection segments.
#' @param return_interpolated Logical. If \code{TRUE}, include the interpolated
#'   grid data used for event detection in the returned list. Defaults to
#'   \code{TRUE}.
#' @usage detect_hypoglycemic_events(df, ..., type = "extended",
#'  reading_minutes = NULL, sort_time = FALSE, inter_gap = 45,
#'  return_interpolated = TRUE)
#' @section Methods:
#' Hypoglycemic events can be detected using either the recommended
#' \code{type} argument or named custom threshold and duration criteria.
#'
#' \strong{1. Preset method using \code{type} (recommended):}
#' Use \code{type} when you want the standard Level 1, Level 2, or Extended
#' hypoglycemia definitions without manually entering thresholds.
#' \itemize{
#'   \item \code{type = "lv1"} uses \code{start_gl = 70},
#'     \code{dur_length = 15}, and \code{end_length = 15}.
#'   \item \code{type = "lv2"} uses \code{start_gl = 54},
#'     \code{dur_length = 15}, and \code{end_length = 15}.
#'   \item \code{type = "extended"} uses \code{start_gl = 70},
#'     \code{dur_length = 120}, and \code{end_length = 15}.
#'   \item \code{type = "lv1_excl"} returns Level 1 episodes that do not
#'     overlap Level 2 episodes.
#' }
#'
#' \strong{2. Custom criteria method:}
#' Supply \code{start_gl}, \code{dur_length}, and \code{end_length} directly
#' when using a custom definition, for example
#' \code{detect_hypoglycemic_events(df, start_gl = 70, dur_length = 15,
#' end_length = 15)} for Level 1 hypoglycemia. If an explicit \code{type} is
#' supplied together with custom numeric criteria, the function returns results
#' based on \code{type}; the custom criteria are ignored and a warning is
#' issued.
#' @section Units and sampling:
#' - \code{reading_minutes} can be a scalar (all rows) or a vector per-row.
#' - If \code{reading_minutes} is omitted or \code{NULL}, it is calculated
#'   automatically per id from timestamp spacing.
#' - Event classification uses cgmguru's independent C++ implementation of an
#'   iglu-compatible, midnight-aligned full-day grid. Data are linearly
#'   interpolated at the id-specific interval up to \code{inter_gap}; larger
#'   gaps are masked, removed from the event-classification data, and split
#'   segments.
#' - This preprocessing is specific to event calculation and does not affect
#'   \code{\link{grid}}, \code{\link{maxima_grid}}, or \code{\link{excursion}}.
#' @seealso \link{detect_all_events}
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{events_total}: Tibble with summary statistics per subject (id, total_episodes, avg_ep_per_day)
#'   \item \code{events_detailed}: Tibble with detailed event information (id, start_time, start_glucose, end_time, end_glucose, start_index, end_index, duration_below_54_minutes). End fields report the last dysglycemic reading before confirmed recovery starts. \code{start_index} and \code{end_index} are 1-based row positions in the internal interpolated event grid, returned as \code{interpolated_data} when \code{return_interpolated = TRUE}.
#'   \item \code{interpolated_data}: Included when
#'     \code{return_interpolated = TRUE}, with columns \code{id}, \code{time},
#'     and \code{gl}.
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Level 1 Hypoglycemia Event (>=15 consecutive min of <70 mg/dL and event
#' # ends when there is >=15 consecutive min with a CGM sensor value of >=70 mg/dL)
#' hypo_lv1 <- detect_hypoglycemic_events(example_data_5_subject, type = "lv1")
#' print(hypo_lv1$events_total)
#'
#' # Level 2 Hypoglycemia Event (>=15 consecutive min of <54 mg/dL and event
#' # ends when there is >=15 consecutive min with a CGM sensor value of >=54 mg/dL)
#' hypo_lv2 <- detect_hypoglycemic_events(example_data_5_subject, type = "lv2")
#'
#' # Extended Hypoglycemia Event (>120 consecutive min of <70 mg/dL and event
#' # ends when there is >=15 consecutive min with a CGM sensor value of >=70 mg/dL)
#' hypo_extended <- detect_hypoglycemic_events(example_data_5_subject, type = "extended")
#' print(hypo_extended$events_total)
#'
#' # Custom criteria method for the same standard definitions
#' hypo_lv1_custom <- detect_hypoglycemic_events(
#'   example_data_5_subject,
#'   start_gl = 70,
#'   dur_length = 15,
#'   end_length = 15
#' )
#' hypo_lv2_custom <- detect_hypoglycemic_events(
#'   example_data_5_subject,
#'   start_gl = 54,
#'   dur_length = 15,
#'   end_length = 15
#' )
#' hypo_extended_custom <- detect_hypoglycemic_events(
#'   example_data_5_subject,
#'   start_gl = 70,
#'   dur_length = 120,
#'   end_length = 15
#' )
#'
#' # Compare event rates across levels
#' cat("Level 1 episodes:", sum(hypo_lv1$events_total$total_episodes), "\n")
#' cat("Level 2 episodes:", sum(hypo_lv2$events_total$total_episodes), "\n")
#' cat("Extended episodes:", sum(hypo_extended$events_total$total_episodes), "\n")
#'
#' # Analysis on larger dataset with Level 1 criteria
#' large_hypo <- detect_hypoglycemic_events(example_data_hall, type = "lv1")
#' print(large_hypo$events_total)
#'
#' # Analysis on larger dataset with Level 2 criteria
#' large_hypo_lv2 <- detect_hypoglycemic_events(example_data_hall, type = "lv2")
#' print(large_hypo_lv2$events_total)
#'
#' # Analysis on larger dataset with Extended criteria
#' large_hypo_extended <- detect_hypoglycemic_events(example_data_hall, type = "extended")
#' print(large_hypo_extended$events_total)
NULL

#' @title Detect Rebound Hypoglycemia and Hyperglycemia
#' @name rebound_events
#' @description
#' Detects rebound hypoglycemia (Rhypo) and rebound hyperglycemia (Rhyper)
#' as derived events built from cgmguru Level 1 event starts and a later
#' opposite-threshold crossing within a configurable time window.
#'
#' Rhypo is a Level 1 hyperglycemic event (\eqn{>}180 mg/dL for at least
#' 15 minutes) followed by any glucose value \eqn{<}70 mg/dL within
#' \code{rebound_minutes}. Rhyper is a Level 1 hypoglycemic event
#' (\eqn{<}70 mg/dL for at least 15 minutes) followed by any glucose value
#' \eqn{>}180 mg/dL within \code{rebound_minutes}. The later rebound side
#' only needs a single qualifying threshold crossing.
#'
#' These rebound hypoglycemia and hyperglycemia definitions are documented by
#' Hansen and Bibby (2024); cgmguru applies them on its own event-preprocessed
#' CGM grid.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data
#'   with columns \code{id}, \code{time}, and \code{gl}.
#' @param type Rebound direction to return. \code{"hypo"} returns Rhypo,
#'   \code{"hyper"} returns Rhyper, and \code{"all"} returns both.
#' @param data_source Source interpretation for \code{df}. \code{"raw"}
#'   applies cgmguru event preprocessing first. \code{"preprocessed"} treats
#'   \code{df} as an already-preprocessed event grid and does not interpolate
#'   again.
#' @param reading_minutes Time interval between readings in minutes. Can be a
#'   scalar, a vector matching \code{nrow(df)}, or \code{NULL}. If omitted, it
#'   is inferred per id from positive timestamp differences.
#' @param sort_time Logical. If \code{TRUE}, sort rows within each id by
#'   \code{time}. Defaults to \code{FALSE}.
#' @param inter_gap Maximum gap in minutes to interpolate across when
#'   \code{data_source = "raw"}. Defaults to 45.
#' @param rebound_minutes Maximum bridge interval in minutes from the initial
#'   Level 1 event end to the later rebound threshold crossing. Defaults to
#'   120.
#' @param return_interpolated Logical. If \code{TRUE}, include the
#'   preprocessed event grid used for rebound detection as
#'   \code{interpolated_data}. Defaults to \code{TRUE}, so
#'   \code{rebound_events()} returns the preprocessed data by default.
#' @usage rebound_events(df, type = c("all", "hypo", "hyper"),
#'  data_source = c("raw", "preprocessed"), reading_minutes = NULL,
#'  sort_time = FALSE, inter_gap = 45, rebound_minutes = 120,
#'  return_interpolated = TRUE)
#' @return A list containing:
#' \itemize{
#'   \item \code{events_total}: Tibble with \code{id}, \code{type},
#'     \code{total_episodes}, and \code{avg_ep_per_day}.
#'   \item \code{events_detailed}: Tibble with bridge boundaries
#'     (\code{start_time}, \code{end_time}, indices, and glucose values), the
#'     initial Level 1 event boundaries, rebound threshold crossing fields, and
#'     \code{minutes_to_rebound}.
#'   \item \code{interpolated_data}: The preprocessed event grid used for
#'     rebound detection, included by default when
#'     \code{return_interpolated = TRUE}, with columns \code{id}, \code{time},
#'     and \code{gl}.
#' }
#' @references
#' Hansen, K. W., and Bibby, B. M. (2024). Rebound hypoglycemia and
#' hyperglycemia in type 1 diabetes. \emph{Journal of Diabetes Science and
#' Technology}, 18(6), 1392-1398.
#' @seealso \link{detect_all_events}, \link{detect_hyperglycemic_events},
#'   \link{detect_hypoglycemic_events}, \link{interpolate_cgm}
#'
#' @export
#' @examples
#' df <- data.frame(
#'   id = "A",
#'   time = as.POSIXct("2026-01-01 00:00:00", tz = "UTC") + 0:8 * 5 * 60,
#'   gl = c(190, 195, 200, 170, 165, 160, 65, 100, 110)
#' )
#' rebound_events(df, type = "hypo", reading_minutes = 5)
NULL

#' @title Detect All Glycemic Events
#' @name detect_all_events
#' @description
#' Comprehensive function to detect all types of glycemic events aligned with
#' international consensus CGM metrics (Battelino et al., 2023). This function
#' provides a unified interface for detecting multiple event types including
#' Level 1/2/Extended hypo- and hyperglycemia, Level 1 excluded events, and
#' rebound hypo-/hyperglycemia summaries.
#' Rebound hypoglycemia and hyperglycemia definitions follow Hansen and Bibby
#' (2024).
#' Events are counted only after the required recovery condition is confirmed;
#' duration summaries use the event boundary immediately before recovery starts.
#' Event preprocessing uses cgmguru's independent C++ implementation of an
#' iglu-compatible day-based grid: each subject is interpolated from the first
#' observed day's midnight plus one reading interval, rather than from the first
#' observed timestamp. Larger gaps are masked and removed before event
#' classification, preserving gap-based segment boundaries. This preprocessing
#' is specific to event calculation and does not affect \code{\link{grid}},
#' \code{\link{maxima_grid}}, or \code{\link{excursion}}.
#' CGM summary metrics in \code{subject_summary} are calculated from the original
#' raw glucose values by default. Set
#' \code{summary_metrics_source = "preprocessed"} to calculate them from the
#' internal event-preprocessed grid. Numeric summary outputs are rounded to
#' \code{summary_digits} decimal places; set \code{summary_digits = NULL} or
#' \code{summary_digits = "none"} to return unrounded values.
#'
#' @references
#' Battelino, T., et al. (2023). Continuous glucose monitoring and metrics for clinical trials: an international consensus statement. The Lancet Diabetes & Endocrinology, 11(1), 42-57.
#'
#' Hansen, K. W., and Bibby, B. M. (2024). Rebound hypoglycemia and hyperglycemia in type 1 diabetes. Journal of Diabetes Science and Technology, 18(6), 1392-1398.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param reading_minutes Time interval between readings in minutes (optional).
#'   Can be a single integer/numeric value (applied to all subjects), a vector
#'   matching data length, or \code{NULL}. If omitted or \code{NULL}, the
#'   interval is calculated automatically per id as the median positive time
#'   difference in the data.
#' @param sort_time Logical. If \code{TRUE}, sort rows within each id by
#'   \code{time} in C++ before interpolation. Defaults to \code{FALSE}.
#' @param inter_gap Maximum gap in minutes to interpolate across. Defaults to
#'   45; larger gaps split event-detection segments.
#' @param return_interpolated Logical. If \code{TRUE}, include the internal
#'   event-preprocessed grid as \code{interpolated_data}. Defaults to
#'   \code{FALSE}.
#' @param summary_metrics_source Character. Source glucose values for CGM
#'   summary metrics. Defaults to \code{"raw"} for original observed data; use
#'   \code{"preprocessed"} for the internal event-preprocessed grid after
#'   interpolation and gap masking. \code{sensor_wear_percent} is always
#'   calculated from the original timestamps and glucose readings.
#' @param sensor_wear_ndays Number of days for fixed-window
#'   \code{sensor_wear_percent} calculation. Defaults to \code{NULL}, which
#'   uses the original timestamp span. Set to a positive number such as
#'   \code{90} to calculate observed readings over the last 90 days for each
#'   subject divided by the expected number of readings in 90 days.
#' @param summary_digits Number of decimal places for numeric summary outputs
#'   in \code{subject_summary} and rate/duration columns in
#'   \code{glycemic_event_summary}. Defaults to \code{2}. Use \code{NULL} or
#'   \code{"none"} to return unrounded values.
#' @usage detect_all_events(df, reading_minutes = NULL, sort_time = FALSE,
#'  inter_gap = 45, return_interpolated = FALSE,
#'  summary_metrics_source = c("raw", "preprocessed"),
#'  sensor_wear_ndays = NULL, summary_digits = 2)
#' @section Event types:
#' - Hypoglycemia: lv1 (\eqn{<} 70 mg/dL, \eqn{\geq} 15 min), lv2 (\eqn{<} 54 mg/dL, \eqn{\geq} 15 min), extended (\eqn{<} 70 mg/dL, \eqn{\geq} 120 min).
#' - Hyperglycemia: lv1 (\eqn{>} 180 mg/dL, \eqn{\geq} 15 min), lv2 (\eqn{>} 250 mg/dL, \eqn{\geq} 15 min), extended (\eqn{>} 250 mg/dL, \eqn{\geq} 90 min in 120 min, end \eqn{\leq} 180 mg/dL for \eqn{\geq} 15 min).
#' - Rebound: hypo rebound is Level 1 hyperglycemia followed by \eqn{<}70 mg/dL within 120 minutes; hyper rebound is Level 1 hypoglycemia followed by \eqn{>}180 mg/dL within 120 minutes.
#' @seealso \link{detect_hyperglycemic_events}, \link{detect_hypoglycemic_events},
#'   \link{rebound_events}
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{subject_summary}: One row per subject. CGM summary metric columns
#'     are calculated on the original raw glucose values by default; set
#'     \code{summary_metrics_source = "preprocessed"} to use the
#'     event-preprocessed glucose grid. Event summaries are included as wide
#'     \code{*_total_episodes} columns only. Numeric summary columns are
#'     rounded according to \code{summary_digits}.
#'   \item \code{glycemic_event_summary}: One row per subject, event
#'     type, and event level. Contains the full event summary: \code{id},
#'     \code{type}, \code{level}, \code{total_episodes},
#'     \code{avg_ep_per_day}, and
#'     \code{avg_minutes_below_54_per_episode}.
#'   \item \code{interpolated_data}: Included when
#'     \code{return_interpolated = TRUE}, with columns \code{id}, \code{time},
#'     and \code{gl}.
#' }
#' \code{subject_summary} includes:
#' \itemize{
#'   \item \code{id}: Subject identifier
#'   \item \code{TIR}: Percent of glucose readings in range 70-180 mg/dL
#'   \item \code{TITR}: Percent of glucose readings in tight range 70-140 mg/dL
#'   \item \code{TBR70}: Percent of glucose readings below 70 mg/dL
#'   \item \code{TBR54}: Percent of glucose readings below 54 mg/dL
#'   \item \code{TAR180}: Percent of glucose readings above 180 mg/dL
#'   \item \code{TAR250}: Percent of glucose readings above 250 mg/dL
#'   \item \code{CV}: Coefficient of variation in percent,
#'     \eqn{100 * SD / mean_glucose}
#'   \item \code{SD}: Sample standard deviation of glucose, mg/dL
#'   \item \code{mean_glucose}: Mean glucose, mg/dL
#'   \item \code{GMI}: Glucose Management Indicator,
#'     \eqn{3.31 + 0.02392 * mean_glucose}
#'   \item \code{uGMI}: Unitless GMI-style metric,
#'     \eqn{1 / (15.36 / mean_glucose + 0.0425)}
#'   \item \code{GRI}: Glycemia Risk Index,
#'     \eqn{3.0 * VLow + 2.4 * Low + 1.6 * VHigh + 0.8 * High}, where
#'     \code{VLow} is percent time \eqn{<}54 mg/dL, \code{Low} is 54-\eqn{<}70
#'     mg/dL, \code{VHigh} is \eqn{>}250 mg/dL, and \code{High} is
#'     \eqn{>}180-\eqn{\leq}250 mg/dL
#'   \item \code{sensor_wear_percent}: Percent of expected CGM readings observed,
#'     calculated from the original timestamps using the same automatic range
#'     method as \code{iglu::active_percent()} by default. If
#'     \code{sensor_wear_ndays} is supplied, this is calculated over the last
#'     N days for each subject.
#'   \item \code{hypo_lv1_total_episodes}: Number of Level 1 hypoglycemia
#'     episodes
#'   \item \code{hypo_lv2_total_episodes}: Number of Level 2 hypoglycemia
#'     episodes
#'   \item \code{hypo_extended_total_episodes}: Number of extended
#'     hypoglycemia episodes
#'   \item \code{hypo_lv1_excl_total_episodes}: Number of Level 1
#'     hypoglycemia episodes that do not overlap a Level 2 episode
#'   \item \code{hypo_rebound_total_episodes}: Number of rebound
#'     hypoglycemia episodes
#'   \item \code{hyper_lv1_total_episodes}: Number of Level 1 hyperglycemia
#'     episodes
#'   \item \code{hyper_lv2_total_episodes}: Number of Level 2 hyperglycemia
#'     episodes
#'   \item \code{hyper_extended_total_episodes}: Number of extended
#'     hyperglycemia episodes
#'   \item \code{hyper_lv1_excl_total_episodes}: Number of Level 1
#'     hyperglycemia episodes that do not overlap a Level 2 episode
#'   \item \code{hyper_rebound_total_episodes}: Number of rebound
#'     hyperglycemia episodes
#' }
#' \code{glycemic_event_summary} includes:
#' \itemize{
#'   \item \code{id}: Subject identifier
#'   \item \code{type}: Event direction, either \code{"hypo"} or
#'     \code{"hyper"}
#'   \item \code{level}: Event level, one of \code{"lv1"}, \code{"lv2"},
#'     \code{"extended"}, \code{"lv1_excl"}, or \code{"rebound"}
#'   \item \code{total_episodes}: Number of episodes for the subject, event
#'     direction, and event level
#'   \item \code{avg_ep_per_day}: Average episodes per day for the subject,
#'     event direction, and event level, rounded according to
#'     \code{summary_digits}
#'   \item \code{avg_minutes_below_54_per_episode}: For hypoglycemia rows,
#'     average minutes below 54 mg/dL per episode, rounded according to
#'     \code{summary_digits}; for hyperglycemia rows, 0
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Detect all glycemic events; reading_minutes is calculated automatically
#' # from the timestamp spacing when omitted
#' all_outputs <- detect_all_events(example_data_5_subject)
#' print(all_outputs$subject_summary)
#' print(all_outputs$glycemic_event_summary)
#'
#' # Detect all events on larger dataset
#' large_outputs <- detect_all_events(example_data_hall)
#' print(paste("Total subjects analyzed:", nrow(large_outputs$subject_summary)))
NULL

#' @title Find Local Maxima in Glucose Time Series
#' @name find_local_maxima
#' @description
#' Identifies local maxima (peaks) in glucose concentration time series data.
#' Uses a difference-based algorithm to detect peaks where glucose values
#' increase or remain constant for two consecutive points before the peak point,
#' and decrease or remain constant for two consecutive points after the peak point.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @usage find_local_maxima(df)
#' @seealso \link{grid}, \link{mod_grid}, \link{find_new_maxima}
#' @family GRID pipeline
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{local_maxima_vector}: Tibble with R-based (1-indexed) row numbers of local maxima (\code{local_maxima}). The corresponding occurrence time is \code{df$time[local_maxima]} and glucose is \code{df$gl[local_maxima]}.
#'   \item \code{merged_results}: Tibble with local maxima details (\code{id}, \code{time}, \code{gl})
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Find local maxima
#' maxima_result <- find_local_maxima(example_data_5_subject)
#' print(paste("Found", nrow(maxima_result$local_maxima_vector), "local maxima"))
#'
#' # Find maxima on larger dataset
#' large_maxima <- find_local_maxima(example_data_hall)
#' print(paste("Found", nrow(large_maxima$local_maxima_vector), "local maxima in larger dataset"))
#'
#' # View first few maxima
#' head(maxima_result$local_maxima_vector)
#'
#' # View merged results
#' head(maxima_result$merged_results)
NULL

#' @title Find Maximum Glucose After Specified Hours
#' @name find_max_after_hours
#' @description
#' Identifies the maximum glucose value occurring within a specified time window
#' after a given start point. This function is useful for analyzing glucose
#' patterns following specific events or time points.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param start_point_df A dataframe with column \code{start_index} (R-based index into \code{df})
#' @param hours Number of hours to look ahead from the start point
#' @usage find_max_after_hours(df, start_point_df, hours)
#' @section Notes:
#' - \code{start_index} must be valid row numbers in \code{df} (1-indexed).
#' - The search window is (0, \code{hours}] hours after each start index.
#' @seealso \link{mod_grid}, \link{find_local_maxima}, \link{find_new_maxima}, \link{transform_df}
#' @family GRID pipeline
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{max_index}: Tibble with R-based (1-indexed) row numbers of maximum glucose (\code{max_index}).
#'     The corresponding occurrence time is \code{df$time[max_index]} and glucose is \code{df$gl[max_index]}.
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#'   \item \code{episode_start}: Tibble with all episode starts with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{time}: Timestamp at which the maximum occurs; equivalent to \code{df$time[index]}
#'       \item \code{gl}: Glucose value at the maximum; equivalent to \code{df$gl[index]}
#'       \item \code{index}: R-based (1-indexed) row number(s) in \code{df} denoting where the maximum occurs
#'     }
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Create start points for demonstration (using row index)
#' start_index <- seq(1, nrow(example_data_5_subject), by = 100)
#' start_points <- data.frame(start_index = start_index)
#'
#' # Find maximum glucose in next 2 hours
#' max_after <- find_max_after_hours(example_data_5_subject, start_points, hours = 2)
#' print(paste("Found", length(max_after$max_index), "maximum points"))
#'
#' # Find maximum glucose in next 1 hour
#' max_after_1h <- find_max_after_hours(example_data_5_subject, start_points, hours = 1)
#'
#' # Analysis on larger dataset
#' large_start_index <- seq(1, nrow(example_data_hall), by = 200)
#' large_start_points <- data.frame(start_index = large_start_index)
#' large_max_after <- find_max_after_hours(example_data_hall, large_start_points, hours = 2)
#' print(paste("Found", length(large_max_after$max_index), "maximum points in larger dataset"))
NULL

#' @title Find Maximum Glucose Before Specified Hours
#' @name find_max_before_hours
#' @description
#' Identifies the maximum glucose value occurring within a specified time window
#' before a given start point. This function is useful for analyzing glucose
#' patterns preceding specific events or time points.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param start_point_df A dataframe with column \code{start_index} (R-based index into \code{df})
#' @param hours Number of hours to look back from the start point
#' @usage find_max_before_hours(df, start_point_df, hours)
#' @section Notes:
#' - The search window is [\code{hours}, 0) hours before each start index.
#' @seealso \link{mod_grid}, \link{find_local_maxima}, \link{find_new_maxima}
#' @family GRID pipeline
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{max_index}: Tibble with R-based (1-indexed) row numbers of maximum glucose (\code{max_index}).
#'     The corresponding occurrence time is \code{df$time[max_index]} and glucose is \code{df$gl[max_index]}.
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#'   \item \code{episode_start}: Tibble with all episode starts with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{time}: Timestamp at which the maximum occurs; equivalent to \code{df$time[index]}
#'       \item \code{gl}: Glucose value at the maximum; equivalent to \code{df$gl[index]}
#'       \item \code{index}: R-based (1-indexed) row number(s) in \code{df} denoting where the maximum occurs
#'     }
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Create start points for demonstration (using row index)
#' start_index <- seq(1, nrow(example_data_5_subject), by = 100)
#' start_points <- data.frame(start_index = start_index)
#'
#' # Find maximum glucose in previous 2 hours
#' max_before <- find_max_before_hours(example_data_5_subject, start_points, hours = 2)
#' print(paste("Found", length(max_before$max_index), "maximum points"))
#'
#' # Find maximum glucose in previous 1 hour
#' max_before_1h <- find_max_before_hours(example_data_5_subject, start_points, hours = 1)
#'
#' # Analysis on larger dataset
#' large_start_index <- seq(1, nrow(example_data_hall), by = 200)
#' large_start_points <- data.frame(start_index = large_start_index)
#' large_max_before <- find_max_before_hours(example_data_hall, large_start_points, hours = 2)
#' print(paste("Found", length(large_max_before$max_index), "maximum points in larger dataset"))
NULL

#' @title Find Minimum Glucose After Specified Hours
#' @name find_min_after_hours
#' @description
#' Identifies the minimum glucose value occurring within a specified time window
#' after a given start point. This function is useful for analyzing glucose
#' patterns following specific events or time points.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param start_point_df A dataframe with column \code{start_index} (R-based index into \code{df})
#' @param hours Number of hours to look ahead from the start point
#' @usage find_min_after_hours(df, start_point_df, hours)
#' @seealso \link{mod_grid}, \link{find_local_maxima}
#' @family GRID pipeline
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{min_index}: Tibble with R-based (1-indexed) row numbers of minimum glucose (\code{min_index}). The corresponding occurrence time is \code{df$time[min_index]} and glucose is \code{df$gl[min_index]}.
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#'   \item \code{episode_start}: Tibble with all episode starts with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{time}: Timestamp at which the minimum occurs; equivalent to \code{df$time[index]}
#'       \item \code{gl}: Glucose value at the minimum; equivalent to \code{df$gl[index]}
#'       \item \code{index}: R-based (1-indexed) row number(s) in \code{df} denoting where the minimum occurs
#'     }
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Create start points for demonstration (using row index)
#' start_index <- seq(1, nrow(example_data_5_subject), by = 100)
#' start_points <- data.frame(start_index = start_index)
#'
#' # Find minimum glucose in next 2 hours
#' min_after <- find_min_after_hours(example_data_5_subject, start_points, hours = 2)
#' print(paste("Found", length(min_after$min_index), "minimum points"))
#'
#' # Find minimum glucose in next 1 hour
#' min_after_1h <- find_min_after_hours(example_data_5_subject, start_points, hours = 1)
#'
#' # Analysis on larger dataset
#' large_start_index <- seq(1, nrow(example_data_hall), by = 200)
#' large_start_points <- data.frame(start_index = large_start_index)
#' large_min_after <- find_min_after_hours(example_data_hall, large_start_points, hours = 2)
#' print(paste("Found", length(large_min_after$min_index), "minimum points in larger dataset"))
NULL

#' @title Find Minimum Glucose Before Specified Hours
#' @name find_min_before_hours
#' @description
#' Identifies the minimum glucose value occurring within a specified time window
#' before a given start point. This function is useful for analyzing glucose
#' patterns preceding specific events or time points.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param start_point_df A dataframe with column \code{start_index} (R-based index into \code{df})
#' @param hours Number of hours to look back from the start point
#' @usage find_min_before_hours(df, start_point_df, hours)
#' @seealso \link{mod_grid}, \link{find_local_maxima}
#' @family GRID pipeline
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{min_index}: Tibble with R-based (1-indexed) row numbers of minimum glucose (\code{min_index}). The corresponding occurrence time is \code{df$time[min_index]} and glucose is \code{df$gl[min_index]}.
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#'   \item \code{episode_start}: Tibble with all episode starts with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{time}: Timestamp at which the minimum occurs; equivalent to \code{df$time[index]}
#'       \item \code{gl}: Glucose value at the minimum; equivalent to \code{df$gl[index]}
#'       \item \code{index}: R-based (1-indexed) row number(s) in \code{df} denoting where the minimum occurs
#'     }
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Create start points for demonstration (using row index)
#' start_index <- seq(1, nrow(example_data_5_subject), by = 100)
#' start_points <- data.frame(start_index = start_index)
#'
#' # Find minimum glucose in previous 2 hours
#' min_before <- find_min_before_hours(example_data_5_subject, start_points, hours = 2)
#' print(paste("Found", length(min_before$min_index), "minimum points"))
#'
#' # Find minimum glucose in previous 1 hour
#' min_before_1h <- find_min_before_hours(example_data_5_subject, start_points, hours = 1)
#'
#' # Analysis on larger dataset
#' large_start_index <- seq(1, nrow(example_data_hall), by = 200)
#' large_start_points <- data.frame(start_index = large_start_index)
#' large_min_before <- find_min_before_hours(example_data_hall, large_start_points, hours = 2)
#' print(paste("Found", length(large_min_before$min_index), "minimum points in larger dataset"))
NULL

#' @title Find New Maxima Around Grid Points
#' @name find_new_maxima
#' @description
#' Identifies new maxima in the vicinity of previously identified grid points,
#' useful for refining maxima detection in GRID analysis. This function helps
#' improve the accuracy of peak detection by searching around known event points.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param mod_grid_max_point_df A dataframe with column \code{index} (candidate maxima index)
#' @param local_maxima_df A dataframe with column \code{local_maxima} (index of local peaks)
#' @usage find_new_maxima(df, mod_grid_max_point_df, local_maxima_df)
#' @seealso \link{find_local_maxima}, \link{find_max_after_hours}, \link{transform_df}
#' @family GRID pipeline
#'
#' @return A tibble with updated maxima information containing columns (\code{id}, \code{time}, \code{gl}, \code{index})
#' The \code{index} column contains R-based (1-indexed) row number(s) in \code{df}; thus, \code{time == df$time[index]} and \code{gl == df$gl[index]}.
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # First, get grid points and local maxima
#' grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
#' maxima_result <- find_local_maxima(example_data_5_subject)
#'
#' # Create modified grid points (simplified for example)
#' mod_grid_indices <- data.frame(index = grid_result$episode_start$index[1:10])
#'
#' # Find new maxima around grid points
#' new_maxima <- find_new_maxima(example_data_5_subject,
#'                               mod_grid_indices,
#'                               maxima_result$local_maxima_vector)
#' print(paste("Found", nrow(new_maxima), "new maxima"))
#'
#' # Analysis on larger dataset
#' large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
#' large_maxima <- find_local_maxima(example_data_hall)
#' large_mod_grid <- data.frame(index = large_grid$episode_start$index[1:20])
#' large_new_maxima <- find_new_maxima(example_data_hall,
#'                                     large_mod_grid,
#'                                     large_maxima$local_maxima_vector)
#' print(paste("Found", nrow(large_new_maxima), "new maxima in larger dataset"))
NULL

#' @title Modified GRID Analysis
#' @name mod_grid
#' @description
#' Constructs a modified GRID series by reapplying the GRID logic with a designated
#' gap (e.g., 60 minutes) and analysis window in hours (e.g., 2 hours). It
#' reassigns GRID events under these constraints to produce a modified grid
#' suitable for downstream maxima mapping and episode analysis.
#'
#' @references
#' Park, Sang Ho, et al. "Identification of clinically meaningful automatically detected postprandial glucose excursions in individuals with type 1 diabetes using personal continuous glucose monitoring." Diabetes Research and Clinical Practice (2025): 112951.
#'
#' Park, Soojin, et al. "High-Amplitude and Prolonged Glucose Excursions as a Key Determinant of Discordance Between Glucose Management Indicator and Glycated Hemoglobin in Type 1 Diabetes." Diabetes Care (2026): dc252820. https://doi.org/10.2337/dc25-2820
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param grid_point_df A dataframe with column \code{start_index} (start points for re-applied GRID)
#' @param hours Time window in hours for analysis (default: 2)
#' @param gap Gap threshold in minutes for event detection (default: 15).
#'   This parameter defines the minimum time interval between consecutive GRID events.
#' @usage mod_grid(df, grid_point_df, hours = 2, gap = 15)
#' @section Units and sampling:
#' - \code{gap} is minutes; \code{hours} is hours; \code{time} is POSIXct.
#' @seealso \link{grid}, \link{find_max_after_hours}, \link{find_new_maxima}
#' @family GRID pipeline
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{mod_grid_vector}: Tibble with modified GRID results (\code{mod_grid})
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#'   \item \code{episode_start}: Tibble with all episode starts with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{time}: Timestamp at which the event occurs; equivalent to \code{df$time[index]}
#'       \item \code{gl}: Glucose value at the event; equivalent to \code{df$gl[index]}
#'       \item \code{index}: R-based (1-indexed) row number(s) in \code{df} denoting where the event occurs
#'     }
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # First, get grid points
#' grid_result <- grid(example_data_5_subject, gap = 60, threshold = 130)
#'
#' # Perform modified GRID analysis
#' mod_result <- mod_grid(example_data_5_subject, grid_result$grid_vector, hours = 2, gap = 60)
#' print(paste("Modified grid points:", nrow(mod_result$mod_grid_vector)))
#'
#' # Modified analysis with different parameters
#' mod_result_1h <- mod_grid(example_data_5_subject, grid_result$grid_vector, hours = 1, gap = 40)
#'
#' # Analysis on larger dataset
#' large_grid <- grid(example_data_hall, gap = 60, threshold = 130)
#' large_mod_result <- mod_grid(example_data_hall, large_grid$grid_vector, hours = 2, gap = 60)
#' print(paste("Modified grid points in larger dataset:", nrow(large_mod_result$mod_grid_vector)))
NULL

#' @title Detect Events Between Maxima
#' @name detect_between_maxima
#' @description
#' Identifies and analyzes events occurring between detected maxima points,
#' providing detailed episode information for GRID analysis. This function
#' helps characterize the glucose dynamics between identified peaks.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param transform_df A dataframe containing summary information from previous transformations
#' @usage detect_between_maxima(df, transform_df)
#' @seealso \link{grid}, \link{mod_grid}, \link{find_new_maxima}, \link{transform_df}
#' @family GRID pipeline
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{results}: Tibble with events between maxima (\code{id}, \code{grid_time}, \code{grid_gl}, \code{maxima_time}, \code{maxima_glucose}, \code{time_to_peak})
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Complete pipeline to get transform_df
#' grid_result <- grid(example_data_5_subject, gap = 60, threshold = 130)
#' maxima_result <- find_local_maxima(example_data_5_subject)
#' mod_result <- mod_grid(example_data_5_subject, grid_result$grid_vector, hours = 2, gap = 60)
#' max_after <- find_max_after_hours(example_data_5_subject, mod_result$mod_grid_vector, hours = 2)
#' new_maxima <- find_new_maxima(example_data_5_subject,
#'                               max_after$max_index,
#'                               maxima_result$local_maxima_vector)
#' transformed <- transform_df(grid_result$episode_start, new_maxima)
#'
#' # Detect events between maxima
#' between_events <- detect_between_maxima(example_data_5_subject, transformed)
#' print(paste("Events between maxima:", length(between_events)))
#'
#' # Analysis on larger dataset
#' large_grid <- grid(example_data_hall, gap = 60, threshold = 130)
#' large_maxima <- find_local_maxima(example_data_hall)
#' large_mod <- mod_grid(example_data_hall, large_grid$grid_vector, hours = 2, gap = 60)
#' large_max_after <- find_max_after_hours(example_data_hall, large_mod$mod_grid_vector, hours = 2)
#' large_new_maxima <- find_new_maxima(example_data_hall,
#'                                     large_max_after$max_index,
#'                                     large_maxima$local_maxima_vector)
#' large_transformed <- transform_df(large_grid$episode_start, large_new_maxima)
#' large_between <- detect_between_maxima(example_data_hall, large_transformed)
#' print(paste("Events between maxima in larger dataset:", length(large_between)))
NULL

#' @title Calculate Glucose Excursions
#' @name excursion
#' @description
#' Calculates glucose excursions in CGM data. An excursion is defined as
#' a \eqn{>} 70 mg/dL (\eqn{>} 3.9 mmol/L) rise within 2 hours, not preceded by a value
#' \eqn{<} 70 mg/dL (\eqn{<} 3.9 mmol/L).
#'
#' @references
#' Edwards, S., et al. (2022). Use of connected pen as a diagnostic tool to evaluate missed bolus dosing behavior in people with type 1 and type 2 diabetes. Diabetes Technology & Therapeutics, 24(1), 61-66.
#'
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param gap Gap threshold in minutes for excursion calculation (default: 15).
#'   This parameter defines the minimum time interval between consecutive GRID events.
#' @usage excursion(df, gap = 15)
#' @section Notes:
#' - \code{gap} is minutes; change to enforce minimum separation between excursions.
#' - This function operates on the rows supplied in \code{df}. It does not use
#'   \code{\link{interpolate_cgm}} or the full-day event preprocessing grid.
#' @seealso \link{grid}
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{excursion_vector}: Tibble with excursion results (\code{excursion})
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#'   \item \code{episode_start}: Tibble with all episode starts with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{time}: Timestamp at which the event occurs; equivalent to \code{df$time[index]}
#'       \item \code{gl}: Glucose value at the event; equivalent to \code{df$gl[index]}
#'       \item \code{index}: R-based (1-indexed) row number(s) in \code{df} denoting where the event occurs
#'     }
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Calculate glucose excursions
#' excursion_result <- excursion(example_data_5_subject, gap = 15)
#' print(paste("Excursion vector length:", length(excursion_result$excursion_vector)))
#' print(excursion_result$episode_counts)
#'
#' # Excursion analysis with different gap
#' excursion_30min <- excursion(example_data_5_subject, gap = 30)
#'
#' # Analysis on larger dataset
#' large_excursion <- excursion(example_data_hall, gap = 15)
#' print(paste("Excursion vector length in larger dataset:", length(large_excursion$excursion_vector)))
#' print(paste("Total episodes:", sum(large_excursion$episode_counts$episode_counts)))
NULL

#' @title Find Start Points for Event Analysis
#' @name start_finder
#' @description
#' Finds R-based (1-indexed) positions where the value is 1 in an integer vector
#' of 0s and 1s, specifically identifying episode start points. This function
#' looks for positions where a 1 follows a 0 or is at the beginning of the vector,
#' which is useful for identifying the start of glycemic events or episodes.
#'
#' @param df A dataframe with the first column containing an integer vector of 0s and 1s
#' @usage start_finder(df)
#' @section Notes:
#' - Returns R-based \code{start_index} positions relative to the provided input vector/dataframe.
#' - If used on vectors derived from a CGM \code{df}, index map directly to \code{df} rows.
#' @seealso \link{grid}, \link{mod_grid}
#' @family GRID pipeline
#'
#' @return A tibble containing start_index with R-based (1-indexed) positions where episodes start
#' Note: These index refer to positions in the provided input vector/dataframe, not necessarily rows of the original CGM \code{df} unless that vector was derived directly from \code{df} in row order.
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Create a binary vector indicating episode starts
#' binary_vector <- c(0, 0, 1, 1, 0, 1, 0, 0, 1, 1)
#' df <- data.frame(episode_starts = binary_vector)
#'
#' # Find R-based index where episodes start
#' start_points <- start_finder(df)
#' print(paste("Start index:", paste(start_points$start_index, collapse = ", ")))
#'
#' # Use with actual GRID results
#' grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
#' grid_starts <- start_finder(grid_result$grid_vector)
#' print(paste("GRID episode starts:", length(grid_starts$start_index)))
#'
#' # Analysis on larger dataset
#' large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
#' large_starts <- start_finder(large_grid$grid_vector)
#' print(paste("GRID episode starts in larger dataset:", length(large_starts$start_index)))
NULL

#' @title Transform Dataframe for Analysis
#' @name transform_df
#' @description
#' Performs data transformations required for GRID analysis, including
#' mapping GRID episode starts to maxima within a 4-hour window and
#' merging grid and maxima information. This function prepares data
#' for downstream analysis by combining these results.
#'
#' @param grid_df A dataframe containing grid analysis results
#' @param maxima_df A dataframe containing maxima detection results
#' @usage transform_df(grid_df, maxima_df)
#' @seealso \link{grid}, \link{find_new_maxima}, \link{detect_between_maxima}
#' @family GRID pipeline
#'
#' @return A tibble with transformed data containing columns (\code{id}, \code{grid_time}, \code{grid_gl}, \code{maxima_time}, \code{maxima_gl})
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Complete pipeline example with smaller dataset
#' threshold <- 130
#' gap <- 60
#' hours <- 2
#' # 1) Find GRID points
#' grid_result <- grid(example_data_5_subject, gap = gap, threshold = threshold)

#' # 2) Find modified GRID points before 2 hours minimum
#' mod_grid <- mod_grid(example_data_5_subject,
#'                      start_finder(grid_result$grid_vector),
#'                      hours = hours,
#'                      gap = gap)
#'
#' # 3) Find maximum point 2 hours after mod_grid point
#' mod_grid_maxima <- find_max_after_hours(example_data_5_subject,
#'                                         start_finder(mod_grid$mod_grid_vector),
#'                                         hours = hours)
#'
#' # 4) Identify local maxima around episodes/windows
#' local_maxima <- find_local_maxima(example_data_5_subject)
#'
#' # 5) Among local maxima, find maximum point after two hours
#' final_maxima <- find_new_maxima(example_data_5_subject,
#'                                 mod_grid_maxima$max_index,
#'                                 local_maxima$local_maxima_vector)
#'
#' # 6) Map GRID points to maximum points (within 4 hours)
#' transform_maxima <- transform_df(grid_result$episode_start, final_maxima)
#'
#' # 7) Redistribute overlapping maxima between GRID points
#' final_between_maxima <- detect_between_maxima(example_data_5_subject, transform_maxima)

#' # Complete pipeline example with larger dataset (example_data_hall)
#' # This demonstrates the same workflow on a more comprehensive dataset
#' hall_threshold <- 130
#' hall_gap <- 60
#' hall_hours <- 2
#'
#' # 1) Find GRID points on larger dataset
#' hall_grid_result <- grid(example_data_hall, gap = hall_gap, threshold = hall_threshold)
#'
#' # 2) Find modified GRID points
#' hall_mod_grid <- mod_grid(example_data_hall,
#'                          start_finder(hall_grid_result$grid_vector),
#'                          hours = hall_hours,
#'                          gap = hall_gap)
#'
#' # 3) Find maximum points after mod_grid
#' hall_mod_grid_maxima <- find_max_after_hours(example_data_hall,
#'                                             start_finder(hall_mod_grid$mod_grid_vector),
#'                                             hours = hall_hours)
#'
#' # 4) Identify local maxima
#' hall_local_maxima <- find_local_maxima(example_data_hall)
#'
#' # 5) Find new maxima
#' hall_final_maxima <- find_new_maxima(example_data_hall,
#'                                     hall_mod_grid_maxima$max_index,
#'                                     hall_local_maxima$local_maxima_vector)
#'
#' # 6) Transform data
#' hall_transform_maxima <- transform_df(hall_grid_result$episode_start, hall_final_maxima)
#'
#' # 7) Detect between maxima
#' hall_final_between_maxima <- detect_between_maxima(example_data_hall, hall_transform_maxima)

NULL

#' @title Interpolate CGM Data
#' @name interpolate_cgm
#' @description
#' Interpolates continuous glucose monitoring (CGM) data onto the same
#' iglu-compatible, midnight-aligned full-day grid used internally by
#' cgmguru's event-detection functions. The interpolation is implemented in C++
#' and is intended for users who want to inspect or reuse the preprocessed grid
#' behind \code{\link{detect_all_events}},
#' \code{\link{detect_hyperglycemic_events}}, and
#' \code{\link{detect_hypoglycemic_events}}.
#'
#' For each subject, \code{interpolate_cgm()} builds an equally spaced grid at
#' \code{reading_minutes} intervals. If \code{reading_minutes} is omitted, it is
#' inferred per subject from the median positive timestamp difference. Glucose
#' values are linearly interpolated only across gaps up to \code{inter_gap};
#' larger gaps are treated as missing and removed from the returned data,
#' preserving segment boundaries used by event calculation.
#'
#' The GRID-family functions \code{\link{grid}}, \code{\link{maxima_grid}}, and
#' \code{\link{excursion}} do not call this helper automatically; they operate
#' on the rows supplied by the user unless the caller explicitly passes an
#' interpolated dataset.
#'
#' @param df A dataframe containing CGM data with columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier
#'     \item \code{time}: POSIXct measurement timestamp
#'     \item \code{gl}: Glucose value in mg/dL
#'   }
#' @param reading_minutes Time interval for the interpolation grid in minutes.
#'   If omitted or \code{NULL}, it is calculated automatically per id as the
#'   median positive time difference in the data.
#' @param sort_time Logical. If \code{TRUE}, sort rows within each id by
#'   \code{time} in C++ before interpolation. Defaults to \code{FALSE}.
#' @param inter_gap Maximum gap in minutes to interpolate across. Defaults to
#'   45; larger gaps split the event-detection grid.
#' @usage interpolate_cgm(df, reading_minutes = NULL, sort_time = FALSE,
#'  inter_gap = 45)
#' @return A tibble with columns \code{id}, interpolated \code{time}, and
#'   interpolated \code{gl}. Rows inside gaps larger than \code{inter_gap} are
#'   omitted.
#' @seealso \link{detect_all_events}, \link{detect_hyperglycemic_events},
#'   \link{detect_hypoglycemic_events}
#' @export
#' @examples
#' df <- data.frame(
#'   id = "A",
#'   time = as.POSIXct(c("2026-01-01 00:15:00", "2026-01-01 00:25:00"),
#'                     tz = "UTC"),
#'   gl = c(100, 120)
#' )
#' interpolate_cgm(df)
NULL

#' @title Rcpp CONGA Calculation
#' @name conga_rcpp
#' @description
#' Calculates Continuous Overall Net Glycemic Action (CONGA) with an Rcpp
#' backend. The implementation follows the CONGA calculation approach used by
#' \code{\link[iglu:conga]{iglu::conga}}: after interpolation to a regular
#' day-aligned CGM grid, CONGA is the sample standard deviation of glucose
#' differences separated by \code{n} hours.
#'
#' @param data A dataframe containing CGM data with columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier
#'     \item \code{time}: POSIXct measurement timestamp
#'     \item \code{gl}: Glucose value in mg/dL
#'   }
#' @param n Whole number of hours separating paired glucose observations.
#'   Defaults to 24.
#' @param tz Time zone used for day-grid alignment when supplied.
#' @param inter_gap Maximum gap, in minutes, over which linear interpolation is
#'   allowed. Defaults to 45, matching iglu's internal default.
#' @return A tibble with columns \code{id} and \code{CONGA}.
#' @references
#' McDonnell, C. M., et al. (2005). A novel approach to continuous glucose
#' analysis utilizing glycemic variation. \emph{Diabetes Technology and
#' Therapeutics}, 7(2), 253-263. \doi{10.1089/dia.2005.7.253}
#' @seealso \code{\link[iglu:conga]{iglu::conga}}, \link{mage_rcpp}
#' @export
#' @examples
#' library(iglu)
#' data(example_data_5_subject)
#' conga_rcpp(example_data_5_subject)
NULL

#' @title Rcpp MAGE Calculation
#' @name mage_rcpp
#' @description
#' Calculates Mean Amplitude of Glycemic Excursions (MAGE) with an Rcpp backend.
#' The implementation follows the MAGE calculation approaches used by
#' \code{\link[iglu:mage]{iglu::mage}}. The default \code{version = "ma"}
#' follows iglu's moving-average approach: CGM is interpolated to 5-minute
#' intervals, short and long moving-average crossings identify candidate
#' peak/nadir intervals, and countable excursions are those whose peak-to-nadir
#' or nadir-to-peak amplitude is at least one glucose standard deviation. The
#' \code{version = "naive"} option matches iglu's older standard-deviation
#' based calculation.
#'
#' This function is calculation-only and does not implement iglu's plotting
#' options.
#'
#' @param data A dataframe containing CGM data with columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier
#'     \item \code{time}: POSIXct measurement timestamp
#'     \item \code{gl}: Glucose value in mg/dL
#'   }
#' @param version Either \code{"ma"} or \code{"naive"}. Defaults to \code{"ma"}.
#' @param sd_multiplier Multiplier for the standard deviation threshold in
#'   \code{version = "naive"}.
#' @param short_ma Short moving-average window length. Defaults to 5.
#' @param long_ma Long moving-average window length. Defaults to 32.
#' @param return_type Either \code{"num"} for one metric per id or \code{"df"}
#'   for a list-column of segment-level MAGE rows.
#' @param direction One of \code{"avg"}, \code{"service"}, \code{"max"},
#'   \code{"plus"}, or \code{"minus"}.
#' @param tz Time zone used for day-grid alignment when supplied.
#' @param inter_gap Maximum gap, in minutes, over which linear interpolation is
#'   allowed. Defaults to 45.
#' @param max_gap Gap length, in minutes, above which MAGE is calculated on
#'   separate trace segments. Defaults to 180.
#' @return A tibble with columns \code{id} and \code{MAGE}. With
#'   \code{return_type = "df"}, \code{MAGE} is a list-column of tibbles with
#'   \code{start}, \code{end}, \code{mage}, \code{plus_or_minus}, and
#'   \code{first_excursion}.
#' @references
#' Service, F. John, et al. (1970). Mean amplitude of glycemic excursions, a
#' measure of diabetic instability. \emph{Diabetes}, 19(9), 644-655.
#' \doi{10.2337/diab.19.9.644}
#'
#' Fernandes, Nathaniel J., et al. (2022). Open-source algorithm to calculate
#' mean amplitude of glycemic excursions using short and long moving averages.
#' \emph{Journal of Diabetes Science and Technology}, 16(2), 576-577.
#' \doi{10.1177/19322968211061165}
#' @seealso \code{\link[iglu:mage]{iglu::mage}}, \link{conga_rcpp}
#' @export
#' @examples
#' library(iglu)
#' data(example_data_5_subject)
#' mage_rcpp(example_data_5_subject)
#' mage_rcpp(example_data_5_subject, version = "naive")
NULL

#' @title Calculate Sensor Wear
#' @name sensor_wear
#' @description
#' Calculates the percent of expected CGM readings observed. By default, the
#' calculation uses each subject's original timestamp span from first valid
#' reading to last valid reading. If \code{ndays} is supplied, valid readings
#' in \code{[end_date - ndays, end_date]} are divided by the expected number of
#' readings over that fixed retrospective window.
#'
#' For fixed-window calculations, if \code{end_date = NULL}, each subject's last
#' valid timestamp defines that subject's retrospective window. If \code{end_date}
#' is supplied, the same endpoint is used for all subjects, which is useful for a
#' common study cutoff or report date. Duplicate timestamps within a subject are
#' de-duplicated after sorting, and rows with missing \code{time} or \code{gl}
#' do not count as observed readings.
#'
#' @param df A dataframe containing CGM data with columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier
#'     \item \code{time}: POSIXct measurement timestamp
#'     \item \code{gl}: Glucose value in mg/dL
#'   }
#' @param end_date End timestamp for a fixed-window calculation. Requires
#'   \code{ndays}. If \code{NULL}, each subject's last valid timestamp is used.
#'   \code{Date} values are converted with
#'   \code{as.POSIXct()}, matching iglu's manual active-percent behavior.
#' @param ndays Number of days in the fixed retrospective window. Defaults to
#'   \code{NULL}, which uses the original timestamp span.
#' @param reading_minutes Reading interval in minutes. If \code{NULL}, it is
#'   inferred per id from the median positive difference between valid readings.
#' @usage sensor_wear(df, end_date = NULL, ndays = NULL,
#'  reading_minutes = NULL)
#' @return A tibble with columns \code{id}, \code{sensor_wear_percent},
#'   \code{sensor_wear}, \code{ndays}, \code{start_date}, and
#'   \code{end_date}. \code{sensor_wear} is retained as a backward-compatible
#'   alias.
#' @seealso \link{detect_all_events}, \code{\link[iglu:active_percent]{iglu::active_percent}}
#' @export
#' @examples
#' library(iglu)
#' data(example_data_5_subject)
#' sensor_wear(example_data_5_subject, reading_minutes = 5)
#' sensor_wear(example_data_5_subject, ndays = 90, reading_minutes = 5)
NULL

#' @title Fast Ordering Function
#' @name orderfast
#' @description
#' Orders a dataframe by \code{id} and \code{time} columns using a C++
#' \code{std::sort} backend. Optimized for large CGM datasets, it returns the
#' input with rows sorted by subject then timestamp while preserving all columns.
#'
#' @param df A dataframe with 'id' and 'time' columns
#' @return A dataframe ordered by id and time
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#'
#' # Shuffle without replacement, then order and compare to baseline
#' set.seed(123)
#' shuffled <- example_data_5_subject[sample(seq_len(nrow(example_data_5_subject)),
#'                                           replace = FALSE), ]
#' baseline <- orderfast(example_data_5_subject)
#' ordered_shuffled <- orderfast(shuffled)
#'
#' # Compare results
#' print(paste("Identical after ordering:", identical(baseline, ordered_shuffled)))
#' head(baseline[, c("id", "time", "gl")])
#' head(ordered_shuffled[, c("id", "time", "gl")])
#'
#' # Order larger dataset
#' ordered_large <- orderfast(example_data_hall)
#' print(paste("Ordered", nrow(ordered_large), "rows in larger dataset"))
NULL
