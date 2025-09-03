#' Fast Ordering Function
#'
#' Orders a dataframe by id and time columns
#'
#' @param df A dataframe with 'id' and 'time' columns
#' @return A dataframe ordered by id and time
#' @export
#' @examples
#' df <- data.frame(id = c("b", "a", "a"), time = as.POSIXct(
#'   c("2024-01-01 01:00:00", "2024-01-01 00:00:00", "2024-01-01 01:00:00"), tz = "UTC"
#' ))
#' orderfast(df)
orderfast <- function(df) {
  df[order(df$id, df$time), ]
}
