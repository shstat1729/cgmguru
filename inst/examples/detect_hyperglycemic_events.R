library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Level 1 Hyperglycemia Event (≥15 consecutive min of >180 mg/dL)
hyper_lv1 <- detect_hyperglycemic_events(
  example_data_5_subject,
  type = "lv1"
)
print(hyper_lv1$events_total)

# Level 2 Hyperglycemia Event (≥15 consecutive min of >250 mg/dL)
hyper_lv2 <- detect_hyperglycemic_events(
  example_data_5_subject,
  type = "lv2"
)
print(hyper_lv2$events_total)
# Extended Hyperglycemia Event (default parameters)
hyper_extended <- detect_hyperglycemic_events(example_data_5_subject, type = "extended")

# Analysis on larger dataset
large_hyper <- detect_hyperglycemic_events(
  example_data_hall,
  type = "lv1"
)
print(paste("Total hyperglycemic episodes:", sum(large_hyper$events_total$total_episodes)))

# View detailed events for first subject
if(nrow(hyper_lv1$events_detailed) > 0) {
  first_subject <- hyper_lv1$events_detailed$id[1]
  subject_events <- hyper_lv1$events_detailed[hyper_lv1$events_detailed$id == first_subject, ]
  head(subject_events)
}
