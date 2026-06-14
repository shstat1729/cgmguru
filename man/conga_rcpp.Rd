\name{conga_rcpp}
\alias{conga_rcpp}
\title{Rcpp CONGA Calculation}
\usage{
conga_rcpp(data, n = 24, tz = "", inter_gap = 45)
}
\arguments{
\item{data}{A dataframe containing CGM data with columns \code{id},
\code{time}, and \code{gl}.}

\item{n}{Whole number of hours separating paired glucose observations.
Defaults to 24.}

\item{tz}{Time zone used for day-grid alignment when supplied.}

\item{inter_gap}{Maximum gap, in minutes, over which linear interpolation is
allowed. Defaults to 45, matching iglu's internal default.}
}
\value{
A tibble with columns \code{id} and \code{CONGA}.
}
\description{
Calculates Continuous Overall Net Glycemic Action (CONGA) with an Rcpp
backend.
}
\details{
The calculation follows the iglu definition: after interpolation to a regular
day-aligned CGM grid, CONGA is the sample standard deviation of glucose
differences separated by \code{n} hours.
}
\examples{
library(iglu)
data(example_data_5_subject)
conga_rcpp(example_data_5_subject)
}
\references{
McDonnell, C. M., et al. (2005). A novel approach to continuous glucose
analysis utilizing glycemic variation. \emph{Diabetes Technology and
Therapeutics}, 7, 253-263. \doi{10.1089/dia.2005.7.253}
}
\seealso{
\code{\link[iglu:conga]{iglu::conga}}, \code{\link{mage_rcpp}}
}
