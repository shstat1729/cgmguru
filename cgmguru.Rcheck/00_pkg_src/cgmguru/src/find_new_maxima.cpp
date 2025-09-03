#include "id_based_calculator.h"

using namespace Rcpp;
using namespace std;

// NewMaxima-specific calculator class
class NewMaximaCalculator : public IdBasedCalculator {
private:
  // Find new maxima for a single ID
  IntegerVector calculate_new_maxima_for_id(const NumericVector& time_subset,
                                           const NumericVector& gl_subset,
                                           const IntegerVector& mod_grid_max_point_subset,
                                           const IntegerVector& local_maxima_subset) {
    int n_subset = time_subset.length();
    IntegerVector maxima_point(n_subset, 0); // Initialize all to 0

    for(int i = 0; i < mod_grid_max_point_subset.size(); i++) {
      int mod_index = mod_grid_max_point_subset[i] - 1; // Adjust for 0-based indexing

      // Check if modIndex is valid for this subset
      if (mod_index < 0 || mod_index >= n_subset) continue;

      double mod_time = time_subset[mod_index];
      double window_start = mod_time;
      double window_end = mod_time + 2 * 3600; // 2 hours in seconds

      // Find indices within 2 hours after the mod_GRID_maxpoint
      std::vector<int> new_maxima_points;
      new_maxima_points.reserve(local_maxima_subset.size()); // Reserve space for efficiency

      for(int j = 0; j < local_maxima_subset.size(); j++) {
        int local_max_index = local_maxima_subset[j] - 1; // Adjust for 0-based indexing

        // Check if localMaxIndex is valid for this subset
        if (local_max_index < 0 || local_max_index >= n_subset) continue;

        if (time_subset[local_max_index] >= window_start && time_subset[local_max_index] <= window_end) {
          new_maxima_points.push_back(local_max_index);
        }
      }

      if(new_maxima_points.empty()) {
        maxima_point[mod_index] = 1;
      } else {
        NumericVector values_to_compare;

        for(int k : new_maxima_points) {
          values_to_compare.push_back(gl_subset[k]);
        }
        values_to_compare.push_back(gl_subset[mod_index]);

        // Find the index of the maximum value
        int max_index = 0;
        double max_value = values_to_compare[0];
        for(int idx = 1; idx < values_to_compare.size(); idx++) {
          if(values_to_compare[idx] > max_value) {
            max_value = values_to_compare[idx];
            max_index = idx;
          }
        }

        if(max_index == (values_to_compare.size() - 1)) {
          maxima_point[mod_index] = 1;
        } else {
          maxima_point[new_maxima_points[max_index]] = 1;
        }
      }
    }

    return maxima_point;
  }

