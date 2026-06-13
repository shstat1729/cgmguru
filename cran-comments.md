## Test environments

* local R installation, R 4.5.1
* R-hub (platforms; pending)
* win-builder (R-release and R-devel; pending)

## R CMD check results

Local `R CMD check --no-manual --no-build-vignettes --ignore-vignettes`
results for cgmguru 1.1.1:

0 errors | 0 warnings | 0 notes

## Package update

This release updates cgmguru to version 1.1.1.

Changes in this release include:

* Stabilized `sensor_wear()` tests across DST-sensitive timezones by comparing
  fixed-window observed/expected reading counts instead of timezone-dependent
  one-to-one `start_date` values from `iglu::active_percent()` manual windows.
* Expanded the package-level `cgmguru` vignette into a practical CGM analysis
  guide covering sensor wear, event summaries, event-grid inspection, GRID
  analysis, postprandial maxima workflows, excursions, visualization, and
  larger datasets.
