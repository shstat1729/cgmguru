\name{mage_rcpp}
\alias{mage_rcpp}
\title{Rcpp MAGE Calculation}
\usage{
mage_rcpp(
  data,
  version = c("ma", "naive"),
  sd_multiplier = 1,
  short_ma = 5,
  long_ma = 32,
  return_type = c("num", "df"),
  direction = c("avg", "service", "max", "plus", "minus"),
  tz = "",
  inter_gap = 45,
  max_gap = 180
)
}
\arguments{
\item{data}{A dataframe containing CGM data with columns \code{id},
\code{time}, and \code{gl}.}

\item{version}{Either \code{"ma"} or \code{"naive"}. Defaults to
\code{"ma"}.}

\item{sd_multiplier}{Multiplier for the standard deviation threshold in
\code{version = "naive"}.}

\item{short_ma}{Short moving-average window length. Defaults to 5.}

\item{long_ma}{Long moving-average window length. Defaults to 32.}

\item{return_type}{Either \code{"num"} for one metric per id or \code{"df"}
for a list-column of segment-level MAGE rows.}

\item{direction}{One of \code{"avg"}, \code{"service"}, \code{"max"},
\code{"plus"}, or \code{"minus"}.}

\item{tz}{Time zone used for day-grid alignment when supplied.}

\item{inter_gap}{Maximum gap, in minutes, over which linear interpolation is
allowed. Defaults to 45.}

\item{max_gap}{Gap length, in minutes, above which MAGE is calculated on
separate trace segments. Defaults to 180.}
}
\value{
A tibble with columns \code{id} and \code{MAGE}. With \code{return_type =
"df"}, \code{MAGE} is a list-column of tibbles with \code{start},
\code{end}, \code{mage}, \code{plus_or_minus}, and
\code{first_excursion}.
}
\description{
Calculates Mean Amplitude of Glycemic Excursions (MAGE) with an Rcpp backend.
}
\details{
The implementation follows the MAGE calculation approaches used by
\code{\link[iglu:mage]{iglu::mage}}. The default \code{version = "ma"}
follows iglu's moving-average approach: CGM is interpolated to 5-minute
intervals, short and long moving-average crossings identify candidate
peak/nadir intervals, and countable excursions are those whose peak-to-nadir
or nadir-to-peak amplitude is at least one glucose standard deviation.

The \code{version = "naive"} option matches iglu's older standard-deviation
based calculation. This function is calculation-only and does not implement
iglu's plotting options.
}
\examples{
library(iglu)
data(example_data_5_subject)
mage_rcpp(example_data_5_subject)
mage_rcpp(example_data_5_subject, version = "naive")
}
\references{
Service, F. John, et al. (1970). Mean amplitude of glycemic excursions, a
measure of diabetic instability. \emph{Diabetes}, 19(9), 644-655.
\doi{10.2337/diab.19.9.644}

Fernandes, Nathaniel J., et al. (2022). Open-source algorithm to calculate
mean amplitude of glycemic excursions using short and long moving averages.
\emph{Journal of Diabetes Science and Technology}, 16(2), 576-577.
\doi{10.1177/19322968211061165}
}
\seealso{
\code{\link[iglu:mage]{iglu::mage}}, \code{\link{conga_rcpp}}
}
