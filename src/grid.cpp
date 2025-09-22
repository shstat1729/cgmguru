#include "id_based_calculator.h"

using namespace Rcpp;
using namespace std;

// GRID-specific calculator class
class GridCalculator : public IdBasedCalculator {
private:
  // Store episode start information for total DataFrame
  std::vector<std::string> total_episode_ids;
  std::vector<double> total_episode_times;
  std::vector<double> total_episode_gls;
  std::vector<int> total_episode_indices;

  // Calculate GRID for a single ID
  IntegerVector calculate_grid_for_id(const NumericVector& time_subset,
                                     const NumericVector& gl_subset,
                                     double gap,
                                     double threshold) {
    int n_subset = time_subset.length();
    IntegerVector grid_subset(n_subset, 0);

    if (n_subset < 4) return grid_subset; // Need at least 4 points

    double rate1, rate2, rate3;

    for (int j = 3; j < n_subset; ++j) {
      // Check for NA values
      if (NumericVector::is_na(gl_subset[j]) ||
          NumericVector::is_na(gl_subset[j-1]) ||
          NumericVector::is_na(gl_subset[j-2]) ||
          NumericVector::is_na(gl_subset[j-3])) {
        continue;
      }

      // Calculate rates (mg/dL per hour)
      rate1 = (gl_subset[j] - gl_subset[j-1]) / ((time_subset[j] - time_subset[j-1]) / 3600.0);
      rate2 = (gl_subset[j-1] - gl_subset[j-2]) / ((time_subset[j-1] - time_subset[j-2]) / 3600.0);
      rate3 = (gl_subset[j-2] - gl_subset[j-3]) / ((time_subset[j-2] - time_subset[j-3]) / 3600.0);

      // Apply GRID criteria
      if (rate1 >= 95 && rate2 >= 95 && threshold <= gl_subset[j-2]) {
        // Mark points within gap window
        for (int k = j; k < n_subset && (time_subset[k] - time_subset[j]) <= gap*60; ++k) {
          if (k >= 2) grid_subset[k-2] = 1;
        }
      } else if ((rate2 >= 90 && rate3 >= 90 && threshold <= gl_subset[j-3]) ||
                 (rate3 >= 90 && rate1 >= 90 && threshold <= gl_subset[j-3])) {
        // Mark points within gap window
        for (int k = j; k < n_subset && (time_subset[k] - time_subset[j]) <= gap*60; ++k) {
          if (k >= 3) grid_subset[k-3] = 1;
        }
      }
    }

    return grid_subset;
  }

  // Enhanced episode processing that also stores data for total DataFrame
  void process_episodes_with_total(const std::string& current_id,
                                 const IntegerVector& grid_subset,
                                 const NumericVector& time_subset,
                                 const NumericVector& gl_subset) {
    // First do the standard episode processing
    process_episodes(current_id, grid_subset, time_subset, gl_subset);

    // Then collect data for total DataFrame
    const std::vector<int>& indices = id_indices[current_id];
    for (int i = 0; i < grid_subset.length(); ++i) {
      bool is_episode_start = (grid_subset[i] == 1) &&
                             (i == 0 || grid_subset[i-1] == 0);

      if (is_episode_start) {
        total_episode_ids.push_back(current_id);
        total_episode_times.push_back(time_subset[i]);
        total_episode_gls.push_back(gl_subset[i]);
        total_episode_indices.push_back(indices[i]); // Original row index
      }
    }
  }

