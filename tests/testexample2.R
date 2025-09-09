# remove.packages("cgmguru")
# R -e "Rcpp::compileAttributes()"
# find . -name "*.o" -delete && find . -name "*.so" -delete
# R CMD build . --no-build-vignettes
# R CMD INSTALL cgmguru_0.1.0.tar.gz

# test new code
library(iglu)
library(Rcpp)
library(dplyr)
orderfast <- function(df){
  return(df[order(df$id,df$time),])
}
setwd("/Users/bagsangho/Library/CloudStorage/OneDrive-개인/삼성서울병원/CGM/cgmguru_others")

# Macstudio
# setwd("/Users/sanghopark/Library/CloudStorage/OneDrive-개인/삼성서울병원/CGM/cgmguru_others")

# Find all CSV files in a specific directory
csv_files <- list.files(path = "./HUPA-UCM Diabetes Dataset/Preprocessed/",
                        pattern = "\\.csv$",
                        full.names = TRUE)
dataset <- data.frame()
for (i in 1:25){
  tmp <- read.csv(csv_files[i],sep=";")
  tmp <- data.frame(rep(paste0(i,"_id"),nrow(tmp)),tmp)
  colnames(tmp)[1] <- "id"
  dataset <- rbind(dataset,tmp)
}
dataset <- dataset[,1:3]
colnames(dataset) <- c("id","time","gl")
dataset$time <- paste0(substr(dataset$time,1,10)," ",substr(dataset$time,12,19))
dataset$time <- as.POSIXct(dataset$time,tz="UTC")
# sourceCpp('/Users/sanghopark/Library/CloudStorage/OneDrive-개인/삼성서울병원/CGM/cgmguru/241126_maximum_correction.cpp')
# sourceCpp('/Users/bagsangho/Library/CloudStorage/OneDrive-개인/삼성서울병원/CGM/cgmguru/241126_maximum_correction.cpp')
# total_result <- data.frame()
#
# a <- Sys.time()
# for (j in 1:5){
#   new_df <- example_data_5_subject
#   new_df <- orderfast(example_data_5_subject)
#   colnames(new_df) <- c("id","Time","Glucose")
#   unique_id <- unique(new_df$id)
#
#   new_df <- new_df %>%
#     filter(id==unique_id[j])
#   new_df[,"Time"] <- as.POSIXct(new_df[,"Time"],tz="UTC")
#   new_df$GRID <- GRID(new_df)
#
#   # make a columns
#
#   new_df$GRID_start <- 0
#   new_df$GRID_max <- 0
#   new_df$nadir <- 0
#   new_df$mod_GRID_start <- 0
#   new_df$mod_GRID_max <- 0
#
#
#   GRID_point <- NA
#   GRID_point <- which(diff(new_df$GRID)==1)+1
#   new_df[GRID_point,"GRID_start"] <-1
#   if(length(GRID_point)==0){
#     # parallel 주석
#     # parallel을 활용하고 싶으면 next도 주석하고 나머지는 주석 해제
#     # information_df <- original_format[ind,]
#     next
#     # return(information_df)
#   }
#
#   new_df$GRID_max <- findMaxTwoHours(new_df,GRID_point)
#
#
#   # modified GRID method
#   # before 2 hour
#   new_df$mod_GRID <- modGRID(new_df,GRID_point)
#
#
#   #find max point for modified algorithms
#   mod_GRID_point <- which(diff(c(0,new_df$mod_GRID))==1)
#   new_df[mod_GRID_point,"mod_GRID_start"] <-1
#   new_df$mod_GRID_max <- findMaxTwoHours(new_df,mod_GRID_point)
#
#
#
#   new_df$ID <- 1:nrow(new_df)
#
#   grad_df <- new_df %>%
#     filter(!is.na(Glucose))
#
#
#
#   grad_time <- as.character(paste(substr(grad_df[,1],1,10), substr(grad_df[,1],12,19), sep = ' '))
#   grad_time <- strptime(grad_time,
#                         format = "%Y-%m-%d %H:%M:%S",
#                         tz = '')
#   #####################
#
#   differ_glucose <- diff(grad_df[,"Glucose"])
#   local_minima <- c()
#   for (i in 1:(nrow(grad_df)-2)){
#     if (i==1 | i==2|i==3){
#
#     }else{
#       if(differ_glucose[i-2]<=0&differ_glucose[i-1]<=0&differ_glucose[i]>=0&differ_glucose[i+1]>=0){
#         local_minima <- c(local_minima,i)
#       }
#
#     }
#   }
#
#   final_minima=grad_df$ID[local_minima]
#
#   local_maxima <- c()
#   for (i in 1:(nrow(grad_df)-2)){
#     if (i==1 | i==2|i==3){
#
#     }else{
#       if(differ_glucose[i-2]>=0&differ_glucose[i-1]>=0&differ_glucose[i]<=0&differ_glucose[i+1]<=0){
#         local_maxima <- c(local_maxima,i)
#       }
#
#     }
#   }
#
#   final_maxima=grad_df$ID[local_maxima]
#
#   new_df$minima_GRID_point <- 0
#
#   new_df$minima_GRID_point <- newMinimaGRID(new_df,mod_GRID_point,final_minima)
#   mod_GRID_maxpoint <- which(new_df$mod_GRID_max==1)
#   new_df$maxima_GRID_point <- 0
#   new_df$maxima_GRID_point <- findNewMaxima(new_df,mod_GRID_maxpoint,final_maxima)
#
#
#
#   replacement_df <- new_df[which(new_df$GRID_start==1),1:2]
#
#   if(nrow(replacement_df) > 0){
#
#     tmp <- transformSummaryDF(new_df[which(new_df$GRID_start==1),"Time"],new_df[which(new_df$GRID_start==1),"Glucose"],new_df[which(new_df$maxima_GRID_point==1),"Time"],new_df[which(new_df$maxima_GRID_point==1),"Glucose"])
#     tmp <- tmp[which(tmp[,1]!=""),]
#
#   }
#
#
#   result <- detectBetweenMaxima(new_df,as.POSIXct(tmp$minima_point,tz="UTC"),as.POSIXct(tmp$maxima_point,tz="UTC"),tmp$maxima_glucose)
#
#
#   total_result <- rbind(total_result,data.frame(id=rep(unique_id[j],nrow(tmp)),GRID_time=tmp$minima_point,GRID_gl=tmp$minima_glucose,maxima_time=result$time,maxima_gl=result$glucose))
# }
# b <- Sys.time()
# b-a
# remove(checkValueDropBelow)
# remove(countTimeDropBelow)
# remove(detectBetweenMaxima)
# remove(detectHyperglycemicEvents)
# remove(detectHypoglycemicEvents)
# remove(excursion)
# remove(findMaxTwoHours)
# remove(findNewMaxima)
# remove(gapImpute)
# remove(GRID)
# remove(transformSummaryDF)
# remove(hypoglycemic)
# remove(libreNAImpute)
# remove(modExcursion)
# remove(modGRID)
# remove(newMinimaExcursion)
# remove(newMinimaGRID)
# remove(rcpp_mean_with_pointer)
# remove(rcpp_mean_with_pointer_indexed)


