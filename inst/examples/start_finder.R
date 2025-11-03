library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Create a binary vector indicating episode starts
binary_vector <- c(0, 0, 1, 1, 0, 1, 0, 0, 1, 1)
df <- data.frame(episode_starts = binary_vector)

# Find R-based indices where episodes start
start_points <- start_finder(df)
print(paste("Start indices:", paste(start_points$start_indices, collapse = ", ")))

# Use with actual GRID results
grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)
grid_starts <- start_finder(grid_result$grid_vector)
print(paste("GRID episode starts:", length(grid_starts$start_indices)))

# Analysis on larger dataset
large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
large_starts <- start_finder(large_grid$grid_vector)
print(paste("GRID episode starts in larger dataset:", length(large_starts$start_indices)))

