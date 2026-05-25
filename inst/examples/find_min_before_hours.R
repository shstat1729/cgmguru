library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Create start points for demonstration (using row index)
start_index <- seq(1, nrow(example_data_5_subject), by = 100)
start_points <- data.frame(start_index = start_index)

# Find minimum glucose in previous 2 hours
min_before <- find_min_before_hours(example_data_5_subject, start_points, hours = 2)
print(paste("Found", length(min_before$min_index), "minimum points"))

# Find minimum glucose in previous 1 hour
min_before_1h <- find_min_before_hours(example_data_5_subject, start_points, hours = 1)

# Analysis on larger dataset
large_start_index <- seq(1, nrow(example_data_hall), by = 200)
large_start_points <- data.frame(start_index = large_start_index)
large_min_before <- find_min_before_hours(example_data_hall, large_start_points, hours = 2)
print(paste("Found", length(large_min_before$min_index), "minimum points in larger dataset"))

