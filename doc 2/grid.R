## ----setup, include=FALSE-----------------------------------------------------
knitr::opts_chunk$set(collapse = TRUE, comment = "#>")
library(cgmguru)
library(iglu)

## ----data-load----------------------------------------------------------------
data(example_data_5_subject)
data(example_data_hall)

## ----grid-analysis------------------------------------------------------------
grid_result <- grid(example_data_5_subject, gap = 15, threshold = 130)

# View the number of GRID events detected per subject
print(grid_result$episode_counts)

# View the start points of GRID events
print(head(grid_result$episode_start))

# See the identified points in the time series
grid_points <- head(grid_result$grid_vector)
print(grid_points)

## ----grid-sensitive-----------------------------------------------------------
sensitive_result <- grid(example_data_5_subject, gap = 10, threshold = 120)

print(head(sensitive_result$episode_counts))

## ----grid-large---------------------------------------------------------------
large_grid <- grid(example_data_hall, gap = 15, threshold = 130)
print(paste("Detected", sum(large_grid$episode_counts$episode_counts), "episodes"))

## ----plot-grid, fig.height=4--------------------------------------------------
library(ggplot2)
subid <- example_data_5_subject$id[1]
subdata <- example_data_5_subject[example_data_5_subject$id == subid, ]
substarts <- grid_result$episode_start[grid_result$episode_start$id == subid, ]

plot <- ggplot(subdata, aes(x = time, y = gl)) +
  geom_line() +
  geom_point(data = substarts, aes(x = time, y = gl), color = 'red', size = 2) +
  labs(title = paste("GRID Events for Subject", subid), y = "Glucose (mg/dL)")
plot

