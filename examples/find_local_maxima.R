library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Find local maxima
maxima_result <- find_local_maxima(example_data_5_subject)
print(paste("Found", nrow(maxima_result$local_maxima_vector), "local maxima"))

# Find maxima on larger dataset
large_maxima <- find_local_maxima(example_data_hall)
print(paste("Found", nrow(large_maxima$local_maxima_vector), "local maxima in larger dataset"))

# View first few maxima
head(maxima_result$local_maxima_vector)
# View merged results
head(maxima_result$merged_results)
