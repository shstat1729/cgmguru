library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Create start points for demonstration (using row indices)
start_indices <- seq(1, nrow(example_data_5_subject), by = 100)
start_points <- data.frame(start_indices = start_indices)

# Find maximum glucose in next 2 hours
max_after <- find_max_after_hours(example_data_5_subject, start_points, hours = 2)
print(paste("Found", length(max_after$max_indices), "maximum points"))

# Find maximum glucose in next 1 hour
max_after_1h <- find_max_after_hours(example_data_5_subject, start_points, hours = 1)

# Analysis on larger dataset
large_start_indices <- seq(1, nrow(example_data_hall), by = 200)
large_start_points <- data.frame(start_indices = large_start_indices)
large_max_after <- find_max_after_hours(example_data_hall, large_start_points, hours = 2)
print(paste("Found", length(large_max_after$max_indices), "maximum points in larger dataset"))