# This script is designed to perform a clean reinstall of the cgmguru
# and then test the splitById function using data from the iglu package.
# 1. Remove the existing package to ensure a clean install
cat("---\n Step 1: Removing existing cgmguru ---\n")
try({
  remove.packages("cgmguru")
  cat("cgmguru removed successfully.\n")
}, silent = TRUE)

# 2. Recompile attributes and reinstall the package from source
# Note: This assumes the script is run from the package's root directory.
cat("\n--- Step 2: Recompiling and Reinstalling cgmguru ---\n")
tryCatch({
  cat("Running Rcpp::compileAttributes()...\n")
  Rcpp::compileAttributes()
  cat("Installing package from local source...\n")
  install.packages(".", repos = NULL, type = "source", quiet = TRUE)
  cat("cgmguru reinstalled successfully.\n")
}, error = function(e) {
  stop("Failed to reinstall the package. Error: ", e$message)
})


# 3. Load necessary libraries
cat("\n--- Step 3: Loading libraries ---\n")
library(cgmguru)
library(iglu)
library(dplyr)


cat("Libraries loaded.\n")
# 4. Load example data from the iglu package
cat("\n--- Step 4: Loading data ---\n")

data(example_data_5_subject)
example_data_5_subject
# example_data_5_subject <- orderfast(example_data_5_subject)
# cat("Loaded `example_subject_data5` from iglu package.\n")

