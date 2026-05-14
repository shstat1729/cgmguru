interpolate_cgm <- function(df, reading_minutes = NULL, sort_time = FALSE,
                            inter_gap = 45) {
  tryCatch({
    validated_df <- validate_cgm_data(df)
  }, error = function(e) {
    stop("Error in interpolate_cgm(): ", e$message, call. = FALSE)
  })

  reading_minutes <- validate_reading_minutes(reading_minutes, nrow(validated_df))
  sort_time <- validate_logical_param(sort_time, "sort_time")
  inter_gap <- validate_numeric_param(inter_gap, "inter_gap", min_val = 0.1)

  tryCatch({
    interpolate_cgm_cpp(validated_df, reading_minutes, sort_time, inter_gap)
  }, error = function(e) {
    stop("Error in interpolate_cgm: ", e$message, call. = FALSE)
  })
}
