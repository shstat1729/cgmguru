#include "id_based_calculator.h"

using namespace Rcpp;
using namespace std;

// BetweenMaxima-specific calculator class
class BetweenMaximaCalculator : public IdBasedCalculator {
private:
  // Store results for total DataFrame
  std::vector<std::string> total_result_ids;
  std::vector<double> total_result_grid_times;
  std::vector<double> total_result_grid_gls;
  std::vector<double> total_result_maxima_times;
  std::vector<double> total_result_maxima_gls;
  std::vector<double> total_result_time_to_peak; // Added for time-to-peak

  // Store episode counts (number of results per ID)
  std::map<std::string, int> result_counts_per_id;

  // Clear/initialize result storage
  void clear_results() {
    total_result_ids.clear();
    total_result_grid_times.clear();
    total_result_grid_gls.clear();
    total_result_maxima_times.clear();
    total_result_maxima_gls.clear();
    total_result_time_to_peak.clear();
    result_counts_per_id.clear();
  }

  // Detect between maxima for a single ID
  void detect_between_maxima_for_id(const std::string& current_id,
                                    const NumericVector& original_time_subset,
                                    const NumericVector& original_gl_subset,
                                    const NumericVector& grid_time_subset,
                                    const NumericVector& grid_gl_subset,
                                    const NumericVector& maxima_time_subset,
                                    const NumericVector& maxima_gl_subset) {

    int n = grid_time_subset.size();
    if (n < 1) return; // Need at least 1 GRID point

    int row_count = 0; // Count actual rows generated for this ID

    // Process consecutive pairs of GRID times (following original logic exactly)
    for (int i = 1; i < n; ++i) {
      double prev_grid_time = grid_time_subset[i-1];
      double curr_grid_time = grid_time_subset[i];

      if (NumericVector::is_na(prev_grid_time) ||
          NumericVector::is_na(curr_grid_time)) {
        continue;
      }

      double max_value = R_NegInf;
      double max_time = 0;

      // Check if maxima_time for consecutive indices are the same (original condition)
      bool same_maxima_time = false;
      if (i-1 < maxima_time_subset.size() && i < maxima_time_subset.size()) {
        if (!NumericVector::is_na(maxima_time_subset[i-1]) &&
            !NumericVector::is_na(maxima_time_subset[i]) &&
            maxima_time_subset[i-1] == maxima_time_subset[i]) {
          same_maxima_time = true;
        }
      }

      if (same_maxima_time) {
        // Find maximum between these two GRID times
        for (int j = 0; j < original_time_subset.size(); ++j) {
          double time_point = original_time_subset[j];
          double gl_value = original_gl_subset[j];

          if (NumericVector::is_na(time_point) || NumericVector::is_na(gl_value)) {
            continue;
          }

          // Check if this point is between the two GRID times
          if (time_point > prev_grid_time && time_point < curr_grid_time) {
            if (gl_value > max_value) {
              max_value = gl_value;
              max_time = time_point;
            }
          }
        }
      }

      // CRITICAL: Apply fallback logic EXACTLY like original
      double result_time;
      double result_value;

      if (max_value == R_NegInf) {
        // Use maxima data as fallback (original logic)
        if (i-1 < maxima_time_subset.size() && i-1 < maxima_gl_subset.size()) {
          result_time = maxima_time_subset[i-1];
          result_value = maxima_gl_subset[i-1];
        } else {
          result_time = NA_REAL;
          result_value = NA_REAL;
        }
      } else {
        result_time = max_time;
        result_value = max_value;
      }

      // Handle the "1970-01-01 00:00:00" case from original (time = 0)
      if (result_time == 0) {
        result_time = NA_REAL;
        result_value = NA_REAL;
      }

      // Calculate time-to-peak
      double time_to_peak = NA_REAL;
      if (!NumericVector::is_na(result_time) && !NumericVector::is_na(prev_grid_time)) {
        time_to_peak = (result_time - prev_grid_time);
      }

      // Store the result
      total_result_ids.push_back(current_id);
      total_result_grid_times.push_back(prev_grid_time);
      total_result_grid_gls.push_back(grid_gl_subset[i-1]);
      total_result_maxima_times.push_back(result_time);
      total_result_maxima_gls.push_back(result_value);
      total_result_time_to_peak.push_back(time_to_peak);

      row_count++; // Increment row count for each result
    }

    // Handle last element (from reference code)
    if (n > 0) {
      double last_result_time;
      double last_result_value;

      if (n-1 < maxima_time_subset.size() &&
          !NumericVector::is_na(maxima_time_subset[n-1])) {
        last_result_time = maxima_time_subset[n-1];
        last_result_value = (n-1 < maxima_gl_subset.size()) ?
                         maxima_gl_subset[n-1] : NA_REAL;
      } else {
        last_result_time = NA_REAL;
        last_result_value = NA_REAL;
      }

      // Calculate time-to-peak for last element
      double last_time_to_peak = NA_REAL;
      if (!NumericVector::is_na(last_result_time) && !NumericVector::is_na(grid_time_subset[n-1])) {
        last_time_to_peak = (last_result_time - grid_time_subset[n-1]);
      }

      total_result_ids.push_back(current_id);
      total_result_grid_times.push_back(grid_time_subset[n-1]);
      total_result_grid_gls.push_back(grid_gl_subset[n-1]);
      total_result_maxima_times.push_back(last_result_time);
      total_result_maxima_gls.push_back(last_result_value);
      total_result_time_to_peak.push_back(last_time_to_peak);

      row_count++; // Increment row count for last element
    }

    // Store the actual row count for this ID
    result_counts_per_id[current_id] = row_count;
  }

