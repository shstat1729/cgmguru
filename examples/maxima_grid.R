library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Combined analysis on smaller dataset
maxima_result <- maxima_grid(example_data_5_subject, threshold = 130, gap = 60, hours = 2)
print(maxima_result$episode_counts)
print(maxima_result$results)

# More sensitive analysis
sensitive_maxima <- maxima_grid(example_data_5_subject, threshold = 120, gap = 30, hours = 1)
print(sensitive_maxima$episode_counts)
print(sensitive_maxima$results)

# Analysis on larger dataset
large_maxima <- maxima_grid(example_data_hall, threshold = 130, gap = 60, hours = 2)
print(large_maxima$episode_counts)
print(large_maxima$results)

