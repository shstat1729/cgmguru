conga_rcpp <- function(data, n = 24, tz = "", inter_gap = 45) {
  tryCatch({
    validated_df <- validate_cgm_data(data)
  }, error = function(e) {
    stop("Error in conga_rcpp(): ", e$message, call. = FALSE)
  })

  n <- validate_numeric_param(n, "n", min_val = 1)
  if (n != round(n)) {
    stop("n must be a whole number of hours", call. = FALSE)
  }
  inter_gap <- validate_numeric_param(inter_gap, "inter_gap", min_val = 0.1)
  if (!is.character(tz) || length(tz) != 1 || is.na(tz)) {
    stop("tz must be a single character string", call. = FALSE)
  }

  tryCatch({
    conga_rcpp_cpp(validated_df, as.integer(n), tz, inter_gap)
  }, error = function(e) {
    stop("Error in conga_rcpp: ", e$message, call. = FALSE)
  })
}

modd_rcpp <- function(data, lag = 1, tz = "", inter_gap = 45) {
  tryCatch({
    validated_df <- validate_cgm_data(data)
  }, error = function(e) {
    stop("Error in modd_rcpp(): ", e$message, call. = FALSE)
  })

  lag <- validate_numeric_param(lag, "lag", min_val = 1)
  if (lag != round(lag)) {
    stop("lag must be a whole number of days", call. = FALSE)
  }
  inter_gap <- validate_numeric_param(inter_gap, "inter_gap", min_val = 0.1)
  if (!is.character(tz) || length(tz) != 1 || is.na(tz)) {
    stop("tz must be a single character string", call. = FALSE)
  }

  tryCatch({
    modd_rcpp_cpp(validated_df, as.integer(lag), tz, inter_gap)
  }, error = function(e) {
    stop("Error in modd_rcpp: ", e$message, call. = FALSE)
  })
}

mage_rcpp <- function(data,
                      version = c("ma", "naive"),
                      sd_multiplier = 1,
                      short_ma = 5,
                      long_ma = 32,
                      return_type = c("num", "df"),
                      direction = c("avg", "service", "max", "plus", "minus"),
                      tz = "",
                      inter_gap = 45,
                      max_gap = 180) {
  version <- match.arg(version)
  return_type <- match.arg(return_type)
  direction <- match.arg(direction)

  tryCatch({
    validated_df <- validate_cgm_data(data)
  }, error = function(e) {
    stop("Error in mage_rcpp(): ", e$message, call. = FALSE)
  })

  sd_multiplier <- validate_numeric_param(sd_multiplier, "sd_multiplier", min_val = 0)
  short_ma <- validate_numeric_param(short_ma, "short_ma", min_val = 1)
  long_ma <- validate_numeric_param(long_ma, "long_ma", min_val = 1)
  if (short_ma != round(short_ma)) {
    stop("short_ma must be a whole number", call. = FALSE)
  }
  if (long_ma != round(long_ma)) {
    stop("long_ma must be a whole number", call. = FALSE)
  }
  inter_gap <- validate_numeric_param(inter_gap, "inter_gap", min_val = 0.1)
  max_gap <- validate_numeric_param(max_gap, "max_gap", min_val = 0.1)
  if (!is.character(tz) || length(tz) != 1 || is.na(tz)) {
    stop("tz must be a single character string", call. = FALSE)
  }

  tryCatch({
    mage_rcpp_cpp(
      validated_df,
      version,
      sd_multiplier,
      as.integer(short_ma),
      as.integer(long_ma),
      return_type,
      direction,
      tz,
      inter_gap,
      max_gap
    )
  }, error = function(e) {
    stop("Error in mage_rcpp: ", e$message, call. = FALSE)
  })
}
