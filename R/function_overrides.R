# Function overrides to provide safe versions of all C++ functions
# This file overrides the original function names with safe, validated versions

# Store original functions for potential internal use
.detect_hyperglycemic_events_original <- detect_hyperglycemic_events
.detect_hypoglycemic_events_original <- detect_hypoglycemic_events
.detect_all_events_original <- detect_all_events
.find_local_maxima_original <- find_local_maxima
.grid_original <- grid
.maxima_grid_original <- maxima_grid
.excursion_original <- excursion
.start_finder_original <- start_finder
.find_max_after_hours_original <- find_max_after_hours
.find_max_before_hours_original <- find_max_before_hours
.find_min_after_hours_original <- find_min_after_hours
.find_min_before_hours_original <- find_min_before_hours
.detect_between_maxima_original <- detect_between_maxima
.find_new_maxima_original <- find_new_maxima
.mod_grid_original <- mod_grid
.transform_df_original <- transform_df

# Override original functions with safe versions
detect_hyperglycemic_events <- function(new_df, reading_minutes = NULL, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(new_df)
  }, error = function(e) {
    stop("Error in detect_hyperglycemic_events(): ", e$message, call. = FALSE)
  })
  
  # Validate parameters
  dur_length <- validate_numeric_param(dur_length, "dur_length", min_val = 0)
  end_length <- validate_numeric_param(end_length, "end_length", min_val = 0)
  start_gl <- validate_numeric_param(start_gl, "start_gl", min_val = 0)
  end_gl <- validate_numeric_param(end_gl, "end_gl", min_val = 0)
  
  # Validate reading_minutes
  reading_minutes <- validate_reading_minutes(reading_minutes, nrow(validated_df))
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .detect_hyperglycemic_events_original(validated_df, reading_minutes, dur_length, end_length, start_gl, end_gl)
    return(result)
  }, error = function(e) {
    stop("Error in detect_hyperglycemic_events: ", e$message, call. = FALSE)
  })
}

detect_hypoglycemic_events <- function(new_df, reading_minutes = NULL, dur_length = 120, end_length = 15, start_gl = 70) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(new_df)
  }, error = function(e) {
    stop("Error in detect_hypoglycemic_events(): ", e$message, call. = FALSE)
  })
  
  # Validate parameters
  dur_length <- validate_numeric_param(dur_length, "dur_length", min_val = 0)
  end_length <- validate_numeric_param(end_length, "end_length", min_val = 0)
  start_gl <- validate_numeric_param(start_gl, "start_gl", min_val = 0)
  
  # Validate reading_minutes
  reading_minutes <- validate_reading_minutes(reading_minutes, nrow(validated_df))
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .detect_hypoglycemic_events_original(validated_df, reading_minutes, dur_length, end_length, start_gl)
    return(result)
  }, error = function(e) {
    stop("Error in detect_hypoglycemic_events: ", e$message, call. = FALSE)
  })
}

detect_all_events <- function(df, reading_minutes = NULL) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(df)
  }, error = function(e) {
    stop("Error in detect_all_events(): ", e$message, call. = FALSE)
  })
  
  # Validate reading_minutes
  reading_minutes <- validate_reading_minutes(reading_minutes, nrow(validated_df))
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .detect_all_events_original(validated_df, reading_minutes)
    return(result)
  }, error = function(e) {
    stop("Error in detect_all_events: ", e$message, call. = FALSE)
  })
}

find_local_maxima <- function(df) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(df)
  }, error = function(e) {
    stop("Error in find_local_maxima(): ", e$message, call. = FALSE)
  })
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .find_local_maxima_original(validated_df)
    return(result)
  }, error = function(e) {
    stop("Error in find_local_maxima: ", e$message, call. = FALSE)
  })
}

grid <- function(df, gap = 15, threshold = 130) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(df)
  }, error = function(e) {
    stop("Error in grid(): ", e$message, call. = FALSE)
  })
  
  # Validate parameters
  gap <- validate_numeric_param(gap, "gap", min_val = 0)
  threshold <- validate_numeric_param(threshold, "threshold", min_val = 0)
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .grid_original(validated_df, gap, threshold)
    return(result)
  }, error = function(e) {
    stop("Error in grid: ", e$message, call. = FALSE)
  })
}

maxima_grid <- function(df, threshold = 130, gap = 60, hours = 2) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(df)
  }, error = function(e) {
    stop("Error in maxima_grid(): ", e$message, call. = FALSE)
  })
  
  # Validate parameters
  threshold <- validate_numeric_param(threshold, "threshold", min_val = 0)
  gap <- validate_numeric_param(gap, "gap", min_val = 0)
  hours <- validate_numeric_param(hours, "hours", min_val = 0)
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .maxima_grid_original(validated_df, threshold, gap, hours)
    return(result)
  }, error = function(e) {
    stop("Error in maxima_grid: ", e$message, call. = FALSE)
  })
}

excursion <- function(df, gap = 15) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(df)
  }, error = function(e) {
    stop("Error in excursion(): ", e$message, call. = FALSE)
  })
  
  # Validate parameters
  gap <- validate_numeric_param(gap, "gap", min_val = 0)
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .excursion_original(validated_df, gap)
    return(result)
  }, error = function(e) {
    stop("Error in excursion: ", e$message, call. = FALSE)
  })
}