  // Create episode counts DataFrame based on actual row counts
  DataFrame create_row_counts_df() {
    std::vector<std::string> ids_for_df;
    std::vector<int> counts_for_df;

    ids_for_df.reserve(result_counts_per_id.size());
    counts_for_df.reserve(result_counts_per_id.size());

    for (auto const& pair : result_counts_per_id) {
      ids_for_df.push_back(pair.first);
      counts_for_df.push_back(pair.second);
    }

    DataFrame df = DataFrame::create(
      _["id"] = ids_for_df,
      _["episode_counts"] = counts_for_df
    );
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    return df;
  }

  // Create the result DataFrame
  DataFrame create_result_df() {
    if (total_result_ids.empty()) {
      DataFrame empty_df = DataFrame::create(
        _["id"] = CharacterVector::create(),
        _["grid_time"] = NumericVector::create(),
        _["grid_gl"] = NumericVector::create(),
        _["maxima_time"] = NumericVector::create(),
        _["maxima_glucose"] = NumericVector::create(),
        _["time_to_peak"] = NumericVector::create(),
        _["stringsAsFactors"] = false
      );
      empty_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
      return empty_df;
    }

    // Create POSIXct time vectors
    NumericVector grid_time_vec = wrap(total_result_grid_times);
    grid_time_vec.attr("class") = CharacterVector::create("POSIXct");
    grid_time_vec.attr("tzone") = "UTC";

    NumericVector maxima_time_vec = wrap(total_result_maxima_times);
    maxima_time_vec.attr("class") = CharacterVector::create("POSIXct");
    maxima_time_vec.attr("tzone") = "UTC";

    DataFrame df = DataFrame::create(
      _["id"] = wrap(total_result_ids),
      _["grid_time"] = grid_time_vec,
      _["grid_gl"] = wrap(total_result_grid_gls),
      _["maxima_time"] = maxima_time_vec,
      _["maxima_glucose"] = wrap(total_result_maxima_gls),
      _["time_to_peak"] = wrap(total_result_time_to_peak),
      _["stringsAsFactors"] = false
    );
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    return df;
  }

public:
  List calculate(const DataFrame& original_df,
                 const DataFrame& transform_summary_df) {

    clear_results();

    // Extract columns from original DataFrame
    StringVector original_id = original_df["id"];
    NumericVector original_time = original_df["time"];
    NumericVector original_gl = original_df["gl"];

    // Extract columns from transform summary DataFrame
    StringVector summary_id = transform_summary_df["id"];
    NumericVector summary_grid_time = transform_summary_df["grid_time"];
    NumericVector summary_grid_gl = transform_summary_df["grid_gl"];
    NumericVector summary_maxima_time = transform_summary_df["maxima_time"];
    NumericVector summary_maxima_gl = transform_summary_df["maxima_gl"];

    // Group original data by ID
    int n_original = original_df.nrows();
    group_by_id(original_id, n_original);

    // Create a map for transform summary data by ID
    std::map<std::string, std::vector<int>> summary_id_indices;
    int n_summary = transform_summary_df.nrows();
    for (int i = 0; i < n_summary; ++i) {
      std::string current_id = as<std::string>(summary_id[i]);
      summary_id_indices[current_id].push_back(i);
    }

    // Calculate for each ID separately
    for (auto const& id_pair : id_indices) {
      std::string current_id = id_pair.first;
      const std::vector<int>& original_indices = id_pair.second;

      // Extract original data subset for this ID
      NumericVector original_time_subset(original_indices.size());
      NumericVector original_gl_subset(original_indices.size());
      extract_id_subset(current_id, original_indices, original_time, original_gl,
                        original_time_subset, original_gl_subset);

      // Extract transform summary data for this ID
      if (summary_id_indices.count(current_id) > 0) {
        const std::vector<int>& summary_indices = summary_id_indices[current_id];

        NumericVector grid_time_subset(summary_indices.size());
        NumericVector grid_gl_subset(summary_indices.size());
        NumericVector maxima_time_subset(summary_indices.size());
        NumericVector maxima_gl_subset(summary_indices.size());

        for (size_t i = 0; i < summary_indices.size(); ++i) {
          grid_time_subset[i] = summary_grid_time[summary_indices[i]];
          grid_gl_subset[i] = summary_grid_gl[summary_indices[i]];
          maxima_time_subset[i] = summary_maxima_time[summary_indices[i]];
          maxima_gl_subset[i] = summary_maxima_gl[summary_indices[i]];
        }

        // Sort by GRID_time to ensure proper order
        std::vector<std::pair<double, int>> time_index_pairs;
        for (int i = 0; i < grid_time_subset.size(); ++i) {
          if (!NumericVector::is_na(grid_time_subset[i])) {
            time_index_pairs.push_back(std::make_pair(grid_time_subset[i], i));
          }
        }
        std::sort(time_index_pairs.begin(), time_index_pairs.end());

        // Create sorted subsets
        NumericVector sorted_grid_time(time_index_pairs.size());
        NumericVector sorted_grid_gl(time_index_pairs.size());
        NumericVector sorted_maxima_time(time_index_pairs.size());
        NumericVector sorted_maxima_gl(time_index_pairs.size());

        for (size_t i = 0; i < time_index_pairs.size(); ++i) {
          int original_idx = time_index_pairs[i].second;
          sorted_grid_time[i] = grid_time_subset[original_idx];
          sorted_grid_gl[i] = grid_gl_subset[original_idx];
          sorted_maxima_time[i] = maxima_time_subset[original_idx];
          sorted_maxima_gl[i] = maxima_gl_subset[original_idx];
        }

        // Detect between maxima for this ID
        detect_between_maxima_for_id(current_id, original_time_subset, original_gl_subset,
                                     sorted_grid_time, sorted_grid_gl,
                                     sorted_maxima_time, sorted_maxima_gl);
      }
    }

    // Create output structures
    DataFrame result_df = create_result_df();
    DataFrame episode_counts_df = create_row_counts_df(); // Use row counts instead of episode transitions

    // Return results as a List
    return List::create(
      _["results"] = result_df,
      _["episode_counts"] = episode_counts_df
    );
  }
};

// [[Rcpp::export]]
List detect_between_maxima(DataFrame new_df,
                         DataFrame transform_summary_df) {
  BetweenMaximaCalculator calculator;
  return calculator.calculate(new_df, transform_summary_df);
}
