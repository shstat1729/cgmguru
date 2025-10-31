library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Complete pipeline example with smaller dataset
threshold <- 130
gap <- 60
hours <- 2
# 1) Find GRID points
grid_result <- grid(example_data_5_subject, gap = gap, threshold = threshold)

# 2) Find modified GRID points before 2 hours minimum
mod_grid <- mod_grid(example_data_5_subject,
                     start_finder(grid_result$grid_vector),
                     hours = hours,
                     gap = gap)

# 3) Find maximum point 2 hours after mod_grid point
mod_grid_maxima <- find_max_after_hours(example_data_5_subject,
                                        start_finder(mod_grid$mod_grid_vector),
                                        hours = hours)

# 4) Identify local maxima around episodes/windows
local_maxima <- find_local_maxima(example_data_5_subject)

# 5) Among local maxima, find maximum point after two hours
final_maxima <- find_new_maxima(example_data_5_subject,
                                mod_grid_maxima$max_indices,
                                local_maxima$local_maxima_vector)

# 6) Map GRID points to maximum points (within 4 hours)
transform_maxima <- transform_df(grid_result$episode_start, final_maxima)

# 7) Redistribute overlapping maxima between GRID points
final_between_maxima <- detect_between_maxima(example_data_5_subject, transform_maxima)

# Complete pipeline example with larger dataset (example_data_hall)
hall_threshold <- 130
hall_gap <- 60
hall_hours <- 2

# 1) Find GRID points on larger dataset
hall_grid_result <- grid(example_data_hall, gap = hall_gap, threshold = hall_threshold)

# 2) Find modified GRID points
hall_mod_grid <- mod_grid(example_data_hall,
                         start_finder(hall_grid_result$grid_vector),
                         hours = hall_hours,
                         gap = hall_gap)

# 3) Find maximum points after mod_grid
hall_mod_grid_maxima <- find_max_after_hours(example_data_hall,
                                            start_finder(hall_mod_grid$mod_grid_vector),
                                            hours = hall_hours)

# 4) Identify local maxima
hall_local_maxima <- find_local_maxima(example_data_hall)

# 5) Find new maxima
hall_final_maxima <- find_new_maxima(example_data_hall,
                                     hall_mod_grid_maxima$max_indices,
                                     hall_local_maxima$local_maxima_vector)

# 6) Transform data
hall_transform_maxima <- transform_df(hall_grid_result$episode_start, hall_final_maxima)

# 7) Detect between maxima
hall_final_between_maxima <- detect_between_maxima(example_data_hall, hall_transform_maxima)

