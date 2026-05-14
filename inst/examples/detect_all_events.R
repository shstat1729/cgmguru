library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Detect all glycemic events
# reading_minutes is calculated automatically from timestamp spacing when omitted
all_events <- detect_all_events(example_data_5_subject)
print(all_events$summary_df)
print(all_events$events_long_df)

# Detect all events on larger dataset
large_all_events <- detect_all_events(example_data_hall)
print(paste("Total subjects analyzed:", nrow(large_all_events$summary_df)))

# Filter for specific event types
hyperglycemia_events <- all_events$events_long_df[all_events$events_long_df$type == "hyper", ]
hypoglycemia_events <- all_events$events_long_df[all_events$events_long_df$type == "hypo", ]

print("Hyperglycemia events:")
print(hyperglycemia_events)
print("Hypoglycemia events:")
print(hypoglycemia_events)
