#include <Rcpp.h>
using namespace Rcpp;

// [[Rcpp::export]]
DataFrame start_finder(DataFrame df) {
  // Check if DataFrame has at least one column
  if (df.length() == 0) {
    stop("DataFrame must have at least one column");
  }
  // Convert the first column to IntegerVector
  IntegerVector start_vector = as<IntegerVector>(df[0]);
  std::vector<int> index_vector;
  index_vector.reserve(start_vector.length());

  for (int i = 0; i < start_vector.length(); ++i) {
    bool is_episode_start = (start_vector[i] == 1) &&
                            (i == 0 || start_vector[i - 1] == 0);
    if (is_episode_start) index_vector.push_back(i + 1);
  }

  IntegerVector start_indices = wrap(index_vector);

  DataFrame start_indices_tibble = DataFrame::create(_["start_indices"] = start_indices);
  start_indices_tibble.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
  return start_indices_tibble;
}