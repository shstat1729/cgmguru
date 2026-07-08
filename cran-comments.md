## Test environments

* local macOS Tahoe 26.3, R 4.5.1 (2025-06-13), aarch64-apple-darwin20
  * C++ compiler: Apple clang version 17.0.0 (clang-1700.6.4.2)
  * SDK: MacOSX26.2.sdk

## R CMD check results

Local `R CMD check --as-cran cgmguru_1.2.1.tar.gz` results for cgmguru 1.2.1:

0 errors | 0 warnings | 2 notes

The two notes are local environment notes:

* `checking for future file timestamps`: unable to verify current time.
* `checking HTML version of manual`: HTML validation was skipped because the
  installed `tidy` does not appear recent enough.

CRAN incoming feasibility: OK.

## Package update

This release updates cgmguru to version 1.2.1.

Changes in this release include:

* Relicensed cgmguru from MIT to GPL-2 because parts of the Rcpp
  implementation port, translate, or structurally adapt GPL-2 licensed
  implementation code from `iglu`.
* Added `LICENSE.note` and source-file notices documenting iglu-derived
  implementation components and upstream copyright notices.
* Added `rebound_events()` to detect rebound hypoglycemia and rebound
  hyperglycemia using cgmguru Level 1 initial events followed by an opposite
  threshold crossing within 120 minutes. `detect_all_events()` now includes
  rebound rows and wide summary columns.
* Added `summary_digits` to `detect_all_events()` to control rounding for
  numeric summary outputs.
* Added Rcpp-backed `conga_rcpp()`, `mage_rcpp()`, and `modd_rcpp()` for
  iglu-compatible CONGA, MAGE, and MODD calculations.
* Updated `excursion()` episode-start output with peak glucose, peak time,
  time to peak, and peak index fields.
* Fixed `detect_all_events()` documentation so formulas involving
  `mean_glucose` render correctly in Rd output.
* Added iglu parity tests for the new Rcpp-backed variability metrics.
