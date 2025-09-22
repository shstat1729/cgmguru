# Investigate why the event starting at index 989 is not detected as a separate event
library(iglu)
library(cgmguru)

# Load the data
data(example_data_5_subject)
example_data_5_subject$time <- as.POSIXct(example_data_5_subject$time, tz="UTC")

cat("=== Investigating Missing Event at Index 989 ===\n")

# Extract Subject 1 data
subject1_data <- example_data_5_subject[example_data_5_subject$id == "Subject 1", ]

# Look at the data around indices 970 and 989
cat("=== Data around index 970 (should be first event start) ===\n")
start_idx_970 <- 970
end_idx_970 <- min(nrow(subject1_data), 970 + 20)
data_970 <- subject1_data[start_idx_970:end_idx_970, ]
for (i in 1:nrow(data_970)) {
  cat(sprintf("Row %d: %s, glucose: %.0f mg/dL\n",
              start_idx_970 + i - 1,
              format(data_970$time[i], "%Y-%m-%d %H:%M:%S"),
              data_970$gl[i]))
}

cat("\n=== Data around index 989 (should be second event start) ===\n")
start_idx_989 <- 989
end_idx_989 <- min(nrow(subject1_data), 989 + 20)
data_989 <- subject1_data[start_idx_989:end_idx_989, ]
for (i in 1:nrow(data_989)) {
  cat(sprintf("Row %d: %s, glucose: %.0f mg/dL\n",
              start_idx_989 + i - 1,
              format(data_989$time[i], "%Y-%m-%d %H:%M:%S"),
              data_989$gl[i]))
}

# Check what happens between the two potential events
cat("\n=== Data between events (indices 982-989) ===\n")
between_start <- 982
between_end <- 989
if (between_start <= between_end) {
  between_data <- subject1_data[between_start:between_end, ]
  for (i in 1:nrow(between_data)) {
    cat(sprintf("Row %d: %s, glucose: %.0f mg/dL\n",
                between_start + i - 1,
                format(between_data$time[i], "%Y-%m-%d %H:%M:%S"),
                between_data$gl[i]))
  }
}

# Check if there's a recovery period between the two events
cat("\n=== Checking for recovery between events ===\n")
cat("Looking for glucose <= 180 mg/dL between the two potential events:\n")

# Check if there's sufficient recovery time between 982 and 989
recovery_found <- FALSE
for (i in between_start:between_end) {
  if (i <= nrow(subject1_data)) {
    if (subject1_data$gl[i] <= 180) {
      cat(sprintf("Recovery found at row %d: %s, glucose: %.0f mg/dL\n",
                  i,
                  format(subject1_data$time[i], "%Y-%m-%d %H:%M:%S"),
                  subject1_data$gl[i]))
      recovery_found <- TRUE
    }
  }
}

if (!recovery_found) {
  cat("No recovery found between the events - this explains why only one event is detected!\n")
  cat("The algorithm treats this as one continuous event rather than two separate events.\n")
} else {
  cat("Recovery found between events - the algorithm should detect two separate events.\n")
}

# Test the detection algorithm step by step
cat("\n=== Testing detection algorithm step by step ===\n")

# Get the current detection results
result <- detect_hyperglycemic_events(subject1_data, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)

cat("Current detection results:\n")
print(result$events_detailed)

# Look for events that start around index 970 and 989
events_970 <- result$events_detailed[result$events_detailed$start_indices == 970, ]
events_989 <- result$events_detailed[result$events_detailed$start_indices == 989, ]

cat(sprintf("\nEvents starting at index 970: %d\n", nrow(events_970)))
cat(sprintf("Events starting at index 989: %d\n", nrow(events_989)))

if (nrow(events_970) > 0) {
  cat("Event at index 970:\n")
  print(events_970)
}

if (nrow(events_989) > 0) {
  cat("Event at index 989:\n")
  print(events_989)
} else {
  cat("No event detected at index 989 - this is the problem!\n")
}

# Let's manually check what should happen
cat("\n=== Manual Analysis ===\n")
cat("Event 1: Should start at index 970 (2015-06-11 20:45:07, glucose: 194 mg/dL)\n")
cat("Event 1: Should end before index 989 when recovery starts\n")
cat("Event 2: Should start at index 989 (2015-06-11 22:25:07, glucose: 195 mg/dL)\n")
cat("Event 2: Should end when recovery starts after the hyperglycemic period\n")

# Check if the first event properly ends before the second begins
if (nrow(events_970) > 0) {
  first_event_end <- events_970$end_indices[1]
  cat(sprintf("First event ends at index: %d\n", first_event_end))
  cat(sprintf("Second event should start at index: %d\n", 989))
  
  if (first_event_end >= 989) {
    cat("PROBLEM: First event extends past where second event should start!\n")
    cat("This means the recovery detection for the first event is not working properly.\n")
  } else {
    cat("First event ends before second event starts - this is correct.\n")
  }
}

# The issue might be in the recovery detection logic
# Let's check what the algorithm thinks about recovery
cat("\n=== Analyzing Recovery Detection Logic ===\n")
cat("The algorithm should detect recovery between the two events.\n")
cat("If it doesn't, it will treat them as one continuous event.\n")

# Check the glucose values in the gap
gap_start <- 982
gap_end <- 989
if (gap_start < gap_end) {
  gap_data <- subject1_data[gap_start:gap_end, ]
  cat("Glucose values in the gap:\n")
  for (i in 1:nrow(gap_data)) {
    cat(sprintf("  %s: %.0f mg/dL\n", 
                format(gap_data$time[i], "%Y-%m-%d %H:%M:%S"),
                gap_data$gl[i]))
  }
  
  # Check if any values are <= 180
  recovery_values <- gap_data$gl <= 180
  cat(sprintf("Values <= 180 in gap: %s\n", paste(recovery_values, collapse = ", ")))
  
  if (any(recovery_values)) {
    cat("Recovery values found in gap - algorithm should detect separate events.\n")
  } else {
    cat("No recovery values in gap - algorithm treats as continuous event.\n")
  }
}
