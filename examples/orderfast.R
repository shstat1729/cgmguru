library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Create unordered data
unordered_data <- data.frame(
  id = c("b", "a", "a"),
  time = as.POSIXct(c("2024-01-01 01:00:00",
                      "2024-01-01 00:00:00",
                      "2024-01-01 01:00:00"), tz = "UTC"),
  gl = c(120, 100, 110)
)

# Order the data
ordered_data <- orderfast(unordered_data)
print("Ordered data:")
print(ordered_data)

# Order sample data
ordered_sample <- orderfast(example_data_5_subject)
print(paste("Ordered", nrow(ordered_sample), "rows"))

# Order larger dataset
ordered_large <- orderfast(example_data_hall)
print(paste("Ordered", nrow(ordered_large), "rows in larger dataset"))
