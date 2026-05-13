#' Interpolate CGM Data
#'
#' Interpolates CGM data on a source-timestamp-aligned grid using the C++ backend.
#'
#' @param df A dataframe with \code{id}, \code{time}, and \code{gl} columns.
#' @param reading_minutes Time interval for the interpolation grid in minutes.
#'   If \code{NULL}, it is inferred per id from the median positive time
#'   difference.
#' @param sort_time Logical. If \code{TRUE}, sort rows within each id by
#'   \code{time} in C++ before interpolation.
#' @param inter_gap Maximum gap in minutes to interpolate across.
#' @return A dataframe with \code{id}, interpolated \code{time}, \code{gl},
#'   \code{segment}, and \code{reading_minutes}.
#' @export
#' @examples
#' df <- data.frame(
#'   id = "A",
#'   time = as.POSIXct(c("2026-01-01 00:15:00", "2026-01-01 00:25:00"),
#'                     tz = "UTC"),
#'   gl = c(100, 120)
#' )
#' interpolate_cgm(df, reading_minutes = 5)
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
