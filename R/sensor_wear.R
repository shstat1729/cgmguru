#' Calculate Sensor Wear
#'
#' Calculates percent sensor wear over a fixed retrospective window using the
#' C++ backend. This follows the manual-range calculation used by
#' \code{iglu::active_percent(range_type = "manual")}: observed valid CGM
#' readings in \code{[end_date - ndays, end_date]} divided by the expected
#' number of readings in \code{ndays} days.
#'
#' @param df A dataframe with \code{id}, \code{time}, and \code{gl} columns.
#' @param end_date End timestamp for the calculation window. If \code{NULL},
#'   each subject's last valid timestamp is used. If supplied, sensor wear is
#'   calculated up to that same \code{end_date} for every subject, making it
#'   useful for a common study cutoff or report date. \code{Date} values are
#'   converted with \code{as.POSIXct()}, matching iglu's manual active-percent
#'   behavior.
#' @param ndays Number of days in the retrospective window. Defaults to 14.
#' @param reading_minutes Reading interval in minutes. If \code{NULL}, it is
#'   inferred per id from the median positive difference between valid readings.
#' @return A tibble with columns \code{id}, \code{sensor_wear}, \code{ndays},
#'   \code{start_date}, and \code{end_date}.
#' @export
#' @examples
#' library(iglu)
#' data(example_data_5_subject)
#' sensor_wear(example_data_5_subject, ndays = 14, reading_minutes = 5)
sensor_wear <- function(df, end_date = NULL, ndays = 14,
                        reading_minutes = NULL) {
  tryCatch({
    validated_df <- validate_cgm_data(df)
  }, error = function(e) {
    stop("Error in sensor_wear(): ", e$message, call. = FALSE)
  })

  reading_minutes <- validate_reading_minutes(reading_minutes, nrow(validated_df))
  ndays <- validate_numeric_param(ndays, "ndays", min_val = 0.1)

  if (!is.null(end_date)) {
    if (length(end_date) != 1) {
      stop("end_date must be NULL or a single Date/POSIXct value", call. = FALSE)
    }
    tz <- attr(validated_df$time, "tzone")
    if (is.null(tz) || length(tz) == 0 || is.na(tz[1])) {
      tz <- ""
    } else {
      tz <- tz[1]
    }
    end_date <- as.POSIXct(end_date, tz = tz)
    if (is.na(end_date)) {
      stop("end_date must be convertible to POSIXct", call. = FALSE)
    }
  }

  tryCatch({
    manual_sensor_wear_cpp(validated_df, reading_minutes, end_date, ndays)
  }, error = function(e) {
    stop("Error in sensor_wear: ", e$message, call. = FALSE)
  })
}
