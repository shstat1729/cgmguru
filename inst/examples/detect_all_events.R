library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Detect all glycemic events
# reading_minutes is calculated automatically from timestamp spacing when omitted
all_events <- detect_all_events(example_data_5_subject)
print(all_events$subject_summary)
print(all_events$glycemic_event_summary)

# Detect all events on larger dataset
large_all_events <- detect_all_events(example_data_hall)
print(paste("Total subjects analyzed:", nrow(large_all_events$subject_summary)))

# Filter for specific event types
hyperglycemia_events <- all_events$glycemic_event_summary[all_events$glycemic_event_summary$type == "hyper", ]
hypoglycemia_events <- all_events$glycemic_event_summary[all_events$glycemic_event_summary$type == "hypo", ]

print("Hyperglycemia events:")
print(hyperglycemia_events)
print("Hypoglycemia events:")
print(hypoglycemia_events)
