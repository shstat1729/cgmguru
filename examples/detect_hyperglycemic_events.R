library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Level 1 Hyperglycemia Event (≥15 consecutive min of >180 mg/dL)
hyper_lv1 <- detect_hyperglycemic_events(
  example_data_5_subject,
  start_gl = 180,
  dur_length = 15,
  end_length = 15,
  end_gl = 180
)
print(hyper_lv1$events_total)

# Level 2 Hyperglycemia Event (≥15 consecutive min of >250 mg/dL)
hyper_lv2 <- detect_hyperglycemic_events(
  example_data_5_subject,
  start_gl = 250,
  dur_length = 15,
  end_length = 15,
  end_gl = 250
)

# Extended Hyperglycemia Event (default parameters)
hyper_extended <- detect_hyperglycemic_events(example_data_5_subject)

# Analysis on larger dataset
large_hyper <- detect_hyperglycemic_events(
  example_data_hall,
  start_gl = 180,
  dur_length = 15,
  end_length = 15,
  end_gl = 180
)
print(paste("Total hyperglycemic events:", sum(large_hyper$events_total$total_events)))

# View detailed events for first subject
if(nrow(hyper_lv1$events_detailed) > 0) {
  first_subject <- hyper_lv1$events_detailed$id[1]
  subject_events <- hyper_lv1$events_detailed[hyper_lv1$events_detailed$id == first_subject, ]
  head(subject_events)
}
