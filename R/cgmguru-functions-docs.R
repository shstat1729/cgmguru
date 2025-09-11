#' @title GRID Algorithm for Glycemic Event Detection
#' @name grid
#' @description
#' Implements the GRID (Glucose Rate Increase Detector) algorithm for detecting 
#' rapid glucose rate increases in continuous glucose monitoring data. The algorithm
#' identifies rapid glucose changes (commonly ≥90–95 mg/dL/hour) based on rate 
#' calculations and applies gap-based criteria for event detection, commonly used
#' for postprandial peak detection.
#'
#' @param df A dataframe containing CGM data with columns: id, time, gl
#' @param gap Gap threshold in minutes for event detection (default: 15)
#' @param threshold GRID slope threshold in mg/dL/hour for event classification (default: 130)
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{results}: Dataframe with GRID analysis results including grid points and timing
#'   \item \code{episode_counts}: Dataframe with episode counts per subject
#'   \item \code{episode_list}: List of episode details for each subject
#' }
#'
#' @export
#' @examples
#' \dontrun{
#' # Example CGM data
#' cgm_data <- data.frame(
#'   id = rep("subject1", 100),
#'   time = seq(as.POSIXct("2024-01-01 00:00:00"), 
#'              as.POSIXct("2024-01-01 23:59:00"), 
#'              by = "15 min"),
#'   gl = rnorm(100, mean = 140, sd = 30)
#' )
#' 
#' # Apply GRID algorithm
#' events <- grid(cgm_data, gap = 15, threshold = 130)
#' }
NULL

#' @title Combined Maxima Detection and GRID Analysis
#' @name maxima_grid
#' @description
#' Fast method for postprandial peak detection that combines local maxima detection 
#' with GRID analysis. Maps maxima to GRID points using a candidate search window
#' and provides comprehensive glycemic event detection and characterization. 
#' Final GRID-to-peak pairing is constrained to at most 4 hours.
#'
#' @param df A dataframe containing CGM data with columns: id, time, gl
#' @param threshold GRID slope threshold in mg/dL/hour for event classification (default: 130)
#' @param gap Gap threshold in minutes for event detection (default: 60)
#' @param hours Time window in hours for maxima analysis (default: 2)
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{results}: Dataframe with combined maxima and GRID analysis results including grid_time, grid_gl, maxima_time, maxima_glucose, time_to_peak, grid_index, maxima_index
#'   \item \code{episode_counts}: Dataframe with episode counts per subject
#'   \item \code{episode_list}: List of episode details for each subject
#' }
#'
#' @export
#' @examples
#' \dontrun{
#' # Perform combined analysis
#' result <- maxima_grid(cgm_data, threshold = 130, gap = 60, hours = 2)
#' }
NULL

#' @title Detect Hyperglycemic Events
#' @name detect_hyperglycemic_events
#' @description
#' Identifies and segments hyperglycemic events in CGM data based on specified
#' glucose thresholds and duration criteria aligned with international consensus 
#' CGM metrics (Battelino et al., 2023). Events are detected when glucose
#' exceeds the start threshold for the minimum duration and ends when glucose
#' falls below the end threshold for the specified end length.
#'
#' @param new_df A dataframe containing CGM data with columns: id, time, gl
#' @param reading_minutes Time interval between readings in minutes (optional). Used to calculate minimum required readings for event validation based on the 3/4 rule: ceil((dur_length / reading_minutes) / 4 * 3)
#' @param dur_length Minimum duration in minutes for event classification (default: 120)
#' @param end_length End length criteria in minutes (default: 15)
#' @param start_gl Starting glucose threshold in mg/dL (default: 250)
#' @param end_gl Ending glucose threshold in mg/dL (default: 180)
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{events_detailed}: Dataframe with detailed event information including start/end times, glucose values, duration, and average glucose
#'   \item \code{episode_counts}: Dataframe with episode counts per subject including total_events, avg_ep_per_day, avg_ep_duration, avg_ep_gl
#'   \item \code{episode_list}: List of episode details for each subject
#' }
#'
#' @export
#' @examples
#' \dontrun{
#' # Level 1 Hyperglycemia Event (≥15 consecutive min of >180 mg/dL)
#' events <- detect_hyperglycemic_events(cgm_data, 
#'                                      start_gl = 180, 
#'                                      dur_length = 15, 
#'                                      end_length = 15, 
#'                                      end_gl = 180)
#' 
#' # Level 2 Hyperglycemia Event (≥15 consecutive min of >250 mg/dL)
#' events <- detect_hyperglycemic_events(cgm_data, 
#'                                      start_gl = 250, 
#'                                      dur_length = 15, 
#'                                      end_length = 15, 
#'                                      end_gl = 250)
#' 
#' # Extended Hyperglycemia Event (>250 mg/dL lasting ≥120 min)
#' events <- detect_hyperglycemic_events(cgm_data)
#' }
NULL

