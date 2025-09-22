#include "id_based_calculator.h"

using namespace Rcpp;
using namespace std;

// Transform Calculator class
class TransformDfCalculator : public IdBasedCalculator {
  private:
    // Store timezone information per ID
    std::map<std::string, std::string> id_timezones;
    // Calculate transform summary for a single ID
  DataFrame calculate_transform_summary_for_id(const std::string& current_id,
                                               const NumericVector& grid_time_subset,
                                               const NumericVector& grid_gl_subset,
                                               const NumericVector& maxima_time_subset,
                                               const NumericVector& maxima_gl_subset) {
    int n = grid_time_subset.size();
    int m = maxima_time_subset.size();

    std::vector<std::string> id_results;
    std::vector<double> grid_time_results;
    std::vector<double> grid_gl_results;
    std::vector<double> maxima_time_results;
    std::vector<double> maxima_gl_results;

    for (int i = 0; i < n; ++i) {
      if (NumericVector::is_na(grid_time_subset[i])) continue; // Skip NA grid points

      double max_gl = -1;
      int max_gl_index = -1;

      for (int j = 0; j < m; ++j) {
        if (NumericVector::is_na(maxima_time_subset[j])) continue; // Skip NA maxima points

        double potential_max_points = maxima_time_subset[j] - grid_time_subset[i];

        if (potential_max_points >= 0 && potential_max_points <= 4 * 3600) {
          if (maxima_gl_subset[j] > max_gl) {
            max_gl = maxima_gl_subset[j];
            max_gl_index = j;
          }
        }
      }

      // If a valid maximum point is found
      if (max_gl_index != -1) {
        id_results.push_back(current_id);
        grid_time_results.push_back(grid_time_subset[i]);
        grid_gl_results.push_back(grid_gl_subset[i]);
        maxima_time_results.push_back(maxima_time_subset[max_gl_index]);
        maxima_gl_results.push_back(max_gl);
      }
    }

    // Create POSIXct time vectors with appropriate timezone
    NumericVector grid_time_vec = wrap(grid_time_results);
    grid_time_vec.attr("class") = CharacterVector::create("POSIXct");
    
    // Get timezone for this ID (default to UTC if not found)
    std::string tz_for_id = "UTC";
    if (id_timezones.count(current_id) > 0) {
      tz_for_id = id_timezones[current_id];
    }
    grid_time_vec.attr("tzone") = tz_for_id;

    NumericVector maxima_time_vec = wrap(maxima_time_results);
    maxima_time_vec.attr("class") = CharacterVector::create("POSIXct");
    maxima_time_vec.attr("tzone") = tz_for_id;

    DataFrame df = DataFrame::create(
      _["id"] = wrap(id_results),
      _["grid_time"] = grid_time_vec,
      _["grid_gl"] = wrap(grid_gl_results),
      _["maxima_time"] = maxima_time_vec,
      _["maxima_gl"] = wrap(maxima_gl_results),
      _["stringsAsFactors"] = false
    );

    // Set class attributes to make it a tibble
    df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    return df;
  }

