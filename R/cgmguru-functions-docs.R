#' @title GRID Algorithm for Glycemic Event Detection
#' @name grid
#' @description
#' Implements the GRID (Glucose Rate Increase Detector) algorithm for detecting rapid glucose rate increases in continuous glucose monitoring (CGM) data.
#' This algorithm identifies rapid glucose changes using specific rate-based criteria, and is commonly applied for meal detection.
#' Meals are detected when the CGM value is ≥7.2 mmol/L (≥130 mg/dL) and the rate-of-change is ≥5.3 mmol/L/h [≥95 mg/dL/h] for the last two consecutive readings, or ≥5.0 mmol/L/h [≥90 mg/dL/h] for two of the last three readings.
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
#'       \item \code{indices}: R-based (1-indexed) row number(s) in the original dataframe where the GRID event occurs. The occurrence time equals \code{df$time[indices]} and glucose equals \code{df$gl[indices]}.
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
#' @description
#' Identifies and segments hyperglycemic events in CGM data based on international consensus 
#' CGM metrics (Battelino et al., 2023). Supports three event types:
#' \itemize{
#'   \item \strong{Level 1}: ≥15 consecutive min of >180 mg/dL, ends with ≥15 consecutive min ≤180 mg/dL
#'   \item \strong{Level 2}: ≥15 consecutive min of >250 mg/dL, ends with ≥15 consecutive min ≤250 mg/dL
#'   \item \strong{Extended}: >250 mg/dL lasting ≥90 cumulative min within a 120-min period, ends when glucose returns to ≤180 mg/dL 
#'     for ≥15 consecutive min after
#' }
#' Events are detected when glucose exceeds the start threshold for the minimum duration and ends
#' when glucose falls below the end threshold for the specified end length.
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
#' @param reading_minutes Time interval between readings in minutes (optional)
#' @param dur_length Minimum duration in minutes for event classification (default: 120)
#' @param end_length End length criteria in minutes (default: 15)
#' @param start_gl Starting glucose threshold in mg/dL (default: 250)
#' @param end_gl Ending glucose threshold in mg/dL (default: 180)
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{events_total}: Tibble with summary statistics per subject (id, total_events, avg_ep_per_day)
#'   \item \code{events_detailed}: Tibble with detailed event information (id, start_time, start_glucose, end_time, end_glucose, start_indices, end_indices)
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#' 
#' # Level 1 Hyperglycemia (≥15 consecutive min >180 mg/dL, ends ≤180 mg/dL ≥15 min)
#' hyper_lv1 <- detect_hyperglycemic_events(
#'   example_data_5_subject, 
#'   start_gl = 180, 
#'   dur_length = 15, 
#'   end_length = 15, 
#'   end_gl = 180
#' )
#' print(hyper_lv1$events_total)
#' 
#' # Level 2 Hyperglycemia (≥15 consecutive min >250 mg/dL, ends ≤250 mg/dL ≥15 min)
#' hyper_lv2 <- detect_hyperglycemic_events(
#'   example_data_5_subject, 
#'   start_gl = 250, 
#'   dur_length = 15, 
#'   end_length = 15, 
#'   end_gl = 250
#' )
#' print(hyper_lv2$events_total)
#' 
#' # Extended Hyperglycemia (>250 mg/dL ≥90 cumulative min within 120-min period, ends ≤180 mg/dL ≥15 min after)
#' hyper_extended <- detect_hyperglycemic_events(example_data_5_subject)
#' print(hyper_extended$events_total)
#' 
#' # Compare event rates across levels
#' cat("Level 1 events:", sum(hyper_lv1$events_total$total_events), "\n")
#' cat("Level 2 events:", sum(hyper_lv2$events_total$total_events), "\n")
#' cat("Extended events:", sum(hyper_extended$events_total$total_events), "\n")
#' 
#' # Analysis on larger dataset with Level 1 criteria
#' large_hyper <- detect_hyperglycemic_events(example_data_hall, 
#'                                           start_gl = 180, 
#'                                           dur_length = 15, 
#'                                           end_length = 15, 
#'                                           end_gl = 180)
#' print(large_hyper$events_total)
#' 
#' # Analysis on larger dataset with Level 2 criteria
#' large_hyper_lv2 <- detect_hyperglycemic_events(example_data_hall,
#'                                                start_gl = 250,
#'                                                dur_length = 15,
#'                                                end_length = 15,
#'                                                end_gl = 250)
#' print(large_hyper_lv2$events_total)
#' 
#' # Analysis on larger dataset with Extended criteria
#' large_hyper_extended <- detect_hyperglycemic_events(example_data_hall)
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
#' @description
#' Identifies and segments hypoglycemic events in CGM data based on international consensus 
#' CGM metrics (Battelino et al., 2023). Supports three event types:
#' \itemize{
#'   \item \strong{Level 1}: ≥15 consecutive min of <70 mg/dL, ends with ≥15 consecutive min ≥70 mg/dL
#'   \item \strong{Level 2}: ≥15 consecutive min of <54 mg/dL, ends with ≥15 consecutive min ≥54 mg/dL
#'   \item \strong{Extended}: >120 consecutive min of <70 mg/dL, ends with ≥15 consecutive min ≥70 mg/dL
#' }
#' Events are detected when glucose falls below the start threshold for the minimum duration and ends
#' when glucose rises above the end threshold for the specified end length.
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
#' @param reading_minutes Time interval between readings in minutes (optional)
#' @param dur_length Minimum duration in minutes for event classification (default: 120)
#' @param end_length End length criteria in minutes (default: 15)
#' @param start_gl Starting glucose threshold in mg/dL (default: 70)
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{events_total}: Tibble with summary statistics per subject (id, total_events, avg_ep_per_day)
#'   \item \code{events_detailed}: Tibble with detailed event information (id, start_time, start_glucose, end_time, end_glucose, start_indices, end_indices, duration_below_54_minutes)
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#' 
#' # Level 1 Hypoglycemia (<70 mg/dL ≥15 consecutive min, ends ≥70 mg/dL ≥15 min)
#' hypo_lv1 <- detect_hypoglycemic_events(
#'   example_data_5_subject, 
#'   start_gl = 70, 
#'   dur_length = 15, 
#'   end_length = 15
#' )
#' print(hypo_lv1$events_total)
#' 
#' # Level 2 Hypoglycemia (<54 mg/dL ≥15 consecutive min, ends ≥54 mg/dL ≥15 min)
#' hypo_lv2 <- detect_hypoglycemic_events(
#'   example_data_5_subject, 
#'   start_gl = 54, 
#'   dur_length = 15, 
#'   end_length = 15
#' )
#' 
#' # Extended Hypoglycemia (<70 mg/dL ≥120 consecutive min, ends ≥70 mg/dL ≥15 min)
#' hypo_extended <- detect_hypoglycemic_events(example_data_5_subject)
#' print(hypo_extended$events_total)
#' 
#' # Compare event rates across levels
#' cat("Level 1 events:", sum(hypo_lv1$events_total$total_events), "\n")
#' cat("Level 2 events:", sum(hypo_lv2$events_total$total_events), "\n")
#' cat("Extended events:", sum(hypo_extended$events_total$total_events), "\n")
#' 
#' # Analysis on larger dataset with Level 1 criteria
#' large_hypo <- detect_hypoglycemic_events(example_data_hall, 
#'                                          start_gl = 70, 
#'                                          dur_length = 15, 
#'                                          end_length = 15)
#' print(large_hypo$events_total)
#' 
#' # Analysis on larger dataset with Level 2 criteria
#' large_hypo_lv2 <- detect_hypoglycemic_events(example_data_hall,
#'                                              start_gl = 54,
#'                                              dur_length = 15,
#'                                              end_length = 15)
#' print(large_hypo_lv2$events_total)
#' 
#' # Analysis on larger dataset with Extended criteria
#' large_hypo_extended <- detect_hypoglycemic_events(example_data_hall)
#' print(large_hypo_extended$events_total)
NULL

