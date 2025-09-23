## ----setup, include=FALSE-----------------------------------------------------
knitr::opts_chunk$set(collapse = TRUE, comment = "#>")

## -----------------------------------------------------------------------------
library(cgmguru)
# Minimal example using orderfast
df <- data.frame(
  id = c("b", "a", "a"),
  time = as.POSIXct(c("2024-01-01 01:00:00", "2024-01-01 00:00:00", "2024-01-01 01:00:00"), tz = "UTC")
)
orderfast(df)

