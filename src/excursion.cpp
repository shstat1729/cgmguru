#include "id_based_calculator.h"

using namespace Rcpp;
using namespace std;

// Excursion-specific calculator class
class ExcursionCalculator : public IdBasedCalculator {
private:
  // Store episode start information for total DataFrame
  std::vector<std::string> total_episode_ids;
  std::vector<double> total_episode_times;
  std::vector<double> total_episode_gls;
  std::vector<int> total_episode_indices;

  // Calculate Excursion for a single ID
  IntegerVector calculate_excursion_for_id(const NumericVector& time_subset,
                                     const NumericVector& gl_subset,
                                     double gap) {
    int n_subset = time_subset.length();
    IntegerVector excursion_subset(n_subset, 0);

    if (n_subset < 4) return excursion_subset; // Need at least 4 points

    IntegerVector excursion(n_subset, 0);
    bool condition_met = false;
    for (int j = 0; j < n_subset; ++j) {
        if (j < 3 || NumericVector::is_na(gl_subset[j])) {
          excursion[j] = 0;
        } else {
          if (excursion[j] != 1) {
          if (j == 0) {
            excursion[j] = 0;
          } else if (NumericVector::is_na(gl_subset[j - 1]) ||
                     NumericVector::is_na(gl_subset[j])) { // Corrected check for NA
            excursion[j] = 0;
          } else if (gl_subset[j - 1] < 70) {
            excursion[j] = 0;
          } else {
            condition_met = false;
            for (int k = j + 1; k < n_subset && (time_subset[k] - time_subset[j]) <= 7200; ++k) {
              if (gl_subset[k] > gl_subset[j] + 70) {
                condition_met = true;
              }
              if (condition_met) {
                for (int l = j; l < n_subset && (time_subset[l] - time_subset[j]) <= gap*60; ++l) {
                  excursion[l] = 1;
                }
              }
            }
          }
        }
      }
    }

    return excursion;
  }

  // Enhanced episode processing that also stores data for total DataFrame
  void process_episodes_with_total(const std::string& current_id,
                                 const IntegerVector& result_subset,
                                 const NumericVector& time_subset,
                                 const NumericVector& gl_subset) {
    // First do the standard episode processing
    process_episodes(current_id, result_subset, time_subset, gl_subset);

    // Then collect data for total DataFrame
    const std::vector<int>& indices = id_indices[current_id];
    for (int i = 0; i < result_subset.length(); ++i) {
      bool is_episode_start = (result_subset[i] == 1) &&
                             (i == 0 || result_subset[i-1] == 0);

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
  List calculate(const DataFrame& df, double gap) {
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
    std::map<std::string, IntegerVector> id_excursion_results;

    // Build per-id timezone map
    std::map<std::string, std::string> id_timezones;

    // Calculate excursion for each ID separately
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

      // Calculate excursion for this ID
      IntegerVector excursion_subset = calculate_excursion_for_id(time_subset, gl_subset, gap);

      // Store result
      id_excursion_results[current_id] = excursion_subset;

      // Process episodes for this ID (both standard and total)
      process_episodes_with_total(current_id, excursion_subset, time_subset, gl_subset);
    }

    // --- Step 3: Merge results back to original order ---
    IntegerVector excursion_final = merge_results(id_excursion_results, n);

    // --- Step 4: Create output structures ---
    DataFrame counts_df = create_episode_counts_df();
    DataFrame episode_tibble = create_episode_tibble();
    DataFrame episode_start_total_df = create_episode_start_total_df();

    // Set POSIXct tzone of episode_start_total_df time column to default
    if (episode_start_total_df.containsElementNamed("time")) {
      RObject time_obj = episode_start_total_df["time"];
      time_obj.attr("tzone") = default_tz;
    }

    // Attach per-id timezone mapping to outputs
    if (!id_timezones.empty()) {
      // stable id order by group_by id_indices
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
    // Convert excursion_vector to single-column tibble
    DataFrame excursion_tibble = DataFrame::create(_["excursion"] = excursion_final);
    excursion_tibble.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    return List::create(
      _["excursion_vector"] = excursion_tibble,
      _["episode_counts"] = counts_df,
      _["episode_start_total"] = episode_start_total_df,
      _["episode_start"] = episode_tibble

    );
  }
};

// [[Rcpp::export]]
List excursion(DataFrame df, double gap = 15) {
  ExcursionCalculator calculator;
  return calculator.calculate(df, gap);
}