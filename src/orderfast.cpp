#include <Rcpp.h>
#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

using namespace Rcpp;

namespace {

struct SortKey {
  enum class Type {
    integer,
    numeric,
    character
  };

  Type type;
  std::vector<int> int_values;
  std::vector<double> numeric_values;
  std::vector<std::string> character_values;
  std::vector<bool> missing;

  int compare(int lhs, int rhs) const {
    const bool lhs_missing = missing[lhs];
    const bool rhs_missing = missing[rhs];

    if (lhs_missing || rhs_missing) {
      if (lhs_missing == rhs_missing) return 0;
      return lhs_missing ? 1 : -1;
    }

    switch (type) {
      case Type::integer:
        if (int_values[lhs] < int_values[rhs]) return -1;
        if (int_values[lhs] > int_values[rhs]) return 1;
        return 0;
      case Type::numeric:
        if (numeric_values[lhs] < numeric_values[rhs]) return -1;
        if (numeric_values[lhs] > numeric_values[rhs]) return 1;
        return 0;
      case Type::character:
        if (character_values[lhs] < character_values[rhs]) return -1;
        if (character_values[lhs] > character_values[rhs]) return 1;
        return 0;
    }

    return 0;
  }
};

bool is_factor(SEXP x) {
  return Rf_inherits(x, "factor");
}

DataFrame subset_rows(DataFrame df, const IntegerVector& rows) {
  Function subset("[");
  DataFrame out = subset(df, rows, R_MissingArg, _["drop"] = false);
  return out;
}

SortKey integer_key(SEXP x, int n) {
  SortKey key;
  key.type = SortKey::Type::integer;
  key.int_values.resize(n);
  key.missing.resize(n);

  if (TYPEOF(x) == LGLSXP) {
    LogicalVector values(x);
    for (int i = 0; i < n; ++i) {
      key.missing[i] = LogicalVector::is_na(values[i]);
      key.int_values[i] = key.missing[i] ? 0 : values[i];
    }
  } else {
    IntegerVector values(x);
    for (int i = 0; i < n; ++i) {
      key.missing[i] = IntegerVector::is_na(values[i]);
      key.int_values[i] = key.missing[i] ? 0 : values[i];
    }
  }

  return key;
}

SortKey numeric_key(SEXP x, int n) {
  SortKey key;
  key.type = SortKey::Type::numeric;
  key.numeric_values.resize(n);
  key.missing.resize(n);

  Shield<SEXP> shield(Rf_coerceVector(x, REALSXP));
  NumericVector values(shield);
  for (int i = 0; i < n; ++i) {
    key.missing[i] = NumericVector::is_na(values[i]);
    key.numeric_values[i] = key.missing[i] ? 0.0 : values[i];
  }

  return key;
}

SortKey character_key(SEXP x, int n) {
  SortKey key;
  key.type = SortKey::Type::character;
  key.character_values.resize(n);
  key.missing.resize(n);

  Shield<SEXP> shield(Rf_coerceVector(x, STRSXP));
  CharacterVector values(shield);
  for (int i = 0; i < n; ++i) {
    SEXP value = STRING_ELT(values, i);
    key.missing[i] = (value == NA_STRING);
    key.character_values[i] = key.missing[i] ? std::string() : std::string(CHAR(value));
  }

  return key;
}

SortKey make_id_key(SEXP x, int n) {
  if (is_factor(x) || TYPEOF(x) == INTSXP || TYPEOF(x) == LGLSXP) {
    return integer_key(x, n);
  }
  if (TYPEOF(x) == REALSXP) {
    return numeric_key(x, n);
  }
  return character_key(x, n);
}

SortKey make_time_key(SEXP x, int n) {
  if (TYPEOF(x) == REALSXP || TYPEOF(x) == INTSXP || TYPEOF(x) == LGLSXP) {
    return numeric_key(x, n);
  }
  return character_key(x, n);
}

} // namespace

// [[Rcpp::export]]
DataFrame orderfast_cpp(DataFrame df) {
  if (!df.containsElementNamed("id")) {
    stop("orderfast requires an 'id' column");
  }
  if (!df.containsElementNamed("time")) {
    stop("orderfast requires a 'time' column");
  }

  const int n = df.nrows();
  if (n == 0) {
    return df;
  }

  SEXP id = df["id"];
  SEXP time = df["time"];

  // R's radix order path is already highly optimized for factor vectors.
  if (is_factor(id)) {
    Environment base = Environment::base_env();
    Function order_func = base["order"];
    IntegerVector rows = order_func(id, time);
    return subset_rows(df, rows);
  }

  SortKey id_key = make_id_key(id, n);
  SortKey time_key = make_time_key(time, n);

  std::vector<int> order(n);
  std::iota(order.begin(), order.end(), 0);

  std::sort(order.begin(), order.end(), [&](int lhs, int rhs) {
    const int id_cmp = id_key.compare(lhs, rhs);
    if (id_cmp != 0) return id_cmp < 0;

    const int time_cmp = time_key.compare(lhs, rhs);
    if (time_cmp != 0) return time_cmp < 0;

    return lhs < rhs;
  });

  IntegerVector rows(n);
  for (int i = 0; i < n; ++i) {
    rows[i] = order[i] + 1;
  }

  return subset_rows(df, rows);
}
