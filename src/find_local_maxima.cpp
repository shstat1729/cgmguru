#include "id_based_calculator.h"

using namespace Rcpp;
using namespace std;

// LocalMaxima-specific calculator class
class LocalMaximaCalculator : public IdBasedCalculator {
private:
  // Find local maxima for a single ID - returns binary vector
  IntegerVector find_local_maxima_for_id(const NumericVector& gl_subset) {
    int n_subset = gl_subset.length();
    IntegerVector local_maxima_binary(n_subset, 0); // Initialize with 0s

    if (n_subset < 5) return local_maxima_binary; // Need at least 5 points for the algorithm

    // Calculate differences (equivalent to diff() in R)
    NumericVector differ_glucose(n_subset - 1);
    for (int i = 0; i < n_subset - 1; ++i) {
      if (NumericVector::is_na(gl_subset[i]) || NumericVector::is_na(gl_subset[i+1])) {
        differ_glucose[i] = NA_REAL;
      } else {
        differ_glucose[i] = gl_subset[i+1] - gl_subset[i];
      }
    }

    // Find local maxima following your R logic
    // For i in 1:(nrow(grad_df)-2), but we skip i==1,2,3 (in R indexing)
    // In C++ 0-based indexing: skip i==0,1,2 and check from i=3 to n_subset-3
    for (int i = 3; i < n_subset - 2; ++i) {
      // Check if we have valid differences at all required positions
      if (!NumericVector::is_na(differ_glucose[i-2]) &&
          !NumericVector::is_na(differ_glucose[i-1]) &&
          !NumericVector::is_na(differ_glucose[i]) &&
          !NumericVector::is_na(differ_glucose[i+1])) {

        // Apply the condition:
        // differ_glucose[i-2]>=0 & differ_glucose[i-1]>=0 &
        // differ_glucose[i]<=0 & differ_glucose[i+1]<=0
        if (differ_glucose[i-2] >= 0 &&
            differ_glucose[i-1] >= 0 &&
            differ_glucose[i] <= 0 &&
            differ_glucose[i+1] <= 0) {
          local_maxima_binary[i] = 1; // Mark as local maximum
        }
      }
    }

    return local_maxima_binary;
  }

public:
  List calculate(const DataFrame& df) {
    // --- Step 1: Extract columns from DataFrame ---
    int n = df.nrows();
    StringVector id = df["id"];
    NumericVector time = df["time"];
    NumericVector gl = df["gl"];

    // --- Step 2: Separate calculation by ID ---
    group_by_id(id, n);
    std::map<std::string, IntegerVector> id_maxima_results;

    // Find local maxima for each ID separately
    for (auto const& id_pair : id_indices) {
      std::string current_id = id_pair.first;
      const std::vector<int>& indices = id_pair.second;

      // Extract subset data for this ID
      NumericVector time_subset(indices.size());
      NumericVector gl_subset(indices.size());
      extract_id_subset(current_id, indices, time, gl, time_subset, gl_subset);

      // Find local maxima for this ID
      IntegerVector maxima_subset = find_local_maxima_for_id(gl_subset);

      // Store result
      id_maxima_results[current_id] = maxima_subset;
    }

    // --- Step 3: Merge results back to original order ---
    IntegerVector local_maxima_final = merge_results(id_maxima_results, n);

    // --- Step 4: Create results organized by ID (optional, for backward compatibility) ---
    List result_by_id = List::create();
    for (auto const& id_pair : id_indices) {
      std::string current_id = id_pair.first;
      const std::vector<int>& indices = id_pair.second;
      IntegerVector maxima_subset = id_maxima_results[current_id];

      // Convert binary vector to indices for this ID
      std::vector<int> local_maxima_indices;
      for (int i = 0; i < maxima_subset.length(); ++i) {
        if (maxima_subset[i] == 1) {
          local_maxima_indices.push_back(indices[i] + 1); // R-style 1-based indexing
        }
      }

      result_by_id[current_id] = wrap(local_maxima_indices);
    }

    // --- Step 5: Create merged DataFrame for backward compatibility ---
    std::vector<std::string> merged_ids;
    std::vector<double> merged_times;
    std::vector<double> merged_gls;
    std::vector<int> merged_id_indices;

    for (int i = 0; i < n; ++i) {
      if (local_maxima_final[i] == 1) {
        merged_ids.push_back(as<std::string>(id[i]));
        merged_times.push_back(time[i]);
        merged_gls.push_back(gl[i]);
        merged_id_indices.push_back(i + 1); // R-style 1-based indexing
      }
    }

    DataFrame merged_results;
    if (merged_ids.size() > 0) {
      // Create POSIXct time vector
      NumericVector time_vec = wrap(merged_times);
      time_vec.attr("class") = CharacterVector::create("POSIXct");
      time_vec.attr("tzone") = "UTC";

      merged_results = DataFrame::create(
        _["id"] = merged_ids,
        _["time"] = time_vec,
        _["gl"] = merged_gls
      );
      merged_results.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    } else {
      // Create empty DataFrame with correct structure
      merged_results = DataFrame::create(
        _["id"] = CharacterVector::create(),
        _["time"] = NumericVector::create(),
        _["gl"] = NumericVector::create()
      );
      merged_results.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    }

    // --- Step 6: Return final results ---
    // Convert local_maxima_vector to single-column tibble
    DataFrame local_maxima_tibble = DataFrame::create(_["local_maxima"] = merged_id_indices);
    local_maxima_tibble.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    return List::create(
      _["local_maxima_vector"] = local_maxima_tibble,
      _["merged_results"] = merged_results
    );
  }
};

// [[Rcpp::export]]
List find_local_maxima(DataFrame df) {
  LocalMaximaCalculator calculator;
  return calculator.calculate(df);
}