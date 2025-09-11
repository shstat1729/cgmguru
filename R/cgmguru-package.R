#' Continuous Glucose Monitoring Analysis and GRID-Based Event Detection
#'
#' @description
#' Tools for analyzing Continuous Glucose Monitoring (CGM) time-series and
#' detecting glycemic events using GRID-based methodologies. The package provides
#' comprehensive utilities for identifying local maxima/minima, event segmentation
#' around maxima, and summary transformations for downstream analysis.
#'
#' @details
#' The package implements several key algorithms for CGM analysis:
#' \itemize{
#'   \item \strong{GRID Algorithm}: Detects glycemic events using a gap-based approach
#'   \item \strong{Maxima Detection}: Identifies local maxima in glucose time series
#'   \item \strong{Event Segmentation}: Segments events around detected maxima
#'   \item \strong{Hyper/Hypoglycemic Detection}: Specialized functions for different
#'         glycemic event types
#' }
#'
#' Core algorithms are implemented in C++ via 'Rcpp' for optimal performance,
#' making the package suitable for large-scale CGM data analysis.
#'
#' @section Main Functions:
#' \describe{
#'   \item{\code{\link{grid}}}{GRID algorithm for glycemic event detection}
#'   \item{\code{\link{maxima_grid}}}{Combined maxima detection and GRID analysis}
#'   \item{\code{\link{detect_hyperglycemic_events}}}{Hyperglycemic event detection}
#'   \item{\code{\link{detect_hypoglycemic_events}}}{Hypoglycemic event detection}
#'   \item{\code{\link{find_local_maxima}}}{Local maxima identification}
#'   \item{\code{\link{orderfast}}}{Fast dataframe ordering utility}
#' }
#'
#' @section Data Requirements:
#' Input dataframes should contain:
#' \itemize{
#'   \item \code{id}: Subject identifier
#'   \item \code{time}: Timestamp (POSIXct format)
#'   \item \code{gl}: Glucose concentration values
#' }
#'
#' @section Examples:
#' \preformatted{
#' # Basic GRID analysis
#' result <- grid(cgm_data, gap = 15, threshold = 130)
#'
#' # Maxima detection
#' maxima <- find_local_maxima(cgm_data)
#'
#' # Hyperglycemic event detection
#' events <- detect_hyperglycemic_events(cgm_data, dur_length = 120)
#' }
#'
#' @author Sang Ho Park \email{shstat1729@gmail.com}
#'
#' @references
#' For more information about the GRID algorithm and CGM analysis methodologies,
#' see the package vignette: \code{vignette("intro", package = "cgmguru")}
#'
#' @seealso
#' \code{\link{grid}}, \code{\link{maxima_grid}}, \code{\link{detect_hyperglycemic_events}}
#'
#' @keywords internal
#' @useDynLib cgmguru, .registration = TRUE
#' @importFrom Rcpp evalCpp
"_PACKAGE"

## usethis namespace: start
## usethis namespace: end
NULL