  // Create the total episode DataFrame
  DataFrame create_episode_start_total_df() {
    if (total_episode_ids.empty()) {
      DataFrame empty_df = DataFrame::create();
      empty_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
      return empty_df;
    }

    // Create POSIXct time vector
    NumericVector time_vec = wrap(total_episode_times);
    time_vec.attr("class") = CharacterVector::create("POSIXct");
    time_vec.attr("tzone") = "UTC";

    DataFrame df = DataFrame::create(
      _["id"] = wrap(total_episode_ids),
      _["time"] = time_vec,
      _["gl"] = wrap(total_episode_gls),
      _["indices"] = wrap(total_episode_indices)
    );
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    return df;
  }

public:
  List calculate(const DataFrame& df, double gap, double threshold) {
    // Clear total episode storage
    total_episode_ids.clear();
    total_episode_times.clear();
    total_episode_gls.clear();
    total_episode_indices.clear();

    // --- Step 1: Extract columns from DataFrame ---
    int n = df.nrows();
    StringVector id = df["id"];
    NumericVector time = df["time"];
    NumericVector gl = df["gl"];
    // Optional timezone column
    bool has_tz_col = df.containsElementNamed("tz");
    CharacterVector tz_col;
    if (has_tz_col) {
      tz_col = df["tz"];
    }

    // Default timezone from time's tzone attribute or UTC
    std::string default_tz = "UTC";
    RObject tz_attr = time.attr("tzone");
    if (!tz_attr.isNULL()) {
      CharacterVector tz_attr_cv = as<CharacterVector>(tz_attr);
      if (tz_attr_cv.size() > 0 && !CharacterVector::is_na(tz_attr_cv[0])) {
        default_tz = as<std::string>(tz_attr_cv[0]);
      }
    }

    // --- Step 2: Separate calculation by ID ---
    group_by_id(id, n);
    std::map<std::string, IntegerVector> id_grid_results;
    // Build per-id timezone map
    std::map<std::string, std::string> id_timezones;

    // Calculate GRID for each ID separately
    for (auto const& id_pair : id_indices) {
      std::string current_id = id_pair.first;
      const std::vector<int>& indices = id_pair.second;

      // Extract subset data for this ID
      NumericVector time_subset(indices.size());
      NumericVector gl_subset(indices.size());
      extract_id_subset(current_id, indices, time, gl, time_subset, gl_subset);

      // Assign timezone for this id (first row's tz if available; else default)
      std::string tz_for_id = default_tz;
      if (has_tz_col && !indices.empty()) {
        int idx0 = indices.front();
        if (idx0 >= 0 && idx0 < tz_col.size() && !CharacterVector::is_na(tz_col[idx0])) {
          tz_for_id = as<std::string>(tz_col[idx0]);
        }
      }
      if (tz_for_id.empty()) tz_for_id = default_tz;
      id_timezones[current_id] = tz_for_id;

      // Calculate GRID for this ID
      IntegerVector grid_subset = calculate_grid_for_id(time_subset, gl_subset, gap, threshold);

      // Store result
      id_grid_results[current_id] = grid_subset;

      // Process episodes for this ID (both standard and total)
      process_episodes_with_total(current_id, grid_subset, time_subset, gl_subset);
    }

    // --- Step 3: Merge results back to original order ---
    IntegerVector grid_final = merge_results(id_grid_results, n);

    // --- Step 4: Create output structures ---
    DataFrame counts_df = create_episode_counts_df();
    DataFrame episode_tibble = create_episode_tibble();  // New comprehensive tibble
    DataFrame episode_start_total_df = create_episode_start_total_df();

    // Set POSIXct tzone of episode_start_total_df time column to default
    if (episode_start_total_df.containsElementNamed("time")) {
      RObject time_obj = episode_start_total_df["time"];
      time_obj.attr("tzone") = default_tz;
    }

    // Attach per-id timezone mapping to outputs
    if (!id_timezones.empty()) {
      std::vector<std::string> id_list;
      id_list.reserve(id_indices.size());
      for (auto const& id_pair : id_indices) {
        id_list.push_back(id_pair.first);
      }
      CharacterVector tz_map(id_list.size());
      CharacterVector name_vec(id_list.size());
      for (size_t i = 0; i < id_list.size(); ++i) {
        name_vec[i] = id_list[i];
        tz_map[i] = id_timezones[id_list[i]];
      }
      tz_map.attr("names") = name_vec;
      episode_start_total_df.attr("tzone_by_id") = tz_map;
      episode_tibble.attr("tzone_by_id") = tz_map;
      counts_df.attr("tzone_by_id") = tz_map;
    }

    // --- Step 5: Return final results ---
    // Convert GRID_vector to single-column tibble
    DataFrame grid_tibble = DataFrame::create(_["grid"] = grid_final);
    grid_tibble.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    return List::create(
      _["grid_vector"] = grid_tibble,
      _["episode_counts"] = counts_df,
      _["episode_start_total"] = episode_start_total_df,
      _["episode_start"] = episode_tibble     // New comprehensive tibble

    );
  }
};

// [[Rcpp::export]]
List grid(DataFrame df, double gap = 15, double threshold = 130) {
  GridCalculator calculator;
  return calculator.calculate(df, gap, threshold);
}