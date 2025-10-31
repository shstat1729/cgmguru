library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Level 1 Hypoglycemia Event (≥15 consecutive min of <70 mg/dL)
hypo_lv1 <- detect_hypoglycemic_events(
  example_data_5_subject,
  start_gl = 70,
  dur_length = 15,
  end_length = 15
)
print(hypo_lv1$events_total)

# Level 2 Hypoglycemia Event (≥15 consecutive min of <54 mg/dL)
hypo_lv2 <- detect_hypoglycemic_events(
  example_data_5_subject,
  start_gl = 54,
  dur_length = 15,
  end_length = 15
)

# Extended Hypoglycemia Event (default parameters)
hypo_extended <- detect_hypoglycemic_events(example_data_5_subject)

# Analysis on larger dataset
large_hypo <- detect_hypoglycemic_events(
  example_data_hall,
  start_gl = 70,
  dur_length = 15,
  end_length = 15
)
print(paste("Total hypoglycemic events:", sum(large_hypo$events_total$total_events)))

# Compare Level 1 vs Level 2 hypoglycemia
cat("Level 1 events:", sum(hypo_lv1$events_total$total_events), "\n")
cat("Level 2 events:", sum(hypo_lv2$events_total$total_events), "\n")

