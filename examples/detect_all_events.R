library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Detect all glycemic events with 5-minute reading intervals
all_events <- detect_all_events(example_data_5_subject, reading_minutes = 5)
print(all_events)

# Detect all events on larger dataset
large_all_events <- detect_all_events(example_data_hall, reading_minutes = 5)
print(paste("Total event types analyzed:", nrow(large_all_events)))

# Filter for specific event types
hyperglycemia_events <- all_events[all_events$type == "hyper", ]
hypoglycemia_events <- all_events[all_events$type == "hypo", ]

print("Hyperglycemia events:")
print(hyperglycemia_events)
print("Hypoglycemia events:")
print(hypoglycemia_events)
