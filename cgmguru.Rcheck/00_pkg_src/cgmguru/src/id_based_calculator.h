#ifndef ID_BASED_CALCULATOR_H
#define ID_BASED_CALCULATOR_H

#include <Rcpp.h>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include "utils.h"

using namespace Rcpp;
using namespace std;

// Base class for ID-based calculations
class IdBasedCalculator {
protected:
  std::map<std::string, std::vector<int>> id_indices;
  std::map<std::string, int> episode_counts;
  std::map<std::string, std::vector<double>> episode_time_formatted;
  std::map<std::string, std::vector<double>> episode_gl_values;

  // Group indices by ID
  void group_by_id(const StringVector& id, int n);

  // Extract subset data for a specific ID
  void extract_id_subset(const std::string& current_id,
                         const std::vector<int>& indices,
                         const NumericVector& time,
                         const NumericVector& gl,
                         NumericVector& time_subset,
                         NumericVector& gl_subset);

  // Count episodes and find start times for a specific ID
  void process_episodes(const std::string& current_id,
                       const IntegerVector& result_subset,
                       const NumericVector& time_subset,
                       const NumericVector& gl_subset);

  // Merge results back to original order
  template<typename T>
  T merge_results(const std::map<std::string, T>& id_results, int n);

  // Create episode counts DataFrame
  DataFrame create_episode_counts_df();

  // Create episode times list
  List create_episode_list();

  // Create comprehensive episode tibble (alternative to list)
  DataFrame create_episode_tibble();

public:
  virtual ~IdBasedCalculator() = default;
};

// Template function implementation (must be in header)
template<typename T>
T IdBasedCalculator::merge_results(const std::map<std::string, T>& id_results, int n) {
  T final_result(n);
  std::fill(final_result.begin(), final_result.end(), 0.0);
  for (auto const& id_pair : id_indices) {
    std::string current_id = id_pair.first;
    const std::vector<int>& indices = id_pair.second;
    const T& result_subset = id_results.at(current_id);

    for (size_t i = 0; i < indices.size(); ++i) {
      final_result[indices[i]] = result_subset[i];
    }
  }
  return final_result;
}

#endif // ID_BASED_CALCULATOR_H
