#include <Rcpp.h>

#include <cmath>
#include <map>
#include <string>
#include <vector>

using namespace Rcpp;

namespace {

bool is_missing_glucose(const double value) {
  return R_IsNA(value) || R_IsNaN(value);
}

constexpr double kBinSeconds = 15.0 * 60.0;

struct BinSummary {
  double glucose_sum = 0.0;
  int n_observed = 0;
};

} // namespace

// [[Rcpp::export]]
DataFrame interval_down_cpp(DataFrame df, bool n_observed = false) {
  if (!df.containsElementNamed("id")) {
    stop("interval_down requires an 'id' column");
  }
  if (!df.containsElementNamed("time")) {
    stop("interval_down requires a 'time' column");
  }
  if (!df.containsElementNamed("gl")) {
    stop("interval_down requires a 'gl' column");
  }

  const StringVector id = df["id"];
  const NumericVector time = df["time"];
  const NumericVector glucose = df["gl"];
  const int n = df.nrows();

  std::vector<std::string> output_id;
  std::vector<double> output_time;
  std::vector<double> output_glucose;
  std::vector<int> output_n_observed;

  output_id.reserve(n / 3);
  output_time.reserve(n / 3);
  output_glucose.reserve(n / 3);
  output_n_observed.reserve(n / 3);

  // CGM data are arranged as contiguous time-ordered records for each subject.
  // Each subject is downsampled in clock-aligned 15-minute bins rather than
  // by row position, so an absent 5-minute row cannot shift later intervals.
  int start = 0;
  while (start < n) {
    const std::string current_id = as<std::string>(id[start]);
    int end = start + 1;
    while (end < n && as<std::string>(id[end]) == current_id) {
      ++end;
    }

    std::map<long long, BinSummary> bins;
    for (int i = start; i < end; ++i) {
      if (is_missing_glucose(glucose[i])) {
        continue;
      }

      const long long bin = static_cast<long long>(
        std::floor(time[i] / kBinSeconds)
      );
      BinSummary& summary = bins[bin];
      summary.glucose_sum += glucose[i];
      ++summary.n_observed;
    }

    for (const auto& bin_pair : bins) {
      const long long bin = bin_pair.first;
      const BinSummary& summary = bin_pair.second;
      output_id.push_back(current_id);
      // Label each aggregate at the right boundary of its 15-minute interval.
      output_time.push_back(
        static_cast<double>(bin + 1) * kBinSeconds
      );
      output_glucose.push_back(summary.glucose_sum / summary.n_observed);
      output_n_observed.push_back(summary.n_observed);
    }

    start = end;
  }

  CharacterVector output_id_rcpp = wrap(output_id);
  NumericVector output_time_rcpp = wrap(output_time);
  NumericVector output_glucose_rcpp = wrap(output_glucose);
  IntegerVector output_n_observed_rcpp = wrap(output_n_observed);

  RObject time_class = time.attr("class");
  if (!time_class.isNULL()) {
    output_time_rcpp.attr("class") = time_class;
  }
  RObject time_zone = time.attr("tzone");
  if (!time_zone.isNULL()) {
    output_time_rcpp.attr("tzone") = time_zone;
  }

  if (n_observed) {
    return DataFrame::create(
      _["id"] = output_id_rcpp,
      _["time"] = output_time_rcpp,
      _["gl"] = output_glucose_rcpp,
      _["n_observed"] = output_n_observed_rcpp,
      _["stringsAsFactors"] = false
    );
  }

  return DataFrame::create(
    _["id"] = output_id_rcpp,
    _["time"] = output_time_rcpp,
    _["gl"] = output_glucose_rcpp,
    _["stringsAsFactors"] = false
  );
}
