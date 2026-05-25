library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Level 1 Hypoglycemia Event (≥15 consecutive min of <70 mg/dL)
hypo_lv1 <- detect_hypoglycemic_events(
  example_data_5_subject,
  type = "lv1"
)
print(hypo_lv1$events_total)

# Level 2 Hypoglycemia Event (≥15 consecutive min of <54 mg/dL)
hypo_lv2 <- detect_hypoglycemic_events(
  example_data_5_subject,
  type = "lv2"
)

# Extended Hypoglycemia Event (default parameters)
hypo_extended <- detect_hypoglycemic_events(example_data_5_subject, type = "extended")

# Analysis on larger dataset
large_hypo <- detect_hypoglycemic_events(
  example_data_hall,
  type = "lv1"
)
print(paste("Total hypoglycemic episodes:", sum(large_hypo$events_total$total_episodes)))

# Compare Level 1 vs Level 2 hypoglycemia
cat("Level 1 episodes:", sum(hypo_lv1$events_total$total_episodes), "\n")
cat("Level 2 episodes:", sum(hypo_lv2$events_total$total_episodes), "\n")