start_finder <- function(df) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(df)
  }, error = function(e) {
    stop("Error in start_finder(): ", e$message, call. = FALSE)
  })
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .start_finder_original(validated_df)
    return(result)
  }, error = function(e) {
    stop("Error in start_finder: ", e$message, call. = FALSE)
  })
}

find_max_after_hours <- function(df, start_point_df, hours) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(df)
    validated_start_df <- validate_intermediary_df(start_point_df)
  }, error = function(e) {
    stop("Error in find_max_after_hours(): ", e$message, call. = FALSE)
  })
  
  # Validate parameters
  hours <- validate_numeric_param(hours, "hours", min_val = 0)
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .find_max_after_hours_original(validated_df, validated_start_df, hours)
    return(result)
  }, error = function(e) {
    stop("Error in find_max_after_hours: ", e$message, call. = FALSE)
  })
}

find_max_before_hours <- function(df, start_point_df, hours) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(df)
    validated_start_df <- validate_intermediary_df(start_point_df)
  }, error = function(e) {
    stop("Error in find_max_before_hours(): ", e$message, call. = FALSE)
  })
  
  # Validate parameters
  hours <- validate_numeric_param(hours, "hours", min_val = 0)
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .find_max_before_hours_original(validated_df, validated_start_df, hours)
    return(result)
  }, error = function(e) {
    stop("Error in find_max_before_hours: ", e$message, call. = FALSE)
  })
}

find_min_after_hours <- function(df, start_point_df, hours) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(df)
    validated_start_df <- validate_intermediary_df(start_point_df)
  }, error = function(e) {
    stop("Error in find_min_after_hours(): ", e$message, call. = FALSE)
  })
  
  # Validate parameters
  hours <- validate_numeric_param(hours, "hours", min_val = 0)
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .find_min_after_hours_original(validated_df, validated_start_df, hours)
    return(result)
  }, error = function(e) {
    stop("Error in find_min_after_hours: ", e$message, call. = FALSE)
  })
}

find_min_before_hours <- function(df, start_point_df, hours) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(df)
    validated_start_df <- validate_intermediary_df(start_point_df)
  }, error = function(e) {
    stop("Error in find_min_before_hours(): ", e$message, call. = FALSE)
  })
  
  # Validate parameters
  hours <- validate_numeric_param(hours, "hours", min_val = 0)
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .find_min_before_hours_original(validated_df, validated_start_df, hours)
    return(result)
  }, error = function(e) {
    stop("Error in find_min_before_hours: ", e$message, call. = FALSE)
  })
}

detect_between_maxima <- function(new_df, transform_df) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_new_df <- validate_cgm_data(new_df)
    validated_transform_df <- validate_intermediary_df(transform_df)
  }, error = function(e) {
    stop("Error in detect_between_maxima(): ", e$message, call. = FALSE)
  })
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .detect_between_maxima_original(validated_new_df, validated_transform_df)
    return(result)
  }, error = function(e) {
    stop("Error in detect_between_maxima: ", e$message, call. = FALSE)
  })
}

find_new_maxima <- function(new_df, mod_grid_max_point_df, local_maxima_df) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_new_df <- validate_cgm_data(new_df)
    validated_mod_grid_df <- validate_intermediary_df(mod_grid_max_point_df)
    validated_maxima_df <- validate_intermediary_df(local_maxima_df)
  }, error = function(e) {
    stop("Error in find_new_maxima(): ", e$message, call. = FALSE)
  })
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .find_new_maxima_original(validated_new_df, validated_mod_grid_df, validated_maxima_df)
    return(result)
  }, error = function(e) {
    stop("Error in find_new_maxima: ", e$message, call. = FALSE)
  })
}

mod_grid <- function(df, grid_point_df, hours = 2, gap = 15) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_df <- validate_cgm_data(df)
    validated_grid_df <- validate_intermediary_df(grid_point_df)
  }, error = function(e) {
    stop("Error in mod_grid(): ", e$message, call. = FALSE)
  })
  
  # Validate parameters
  hours <- validate_numeric_param(hours, "hours", min_val = 0)
  gap <- validate_numeric_param(gap, "gap", min_val = 0)
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .mod_grid_original(validated_df, validated_grid_df, hours, gap)
    return(result)
  }, error = function(e) {
    stop("Error in mod_grid: ", e$message, call. = FALSE)
  })
}

transform_df <- function(grid_df, maxima_df) {
  # Validate input data with context-aware error messages
  tryCatch({
    validated_grid_df <- validate_intermediary_df(grid_df)
    validated_maxima_df <- validate_intermediary_df(maxima_df)
  }, error = function(e) {
    stop("Error in transform_df(): ", e$message, call. = FALSE)
  })
  
  # Call the original C++ function with validated inputs
  tryCatch({
    result <- .transform_df_original(validated_grid_df, validated_maxima_df)
    return(result)
  }, error = function(e) {
    stop("Error in transform_df: ", e$message, call. = FALSE)
  })
}
