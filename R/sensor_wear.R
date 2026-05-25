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
    sensor_wear_cpp(validated_df, reading_minutes, end_date, ndays)
  }, error = function(e) {
    stop("Error in sensor_wear: ", e$message, call. = FALSE)
  })
}
