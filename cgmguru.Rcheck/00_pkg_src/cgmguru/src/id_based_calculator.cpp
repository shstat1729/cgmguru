#include "id_based_calculator.h"


// Group indices by ID
void IdBasedCalculator::group_by_id(const StringVector& id, int n) {
  id_indices.clear();
  for (int i = 0; i < n; ++i) {
    std::string current_id = as<std::string>(id[i]);
    id_indices[current_id].push_back(i);
  }
}

// Extract subset data for a specific ID
void IdBasedCalculator::extract_id_subset(const std::string& current_id,
                       const std::vector<int>& indices,
                       const NumericVector& time,
                       const NumericVector& gl,
                       NumericVector& time_subset,
                       NumericVector& gl_subset) {
  for (size_t i = 0; i < indices.size(); ++i) {
    time_subset[i] = time[indices[i]];
    gl_subset[i] = gl[indices[i]];
  }
}

// Count episodes and find start times for a specific ID
void IdBasedCalculator::process_episodes(const std::string& current_id,
                     const IntegerVector& result_subset,
                     const NumericVector& time_subset,
                     const NumericVector& gl_subset) {
  int episode_count = 0;
  std::vector<double> episode_time;
  std::vector<double> episode_gl;
  for (int i = 0; i < result_subset.length(); ++i) {
    bool is_episode_start = (result_subset[i] == 1) &&
                           (i == 0 || result_subset[i-1] == 0);

    if (is_episode_start) {
      episode_count++;
      episode_time.push_back(time_subset[i]);
      episode_gl.push_back(gl_subset[i]);
    }
  }

  episode_counts[current_id] = episode_count;
  episode_time_formatted[current_id] = episode_time;
  episode_gl_values[current_id] = episode_gl;

}

// Create episode counts DataFrame
DataFrame IdBasedCalculator::create_episode_counts_df() {
  std::vector<std::string> ids_for_df;
  std::vector<int> counts_for_df;

  ids_for_df.reserve(episode_counts.size());
  counts_for_df.reserve(episode_counts.size());

  for (auto const& pair : episode_counts) {
    ids_for_df.push_back(pair.first);
    counts_for_df.push_back(pair.second);
  }

  DataFrame counts_df = DataFrame::create(
    _["id"] = ids_for_df,
    _["episode_counts"] = counts_for_df
  );

  // Set class attributes to make it a tibble
  counts_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

  return counts_df;
}

// Create episode times and gl list by id
List IdBasedCalculator::create_episode_list() {
  List episode_list;
  for (auto const& pair : episode_time_formatted) {
    std::string current_id = pair.first;
    const std::vector<double>& episode_time = pair.second;
    const std::vector<double>& episode_gl = episode_gl_values[current_id];

    // Create POSIXct time vector
    NumericVector time_vec = wrap(episode_time);
    time_vec.attr("class") = CharacterVector::create("POSIXct");
  time_vec.attr("tzone") = default_output_tz;

    // Create a DataFrame for this ID with episode times and gl values
    DataFrame result_df = DataFrame::create(
      _["time"] = time_vec,
      _["gl"] = wrap(episode_gl)
    );

    // Set class attributes to make it a tibble
    result_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    // Add this DataFrame to the result list
    episode_list[current_id] = result_df;
  }
  return episode_list;
}

// Create comprehensive episode tibble (alternative to list of DataFrames)
DataFrame IdBasedCalculator::create_episode_tibble() {
  std::vector<std::string> all_ids;
  std::vector<double> all_times;
  std::vector<double> all_gls;

  for (auto const& pair : episode_time_formatted) {
    std::string current_id = pair.first;
    const std::vector<double>& episode_time = pair.second;
    const std::vector<double>& episode_gl = episode_gl_values[current_id];

    // Add data for this ID
    for (size_t i = 0; i < episode_time.size(); ++i) {
      all_ids.push_back(current_id);
      all_times.push_back(episode_time[i]);
      all_gls.push_back(episode_gl[i]);
    }
  }

  if (all_ids.empty()) {
    // Return empty tibble with correct structure
    NumericVector empty_time = NumericVector::create();
    empty_time.attr("class") = CharacterVector::create("POSIXct");
    empty_time.attr("tzone") = default_output_tz;

    DataFrame empty_df = DataFrame::create(
      _["id"] = CharacterVector::create(),
      _["time"] = empty_time,
      _["gl"] = NumericVector::create()
    );
    empty_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    return empty_df;
  }

  // Create POSIXct time vector
  NumericVector time_vec = wrap(all_times);
  time_vec.attr("class") = CharacterVector::create("POSIXct");
  time_vec.attr("tzone") = default_output_tz;

  DataFrame result_df = DataFrame::create(
    _["id"] = wrap(all_ids),
    _["time"] = time_vec,
    _["gl"] = wrap(all_gls)
  );

  // Set class attributes to make it a tibble
  result_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

  return result_df;
}