# threshold=130
# gap=60
# grid_result <- GRID(example_data_5_subject, gap=gap, threshold=threshold)
#
# mod_grid <- modGRID(example_data_5_subject,startFinder(grid_result$GRID_vector),hours=2,gap=gap)
#
# mod_grid_maxima <- findMaxAfterHours(example_data_5_subject, startFinder(mod_grid$modGRID_vector),hours=2)
#
# local_maxima <- findLocalMaxima(example_data_5_subject)
# final_maxima <- findNewMaxima(example_data_5_subject, mod_grid_maxima$max_indices, local_maxima$localMaxima_vector)
# transform_maxima <- transformSummaryDf(grid_result$episode_start_total,final_maxima)
#
# final_between_maxima <- detectBetweenMaxima(example_data_5_subject, transform_maxima)

# test1 <- maxima_GRID(example_data_5_subject)
# test2 <- maxima_GRID_optimized_test(example_data_5_subject)
# test1
# test2


# dataset <- dataset %>%
#   filter(id=="1_id")


iglu_episode <- episode_calculation(example_data_5_subject)
iglu_episode

detectHypoglycemicEvents(dataset,start_gl = 70,dur_length=15,end_length=15) # type : hypo, level = lv1
detectHypoglycemicEvents(dataset,start_gl = 54,dur_length=15,end_length=15) # type : hypo, level = lv2
detectHypoglycemicEvents(dataset) # type : hypo, level = extended
detectLevel1HypoglycemicEvents(dataset) # type : hypo, level = lv1_excl
detectHyperglycemicEvents(dataset, start_gl = 181,dur_length=15,end_length=15,end_gl=180) # type : hyper, level = lv1
detectHyperglycemicEvents(dataset, start_gl = 251,dur_length=15,end_length=15,end_gl=250) # type : hyper, level = lv2
detectHyperglycemicEvents(dataset) # type : hyper, level = extended
detectLevel1HyperglycemicEvents(dataset) # type : hyper, level = lv1_excl

a <- Sys.time()
test1 <- episode_calculation(example_data_5_subject)
b <- Sys.time()
b-a

a <- Sys.time()
test2 <- detectAllEvents(example_data_5_subject)
b <- Sys.time()
b-a

test1
test2



a <- Sys.time()
test1 <- episode_calculation(dataset)
b <- Sys.time()
b-a

a <- Sys.time()
test2 <- detectAllEvents(dataset)
b <- Sys.time()
b-a
remove.packages("cgmguru")
remotes::install_github("shstat1729/cgmguru")
library(cgmguru)
library(iglu)

setwd("/Users/bagsangho/Library/CloudStorage/OneDrive-개인/삼성서울병원/CGM/cgmguru_others")

# Macstudio
# setwd("/Users/sanghopark/Library/CloudStorage/OneDrive-개인/삼성서울병원/CGM/cgmguru_others")

# Find all CSV files in a specific directory
csv_files <- list.files(path = "./HUPA-UCM Diabetes Dataset/Preprocessed/",
                        pattern = "\\.csv$",
                        full.names = TRUE)