  public:
    DataFrame calculate(const DataFrame& grid_df, const DataFrame& maxima_df) {
      // Extract columns from GRID DataFrame
      StringVector grid_id = grid_df["id"];
      NumericVector grid_time = grid_df["time"];
      NumericVector grid_gl = grid_df["gl"];

      // Extract columns from maxima DataFrame
      StringVector maxima_id = maxima_df["id"];
      NumericVector maxima_time = maxima_df["time"];
      NumericVector maxima_gl = maxima_df["gl"];

      // --- Extract timezone information ---
      // Get input timezone from grid time column attributes
      std::string input_tz = "UTC"; // default
      if (grid_time.hasAttribute("tzone")) {
        CharacterVector tz_attr = grid_time.attr("tzone");
        if (tz_attr.length() > 0) {
          input_tz = as<std::string>(tz_attr[0]);
        }
      }
      
      // Set output timezone for base calculator - default to UTC
      set_default_output_tz("UTC");

      // Group GRID data by ID
      int n_grid = grid_df.nrows();
      group_by_id(grid_id, n_grid);
      
      // --- Store timezone information per ID ---
      id_timezones.clear();
      for (auto const& id_pair : id_indices) {
        std::string current_id = id_pair.first;
        id_timezones[current_id] = input_tz; // Use input timezone for each ID
      }

      // Create a map for maxima data by ID
      std::map<std::string, std::vector<int>> maxima_id_indices;
      int n_maxima = maxima_df.nrows();
      for (int i = 0; i < n_maxima; ++i) {
        std::string current_id = as<std::string>(maxima_id[i]);
        maxima_id_indices[current_id].push_back(i);
      }

      // Store all results to combine later
      std::vector<DataFrame> all_results;

      // Calculate transform summary for each ID separately
      for (auto const& id_pair : id_indices) {
        std::string current_id = id_pair.first;
        const std::vector<int>& grid_indices = id_pair.second;

        // Extract GRID subset data for this ID
        NumericVector grid_time_subset(grid_indices.size());
        NumericVector grid_gl_subset(grid_indices.size());
        extract_id_subset(current_id, grid_indices, grid_time, grid_gl,
                          grid_time_subset, grid_gl_subset);

        // Extract maxima subset data for this ID (if exists)
        NumericVector maxima_time_subset, maxima_gl_subset;
        if (maxima_id_indices.count(current_id) > 0) {
          const std::vector<int>& maxima_indices = maxima_id_indices[current_id];
          maxima_time_subset = NumericVector(maxima_indices.size());
          maxima_gl_subset = NumericVector(maxima_indices.size());

          for (size_t i = 0; i < maxima_indices.size(); ++i) {
            maxima_time_subset[i] = maxima_time[maxima_indices[i]];
            maxima_gl_subset[i] = maxima_gl[maxima_indices[i]];
          }
        } else {
          // No maxima data for this ID
          maxima_time_subset = NumericVector(0);
          maxima_gl_subset = NumericVector(0);
        }

        // Calculate transform summary for this ID
        DataFrame id_result = calculate_transform_summary_for_id(current_id,
                                                                 grid_time_subset,
                                                                 grid_gl_subset,
                                                                 maxima_time_subset,
                                                                 maxima_gl_subset);

        if (id_result.nrows() > 0) {
          all_results.push_back(id_result);
        }
      }

      // Combine all results into a single DataFrame
      if (all_results.empty()) {
        // Return empty DataFrame with correct structure as tibble
        NumericVector empty_grid_time = NumericVector(0);
        empty_grid_time.attr("class") = CharacterVector::create("POSIXct");
        empty_grid_time.attr("tzone") = default_output_tz;
        
        NumericVector empty_maxima_time = NumericVector(0);
        empty_maxima_time.attr("class") = CharacterVector::create("POSIXct");
        empty_maxima_time.attr("tzone") = default_output_tz;
        
        DataFrame empty_df = DataFrame::create(
          _["id"] = CharacterVector(0),
          _["grid_time"] = empty_grid_time,
          _["grid_gl"] = NumericVector(0),
          _["maxima_time"] = empty_maxima_time,
          _["maxima_gl"] = NumericVector(0),
          _["stringsAsFactors"] = false
        );
        empty_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
        return empty_df;
      }

      // Combine all DataFrames
      int total_rows = 0;
      for (const DataFrame& df : all_results) {
        total_rows += df.nrows();
      }

      std::vector<std::string> combined_id;
      std::vector<double> combined_grid_time;
      std::vector<double> combined_grid_gl;
      std::vector<double> combined_maxima_time;
      std::vector<double> combined_maxima_gl;

      combined_id.reserve(total_rows);
      combined_grid_time.reserve(total_rows);
      combined_grid_gl.reserve(total_rows);
      combined_maxima_time.reserve(total_rows);
      combined_maxima_gl.reserve(total_rows);

      for (const DataFrame& df : all_results) {
        CharacterVector df_id = df["id"];
        NumericVector df_grid_time = df["grid_time"];
        NumericVector df_grid_gl = df["grid_gl"];
        NumericVector df_maxima_time = df["maxima_time"];
        NumericVector df_maxima_gl = df["maxima_gl"];

        for (int i = 0; i < df.nrows(); ++i) {
          combined_id.push_back(as<std::string>(df_id[i]));
          combined_grid_time.push_back(df_grid_time[i]);
          combined_grid_gl.push_back(df_grid_gl[i]);
          combined_maxima_time.push_back(df_maxima_time[i]);
          combined_maxima_gl.push_back(df_maxima_gl[i]);
        }
      }

      // Create final POSIXct time vectors with appropriate timezone
      NumericVector final_grid_time = wrap(combined_grid_time);
      final_grid_time.attr("class") = CharacterVector::create("POSIXct");
      final_grid_time.attr("tzone") = default_output_tz;

      NumericVector final_maxima_time = wrap(combined_maxima_time);
      final_maxima_time.attr("class") = CharacterVector::create("POSIXct");
      final_maxima_time.attr("tzone") = default_output_tz;

      DataFrame final_df = DataFrame::create(
        _["id"] = wrap(combined_id),
        _["grid_time"] = final_grid_time,
        _["grid_gl"] = wrap(combined_grid_gl),
        _["maxima_time"] = final_maxima_time,
        _["maxima_gl"] = wrap(combined_maxima_gl),
        _["stringsAsFactors"] = false
      );

      // Set class attributes to make it a tibble
      final_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

      return final_df;
    }
};

// [[Rcpp::export]]
DataFrame transform_df(DataFrame grid_df, DataFrame maxima_df) {
  TransformDfCalculator calculator;
  return calculator.calculate(grid_df, maxima_df);
}