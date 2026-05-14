#include "event_preprocessing.h"
#include <algorithm>
#include <map>
#include <string>
#include <vector>

using namespace Rcpp;

namespace {

double infer_reading_minutes_from_valid_times(const std::vector<double>& times) {
  std::vector<double> diffs;
  diffs.reserve(times.size() > 0 ? times.size() - 1 : 0);

  for (size_t i = 1; i < times.size(); ++i) {
    const double diff_minutes = (times[i] - times[i - 1]) / 60.0;
    if (diff_minutes > 0.0) {
      diffs.push_back(diff_minutes);
    }
  }

  if (diffs.empty()) {
    Rcpp::stop("reading_minutes could not be inferred: each id needs at least two distinct valid time points or an explicit reading_minutes value");
  }

  std::sort(diffs.begin(), diffs.end());
  const size_t n = diffs.size();
  const double median = (n % 2 == 1)
    ? diffs[n / 2]
    : (diffs[n / 2 - 1] + diffs[n / 2]) / 2.0;
  const double rounded = std::round(median);
  return rounded > 0.0 ? rounded : median;
}

double reading_minutes_for_sensor_wear(
    SEXP reading_minutes_sexp,
    const std::vector<int>& original_indices,
    const std::vector<double>& valid_times,
    int full_length) {
  if (reading_minutes_sexp == R_NilValue) {
    return infer_reading_minutes_from_valid_times(valid_times);
  }

  if (TYPEOF(reading_minutes_sexp) == INTSXP) {
    IntegerVector reading_minutes_int = as<IntegerVector>(reading_minutes_sexp);
    if (reading_minutes_int.length() == 1) {
      return static_cast<double>(reading_minutes_int[0]);
    }
    if (reading_minutes_int.length() != full_length) {
      Rcpp::stop("reading_minutes vector length must match data length");
    }
    return static_cast<double>(reading_minutes_int[original_indices[0]]);
  }

  if (TYPEOF(reading_minutes_sexp) == REALSXP) {
    NumericVector reading_minutes_num = as<NumericVector>(reading_minutes_sexp);
    if (reading_minutes_num.length() == 1) {
      return reading_minutes_num[0];
    }
    if (reading_minutes_num.length() != full_length) {
      Rcpp::stop("reading_minutes vector length must match data length");
    }
    return reading_minutes_num[original_indices[0]];
  }

  Rcpp::stop("reading_minutes must be numeric or integer");
}

} // namespace

// [[Rcpp::export]]
DataFrame sensor_wear_cpp(DataFrame df,
                          SEXP reading_minutes = R_NilValue,
                          Nullable<NumericVector> end_date = R_NilValue,
                          double ndays = 14.0) {
  const int n = df.nrows();
  StringVector id = df["id"];
  NumericVector time = df["time"];
  NumericVector glucose = df["gl"];

  std::string output_tz = "UTC";
  RObject tz_attr = time.attr("tzone");
  if (!tz_attr.isNULL()) {
    CharacterVector tz_attr_cv = as<CharacterVector>(tz_attr);
    if (tz_attr_cv.size() > 0 && !CharacterVector::is_na(tz_attr_cv[0])) {
      output_tz = as<std::string>(tz_attr_cv[0]);
    }
  }

  std::map<std::string, std::vector<int>> id_indices;
  for (int i = 0; i < n; ++i) {
    id_indices[as<std::string>(id[i])].push_back(i);
  }

  const bool has_common_end_date = end_date.isNotNull();
  double common_end_date = NA_REAL;
  if (has_common_end_date) {
    NumericVector end_date_vec(end_date);
    if (end_date_vec.length() != 1 || NumericVector::is_na(end_date_vec[0])) {
      Rcpp::stop("end_date must be a single non-NA POSIXct value or NULL");
    }
    common_end_date = end_date_vec[0];
  }

  std::vector<std::string> out_ids;
  std::vector<double> out_sensor_wear;
  std::vector<double> out_ndays;
  std::vector<double> out_start_dates;
  std::vector<double> out_end_dates;
  out_ids.reserve(id_indices.size());
  out_sensor_wear.reserve(id_indices.size());
  out_ndays.reserve(id_indices.size());
  out_start_dates.reserve(id_indices.size());
  out_end_dates.reserve(id_indices.size());

  for (auto& id_pair : id_indices) {
    std::string current_id = id_pair.first;
    std::vector<int>& indices = id_pair.second;
    std::sort(indices.begin(), indices.end(), [&](int lhs, int rhs) {
      return time[lhs] < time[rhs];
    });

    std::vector<int> valid_original_indices;
    std::vector<double> valid_times;
    valid_original_indices.reserve(indices.size());
    valid_times.reserve(indices.size());

    for (int idx : indices) {
      if (NumericVector::is_na(time[idx]) || NumericVector::is_na(glucose[idx])) {
        continue;
      }

      const double current_time = time[idx];
      if (!valid_times.empty() &&
          std::fabs(current_time - valid_times.back()) < 1e-7) {
        valid_original_indices.back() = idx;
        valid_times.back() = current_time;
      } else {
        valid_original_indices.push_back(idx);
        valid_times.push_back(current_time);
      }
    }

    double sensor_wear = NA_REAL;
    double start_date_value = NA_REAL;
    double end_date_value = NA_REAL;

    if (!valid_times.empty() && ndays > 0.0) {
      double id_reading_minutes = reading_minutes_for_sensor_wear(
        reading_minutes, valid_original_indices, valid_times, n);
      if (id_reading_minutes <= 0.0) {
        Rcpp::stop("reading_minutes must be positive");
      }

      end_date_value = has_common_end_date ? common_end_date : valid_times.back();
      start_date_value = end_date_value - ndays * 24.0 * 60.0 * 60.0;

      int observed_count = 0;
      for (double valid_time : valid_times) {
        if (valid_time >= start_date_value && valid_time <= end_date_value) {
          ++observed_count;
        }
      }

      const double expected_count = ndays * 24.0 * (60.0 / id_reading_minutes);
      if (expected_count > 0.0) {
        sensor_wear = 100.0 * static_cast<double>(observed_count) / expected_count;
      }
    }

    out_ids.push_back(current_id);
    out_sensor_wear.push_back(sensor_wear);
    out_ndays.push_back(ndays);
    out_start_dates.push_back(start_date_value);
    out_end_dates.push_back(end_date_value);
  }

  NumericVector start_date_vec = wrap(out_start_dates);
  start_date_vec.attr("class") = CharacterVector::create("POSIXct", "POSIXt");
  start_date_vec.attr("tzone") = output_tz;

  NumericVector end_date_vec = wrap(out_end_dates);
  end_date_vec.attr("class") = CharacterVector::create("POSIXct", "POSIXt");
  end_date_vec.attr("tzone") = output_tz;

  DataFrame out = DataFrame::create(
    _["id"] = wrap(out_ids),
    _["sensor_wear"] = wrap(out_sensor_wear),
    _["ndays"] = wrap(out_ndays),
    _["start_date"] = start_date_vec,
    _["end_date"] = end_date_vec
  );
  out.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
  return out;
}