  // Convert global indices to subset indices
  IntegerVector convert_to_subset_indices(const IntegerVector& global_indices,
                                         const std::vector<int>& subset_mapping) {
    std::vector<int> subset_indices;
    subset_indices.reserve(global_indices.size()); // Reserve space for efficiency

    for(int i = 0; i < global_indices.size(); i++) {
      int global_idx = global_indices[i] - 1; // Convert to 0-based

      // Find this global index in our subset mapping
      for(size_t j = 0; j < subset_mapping.size(); j++) {
        if(subset_mapping[j] == global_idx) {
          subset_indices.push_back(j + 1); // Convert back to 1-based for subset
          break;
        }
      }
    }

    return wrap(subset_indices);
  }

public:
  DataFrame calculate(const DataFrame& df,
                     const IntegerVector& mod_grid_max_point,
                     const IntegerVector& local_maxima) {
    // --- Step 0: Input validation ---
    if (df.nrows() == 0) {
      // Return empty DataFrame with correct structure
      NumericVector empty_time = NumericVector::create();
      empty_time.attr("class") = CharacterVector::create("POSIXct");
      empty_time.attr("tzone") = "UTC";

      DataFrame empty_df = DataFrame::create(
        _["id"] = CharacterVector::create(),
        _["time"] = empty_time,
        _["gl"] = NumericVector::create(),
        _["indices"] = IntegerVector::create()
      );
      empty_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
      return empty_df;
    }

    // --- Step 1: Extract columns from DataFrame ---
    int n = df.nrows();
    StringVector id = df["id"];
    NumericVector time = df["time"];
    NumericVector gl = df["gl"];

    // --- Step 2: Initialize and separate calculation by ID ---
    group_by_id(id, n); // This clears id_indices and rebuilds it
    std::map<std::string, IntegerVector> id_maxima_results;
    id_maxima_results.clear(); // Explicit clear for safety

    // Calculate new maxima for each ID separately
    for (auto const& id_pair : id_indices) {
      std::string current_id = id_pair.first;
      const std::vector<int>& indices = id_pair.second;

      // Skip empty ID groups
      if (indices.empty()) continue;

      // Extract subset data for this ID
      NumericVector time_subset(indices.size());
      NumericVector gl_subset(indices.size());
      extract_id_subset(current_id, indices, time, gl, time_subset, gl_subset);

      // Convert global indices to subset indices for this ID
      IntegerVector mod_grid_max_point_subset = convert_to_subset_indices(mod_grid_max_point, indices);
      IntegerVector local_maxima_subset = convert_to_subset_indices(local_maxima, indices);

      // Calculate new maxima for this ID
      IntegerVector maxima_subset = calculate_new_maxima_for_id(time_subset, gl_subset,
                                                               mod_grid_max_point_subset, local_maxima_subset);

      // Store result
      id_maxima_results[current_id] = maxima_subset;
    }

    // --- Step 3: Merge results back to original order ---
    IntegerVector maxima_final = merge_results(id_maxima_results, n);

    // --- Step 4: Create DataFrame with selected maxima points ---
    std::vector<std::string> result_ids;
    std::vector<double> result_times;
    std::vector<double> result_gls;
    std::vector<int> result_indices;

    // Reserve space based on estimated output size
    int estimated_size = std::min(n, std::max(static_cast<int>(mod_grid_max_point.size()),
                                              static_cast<int>(local_maxima.size())));
    result_ids.reserve(estimated_size);
    result_times.reserve(estimated_size);
    result_gls.reserve(estimated_size);
    result_indices.reserve(estimated_size);

    for (int i = 0; i < n; ++i) {
      if (maxima_final[i] == 1) {
        result_ids.push_back(as<std::string>(id[i]));
        result_times.push_back(time[i]);
        result_gls.push_back(gl[i]);
        result_indices.push_back(i + 1); // R-style 1-based indexing
      }
    }

    DataFrame result_df;
    if (result_ids.size() > 0) {
      // Create POSIXct time vector
      NumericVector time_vec = wrap(result_times);
      time_vec.attr("class") = CharacterVector::create("POSIXct");
      time_vec.attr("tzone") = "UTC";

      result_df = DataFrame::create(
        _["id"] = result_ids,
        _["time"] = time_vec,
        _["gl"] = result_gls,
        _["indices"] = result_indices
      );
      result_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    } else {
      // Create empty DataFrame with correct structure
      NumericVector empty_time = NumericVector::create();
      empty_time.attr("class") = CharacterVector::create("POSIXct");
      empty_time.attr("tzone") = "UTC";

      result_df = DataFrame::create(
        _["id"] = CharacterVector::create(),
        _["time"] = empty_time,
        _["gl"] = NumericVector::create(),
        _["indices"] = IntegerVector::create()
      );
      result_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    }

    return result_df;
  }
};

// [[Rcpp::export]]
DataFrame find_new_maxima(DataFrame new_df, IntegerVector mod_grid_max_point, IntegerVector local_maxima) {
  NewMaximaCalculator calculator;
  return calculator.calculate(new_df, mod_grid_max_point, local_maxima);
}
