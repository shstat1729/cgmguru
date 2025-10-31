library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Basic GRID analysis on smaller dataset
grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
print(grid_result$episode_counts)
print(grid_result$episode_start)
print(grid_result$grid_vector)

# More sensitive GRID analysis
sensitive_result <- grid(example_data_5_subject, gap = 10, threshold = 120)

# GRID analysis on larger dataset
large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
print(paste("Detected", sum(large_grid$episode_counts$episode_counts), "episodes"))
print(large_grid$episode_start)
print(large_grid$grid_vector)

