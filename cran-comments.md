## Test environments

* local R installation, R 4.5.1
* R-hub (platforms; pending)
* win-builder (R-release and R-devel; pending)

## R CMD check results

Local `R CMD check --no-manual --no-build-vignettes --ignore-vignettes`
results for cgmguru 1.1.0:

0 errors | 0 warnings | 0 notes

## Package update

This release updates cgmguru to version 1.1.0.

Changes in this release include:

* Renamed event count output columns to `total_episodes` for standalone
  hypo-/hyperglycemic event summaries and `detect_all_events()` long-format
  event output.
* Updated documentation, examples, vignettes, and tests to use
  `total_episodes` consistently.
