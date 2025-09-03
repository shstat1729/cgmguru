#include <Rcpp.h>
using namespace Rcpp;

// [[Rcpp::export]]
DataFrame start_finder(IntegerVector start_vector) {
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