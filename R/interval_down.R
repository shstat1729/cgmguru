#' Downsample 5-Minute CGM Data to 15-Minute Intervals
#'
#' Converts Dexcom-style 5-minute CGM data to clock-aligned 15-minute
#' intervals. Each subject is grouped by actual time, not by row position, so a
#' missing 5-minute record does not shift later intervals. Each aggregate is
#' labeled at the right boundary of its 15-minute interval: `:15`, `:30`,
#' `:45`, or `:00` past the hour.
#'
#' The glucose value is the mean of all non-missing readings in the interval.
#' A single available value is retained and assigned to the interval-end output
#' time. Intervals with no available glucose values are omitted. The
#' `n_observed = TRUE` adds a column recording how many glucose readings
#' contributed to each result. Rows must be ordered by subject and time before
#' calling this function.
#'
#' @param df A data frame with columns `id`, `time` (POSIXct), and `gl`
#'   (numeric glucose values).
#' @param n_observed Logical. If `TRUE`, include an `n_observed` column with
#'   the number of glucose readings contributing to each result. Defaults to
#'   `FALSE`.
#' @return A data frame with `id`, clock-aligned `time`, and averaged `gl`
#'   columns. If `n_observed = TRUE`, it also includes `n_observed`. The `time`
#'   column retains the input POSIXct time zone.
#' @export
#'
#' @examples
#' dexcom <- data.frame(
#'   id = "participant_1",
#'   time = as.POSIXct(
#'     c("2024-01-01 00:00:00", "2024-01-01 00:05:00",
#'       "2024-01-01 00:10:00", "2024-01-01 00:15:00",
#'       "2024-01-01 00:20:00", "2024-01-01 00:25:00"),
#'     tz = "UTC"
#'   ),
#'   gl = c(100, 110, 120, 130, 140, 150)
#' )
#'
#' interval_down(dexcom)
interval_down <- function(df, n_observed = FALSE) {
  tryCatch({
    validated_df <- validate_cgm_data(df)
  }, error = function(e) {
    stop("Error in interval_down(): ", e$message, call. = FALSE)
  })

  n_observed <- validate_logical_param(n_observed, "n_observed")

  tryCatch({
    interval_down_cpp(validated_df, n_observed)
  }, error = function(e) {
    stop("Error in interval_down: ", e$message, call. = FALSE)
  })
}
