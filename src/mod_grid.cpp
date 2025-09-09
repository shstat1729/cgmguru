#include <Rcpp.h>
#include "id_based_calculator.h"
using namespace Rcpp;
using namespace std;

// ModGrid-specific calculator class
class ModGridCalculator : public IdBasedCalculator {
  private:
    // Store episode start information for total DataFrame
    std::vector<std::string> total_episode_ids;
    std::vector<double> total_episode_times;
    std::vector<double> total_episode_gls;
    std::vector<int> total_episode_indices;

    // Calculate mod_grid for a single ID
    IntegerVector calculate_mod_grid_for_id(const NumericVector& time_subset,
                                         const NumericVector& gl_subset,
                                         const std::vector<int>& original_indices,
                                         IntegerVector grid_point,
                                         double hours,
                                         double gap) {
      int n_subset = time_subset.length();
      IntegerVector mod_grid_subset(n_subset, 0);

      if (n_subset == 0) return mod_grid_subset;

      // Find which GRIDpoint indices belong to this ID subset
      std::vector<int> relevant_grid_points;
      for (int i = 0; i < grid_point.size(); ++i) {
        int grid_point_index = grid_point[i] - 1; // Convert to 0-based

        // Check if this gridpoint belongs to current ID subset
        for (size_t j = 0; j < original_indices.size(); ++j) {
          if (original_indices[j] == grid_point_index) {
            relevant_grid_points.push_back(j); // Store subset index
            break;
          }
        }
      }

      // Process each relevant gridpoint for this ID
      for (int grid_point_subset_idx : relevant_grid_points) {
        if (grid_point_subset_idx >= n_subset) continue;

        int end_index = grid_point_subset_idx;
        double window_start_time = time_subset[end_index] - hours * 60 * 60;

        // Find start_index within the time window
        int start_index = end_index;
        while (start_index >= 0 && time_subset[start_index] >= window_start_time) {
          start_index--;
        }
        start_index++; // Move to first valid index

        // Find minimum within the time window
        double min_value = R_PosInf;
        int mod_grid_min_point = start_index;

        for (int j = start_index; j <= end_index; ++j) {
          if (!NumericVector::is_na(gl_subset[j]) && gl_subset[j] < min_value) {
            min_value = gl_subset[j];
            mod_grid_min_point = j;
          }
        }

        // Mark gap period starting from minimum point
        double gap_end_time = time_subset[mod_grid_min_point] + gap * 60;
        for (int j = mod_grid_min_point; j < n_subset && time_subset[j] <= gap_end_time; ++j) {
          mod_grid_subset[j] = 1;
        }
      }

      return mod_grid_subset;
    }

    // Enhanced episode processing that also stores data for total DataFrame
    void process_episodes_with_total(const std::string& current_id,
                                   const IntegerVector& mod_grid_subset,
                                   const NumericVector& time_subset,
                                   const NumericVector& gl_subset) {
      // First do the standard episode processing
      process_episodes(current_id, mod_grid_subset, time_subset, gl_subset);

      // Then collect data for total DataFrame
      const std::vector<int>& indices = id_indices[current_id];
      for (int i = 0; i < mod_grid_subset.length(); ++i) {
        bool is_episode_start = (mod_grid_subset[i] == 1) &&
                               (i == 0 || mod_grid_subset[i-1] == 0);

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
    List calculate(const DataFrame& df, IntegerVector grid_point, double hours, double gap) {
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

      // --- Step 2: Separate calculation by ID ---
      group_by_id(id, n);
      std::map<std::string, IntegerVector> id_mod_grid_results;

      // Calculate mod_grid for each ID separately
      for (auto const& id_pair : id_indices) {
        std::string current_id = id_pair.first;
        const std::vector<int>& indices = id_pair.second;

        // Extract subset data for this ID
        NumericVector time_subset(indices.size());
        NumericVector gl_subset(indices.size());
        extract_id_subset(current_id, indices, time, gl, time_subset, gl_subset);

        // Calculate mod_grid for this ID
        IntegerVector mod_grid_subset = calculate_mod_grid_for_id(time_subset, gl_subset, indices, grid_point, hours, gap);

        // Store result
        id_mod_grid_results[current_id] = mod_grid_subset;

        // Process episodes for this ID (both standard and total)
        process_episodes_with_total(current_id, mod_grid_subset, time_subset, gl_subset);
      }

      // --- Step 3: Merge results back to original order ---
      IntegerVector mod_grid_final = merge_results(id_mod_grid_results, n);

      // --- Step 4: Create output structures ---
      DataFrame counts_df = create_episode_counts_df();
      DataFrame episode_tibble = create_episode_tibble();  // New comprehensive tibble
      DataFrame episode_start_total_df = create_episode_start_total_df();

      // --- Step 5: Return final results ---
      // Convert mod_grid_vector to single-column tibble
      DataFrame mod_grid_tibble = DataFrame::create(_["mod_grid"] = mod_grid_final);
      mod_grid_tibble.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

      return List::create(
        _["mod_grid_vector"] = mod_grid_tibble,
        _["episode_counts"] = counts_df,
        _["episode_start_total"] = episode_start_total_df,
        _["episode_start"] = episode_tibble

      );
    }
  };

  // [[Rcpp::export]]
  List mod_grid(DataFrame df, DataFrame grid_point_df, double hours = 2, double gap = 15) {
      // Check if DataFrame has at least one column
    if (grid_point_df.length() == 0) {
      stop("DataFrame must have at least one column");
    }
    
    // Convert the first column to IntegerVector
    IntegerVector grid_point = as<IntegerVector>(grid_point_df[0]);
    
    ModGridCalculator calculator;
    return calculator.calculate(df, grid_point, hours, gap);
  }