#' @title Detect Hypoglycemic Events
#' @name detect_hypoglycemic_events
#' @description
#' Identifies and segments hypoglycemic events in CGM data based on specified
#' glucose thresholds and duration criteria aligned with international consensus 
#' CGM metrics (Battelino et al., 2023). Events are detected when glucose
#' falls below the start threshold for the minimum duration and ends when glucose
#' rises above the end threshold for the specified end length.
#'
#' @param new_df A dataframe containing CGM data with columns: id, time, gl
#' @param reading_minutes Time interval between readings in minutes (optional). Used to calculate minimum required readings for event validation based on the 3/4 rule: ceil((dur_length / reading_minutes) / 4 * 3)
#' @param dur_length Minimum duration in minutes for event classification (default: 120)
#' @param end_length End length criteria in minutes (default: 15)
#' @param start_gl Starting glucose threshold in mg/dL (default: 70)
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{events_detailed}: Dataframe with detailed event information including start/end times, glucose values, duration, duration_below_54_minutes, and average glucose
#'   \item \code{episode_counts}: Dataframe with episode counts per subject including total_events, avg_ep_per_day, avg_ep_duration, avg_ep_gl
#'   \item \code{episode_list}: List of episode details for each subject
#' }
#'
#' @export
#' @examples
#' \dontrun{
#' # Level 1 Hypoglycemia Event (≥15 consecutive min of <70 mg/dL)
#' events <- detect_hypoglycemic_events(cgm_data, 
#'                                     start_gl = 70, 
#'                                     dur_length = 15, 
#'                                     end_length = 15)
#' 
#' # Level 2 Hypoglycemia Event (≥15 consecutive min of <54 mg/dL)
#' events <- detect_hypoglycemic_events(cgm_data, 
#'                                     start_gl = 54, 
#'                                     dur_length = 15, 
#'                                     end_length = 15)
#' 
#' # Extended Hypoglycemia Event (>120 consecutive min of <70 mg/dL)
#' events <- detect_hypoglycemic_events(cgm_data)
#' }
NULL

#' @title Detect All Glycemic Events
#' @name detect_all_events
#' @description
#' Comprehensive function to detect all types of glycemic events aligned with 
#' international consensus CGM metrics (Battelino et al., 2023). This function 
#' provides a unified interface for detecting multiple event types including 
#' Level 1/2/Extended hypo- and hyperglycemia, and Level 1 excluded events.
#'
#' @param df A dataframe containing CGM data with columns: id, time, gl
#' @param reading_minutes Time interval between readings in minutes (optional). Can be a single integer/numeric value (applied to all subjects) or a vector matching data length (different intervals per subject)
#'
#' @return A dataframe containing:
#' \itemize{
#'   \item \code{id}: Subject identifier
#'   \item \code{type}: Event type (hypo/hyper)
#'   \item \code{level}: Event level (lv1/lv2/extended/lv1_excl)
#'   \item \code{avg_ep_per_day}: Average episodes per day
#'   \item \code{avg_ep_duration}: Average episode duration in minutes
#'   \item \code{avg_ep_gl}: Average glucose during episodes
#'   \item \code{total_episodes}: Total number of episodes
#' }
#'
#' @export
#' @examples
#' \dontrun{
#' # Detect all glycemic events (comprehensive analysis)
#' all_events <- detect_all_events(cgm_data, reading_minutes = 5)
#' 
#' # View results
#' print(all_events)
#' }
NULL