#' @title Detect All Glycemic Events
#' @name detect_all_events
#' @description
#' Comprehensive function to detect all types of glycemic events aligned with 
#' international consensus CGM metrics (Battelino et al., 2023). This function 
#' provides a unified interface for detecting multiple event types including 
#' Level 1/2/Extended hypo- and hyperglycemia, and Level 1 excluded events.
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
#' @param reading_minutes Time interval between readings in minutes (optional). Can be a single integer/numeric value (applied to all subjects) or a vector matching data length (different intervals per subject)
#'
#' @return A tibble containing comprehensive event analysis with columns:
#' \itemize{
#'   \item \code{id}: Subject identifier
#'   \item \code{type}: Event type (hypo/hyper)
#'   \item \code{level}: Event level (lv1/lv2/extended/lv1_excl)
#'   \item \code{total_episodes}: Total number of episodes
#'   \item \code{avg_ep_per_day}: Average episodes per day
#'   \item \code{avg_episode_duration_below_54}: Average episode duration below 54 mg/dL in minutes (hypoglycemic events only)
#' }
#'
#' @export
#' @examples
#' # Load sample data
#' library(iglu)
#' data(example_data_5_subject)
#' data(example_data_hall)
#' 
#' # Detect all glycemic events with 5-minute reading intervals
#' all_events <- detect_all_events(example_data_5_subject, reading_minutes = 5)
#' print(all_events)
#' 
#' # Detect all events on larger dataset
#' large_all_events <- detect_all_events(example_data_hall, reading_minutes = 5)
#' print(paste("Total event types analyzed:", nrow(large_all_events)))
#' 
#' # Filter for specific event types
#' hyperglycemia_events <- all_events[all_events$type == "hyper", ]
#' hypoglycemia_events <- all_events[all_events$type == "hypo", ]
#' 
#' print("Hyperglycemia events:")
#' print(hyperglycemia_events)
#' print("Hypoglycemia events:")
#' print(hypoglycemia_events)
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
#' @param start_point_df A dataframe containing start points with columns: id, time
#' @param hours Number of hours to look ahead from the start point
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{max_indices}: Tibble with R-based (1-indexed) row numbers of maximum glucose (\code{max_indices}).
#'     The corresponding occurrence time is \code{df$time[max_indices]} and glucose is \code{df$gl[max_indices]}.
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#'   \item \code{episode_start}: Tibble with all episode starts with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{time}: Timestamp at which the maximum occurs; equivalent to \code{df$time[indices]}
#'       \item \code{gl}: Glucose value at the maximum; equivalent to \code{df$gl[indices]}
#'       \item \code{indices}: R-based (1-indexed) row number(s) in \code{df} denoting where the maximum occurs
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
#' # Create start points for demonstration (using row indices)
#' start_indices <- seq(1, nrow(example_data_5_subject), by = 100)
#' start_points <- data.frame(start_indices = start_indices)
#' 
#' # Find maximum glucose in next 2 hours
#' max_after <- find_max_after_hours(example_data_5_subject, start_points, hours = 2)
#' print(paste("Found", length(max_after$max_indices), "maximum points"))
#' 
#' # Find maximum glucose in next 1 hour
#' max_after_1h <- find_max_after_hours(example_data_5_subject, start_points, hours = 1)
#' 
#' # Analysis on larger dataset
#' large_start_indices <- seq(1, nrow(example_data_hall), by = 200)
#' large_start_points <- data.frame(start_indices = large_start_indices)
#' large_max_after <- find_max_after_hours(example_data_hall, large_start_points, hours = 2)
#' print(paste("Found", length(large_max_after$max_indices), "maximum points in larger dataset"))
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
#' @param start_point_df A dataframe containing start points with columns: id, time
#' @param hours Number of hours to look back from the start point
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{max_indices}: Tibble with R-based (1-indexed) row numbers of maximum glucose (\code{max_indices}).
#'     The corresponding occurrence time is \code{df$time[max_indices]} and glucose is \code{df$gl[max_indices]}.
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#'   \item \code{episode_start}: Tibble with all episode starts with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{time}: Timestamp at which the maximum occurs; equivalent to \code{df$time[indices]}
#'       \item \code{gl}: Glucose value at the maximum; equivalent to \code{df$gl[indices]}
#'       \item \code{indices}: R-based (1-indexed) row number(s) in \code{df} denoting where the maximum occurs
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
#' # Create start points for demonstration (using row indices)
#' start_indices <- seq(1, nrow(example_data_5_subject), by = 100)
#' start_points <- data.frame(start_indices = start_indices)
#' 
#' # Find maximum glucose in previous 2 hours
#' max_before <- find_max_before_hours(example_data_5_subject, start_points, hours = 2)
#' print(paste("Found", length(max_before$max_indices), "maximum points"))
#' 
#' # Find maximum glucose in previous 1 hour
#' max_before_1h <- find_max_before_hours(example_data_5_subject, start_points, hours = 1)
#' 
#' # Analysis on larger dataset
#' large_start_indices <- seq(1, nrow(example_data_hall), by = 200)
#' large_start_points <- data.frame(start_indices = large_start_indices)
#' large_max_before <- find_max_before_hours(example_data_hall, large_start_points, hours = 2)
#' print(paste("Found", length(large_max_before$max_indices), "maximum points in larger dataset"))
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
#' @param start_point_df A dataframe containing start points with columns: id, time
#' @param hours Number of hours to look ahead from the start point
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{min_indices}: Tibble with R-based (1-indexed) row numbers of minimum glucose (\code{min_indices}). The corresponding occurrence time is \code{df$time[min_indices]} and glucose is \code{df$gl[min_indices]}.
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#'   \item \code{episode_start}: Tibble with all episode starts with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{time}: Timestamp at which the minimum occurs; equivalent to \code{df$time[indices]}
#'       \item \code{gl}: Glucose value at the minimum; equivalent to \code{df$gl[indices]}
#'       \item \code{indices}: R-based (1-indexed) row number(s) in \code{df} denoting where the minimum occurs
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
#' # Create start points for demonstration (using row indices)
#' start_indices <- seq(1, nrow(example_data_5_subject), by = 100)
#' start_points <- data.frame(start_indices = start_indices)
#' 
#' # Find minimum glucose in next 2 hours
#' min_after <- find_min_after_hours(example_data_5_subject, start_points, hours = 2)
#' print(paste("Found", length(min_after$min_indices), "minimum points"))
#' 
#' # Find minimum glucose in next 1 hour
#' min_after_1h <- find_min_after_hours(example_data_5_subject, start_points, hours = 1)
#' 
#' # Analysis on larger dataset
#' large_start_indices <- seq(1, nrow(example_data_hall), by = 200)
#' large_start_points <- data.frame(start_indices = large_start_indices)
#' large_min_after <- find_min_after_hours(example_data_hall, large_start_points, hours = 2)
#' print(paste("Found", length(large_min_after$min_indices), "minimum points in larger dataset"))
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
#' @param start_point_df A dataframe containing start points with columns: id, time
#' @param hours Number of hours to look back from the start point
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{min_indices}: Tibble with R-based (1-indexed) row numbers of minimum glucose (\code{min_indices}). The corresponding occurrence time is \code{df$time[min_indices]} and glucose is \code{df$gl[min_indices]}.
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#'   \item \code{episode_start}: Tibble with all episode starts with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{time}: Timestamp at which the minimum occurs; equivalent to \code{df$time[indices]}
#'       \item \code{gl}: Glucose value at the minimum; equivalent to \code{df$gl[indices]}
#'       \item \code{indices}: R-based (1-indexed) row number(s) in \code{df} denoting where the minimum occurs
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
#' # Create start points for demonstration (using row indices)
#' start_indices <- seq(1, nrow(example_data_5_subject), by = 100)
#' start_points <- data.frame(start_indices = start_indices)
#' 
#' # Find minimum glucose in previous 2 hours
#' min_before <- find_min_before_hours(example_data_5_subject, start_points, hours = 2)
#' print(paste("Found", length(min_before$min_indices), "minimum points"))
#' 
#' # Find minimum glucose in previous 1 hour
#' min_before_1h <- find_min_before_hours(example_data_5_subject, start_points, hours = 1)
#' 
#' # Analysis on larger dataset
#' large_start_indices <- seq(1, nrow(example_data_hall), by = 200)
#' large_start_points <- data.frame(start_indices = large_start_indices)
#' large_min_before <- find_min_before_hours(example_data_hall, large_start_points, hours = 2)
#' print(paste("Found", length(large_min_before$min_indices), "minimum points in larger dataset"))
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
#' @param mod_grid_max_point_df A dataframe containing modified grid maximum points
#' @param local_maxima_df A dataframe containing previously identified local maxima
#'
#' @return A tibble with updated maxima information containing columns (\code{id}, \code{time}, \code{gl}, \code{indices})
#' The \code{indices} column contains R-based (1-indexed) row number(s) in \code{df}; thus, \code{time == df$time[indices]} and \code{gl == df$gl[indices]}.
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
#' mod_grid_indices <- data.frame(indices = grid_result$episode_start$indices[1:10])
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
#' large_mod_grid <- data.frame(indices = large_grid$episode_start$indices[1:20])
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
#' @param df A dataframe containing continuous glucose monitoring (CGM) data.
#'   Must include columns:
#'   \itemize{
#'     \item \code{id}: Subject identifier (string or factor)
#'     \item \code{time}: Time of measurement (POSIXct)
#'     \item \code{gl}: Glucose value (integer or numeric, mg/dL)
#'   }
#' @param grid_point_df A dataframe containing grid points for analysis
#' @param hours Time window in hours for analysis (default: 2)
#' @param gap Gap threshold in minutes for event detection (default: 15).
#'   This parameter defines the minimum time interval between consecutive GRID events.
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{mod_grid_vector}: Tibble with modified GRID results (\code{mod_grid})
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#'   \item \code{episode_start}: Tibble with all episode starts with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{time}: Timestamp at which the event occurs; equivalent to \code{df$time[indices]}
#'       \item \code{gl}: Glucose value at the event; equivalent to \code{df$gl[indices]}
#'       \item \code{indices}: R-based (1-indexed) row number(s) in \code{df} denoting where the event occurs
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
#'                               max_after$max_indices, 
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
#'                                     large_max_after$max_indices, 
#'                                     large_maxima$local_maxima_vector)
#' large_transformed <- transform_df(large_grid$episode_start, large_new_maxima)
#' large_between <- detect_between_maxima(example_data_hall, large_transformed)
#' print(paste("Events between maxima in larger dataset:", length(large_between)))
NULL

