cat("---\n Step 1: Removing existing cgmguru ---\n")
try({
  remove.packages("cgmguru")
  cat("cgmguru removed successfully.\n")
}, silent = TRUE)

# 2. Recompile attributes and reinstall the package from source
# Note: This assumes the script is run from the package's root directory.
cat("\n--- Step 2: Recompiling and Reinstalling cgmguru ---\n")
tryCatch({
  cat("Running Rcpp::compileAttributes()...\n")
  Rcpp::compileAttributes(pkgdir="/Users/bagsangho/Library/CloudStorage/OneDrive-개인/삼성서울병원/CGM/cgmguru")
  cat("Installing package from local source...\n")
  install.packages("/Users/bagsangho/Library/CloudStorage/OneDrive-개인/삼성서울병원/CGM/cgmguru", repos = NULL, type = "source", quiet = TRUE)
  cat("cgmguru reinstalled successfully.\n")
}, error = function(e) {
  stop("Failed to reinstall the package. Error: ", e$message)
})


library(iglu)
library(cgmguru)
data("example_data_5_subject")
setwd("/Users/bagsangho/Library/CloudStorage/OneDrive-개인/삼성서울병원/CGM/cgmguru_paper")
# Level 1 Hypoglycemia Event (≥15 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
event <- detect_hypoglycemic_events(example_data_5_subject, start_gl = 70, dur_length = 15, end_length = 15)  # hypo, level = lv1
event$events_detailed
event
detect_all_events(example_data_5_subject)


# Level 2 Hypoglycemia Event (≥15 consecutive min of <54 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥54 mg/dL)
event <- detect_hypoglycemic_events(example_data_5_subject, start_gl = 54, dur_length = 15, end_length = 15)  # hypo, level = lv2
event$events_detailed
event
detect_all_events(example_data_5_subject)
# Extended Hypoglycemia Event (>120 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
event <- detect_hypoglycemic_events(example_data_5_subject)                                                    # hypo, extended
event$events_detailed
event
detect_all_events(example_data_5_subject)


# Hypoglycaemia alert value (54–69 mg/dL (3·0–3·9 mmol/L) ≥15 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL

event <- detect_level1_hypoglycemic_events(example_data_5_subject)                                              # hypo, lv1_excl
event$events_detailed
event
detect_all_events(example_data_5_subject)


# Level 1 Hyperglycemia Event (≥15 consecutive min of >180 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≤180 mg/dL)
event <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 181, dur_length = 15, end_length = 15, end_gl = 180)  # hyper, lv1
event$events_detailed
event
detect_all_events(example_data_5_subject)

# Level 2 Hyperglycemia Event (≥15 consecutive min of >250 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≤250 mg/dL)
event <- detect_hyperglycemic_events(example_data_5_subject, start_gl = 251, dur_length = 15, end_length = 15, end_gl = 250)  # hyper, lv2
event$events_detailed
event
detect_all_events(example_data_5_subject)

# Extended Hyperglycemia Event (Number of events with sensor glucose >250 mg/dL (>13·9 mmol/L) lasting at least 120 min; event ends when glucose returns to ≤180 mg/dL (≤10·0 mmol/L) for ≥15 min)
event <- detect_hyperglycemic_events(example_data_5_subject)                                                                 # hyper, extended
event$events_detailed
event
detect_all_events(example_data_5_subject)

# High glucose (Level 1, excluded) (181–250 mg/dL (10·1–13·9 mmol/L) ≥15 consecutive min and Event ends when there is ≥15 consecutive min with a CGM sensor value of ≤180 mg/dL) 
event <- detect_level1_hyperglycemic_events(example_data_5_subject)  
event$events_detailed
event
detect_all_events(example_data_5_subject)