#' @title Find Local Maxima in Glucose Time Series
#' @name find_local_maxima
#' @description
#' Identifies local maxima (peaks) in glucose concentration time series data.
#' Uses a difference-based algorithm to detect peaks where glucose values
#' increase before and decrease after the peak point.
#'
#' @param df A dataframe containing CGM data with columns: id, time, gl
#'
#' @return A list containing:
#' \itemize{
#'   \item \code{results}: Dataframe with local maxima detection results
#'   \item \code{episode_counts}: Dataframe with maxima counts per subject
#'   \item \code{episode_list}: List of maxima details for each subject
#' }
#'
#' @export
#' @examples
#' \dontrun{
#' # Find local maxima
#' maxima <- find_local_maxima(cgm_data)
#' }
NULL

#' @title Find Maximum Glucose After Specified Hours
#' @name find_max_after_hours
#' @description
#' Identifies the maximum glucose value occurring within a specified time window
#' after a given start point. This function is useful for analyzing glucose
#' patterns following specific events or time points.
#'
#' @param df A dataframe containing CGM data with columns: id, time, gl
#' @param start_point_df A dataframe containing start points with columns: id, time
#' @param hours Number of hours to look ahead from the start point
#'
#' @return A dataframe containing maximum glucose values and their timing within the specified window
#'
#' @export
#' @examples
#' \dontrun{
#' # Find maximum glucose in next 2 hours
#' start_points <- data.frame(
#'   id = "subject1",
#'   time = as.POSIXct("2024-01-01 12:00:00")
#' )
#' max_glucose <- find_max_after_hours(cgm_data, start_points, hours = 2)
#' }
NULL

#' @title Find Maximum Glucose Before Specified Hours
#' @name find_max_before_hours
#' @description
#' Identifies the maximum glucose value occurring within a specified time window
#' before a given start point. This function is useful for analyzing glucose
#' patterns preceding specific events or time points.
#'
#' @param df A dataframe containing CGM data with columns: id, time, gl
#' @param start_point_df A dataframe containing start points with columns: id, time
#' @param hours Number of hours to look back from the start point
#'
#' @return A dataframe containing maximum glucose values and their timing within the specified window
#'
#' @export
#' @examples
#' \dontrun{
#' # Find maximum glucose in previous 2 hours
#' start_points <- data.frame(
#'   id = "subject1",
#'   time = as.POSIXct("2024-01-01 12:00:00")
#' )
#' max_glucose <- find_max_before_hours(cgm_data, start_points, hours = 2)
#' }
NULL

#' @title Find Minimum Glucose After Specified Hours
#' @name find_min_after_hours
#' @description
#' Identifies the minimum glucose value occurring within a specified time window
#' after a given start point. This function is useful for analyzing glucose
#' patterns following specific events or time points.
#'
#' @param df A dataframe containing CGM data with columns: id, time, gl
#' @param start_point_df A dataframe containing start points with columns: id, time
#' @param hours Number of hours to look ahead from the start point
#'
#' @return A dataframe containing minimum glucose values and their timing within the specified window
#'
#' @export
#' @examples
#' \dontrun{
#' # Find minimum glucose in next 2 hours
#' start_points <- data.frame(
#'   id = "subject1",
#'   time = as.POSIXct("2024-01-01 12:00:00")
#' )
#' min_glucose <- find_min_after_hours(cgm_data, start_points, hours = 2)
#' }
NULL

#' @title Find Minimum Glucose Before Specified Hours
#' @name find_min_before_hours
#' @description
#' Identifies the minimum glucose value occurring within a specified time window
#' before a given start point. This function is useful for analyzing glucose
#' patterns preceding specific events or time points.
#'
#' @param df A dataframe containing CGM data with columns: id, time, gl
#' @param start_point_df A dataframe containing start points with columns: id, time
#' @param hours Number of hours to look back from the start point
#'
#' @return A dataframe containing minimum glucose values and their timing within the specified window
#'
#' @export
#' @examples
#' \dontrun{
#' # Find minimum glucose in previous 2 hours
#' start_points <- data.frame(
#'   id = "subject1",
#'   time = as.POSIXct("2024-01-01 12:00:00")
#' )
#' min_glucose <- find_min_before_hours(cgm_data, start_points, hours = 2)
#' }
NULL

