rebound_events <- function(df, type = c("all", "hypo", "hyper"),
                           data_source = c("raw", "preprocessed"),
                           reading_minutes = NULL, sort_time = FALSE,
                           inter_gap = 45, rebound_minutes = 120,
                           return_interpolated = TRUE) {
  type <- match.arg(type)
  data_source <- match.arg(data_source)

  tryCatch({
    validated_df <- validate_cgm_data(df)
  }, error = function(e) {
    stop("Error in rebound_events(): ", e$message, call. = FALSE)
  })

  reading_minutes <- validate_reading_minutes(reading_minutes, nrow(validated_df))
  sort_time <- validate_logical_param(sort_time, "sort_time")
  inter_gap <- validate_numeric_param(inter_gap, "inter_gap", min_val = 0.1)
  rebound_minutes <- validate_numeric_param(
    rebound_minutes, "rebound_minutes", min_val = 0
  )
  return_interpolated <- validate_logical_param(
    return_interpolated, "return_interpolated"
  )

  tryCatch({
    rebound_events_cpp(
      validated_df, type, data_source, reading_minutes, sort_time, inter_gap,
      rebound_minutes, return_interpolated
    )
  }, error = function(e) {
    stop("Error in rebound_events: ", e$message, call. = FALSE)
  })
}
