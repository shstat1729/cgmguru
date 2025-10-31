#include "id_based_calculator.h"

using namespace Rcpp;
using namespace std;

// FindMaxBeforeHours-specific calculator class
class FindMaxBeforeHoursCalculator : public IdBasedCalculator {
private:
  // Store episode start information for total DataFrame
  std::vector<std::string> total_episode_ids;
  std::vector<double> total_episode_times;
  std::vector<double> total_episode_gls;
  std::vector<int> total_episode_indices;

  // Calculate max before hours for a single ID
  IntegerVector calculate_max_before_hours_for_id(const NumericVector& time_subset,
                                                 const NumericVector& gl_subset,
                                                 const IntegerVector& start_points_subset,
                                                 double hours) {
    int n_subset = time_subset.length();
    std::vector<int> max_indices; // Store R indices (1-based) instead of binary vector
    int n_starts = start_points_subset.size();
    int start_index, end_index, gl_max_point, j, prev_start_index;
    double max_value, window_first_time, prev_point_time;

    for (int i = 0; i < n_starts; ++i) {
      start_index = start_points_subset[i] - 1; // Convert to 0-based indexing

      // Bounds checking for start_index
      if (start_index < 0 || start_index >= n_subset) {
        continue;
      }

      end_index = 0;
      max_value = R_NegInf;
      gl_max_point = start_index;
      window_first_time = time_subset[start_index] - (hours * 60 * 60); // Subtracting hours in seconds

      if (i == 0) {
        // First start point - search backward from start_index
        j = start_index;
        while (j >= 0 && time_subset[j] >= window_first_time) {
          j--;
        }
        end_index = j + 1;
      } else {
        // Not the first start point
        prev_start_index = start_points_subset[i - 1] - 1;
        if (prev_start_index >= 0 && prev_start_index < n_subset) {
          prev_point_time = time_subset[prev_start_index];
          if ((time_subset[start_index] - prev_point_time) < (hours * 60 * 60)) {
            end_index = prev_start_index;
          } else {
            j = start_index;
            while (j >= 0 && time_subset[j] >= window_first_time) {
              j--;
            }
            end_index = j + 1;
          }
        } else {
          j = start_index;
          while (j >= 0 && time_subset[j] >= window_first_time) {
            j--;
          }
          end_index = j + 1;
        }
      }

      // Find maximum in the range (searching backward from start_index to end_index)
      for (int j = start_index; j >= end_index && j >= 0; --j) {
        if (!NumericVector::is_na(gl_subset[j]) && gl_subset[j] > max_value) {
          max_value = gl_subset[j];
          gl_max_point = j;
        }
      }

      // Store the maximum point index (convert to 1-based R index)
      if (gl_max_point >= 0 && gl_max_point < n_subset) {
        max_indices.push_back(gl_max_point + 1); // Convert to 1-based R index
      }
    }

    return wrap(max_indices);
  }

  // Enhanced episode processing that also stores data for total DataFrame
  void process_episodes_with_total(const std::string& current_id,
                                 const IntegerVector& binary_result,
                                 const NumericVector& time_subset,
                                 const NumericVector& gl_subset) {
    // First do the standard episode processing
    process_episodes(current_id, binary_result, time_subset, gl_subset);

    // Then collect data for total DataFrame
    const std::vector<int>& indices = id_indices[current_id];
    for (int i = 0; i < binary_result.length(); ++i) {
      bool is_episode_start = (binary_result[i] == 1) &&
                             (i == 0 || binary_result[i-1] == 0);

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
  List calculate(const DataFrame& df, const IntegerVector& start_point, double hours) {
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

    // --- Step 2: Group start points by ID ---
    std::map<std::string, std::vector<int>> id_start_points;

    // Group start points by ID
    for (int i = 0; i < start_point.size(); ++i) {
      int idx = start_point[i] - 1; // Convert to 0-based indexing
      if (idx >= 0 && idx < n) {
        std::string current_id = as<std::string>(id[idx]);
        id_start_points[current_id].push_back(start_point[i]); // Keep 1-based for consistency
      }
    }

    // --- Step 3: Separate calculation by ID ---
    group_by_id(id, n);
    std::map<std::string, IntegerVector> id_max_results;
    std::vector<int> start_points_for_id;
    int original_idx, subset_idx;
    // Build per-id timezone map
    std::map<std::string, std::string> id_timezones;
    // Calculate max before hours for each ID separately
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

      // Get start points for this ID and convert to subset indices
      start_points_for_id.clear();
      if (id_start_points.find(current_id) != id_start_points.end()) {
        for (int sp : id_start_points[current_id]) {
          original_idx = sp - 1; // Convert to 0-based
          // Find corresponding index in subset
          auto it = std::find(indices.begin(), indices.end(), original_idx);
          if (it != indices.end()) {
            subset_idx = std::distance(indices.begin(), it) + 1; // Convert to 1-based for subset
            start_points_for_id.push_back(subset_idx);
          }
        }
      }

      IntegerVector start_points_subset = wrap(start_points_for_id);

      // Calculate max before hours for this ID (returns subset indices)
      IntegerVector max_result_subset = calculate_max_before_hours_for_id(
        time_subset, gl_subset, start_points_subset, hours);

      // Convert subset indices back to original DataFrame indices
      IntegerVector max_result_original(max_result_subset.size());
      for (int i = 0; i < max_result_subset.size(); ++i) {
        int subset_idx = max_result_subset[i] - 1; // Convert to 0-based
        if (subset_idx >= 0 && subset_idx < indices.size()) {
          max_result_original[i] = indices[subset_idx] + 1; // Convert to 1-based R index
        }
      }

      // Store result
      id_max_results[current_id] = max_result_original;

      // Create binary vector for episode processing (needed for process_episodes function)
      IntegerVector binary_result(indices.size(), 0);
      for (int i = 0; i < max_result_subset.size(); ++i) {
        int subset_idx = max_result_subset[i] - 1; // Convert to 0-based
        if (subset_idx >= 0 && subset_idx < binary_result.size()) {
          binary_result[subset_idx] = 1;
        }
      }

      // Process episodes for this ID (both standard and total)
      process_episodes_with_total(current_id, binary_result, time_subset, gl_subset);
    }

    // --- Step 4: Combine all results ---
    std::vector<int> all_max_indices;
    for (auto const& id_pair : id_max_results) {
      const IntegerVector& indices = id_pair.second;
      for (int i = 0; i < indices.size(); ++i) {
        all_max_indices.push_back(indices[i]);
      }
    }

    IntegerVector max_final = wrap(all_max_indices);

    // --- Step 5: Create output structures ---
    DataFrame counts_df = create_episode_counts_df();
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
      counts_df.attr("tzone_by_id") = tz_map;
    }

    // --- Step 6: Return final results ---
    // Convert max_indices to single-column tibble
    DataFrame max_indices_tibble = DataFrame::create(_["max_indices"] = max_final);
    max_indices_tibble.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    return List::create(
      _["max_indices"] = max_indices_tibble,
      _["episode_counts"] = counts_df,
      _["episode_start"] = episode_start_total_df
    );
  }
};

// [[Rcpp::export]]
List find_max_before_hours(DataFrame df, DataFrame start_point_df, double hours) {
  IntegerVector start_point = as<IntegerVector>(start_point_df[0]);
  FindMaxBeforeHoursCalculator calculator;
  return calculator.calculate(df, start_point, hours);
}