#' @title Find New Maxima Around Grid Points
#' @name find_new_maxima
#' @description
#' Identifies new maxima in the vicinity of previously identified grid points,
#' useful for refining maxima detection in GRID analysis. This function helps
#' improve the accuracy of peak detection by searching around known event points.
#'
#' @param new_df A dataframe containing CGM data with columns: id, time, gl
#' @param mod_grid_max_point_df A dataframe containing modified grid maximum points
#' @param local_maxima_df A dataframe containing previously identified local maxima
#'
#' @return A dataframe containing updated maxima information incorporating new findings
#'
#' @export
#' @examples
#' \dontrun{
#' # Find new maxima around grid points
#' new_maxima <- find_new_maxima(cgm_data, grid_points, existing_maxima)
#' }
NULL

#' @title Modified GRID Analysis
#' @name mod_grid
#' @description
#' Performs modified GRID analysis with custom parameters for specialized
#' glycemic event detection scenarios. This function allows for more flexible
#' GRID analysis with user-specified grid points and parameters.
#'
#' @param df A dataframe containing CGM data with columns: id, time, gl
#' @param grid_point_df A dataframe containing grid points for analysis
#' @param hours Time window in hours for analysis (default: 2)
#' @param gap Gap threshold in minutes for event detection (default: 15)
#'
#' @return A list containing modified GRID analysis results
#'
#' @export
#' @examples
#' \dontrun{
#' # Perform modified GRID analysis
#' result <- mod_grid(cgm_data, grid_points, hours = 2, gap = 15)
#' }
NULL

#' @title Detect Events Between Maxima
#' @name detect_between_maxima
#' @description
#' Identifies and analyzes events occurring between detected maxima points,
#' providing detailed episode information for GRID analysis. This function
#' helps characterize the glucose dynamics between identified peaks.
#'
#' @param new_df A dataframe containing CGM data with columns: id, time, gl
#' @param transform_df A dataframe containing summary information from previous transformations
#'
#' @return A dataframe containing detailed episode information for events between maxima
#'
#' @export
#' @examples
#' \dontrun{
#' # Detect events between maxima
#' episodes <- detect_between_maxima(cgm_data, summary_data)
#' }
NULL

#' @title Calculate Glucose Excursions
#' @name excursion
#' @description
#' Calculates glucose excursions (deviations from baseline) in CGM data,
#' useful for identifying periods of significant glucose variability.
#' This function helps quantify glucose fluctuations around a baseline value.
#'
#' @param df A dataframe containing CGM data with columns: id, time, gl
#' @param gap Gap threshold in minutes for excursion calculation (default: 15)
#'
#' @return A list containing excursion analysis results with timing and magnitude information
#'
#' @export
#' @examples
#' \dontrun{
#' # Calculate glucose excursions
#' excursions <- excursion(cgm_data, gap = 15)
#' }
NULL

#' @title Find Start Points for Event Analysis
#' @name start_finder
#' @description
#' Identifies optimal start points for glycemic event analysis based on
#' glucose concentration patterns. This function helps determine the best
#' starting points for subsequent event detection algorithms.
#'
#' @param df A dataframe containing CGM data with columns: id, time, gl
#'
#' @return A dataframe containing refined start points optimized for event detection
#'
#' @export
#' @examples
#' \dontrun{
#' # Find start points for analysis
#' start_points <- start_finder(cgm_data)
#' }
NULL

#' @title Transform Dataframe for Analysis
#' @name transform_df
#' @description
#' Performs data transformations required for GRID analysis, including
#' merging grid and maxima information. This function prepares data
#' for downstream analysis by combining different analysis results.
#'
#' @param grid_df A dataframe containing grid analysis results
#' @param maxima_df A dataframe containing maxima detection results
#'
#' @return A dataframe containing transformed data ready for downstream analysis
#'
#' @export
#' @examples
#' \dontrun{
#' # Transform data for analysis
#' transformed_data <- transform_df(grid_results, maxima_results)
#' }
NULL

#' @title Fast Ordering Function
#' @name orderfast
#' @description
#' Orders a dataframe by id and time columns efficiently. This utility function
#' is optimized for large CGM datasets and provides fast sorting capabilities.
#'
#' @param df A dataframe with 'id' and 'time' columns
#' @return A dataframe ordered by id and time
#'
#' @export
#' @examples
#' df <- data.frame(
#'   id = c("b", "a", "a"), 
#'   time = as.POSIXct(c("2024-01-01 01:00:00", "2024-01-01 00:00:00", "2024-01-01 01:00:00"), tz = "UTC")
#' )
#' orderfast(df)
NULL