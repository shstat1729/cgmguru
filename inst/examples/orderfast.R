library(cgmguru)
library(iglu)

data(example_data_5_subject)
data(example_data_hall)

# Shuffle without replacement, then order and compare to baseline
set.seed(123)
shuffled <- example_data_5_subject[sample(seq_len(nrow(example_data_5_subject)),
                                          replace = FALSE), ]
baseline <- orderfast(example_data_5_subject)
ordered_shuffled <- orderfast(shuffled)

# Compare results
print(paste("Identical after ordering:", identical(baseline, ordered_shuffled)))
head(baseline[, c("id", "time", "gl")])
head(ordered_shuffled[, c("id", "time", "gl")])

# Order larger dataset
ordered_large <- orderfast(example_data_hall)
print(paste("Ordered", nrow(ordered_large), "rows in larger dataset"))

