cat("---\n Step 1: Removing existing cgmguru ---\n")
try({
  remove.packages("cgmguru")
  cat("cgmguru removed successfully.\n")
}, silent = TRUE)


# remotes::install_github("shstat1729/cgmguru")
# 2. Recompile attributes and reinstall the package from source
# Note: This assumes the script is run from the package's root directory.
cat("\n--- Step 2: Recompiling and Reinstalling cgmguru ---\n")
tryCatch({
  cat("Running Rcpp::compileAttributes()...\n")
  Rcpp::compileAttributes(pkgdir="~/Library/CloudStorage/OneDrive-개인/삼성서울병원/CGM/cgmguru")
  cat("Installing package from local source...\n")
  install.packages("~/Library/CloudStorage/OneDrive-개인/삼성서울병원/CGM/cgmguru", repos = NULL, type = "source", quiet = TRUE)
  cat("cgmguru reinstalled successfully.\n")
}, error = function(e) {
  stop("Failed to reinstall the package. Error: ", e$message)
})



library(iglu)
library(cgmguru)
data(example_data_5_subject)
example_data_5_subject$time <- as.POSIXct(example_data_5_subject$time,tz="UTC")

setwd("~/Library/CloudStorage/OneDrive-개인/삼성서울병원/CGM/cgmguru_paper")
df <- maxima_grid(example_data_5_subject)


write.csv(df$episode_counts,"episode_counts_maxima_grid.csv",row.names=FALSE)
df$results$grid_time <- format(df$results$grid_time, "%Y-%m-%d %H:%M:%S %Z")
df$results$maxima_time <- format(df$results$maxima_time, "%Y-%m-%d %H:%M:%S %Z")
write.csv(df$results,"maxima_grid_time_subject_ver4.csv",row.names=FALSE)

write.csv(detect_all_events(example_data_5_subject),"all_events_output_cgmguru.csv",row.names=FALSE)
write.csv(episode_calculation(example_data_5_subject),"all_events_output_iglu.csv",row.names=FALSE)


# Level 1 Hypoglycemia Event (≥15 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
event <- detect_hypoglycemic_events(example_data_5_subject, start_gl = 70, dur_length = 15, end_length = 15)  # hypo, level = lv1
detect_all_events(example_data_5_subject)
event$events_detailed
write.csv(event$events_detailed,"Level 1 Hypoglycemia Event.csv",row.names=FALSE)
# Level 2 Hypoglycemia Event (≥15 consecutive min of <54 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥54 mg/dL)
event <- detect_hypoglycemic_events(example_data_5_subject, start_gl = 54, dur_length = 15, end_length = 15)  # hypo, level = lv2

write.csv(event$events_detailed,"Level 2 Hypoglycemia Event.csv",row.names=FALSE)

# Extended Hypoglycemia Event (>120 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
event <- detect_hypoglycemic_events(example_data_5_subject)                                                    # hypo, extended
write.csv(event$events_detailed,"Extended Hypoglycemia Event.csv",row.names=FALSE)

# Hypoglycaemia alert value (54–69 mg/dL (3·0–3·9 mmol/L) ≥15 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL
# event <- detect_level1_hypoglycemic_events(example_data_5_subject)                                              # hypo, lv1_excl
# write.csv(event$events_detailed,"Hypoglycaemia alert value.csv",row.names=FALSE)

# Level 1 Hyperglycemia Event (≥15 consecutive min of >180 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≤180 mg/dL)
event <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 180, dur_length = 15, end_length = 15, end_gl = 180)  # hyper, lv1
write.csv(event$events_detailed,"Level 1 Hyperglycemia Event.csv",row.names=FALSE)
event$events_detailed
# Level 2 Hyperglycemia Event (≥15 consecutive min of >250 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≤250 mg/dL)
event <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 250, dur_length = 15, end_length = 15, end_gl = 250)  # hyper, lv2
write.csv(event$events_detailed,"Level 2 Hyperglycemia Event.csv",row.names=FALSE)

# Extended Hyperglycemia Event (Number of events with sensor glucose >250 mg/dL (>13·9 mmol/L) lasting at least 120 min; event ends when glucose returns to ≤180 mg/dL (≤10·0 mmol/L) for ≥15 min)
event <- detect_hyperglycemic_events(example_data_5_subject)
# hyper, extended
event$events_detailed
write.csv(event$events_detailed,"Extended Hyperglycemia Event.csv",row.names=FALSE)

# Load required packages
library(microbenchmark)

# Perform microbenchmark comparison
benchmark_results <- microbenchmark(
  episode_calculation = episode_calculation(example_data_5_subject),
  detect_all_events = detect_all_events(example_data_5_subject),
  times = 100,
  unit = "ms"  # You can change to "s", "us", "ns" as needed
)
data(example_data_hall)
detect_hypoglycemic_events(example_data_hall)
# Print summary statistics
