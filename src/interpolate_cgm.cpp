#include "event_preprocessing.h"

#include <Rcpp.h>
#include <map>
#include <string>
#include <vector>

using namespace Rcpp;

namespace {

std::string timezone_from_time(const NumericVector& time) {
  std::string tzone = "UTC";
  RObject tz_attr = time.attr("tzone");

  if (!tz_attr.isNULL()) {
    CharacterVector tz_values = as<CharacterVector>(tz_attr);
    if (tz_values.size() > 0 && !CharacterVector::is_na(tz_values[0])) {
      tzone = as<std::string>(tz_values[0]);
    }
  }

  if (tzone.empty()) {
    tzone = "UTC";
  }

  return tzone;
}

} // namespace

// [[Rcpp::export]]
DataFrame interpolate_cgm_cpp(DataFrame df,
                              SEXP reading_minutes = R_NilValue,
                              bool sort_time = false,
                              double inter_gap = 45) {
  if (!df.containsElementNamed("id")) {
    stop("interpolate_cgm requires an 'id' column");
  }
  if (!df.containsElementNamed("time")) {
    stop("interpolate_cgm requires a 'time' column");
  }
  if (!df.containsElementNamed("gl")) {
    stop("interpolate_cgm requires a 'gl' column");
  }

  const int n = df.nrows();
  StringVector id = df["id"];
  NumericVector time = df["time"];
  NumericVector glucose = df["gl"];
  const std::string tzone = timezone_from_time(time);

  std::map<std::string, std::vector<int>> id_indices;
  for (int i = 0; i < n; ++i) {
    if (StringVector::is_na(id[i])) {
      continue;
    }
    id_indices[as<std::string>(id[i])].push_back(i);
  }

  cgmguru_events::sort_or_validate_id_indices(id_indices, time, sort_time);

  cgmguru_events::InterpolatedDataStore interpolated_data;

  for (auto const& id_pair : id_indices) {
    const std::string& current_id = id_pair.first;
    const std::vector<int>& indices = id_pair.second;

    double id_reading_minutes =
      cgmguru_events::reading_minutes_for_id(reading_minutes, time, indices, n);

    if (id_reading_minutes > inter_gap + 1e-7) {
      stop("reading_minutes must be less than or equal to inter_gap");
    }

    id_reading_minutes =
      cgmguru_events::iglu_day_grid_reading_minutes(id_reading_minutes);

    cgmguru_events::PreparedIDData prepared =
      cgmguru_events::prepare_id_data(time, glucose, indices, id_reading_minutes,
                                      inter_gap, tzone, true, true);
    interpolated_data.append(current_id, prepared);
  }

  return interpolated_data.to_dataframe(tzone, false);
}
