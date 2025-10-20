.onLoad <- function(libname, pkgname) {
  ns <- asNamespace(pkgname)

  rebind <- function(name, fun) {
    if (bindingIsLocked(name, ns)) unlockBinding(name, ns)
    assign(name, fun, envir = ns)
    lockBinding(name, ns)
  }

  # Capture originals from current namespace
  .detect_hyperglycemic_events_original <- get("detect_hyperglycemic_events", envir = ns)
  .detect_hypoglycemic_events_original <- get("detect_hypoglycemic_events", envir = ns)
  .detect_all_events_original <- get("detect_all_events", envir = ns)
  .find_local_maxima_original <- get("find_local_maxima", envir = ns)
  .grid_original <- get("grid", envir = ns)
  .maxima_grid_original <- get("maxima_grid", envir = ns)
  .excursion_original <- get("excursion", envir = ns)
  .start_finder_original <- get("start_finder", envir = ns)
  .find_max_after_hours_original <- get("find_max_after_hours", envir = ns)
  .find_max_before_hours_original <- get("find_max_before_hours", envir = ns)
  .find_min_after_hours_original <- get("find_min_after_hours", envir = ns)
  .find_min_before_hours_original <- get("find_min_before_hours", envir = ns)
  .detect_between_maxima_original <- get("detect_between_maxima", envir = ns)
  .find_new_maxima_original <- get("find_new_maxima", envir = ns)
  .mod_grid_original <- get("mod_grid", envir = ns)
  .transform_df_original <- get("transform_df", envir = ns)

  # Load validators from package namespace
  validate_cgm_data <- get("validate_cgm_data", envir = ns)
  validate_numeric_param <- get("validate_numeric_param", envir = ns)
  validate_reading_minutes <- get("validate_reading_minutes", envir = ns)

  # Define safe wrappers locally, then rebind
  rebind("detect_hyperglycemic_events", function(new_df, reading_minutes = NULL, dur_length = 120, end_length = 15, start_gl = 250, end_gl = 180) {
    validated_df <- validate_cgm_data(new_df)
    dur_length <- validate_numeric_param(dur_length, "dur_length", min_val = 0)
    end_length <- validate_numeric_param(end_length, "end_length", min_val = 0)
    start_gl <- validate_numeric_param(start_gl, "start_gl", min_val = 0)
    if (!missing(end_gl)) end_gl <- validate_numeric_param(end_gl, "end_gl", min_val = 0)
    reading_minutes <- validate_reading_minutes(reading_minutes, nrow(validated_df))
    tryCatch({
      .detect_hyperglycemic_events_original(validated_df, reading_minutes, dur_length, end_length, start_gl, end_gl)
    }, error = function(e) stop("Error in detect_hyperglycemic_events: ", e$message))
  })

  rebind("detect_hypoglycemic_events", function(new_df, reading_minutes = NULL, dur_length = 120, end_length = 15, start_gl = 70) {
    validated_df <- validate_cgm_data(new_df)
    dur_length <- validate_numeric_param(dur_length, "dur_length", min_val = 0)
    end_length <- validate_numeric_param(end_length, "end_length", min_val = 0)
    start_gl <- validate_numeric_param(start_gl, "start_gl", min_val = 0)
    reading_minutes <- validate_reading_minutes(reading_minutes, nrow(validated_df))
    tryCatch({
      .detect_hypoglycemic_events_original(validated_df, reading_minutes, dur_length, end_length, start_gl)
    }, error = function(e) stop("Error in detect_hypoglycemic_events: ", e$message))
  })

  rebind("detect_all_events", function(df, reading_minutes = NULL) {
    validated_df <- validate_cgm_data(df)
    reading_minutes <- validate_reading_minutes(reading_minutes, nrow(validated_df))
    tryCatch({
      .detect_all_events_original(validated_df, reading_minutes)
    }, error = function(e) stop("Error in detect_all_events: ", e$message))
  })

  rebind("find_local_maxima", function(df) {
    validated_df <- validate_cgm_data(df)
    tryCatch({
      .find_local_maxima_original(validated_df)
    }, error = function(e) stop("Error in find_local_maxima: ", e$message))
  })

  rebind("grid", function(df, gap = 15, threshold = 130) {
    validated_df <- validate_cgm_data(df)
    gap <- validate_numeric_param(gap, "gap", min_val = 0)
    threshold <- validate_numeric_param(threshold, "threshold", min_val = 0)
    tryCatch({
      .grid_original(validated_df, gap, threshold)
    }, error = function(e) stop("Error in grid: ", e$message))
  })

  rebind("maxima_grid", function(df, threshold = 130, gap = 60, hours = 2) {
    validated_df <- validate_cgm_data(df)
    threshold <- validate_numeric_param(threshold, "threshold", min_val = 0)
    gap <- validate_numeric_param(gap, "gap", min_val = 0)
    hours <- validate_numeric_param(hours, "hours", min_val = 0)
    tryCatch({
      .maxima_grid_original(validated_df, threshold, gap, hours)
    }, error = function(e) stop("Error in maxima_grid: ", e$message))
  })

  rebind("excursion", function(df, gap = 15) {
    validated_df <- validate_cgm_data(df)
    gap <- validate_numeric_param(gap, "gap", min_val = 0)
    tryCatch({
      .excursion_original(validated_df, gap)
    }, error = function(e) stop("Error in excursion: ", e$message))
  })

  rebind("start_finder", function(df) {
    validated_df <- validate_cgm_data(df)
    tryCatch({
      .start_finder_original(validated_df)
    }, error = function(e) stop("Error in start_finder: ", e$message))
  })

  rebind("find_max_after_hours", function(df, start_point_df, hours) {
    validated_df <- validate_cgm_data(df)
    validated_start_df <- validate_cgm_data(start_point_df)
    hours <- validate_numeric_param(hours, "hours", min_val = 0)
    tryCatch({
      .find_max_after_hours_original(validated_df, validated_start_df, hours)
    }, error = function(e) stop("Error in find_max_after_hours: ", e$message))
  })

  rebind("find_max_before_hours", function(df, start_point_df, hours) {
    validated_df <- validate_cgm_data(df)
    validated_start_df <- validate_cgm_data(start_point_df)
    hours <- validate_numeric_param(hours, "hours", min_val = 0)
    tryCatch({
      .find_max_before_hours_original(validated_df, validated_start_df, hours)
    }, error = function(e) stop("Error in find_max_before_hours: ", e$message))
  })

  rebind("find_min_after_hours", function(df, start_point_df, hours) {
    validated_df <- validate_cgm_data(df)
    validated_start_df <- validate_cgm_data(start_point_df)
    hours <- validate_numeric_param(hours, "hours", min_val = 0)
    tryCatch({
      .find_min_after_hours_original(validated_df, validated_start_df, hours)
    }, error = function(e) stop("Error in find_min_after_hours: ", e$message))
  })

  rebind("find_min_before_hours", function(df, start_point_df, hours) {
    validated_df <- validate_cgm_data(df)
    validated_start_df <- validate_cgm_data(start_point_df)
    hours <- validate_numeric_param(hours, "hours", min_val = 0)
    tryCatch({
      .find_min_before_hours_original(validated_df, validated_start_df, hours)
    }, error = function(e) stop("Error in find_min_before_hours: ", e$message))
  })

  rebind("detect_between_maxima", function(new_df, transform_df) {
    validated_new_df <- validate_cgm_data(new_df)
    validated_transform_df <- validate_cgm_data(transform_df)
    tryCatch({
      .detect_between_maxima_original(validated_new_df, validated_transform_df)
    }, error = function(e) stop("Error in detect_between_maxima: ", e$message))
  })

  rebind("find_new_maxima", function(new_df, mod_grid_max_point_df, local_maxima_df) {
    validated_new_df <- validate_cgm_data(new_df)
    validated_mod_grid_df <- validate_cgm_data(mod_grid_max_point_df)
    validated_maxima_df <- validate_cgm_data(local_maxima_df)
    tryCatch({
      .find_new_maxima_original(validated_new_df, validated_mod_grid_df, validated_maxima_df)
    }, error = function(e) stop("Error in find_new_maxima: ", e$message))
  })

  rebind("transform_df", function(grid_df, maxima_df) {
    validated_grid_df <- validate_cgm_data(grid_df)
    validated_maxima_df <- validate_cgm_data(maxima_df)
    tryCatch({
      .transform_df_original(validated_grid_df, validated_maxima_df)
    }, error = function(e) stop("Error in transform_df: ", e$message))
  })
}