#' @title Calculate Glucose Excursions
#' @name excursion
#' @description
#' Calculates glucose excursions in CGM data. An excursion is defined as
#' a >70 mg/dL (>3.9 mmol/L) rise within 2 hours, not preceded by a value
#' <70 mg/dL (<3.9 mmol/L).
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
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{excursion_vector}: Tibble with excursion results (\code{excursion})
#'   \item \code{episode_counts}: Tibble with episode counts per subject (\code{id}, \code{episode_counts})
#'   \item \code{episode_start}: Tibble with all episode starts with columns:
#'     \itemize{
#'       \item \code{id}: Subject identifier
#'       \item \code{time}: Timestamp at which the event occurs; equivalent to \code{df$time[indices]}
#'       \item \code{gl}: Glucose value at the event; equivalent to \code{df$gl[indices]}
#'       \item \code{indices}: R-based (1-indexed) row number(s) in \code{df} denoting where the event occurs
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
#'
#' @return A tibble containing start_indices with R-based (1-indexed) positions where episodes start
#' Note: These indices refer to positions in the provided input vector/dataframe, not necessarily rows of the original CGM \code{df} unless that vector was derived directly from \code{df} in row order.
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
#' # Find R-based indices where episodes start
#' start_points <- start_finder(df)
#' print(paste("Start indices:", paste(start_points$start_indices, collapse = ", ")))
#' 
#' # Use with actual GRID results
#' grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
#' grid_starts <- start_finder(grid_result$grid_vector)
#' print(paste("GRID episode starts:", length(grid_starts$start_indices)))
#' 
#' # Analysis on larger dataset
#' large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
#' large_starts <- start_finder(large_grid$grid_vector)
#' print(paste("GRID episode starts in larger dataset:", length(large_starts$start_indices)))
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
#'                                 mod_grid_maxima$max_indices, 
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
#'                                     hall_mod_grid_maxima$max_indices, 
#'                                     hall_local_maxima$local_maxima_vector)
#' 
#' # 6) Transform data
#' hall_transform_maxima <- transform_df(hall_grid_result$episode_start, hall_final_maxima)
#' 
#' # 7) Detect between maxima
#' hall_final_between_maxima <- detect_between_maxima(example_data_hall, hall_transform_maxima)

NULL

#' @title Fast Ordering Function
#' @name orderfast
#' @description
#' Orders a dataframe by \code{id} and \code{time} columns efficiently using base R's
#' \code{order}. Optimized for large CGM datasets, it returns the input with rows
#' sorted by subject then timestamp while preserving all columns.
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