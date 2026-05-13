library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Calculate glucose excursions
excursion_result <- excursion(example_data_5_subject, gap = 15)
print(paste("Excursion vector length:", length(excursion_result$excursion_vector)))
print(excursion_result$episode_counts)

# Excursion analysis with different gap
excursion_30min <- excursion(example_data_5_subject, gap = 30)

# Analysis on larger dataset
large_excursion <- excursion(example_data_hall, gap = 15)
print(paste("Excursion vector length in larger dataset:", length(large_excursion$excursion_vector)))
print(paste("Total episodes:", sum(large_excursion$episode_counts$episode_counts)))

