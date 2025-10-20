# Input validation functions for cgmguru package
# These functions provide validation helpers for the safe wrapper functions

#' Validate CGM data input
#' @param df Data frame to validate
#' @param required_cols Required column names
#' @param optional_cols Optional column names
#' @return Validated and cleaned data frame
#' @noRd
validate_cgm_data <- function(df, required_cols = c("id", "time", "gl"), optional_cols = c("tz")) {
  # Check if input is a data frame
  if (!is.data.frame(df)) {
    stop("Input must be a data frame")
  }
  
  # Check if required columns exist
  missing_cols <- setdiff(required_cols, names(df))
  if (length(missing_cols) > 0) {
    stop("Missing required columns: ", paste(missing_cols, collapse = ", "))
  }
  
  # Check if data frame is empty
  if (nrow(df) == 0) {
    # Return empty data frame with correct types
    empty_df <- data.frame(
      id = character(0),
      time = as.POSIXct(character(0), tz = "UTC"),
      gl = numeric(0),
      stringsAsFactors = FALSE
    )
    # Add optional columns if they exist in original
    for (col in optional_cols) {
      if (col %in% names(df)) {
        if (col == "tz") {
          empty_df[[col]] <- character(0)
        } else {
          empty_df[[col]] <- numeric(0)
        }
      }
    }
    return(empty_df)
  }
  
  # Validate and convert column types
  validated_df <- df
  
  # Validate id column
  if (!is.character(df$id)) {
    validated_df$id <- as.character(df$id)
  }
  
  # Validate time column
  if (!inherits(df$time, "POSIXct")) {
    if (is.character(df$time)) {
      validated_df$time <- as.POSIXct(df$time, tz = "UTC")
    } else if (is.numeric(df$time)) {
      # Assume numeric time is already in POSIXct format
      validated_df$time <- as.POSIXct(df$time, origin = "1970-01-01", tz = "UTC")
    } else {
      stop("Time column must be POSIXct, character, or numeric")
    }
  }
  
  # Validate glucose column
  if (!is.numeric(df$gl)) {
    validated_df$gl <- as.numeric(df$gl)
  }
  
  # Validate optional timezone column
  if ("tz" %in% names(df)) {
    if (!is.character(df$tz)) {
      validated_df$tz <- as.character(df$tz)
    }
  }
  
  # Check for NA values in critical columns
  if (any(is.na(validated_df$id))) {
    warning("Found NA values in id column, removing those rows")
    validated_df <- validated_df[!is.na(validated_df$id), ]
  }
  
  if (any(is.na(validated_df$time))) {
    warning("Found NA values in time column, removing those rows")
    validated_df <- validated_df[!is.na(validated_df$time), ]
  }
  
  return(validated_df)
}

#' Validate numeric parameters
#' @param param Parameter value to validate
#' @param param_name Name of parameter for error messages
#' @param min_val Minimum allowed value
#' @param max_val Maximum allowed value
#' @return Validated parameter
#' @noRd
validate_numeric_param <- function(param, param_name, min_val = -Inf, max_val = Inf) {
  if (!is.numeric(param) || length(param) != 1) {
    stop(param_name, " must be a single numeric value")
  }
  
  if (is.na(param)) {
    stop(param_name, " cannot be NA")
  }
  
  if (param < min_val || param > max_val) {
    stop(param_name, " must be between ", min_val, " and ", max_val)
  }
  
  return(param)
}

#' Validate reading_minutes parameter
#' @param reading_minutes Parameter to validate
#' @param data_length Length of data for validation
#' @return Validated parameter
#' @noRd
validate_reading_minutes <- function(reading_minutes, data_length) {
  if (!is.null(reading_minutes)) {
    if (is.numeric(reading_minutes)) {
      if (length(reading_minutes) == 1) {
        reading_minutes <- validate_numeric_param(reading_minutes, "reading_minutes", min_val = 0.1)
      } else if (length(reading_minutes) != data_length) {
        stop("reading_minutes vector length must match data length or be a single value")
      }
    } else {
      stop("reading_minutes must be numeric")
    }
  }
  return(reading_minutes)
}

# =============================================================================
# VALIDATION HELPER FUNCTIONS
# =============================================================================
# Note: Safe wrapper functions are now implemented in function_overrides.R
# which overrides the original function names directly

#' Validate intermediary data frames (not raw CGM data)
#' @param df Data frame to validate
#' @return Pass-through of input data frame
#' @noRd
validate_intermediary_df <- function(df) {
  # For intermediary frames, just ensure it's a data frame
  if (!is.data.frame(df)) {
    stop("Input must be a data frame")
  }
  return(df)
}