dataset <- data.frame()
for (i in 1:25){
  tmp <- read.csv(csv_files[i],sep=";")
  tmp <- data.frame(rep(paste0(i,"_id"),nrow(tmp)),tmp)
  colnames(tmp)[1] <- "id"
  dataset <- rbind(dataset,tmp)
}
dataset <- dataset[,1:3]
colnames(dataset) <- c("id","time","gl")
dataset$time <- paste0(substr(dataset$time,1,10)," ",substr(dataset$time,12,19))
dataset$time <- as.POSIXct(dataset$time,tz="UTC")





# Equivalent examples using camelCase naming for readability
# Level 1 Hypoglycemia Event (≥15 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
detect_hypoglycemic_events(dataset, start_gl = 70, dur_length = 15, end_length = 15)  # hypo, level = lv1

# Level 2 Hypoglycemia Event (≥15 consecutive min of <54 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥54 mg/dL)
detect_hypoglycemic_events(dataset, start_gl = 54, dur_length = 15, end_length = 15)  # hypo, level = lv2

# Extended Hypoglycemia Event (>120 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL)
detect_hypoglycemic_events(dataset)                                                    # hypo, extended

# Hypoglycaemia alert value (54–69 mg/dL (3·0–3·9 mmol/L) ≥15 consecutive min of <70 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≥70 mg/dL
detect_level1_hypoglycemic_events(dataset)                                              # hypo, lv1_excl

# Level 1 Hyperglycemia Event (≥15 consecutive min of >180 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≤180 mg/dL)
detect_hyperglycemic_events(example_data_5_subject, start_gl = 181, dur_length = 15, end_length = 15, end_gl = 180)  # hyper, lv1

# Level 2 Hyperglycemia Event (≥15 consecutive min of >250 mg/dL and event ends when there is ≥15 consecutive min with a CGM sensor value of ≤250 mg/dL)
detect_hyperglycemic_events(dataset, start_gl = 251, dur_length = 15, end_length = 15, end_gl = 250)  # hyper, lv2

# Extended Hyperglycemia Event (Number of events with sensor glucose >250 mg/dL (>13·9 mmol/L) lasting at least 120 min; event ends when glucose returns to ≤180 mg/dL (≤10·0 mmol/L) for ≥15 min)
detect_hyperglycemic_events(dataset)                                                                 # hyper, extended

# High glucose (Level 1, excluded) (181–250 mg/dL (10·1–13·9 mmol/L) ≥15 consecutive min and Event ends when there is ≥15 consecutive min with a CGM sensor value of ≤180 mg/dL) 
detect_level1_hyperglycemic_events(dataset)   

a <- Sys.time()
episode_calculation(example_data_5_subject)
b <- Sys.time()
b-a
a <- Sys.time()
detect_all_events(example_data_5_subject)
b <- Sys.time()
b-a


threshold <- 130
gap <- 60
hours <- 2

# 1) Find GRID points
grid_result <- grid(dataset, gap = gap, threshold = threshold)

# 2) Find modified GRID points before 2 hours minimum
mod_grid <- mod_grid(dataset, 
                     start_finder(grid_result$grid_vector), 
                     hours = hours, 
                     gap = gap)

# 3) Find maximum point 2 hours after mod_grid point
mod_grid_maxima <- find_max_after_hours(dataset, 
                                        start_finder(mod_grid$mod_grid_vector), 
                                        hours = hours)

# 4) Identify local maxima around episodes/windows
local_maxima <- find_local_maxima(dataset)

# 5) Among local maxima, find maximum point after two hours
final_maxima <- find_new_maxima(dataset, 
                                mod_grid_maxima$max_indices, 
                                local_maxima$local_maxima_vector)


# 6) Map GRID points to maximum points (within 4 hours)
transform_maxima <- transform_df(grid_result$episode_start_total, final_maxima)

# 7) Redistribute overlapping maxima between GRID points
final_between_maxima <- detect_between_maxima(dataset, transform_maxima)

test1 <- maxima_grid(dataset)
test2 <- final_between_maxima
test1$episode_counts
test2$episode_counts

maxima_grid(example_data_5_subject)
example_data_5_subject

start_finder(as.vector(grid_result$grid_vector)$grid)
