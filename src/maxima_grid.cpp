#include <Rcpp.h>
#include <map>
#include <vector>
#include <algorithm>
#include <string>

using namespace Rcpp;
using namespace std;

// [[Rcpp::export]]
List maxima_grid(DataFrame df, double threshold = 130, double gap = 60, double hours = 2) {
    // Ultra-optimized implementation that combines all algorithm steps in a single function
    // Eliminates function call overhead and optimizes memory allocation

    // --- STEP 0: Pre-allocate and extract data ---
    const int n = df.nrows();
    if (n == 0) {
        DataFrame empty_results = DataFrame::create();
        empty_results.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
        DataFrame empty_counts = DataFrame::create();
        empty_counts.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
        return List::create(_["results"] = empty_results, _["episode_counts"] = empty_counts);
    }

    const StringVector id = df["id"];
    const NumericVector time = df["time"];
    const NumericVector gl = df["gl"];

    // Pre-allocate vectors with reserve for better memory performance
    vector<string> result_ids;
    vector<double> result_grid_times;
    vector<double> result_grid_gls;
    vector<double> result_maxima_times;
    vector<double> result_maxima_gls;
    vector<double> result_time_to_peak;
    vector<int> result_grid_indices;  // Store original GRID indices
    vector<int> result_maxima_indices; // Store maxima indices

    // Estimate output size based on input size and reserve memory
    const int estimated_output = max(10, n / 50); // Conservative estimate
    result_ids.reserve(estimated_output);
    result_grid_times.reserve(estimated_output);
    result_grid_gls.reserve(estimated_output);
    result_maxima_times.reserve(estimated_output);
    result_maxima_gls.reserve(estimated_output);
    result_time_to_peak.reserve(estimated_output);
    result_grid_indices.reserve(estimated_output);
    result_maxima_indices.reserve(estimated_output);

    // Use map to preserve ID order (instead of unordered_map for consistent ordering)
    map<string, vector<int>> id_indices;
    map<string, int> episode_counts;

    // --- STEP 1: Group by ID (optimized) ---
    for (int i = 0; i < n; ++i) {
        const string current_id = as<string>(id[i]);
        id_indices[current_id].push_back(i);
    }

    // --- STEP 2: Process each ID independently (algorithm steps 1-9 combined) ---
    for (const auto& id_pair : id_indices) {
        const string& current_id = id_pair.first;
        const vector<int>& indices = id_pair.second;
        const int id_size = indices.size();

        if (id_size < 4) continue; // Need at least 4 points for GRID

        // Extract data for this ID (pre-allocated vectors)
        vector<double> id_times(id_size);
        vector<double> id_gls(id_size);

        for (int i = 0; i < id_size; ++i) {
            id_times[i] = time[indices[i]];
            id_gls[i] = gl[indices[i]];
        }

        // --- STEP 1: GRID Detection (inline optimized) ---
        vector<int> grid_binary(id_size, 0);
        vector<int> grid_start_indices;
        grid_start_indices.reserve(id_size / 10); // Estimate

        for (int j = 3; j < id_size; ++j) {
            // Skip NA values
            if (NumericVector::is_na(id_gls[j]) || NumericVector::is_na(id_gls[j-1]) ||
                NumericVector::is_na(id_gls[j-2]) || NumericVector::is_na(id_gls[j-3])) {
                continue;
            }

            // Calculate rates (optimized)
            const double dt1 = (id_times[j] - id_times[j-1]) / 3600;
            const double dt2 = (id_times[j-1] - id_times[j-2]) / 3600;
            const double dt3 = (id_times[j-2] - id_times[j-3]) / 3600;

            if (dt1 <= 0 || dt2 <= 0 || dt3 <= 0) continue;

            const double rate1 = (id_gls[j] - id_gls[j-1]) / dt1;
            const double rate2 = (id_gls[j-1] - id_gls[j-2]) / dt2;
            const double rate3 = (id_gls[j-2] - id_gls[j-3]) / dt3;

            // Apply GRID criteria
            bool grid_triggered = false;
            int mark_start_idx = -1;

            if (rate1 >= 95 && rate2 >= 95 && threshold <= id_gls[j-2]) {
                grid_triggered = true;
                mark_start_idx = j-2;
            } else if ((rate2 >= 90 && rate3 >= 90 && threshold <= id_gls[j-3]) ||
                      (rate3 >= 90 && rate1 >= 90 && threshold <= id_gls[j-3])) {
                grid_triggered = true;
                mark_start_idx = j-3;
            }

            if (grid_triggered && mark_start_idx >= 0) {
                const double gap_seconds = gap * 60;

                // Mark points within gap window - EXACT match to original GRID logic
                for (int k = j; k < id_size && (id_times[k] - id_times[j]) <= gap_seconds; ++k) {
                    if (mark_start_idx == j-2) {
                        // Match original: if (k >= 2) GRID_subset[k-2] = 1;
                        if (k >= 2) {
                            grid_binary[k-2] = 1;
                        }
                    } else { // mark_start_idx == j-3
                        // Match original: if (k >= 3) GRID_subset[k-3] = 1;
                        if (k >= 3) {
                            grid_binary[k-3] = 1;
                        }
                    }
                }
            }
        }

        // Find GRID start points (optimized)
        for (int i = 0; i < id_size; ++i) {
            if (grid_binary[i] == 1 && (i == 0 || grid_binary[i-1] == 0)) {
                grid_start_indices.push_back(i);
            }
        }

        if (grid_start_indices.empty()) continue;

        // --- STEP 2: Modified GRID (inline optimized) ---
        vector<int> mod_grid_binary(id_size, 0);
        vector<int> mod_grid_start_indices;
        mod_grid_start_indices.reserve(grid_start_indices.size());

        const double hours_seconds = hours * 3600;
        const double gap_seconds = gap * 60;

        for (int grid_idx : grid_start_indices) {
            const double end_time = id_times[grid_idx];
            const double window_start_time = end_time - hours_seconds;

            // Binary search for start index (optimization)
            int start_idx = grid_idx;
            while (start_idx > 0 && id_times[start_idx-1] >= window_start_time) {
                start_idx--;
            }

            // Find minimum in window
            double min_value = R_PosInf;
            int min_idx = start_idx;

            for (int j = start_idx; j <= grid_idx; ++j) {
                if (!NumericVector::is_na(id_gls[j]) && id_gls[j] < min_value) {
                    min_value = id_gls[j];
                    min_idx = j;
                }
            }

            // Mark gap period from minimum
            const double gap_end_time = id_times[min_idx] + gap_seconds;
            for (int k = min_idx; k < id_size && id_times[k] <= gap_end_time; ++k) {
                mod_grid_binary[k] = 1;
            }
        }

        // Find mod_GRID start points
        for (int i = 0; i < id_size; ++i) {
            if (mod_grid_binary[i] == 1 && (i == 0 || mod_grid_binary[i-1] == 0)) {
                mod_grid_start_indices.push_back(i);
            }
        }

        if (mod_grid_start_indices.empty()) continue;

        // --- STEP 3: Find maxima after hours (inline optimized) ---
        vector<int> max_after_hours_indices;
        max_after_hours_indices.reserve(mod_grid_start_indices.size());

        for (size_t i = 0; i < mod_grid_start_indices.size(); ++i) {
            const int start_idx = mod_grid_start_indices[i];
            const double window_end_time = id_times[start_idx] + hours_seconds;

            int end_idx;
            if (i + 1 < mod_grid_start_indices.size()) {
                const int next_start = mod_grid_start_indices[i + 1];
                const double next_time = id_times[next_start];
                if ((next_time - id_times[start_idx]) < hours_seconds) {
                    end_idx = next_start;
                } else {
                    int j = start_idx;
                    while (j < id_size && id_times[j] <= window_end_time) {
                        j++;
                    }
                    end_idx = j - 1;
                }
            } else { // Last start point
                int j = start_idx;
                while (j < id_size && id_times[j] <= window_end_time) {
                    j++;
                }
                end_idx = j - 1;
            }

            // Find maximum in range
            double max_value = R_NegInf;
            int max_idx = start_idx;

            for (int j = start_idx; j <= end_idx && j < id_size; ++j) {
                if (!NumericVector::is_na(id_gls[j]) && id_gls[j] > max_value) {
                    max_value = id_gls[j];
                    max_idx = j;
                }
            }

            max_after_hours_indices.push_back(max_idx);
        }

        // --- STEP 4: Find local maxima (inline optimized) ---
        vector<int> local_maxima_indices;
        local_maxima_indices.reserve(id_size / 20); // Estimate

        if (id_size >= 5) {
            // Pre-calculate differences for efficiency
            vector<double> diff_gl(id_size - 1);
            for (int i = 0; i < id_size - 1; ++i) {
                if (NumericVector::is_na(id_gls[i]) || NumericVector::is_na(id_gls[i+1])) {
                    diff_gl[i] = NA_REAL;
                } else {
                    diff_gl[i] = id_gls[i+1] - id_gls[i];
                }
            }

            // Find local maxima
            for (int i = 3; i < id_size - 2; ++i) {
                if (!NumericVector::is_na(diff_gl[i-2]) && !NumericVector::is_na(diff_gl[i-1]) &&
                    !NumericVector::is_na(diff_gl[i]) && !NumericVector::is_na(diff_gl[i+1])) {

                    if (diff_gl[i-2] >= 0 && diff_gl[i-1] >= 0 &&
                        diff_gl[i] <= 0 && diff_gl[i+1] <= 0) {
                        local_maxima_indices.push_back(i);
                    }
                }
            }
        }

        // --- STEP 5: Find new maxima (inline optimized) ---
        vector<int> final_maxima_indices;
        final_maxima_indices.reserve(max_after_hours_indices.size());

        for (int mod_idx : max_after_hours_indices) {
            const double mod_time = id_times[mod_idx];
            const double window_start = mod_time;
            const double window_end = mod_time + 2 * 3600; // 2 hours

            // Find local maxima in window (using binary search for efficiency)
            vector<int> candidates_in_window;
            for (int local_idx : local_maxima_indices) {
                const double local_time = id_times[local_idx];
                if (local_time >= window_start && local_time <= window_end) {
                    candidates_in_window.push_back(local_idx);
                }
            }

            if (candidates_in_window.empty()) {
                final_maxima_indices.push_back(mod_idx);
            } else {
                // Find maximum among candidates
                double max_gl = id_gls[mod_idx];
                int best_idx = mod_idx;

                for (int candidate_idx : candidates_in_window) {
                    if (id_gls[candidate_idx] > max_gl) {
                        max_gl = id_gls[candidate_idx];
                        best_idx = candidate_idx;
                    }
                }

                final_maxima_indices.push_back(best_idx);
            }
        }

        // --- STEP 6-9: Transform summary and detect between maxima (inline optimized) ---
        // CRITICAL FIX: Use ORIGINAL GRID episode starts, not modified GRID starts
        // The original maxima_GRID uses grid_result$episode_start_total which comes from
        // the original GRID detection, NOT the modified GRID!

        // Step 6A: Find original GRID episode starts (different from modified GRID)
        vector<int> original_grid_start_indices;

        // Use the same GRID detection logic as the original but collect episode starts
        for (int i = 0; i < id_size; ++i) {
            if (grid_binary[i] == 1 && (i == 0 || grid_binary[i-1] == 0)) {
                original_grid_start_indices.push_back(i);
            }
        }

        // Match original transformSummaryDf logic exactly
        vector<double> transform_grid_times;
        vector<double> transform_grid_gls;
        vector<double> transform_maxima_times;
        vector<double> transform_maxima_gls;
        vector<int> transform_grid_indices;  // Store GRID indices
        vector<int> transform_maxima_indices; // Store maxima indices
        double max_gl;
        int max_gl_index;

        // Process each ORIGINAL GRID start episode to find its best maxima within 4 hours
        for (int grid_idx : original_grid_start_indices) {
            const double grid_time = id_times[grid_idx];
            const double grid_gl = id_gls[grid_idx];

            max_gl = -1;
            max_gl_index = -1;

            // Find best maxima within 4 hours for this GRID episode
            for (size_t j = 0; j < final_maxima_indices.size(); ++j) {
                const int maxima_idx = final_maxima_indices[j];
                const double maxima_time = id_times[maxima_idx];
                const double maxima_gl = id_gls[maxima_idx];

                const double potential_max_points = maxima_time - grid_time;

                if (potential_max_points >= 0 && potential_max_points <= 4 * 3600) {
                    if (maxima_gl > max_gl) {
                        max_gl = maxima_gl;
                        max_gl_index = static_cast<int>(j);
                    }
                }
            }

            // Only include episodes where a valid maxima was found (original logic: max_gl_index != -1)
            if (max_gl_index != -1) {
                const int best_maxima_idx = final_maxima_indices[max_gl_index];
                transform_grid_times.push_back(grid_time);
                transform_grid_gls.push_back(grid_gl);
                transform_maxima_times.push_back(id_times[best_maxima_idx]);
                transform_maxima_gls.push_back(max_gl);
                transform_grid_indices.push_back(grid_idx);  // Store GRID index
                transform_maxima_indices.push_back(best_maxima_idx);  // Store maxima index
            }
        }

    // Detect between maxima (FIXED to match detectBetweenMaxima logic exactly)
    int current_episode_count = 0;
    const int n = transform_grid_times.size();

    // Process consecutive pairs (i from 1 to n-1) - ALWAYS process all pairs
    for (int i = 1; i < n; ++i) {
        const double prev_grid_time = transform_grid_times[i-1];
        const double curr_grid_time = transform_grid_times[i];
        const double prev_grid_gl = transform_grid_gls[i-1];

        if (NumericVector::is_na(prev_grid_time) || NumericVector::is_na(curr_grid_time)) {
            continue;
        }

        double max_value = R_NegInf;
        double max_time = 0;

        // Check if same maxima time (for conditional search logic)
        bool same_maxima_time = false;
        if (i-1 < static_cast<int>(transform_maxima_times.size()) &&
            i < static_cast<int>(transform_maxima_times.size())) {
            if (!NumericVector::is_na(transform_maxima_times[i-1]) &&
                !NumericVector::is_na(transform_maxima_times[i]) &&
                transform_maxima_times[i-1] == transform_maxima_times[i]) {
                same_maxima_time = true;
            }
        }

        // Only search between GRID times if maxima times are the same
        if (same_maxima_time) {
            for (int j = 0; j < id_size; ++j) {
                const double time_point = id_times[j];
                const double gl_value = id_gls[j];

                if (NumericVector::is_na(time_point) || NumericVector::is_na(gl_value)) {
                    continue;
                }

                // Check if this point is between the two GRID times
                if (time_point > prev_grid_time && time_point < curr_grid_time) {
                    if (gl_value > max_value) {
                        max_value = gl_value;
                        max_time = time_point;
                    }
                }
            }
        }

        // CRITICAL FIX: Always determine result values (not conditional on same_maxima_time)
        double result_time;
        double result_value;

        if (max_value != R_NegInf) {
            result_time = max_time;
            result_value = max_value;
        } else {
            // Use maxima data as fallback
            if (i-1 < static_cast<int>(transform_maxima_times.size()) &&
                i-1 < static_cast<int>(transform_maxima_gls.size())) {
                result_time = transform_maxima_times[i-1];
                result_value = transform_maxima_gls[i-1];
            } else {
                result_time = NA_REAL;
                result_value = NA_REAL;
            }
        }

        // Handle the "1970-01-01 00:00:00" case (time = 0)
        if (result_time == 0) {
            result_time = NA_REAL;
            result_value = NA_REAL;
        }

        // Calculate time to peak
        double time_to_peak = NA_REAL;
        if (!NumericVector::is_na(result_time) && !NumericVector::is_na(prev_grid_time)) {
            time_to_peak = result_time - prev_grid_time;
        }

        // ALWAYS store result for episode i-1 (not conditional)
        result_ids.push_back(current_id);
        result_grid_times.push_back(prev_grid_time);
        result_grid_gls.push_back(prev_grid_gl);
        result_maxima_times.push_back(result_time);
        result_maxima_gls.push_back(result_value);
        result_time_to_peak.push_back(time_to_peak);
        result_grid_indices.push_back(transform_grid_indices[i-1]+1);
        result_maxima_indices.push_back(transform_maxima_indices[i-1]+1);
        current_episode_count++;
    }

    // Handle last element (index n-1) - matching second code's logic
    if (n > 0) {
        double last_result_time;
        double last_result_value;

        // CORRECTED: Match second code's last element handling exactly
        if (n-1 < static_cast<int>(transform_maxima_times.size())) {
            if (NumericVector::is_na(transform_maxima_times[n-1])) {
                // If maxima_time[n-1] is NA, set both to NA
                last_result_time = NA_REAL;
                last_result_value = NA_REAL;
            } else {
                // Use the maxima values for the last element
                last_result_time = transform_maxima_times[n-1];
                last_result_value = (n-1 < static_cast<int>(transform_maxima_gls.size())) ?
                                transform_maxima_gls[n-1] : NA_REAL;
            }
        } else {
            // Index out of bounds - set to NA
            last_result_time = NA_REAL;
            last_result_value = NA_REAL;
        }

        // Calculate time-to-peak for last element
        double last_time_to_peak = NA_REAL;
        if (!NumericVector::is_na(last_result_time) &&
            n-1 < static_cast<int>(transform_grid_times.size()) &&
            !NumericVector::is_na(transform_grid_times[n-1])) {
            last_time_to_peak = last_result_time - transform_grid_times[n-1];
        }

        // Store the last result
        result_ids.push_back(current_id);
        result_grid_times.push_back(n-1 < static_cast<int>(transform_grid_times.size()) ?
                                    transform_grid_times[n-1] : NA_REAL);
        result_grid_gls.push_back(n-1 < static_cast<int>(transform_grid_gls.size()) ?
                                transform_grid_gls[n-1] : NA_REAL);
        result_maxima_times.push_back(last_result_time);
        result_maxima_gls.push_back(last_result_value);
        result_time_to_peak.push_back(last_time_to_peak);
        result_grid_indices.push_back(n-1 < static_cast<int>(transform_grid_indices.size()) ?
                                    transform_grid_indices[n-1]+1 : -1);
        result_maxima_indices.push_back(n-1 < static_cast<int>(transform_maxima_indices.size()) ?
                                      transform_maxima_indices[n-1]+1 : -1);
        current_episode_count++;
    }

        // CORRECTED: Episode count should always equal n (matching second code's behavior)
        // Second code always produces n results, one for each minima/GRID point
        episode_counts[current_id] = current_episode_count;  // Changed from current_episode_count to n
    }

    // --- Create final output (optimized) ---
    DataFrame results_df;
    if (result_ids.empty()) {
        results_df = DataFrame::create(
            _["id"] = CharacterVector::create(),
            _["grid_time"] = NumericVector::create(),
            _["grid_gl"] = NumericVector::create(),
            _["maxima_time"] = NumericVector::create(),
            _["maxima_glucose"] = NumericVector::create(),
            _["time_to_peak"] = NumericVector::create(),
            _["grid_index"] = IntegerVector::create(),
            _["maxima_index"] = IntegerVector::create()
        );
        results_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    } else {
        // Create POSIXct time vectors (with proper POSIXt inheritance)
        NumericVector grid_times_vec = wrap(result_grid_times);
        grid_times_vec.attr("class") = "POSIXct";
        grid_times_vec.attr("tzone") = "UTC";

        NumericVector maxima_times_vec = wrap(result_maxima_times);
        maxima_times_vec.attr("class") = "POSIXct";
        maxima_times_vec.attr("tzone") = "UTC";

        results_df = DataFrame::create(
            _["id"] = wrap(result_ids),
            _["grid_time"] = grid_times_vec,
            _["grid_gl"] = wrap(result_grid_gls),
            _["maxima_time"] = maxima_times_vec,
            _["maxima_glucose"] = wrap(result_maxima_gls),
            _["time_to_peak"] = wrap(result_time_to_peak),
            _["grid_index"] = wrap(result_grid_indices),
            _["maxima_index"] = wrap(result_maxima_indices)
        );
        results_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");
    }

    // Create episode counts DataFrame
    vector<string> count_ids;
    vector<int> counts;
    count_ids.reserve(episode_counts.size());
    counts.reserve(episode_counts.size());

    for (const auto& pair : episode_counts) {
        count_ids.push_back(pair.first);
        counts.push_back(pair.second);
    }

    DataFrame counts_df = DataFrame::create(
        _["id"] = wrap(count_ids),
        _["episode_counts"] = wrap(counts)
    );
    counts_df.attr("class") = CharacterVector::create("tbl_df", "tbl", "data.frame");

    return List::create(
        _["results"] = results_df,
        _["episode_counts"] = counts_df
    );
}
