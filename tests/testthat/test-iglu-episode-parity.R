library(testthat)
library(cgmguru)
library(iglu)

data(example_data_5_subject, package = "iglu")
data(example_data_hall, package = "iglu")

# iglu::episode_calculation() does not expose an extended hyperglycemia label,
# so these are the seven shared event definitions.
iglu_event_specs <- data.frame(
  type = c("hypo", "hypo", "hypo", "hypo", "hyper", "hyper", "hyper"),
  level = c("lv1", "lv2", "extended", "lv1_excl", "lv1", "lv2", "lv1_excl"),
  label = c(
    "lv1_hypo", "lv2_hypo", "ext_hypo", "lv1_hypo_excl",
    "lv1_hyper", "lv2_hyper", "lv1_hyper_excl"
  ),
  threshold = c(70, 54, 70, 70, 180, 250, 180),
  stringsAsFactors = FALSE
)

empty_event_spans <- function() {
  data.frame(
    id = character(),
    start_time = character(),
    end_time = character(),
    stringsAsFactors = FALSE
  )
}

event_time_key <- function(x) {
  # iglu reconstructs these example-data timestamps with UTC wall-clock labels
  # while cgmguru preserves the source EST tzone. Compare the displayed event
  # boundary labels rather than POSIXct offsets.
  format(x, "%Y-%m-%d %H:%M:%S")
}

sort_event_spans <- function(x) {
  if (nrow(x) == 0) {
    return(empty_event_spans())
  }

  x <- as.data.frame(x[, c("id", "start_time", "end_time")])
  x$id <- as.character(x$id)
  x$start_time <- as.character(x$start_time)
  x$end_time <- as.character(x$end_time)
  rownames(x) <- NULL
  x <- x[order(x$id, x$start_time, x$end_time), , drop = FALSE]
  rownames(x) <- NULL
  x
}

iglu_excursion_spans <- function(episode_data, spec) {
  event_label <- episode_data[[spec$label]]
  event_rows <- which(event_label > 0)

  if (length(event_rows) == 0) {
    return(empty_event_spans())
  }

  event_groups <- split(
    event_rows,
    paste(
      as.character(episode_data$id[event_rows]),
      episode_data$segment[event_rows],
      event_label[event_rows],
      sep = "\r"
    )
  )

  spans <- do.call(rbind, lapply(event_groups, function(rows) {
    in_excursion <- if (spec$type == "hypo") {
      episode_data$gl[rows] < spec$threshold
    } else {
      episode_data$gl[rows] > spec$threshold
    }
    excursion_rows <- rows[in_excursion]

    data.frame(
      id = as.character(episode_data$id[excursion_rows[1]]),
      start_time = event_time_key(episode_data$time[excursion_rows[1]]),
      end_time = event_time_key(episode_data$time[excursion_rows[length(excursion_rows)]]),
      stringsAsFactors = FALSE
    )
  }))

  sort_event_spans(spans)
}

cgmguru_detailed_spans <- function(data, spec) {
  events <- if (spec$type == "hypo") {
    detect_hypoglycemic_events(data, type = spec$level, reading_minutes = 5)
  } else {
    detect_hyperglycemic_events(data, type = spec$level, reading_minutes = 5)
  }

  details <- events$events_detailed
  if (nrow(details) == 0) {
    return(empty_event_spans())
  }

  sort_event_spans(data.frame(
    id = as.character(details$id),
    start_time = event_time_key(details$start_time),
    end_time = event_time_key(details$end_time),
    stringsAsFactors = FALSE
  ))
}

normalise_counts <- function(x, count_col) {
  out <- data.frame(
    id = as.character(x$id),
    type = as.character(x$type),
    level = as.character(x$level),
    event_count = as.integer(x[[count_col]]),
    stringsAsFactors = FALSE
  )
  rownames(out) <- NULL
  out <- out[order(out$id, out$type, out$level), , drop = FALSE]
  rownames(out) <- NULL
  out
}

expect_iglu_count_parity <- function(data, dataset_name) {
  cgmguru_counts <- detect_all_events(data, reading_minutes = 5)$events_long_df
  iglu_counts <- iglu::episode_calculation(data, dt0 = 5, tz = "UTC")

  for (i in seq_len(nrow(iglu_event_specs))) {
    spec <- iglu_event_specs[i, ]
    info <- paste(dataset_name, spec$type, spec$level)

    cgmguru_subset <- cgmguru_counts[
      cgmguru_counts$type == spec$type & cgmguru_counts$level == spec$level,
    ]
    iglu_subset <- iglu_counts[
      iglu_counts$type == spec$type & iglu_counts$level == spec$level,
    ]

    expect_equal(
      normalise_counts(cgmguru_subset, "event_count"),
      normalise_counts(iglu_subset, "total_episodes"),
      info = info
    )
  }
}

expect_iglu_boundary_parity <- function(data, dataset_name) {
  iglu_episode_data <- iglu::episode_calculation(
    data,
    return_data = TRUE,
    dt0 = 5,
    tz = "UTC"
  )$data

  for (i in seq_len(nrow(iglu_event_specs))) {
    spec <- iglu_event_specs[i, ]
    info <- paste(dataset_name, spec$type, spec$level)

    expect_equal(
      cgmguru_detailed_spans(data, spec),
      iglu_excursion_spans(iglu_episode_data, spec),
      info = info
    )
  }
}

test_that("detect_all_events event counts match iglu episode_calculation examples", {
  expect_iglu_count_parity(example_data_5_subject, "example_data_5_subject")
  expect_iglu_count_parity(example_data_hall, "example_data_hall")
})

test_that("detailed cgmguru event starts and ends match iglu episode labels", {
  expect_iglu_boundary_parity(example_data_5_subject, "example_data_5_subject")
  expect_iglu_boundary_parity(example_data_hall, "example_data_hall")
})
